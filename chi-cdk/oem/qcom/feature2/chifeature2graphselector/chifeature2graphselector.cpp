////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2019-2020 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  chifeature2graphselector.cpp
/// @brief CHI feature2 graph selector implementation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "chifeature2graphselector.h"
#include "chifeature2graphselectoroem.h"
#include "chifeature2graphselectortable.h"
#include "chistatsproperty.h"

// NOWHINE FILE NC009:  CHI files will start with CHI
// NOWHINE FILE CP006:  Need whiner update: std::vector allowed in exceptional cases
// NOWHINE ENTIRE FILE - Temporarily bypassing for existing CHI files, required for table

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiFeature2GraphSelector::PopulateAllTables
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID ChiFeature2GraphSelector::PopulateAllTables()
{
    /// @brief set of cameraIds and its corresponding list of Feature graph Descriptors
    m_cameraIdDescriptorNameSet.insert(
        {
            // cameraId set    // Feature Graph Descriptor Name set
            // Single camera
            { SINGLE_CAMERA,  { RealTimeFeatureGraphDescriptor.pFeatureGraphName,
                                RTBayer2YUVHDRT1JPEGFeatureGraphDescriptor.pFeatureGraphName,
                                RTBayer2YUVSWMFJPEGFeatureGraphDescriptor.pFeatureGraphName,
                                RTBayer2YUVJPEGFeatureGraphDescriptor.pFeatureGraphName,
                                RTBayer2YUVFeatureGraphDescriptor.pFeatureGraphName,
                                RTMemcpyYUVFeatureGraphDescriptor.pFeatureGraphName } },
        }
    );

    /// @brief mapping feature graph descriptor name to feature graph descriptor
    m_featureGraphDescriptorsMap.insert(
    {
        { RealTimeFeatureGraphDescriptor.pFeatureGraphName,                RealTimeFeatureGraphDescriptor },
    });

    m_featureGraphDescriptorsMap.insert(
    {
        { Bayer2YUVFeatureGraphDescriptor.pFeatureGraphName,               Bayer2YUVFeatureGraphDescriptor },
    });

    m_featureGraphDescriptorsMap.insert(
    {
        { RTBayer2YUVHDRT1JPEGFeatureGraphDescriptor.pFeatureGraphName,    RTBayer2YUVHDRT1JPEGFeatureGraphDescriptor },
    });

    m_featureGraphDescriptorsMap.insert(
    {
        { RTBayer2YUVSWMFJPEGFeatureGraphDescriptor.pFeatureGraphName,     RTBayer2YUVSWMFJPEGFeatureGraphDescriptor },
    });

    m_featureGraphDescriptorsMap.insert(
    {
        { RTBayer2YUVJPEGFeatureGraphDescriptor.pFeatureGraphName,         RTBayer2YUVJPEGFeatureGraphDescriptor },
    });

    m_featureGraphDescriptorsMap.insert(
    {
        { RTBayer2YUVFeatureGraphDescriptor.pFeatureGraphName,             RTBayer2YUVFeatureGraphDescriptor },
    });

    m_featureGraphDescriptorsMap.insert(
    {
        { RTMemcpyYUVFeatureGraphDescriptor.pFeatureGraphName,             RTMemcpyYUVFeatureGraphDescriptor },
    });

    /// @brief Keys to Feature Graph Descriptor map table
    FGDKeysForTable keys =
    {
        /*******************************************************************************************************************************/
        /*************************************************Realtime FeatureGraph*********************************************************/
        /*******************************************************************************************************************************/
        // FGD name    // CameraId          // CaptureIntent                         // SceneMode                                // NoiseReduction                       // vendortag  // OpsMode
          RealTimeFeatureGraphDescriptor.pFeatureGraphName,
           SINGLE_CAMERA,  { ControlCaptureIntentPreview,
                             ControlCaptureIntentVideoRecord,
                             ControlCaptureIntentZeroShutterLag },         { SceneModeAll },                           { noiseReductionmodeAll },                { 0 },             0
    };

    m_FDGKeysMap.push_back(keys);

    m_FDGKeysMap.push_back(keys);

    keys =
    {
        /*******************************************************************************************************************************/
        /*************************************************Realtime+B2Y+HDR+JPEG FeatureGraph***********************************************/
        /*******************************************************************************************************************************/
        // FGD name    // CameraId          // CaptureIntent                 // SceneMode                                // NoiseReduction                       // vendortag  // OpsMode
          RTBayer2YUVHDRT1JPEGFeatureGraphDescriptor.pFeatureGraphName,
            SINGLE_CAMERA,  { ControlCaptureIntentStillCapture,
                              ControlCaptureIntentVideoSnapshot,
                              ControlCaptureIntentZeroShutterLag,
                              ControlCaptureIntentManual },            {ControlSceneModeHDR},                         { NoiseReductionModeOff,
                                                                                                                        NoiseReductionModeFast,
                                                                                                                        NoiseReductionModeMinimal,
                                                                                                                        NoiseReductionModeZeroShutterLag, }, { 0 },             0
    };

    m_FDGKeysMap.push_back(keys);

    keys =
    {
        /*******************************************************************************************************************************/
        /*************************************************Realtime+B2Y+JPEG FeatureGraph************************************************/
        /*******************************************************************************************************************************/
        // FGD name    // CameraId          // CaptureIntent                                          // SceneMode                                // NoiseReduction                       // vendortag  // OpsMode
          RTBayer2YUVJPEGFeatureGraphDescriptor.pFeatureGraphName,
        SINGLE_CAMERA, { ControlCaptureIntentStillCapture ,
                         ControlCaptureIntentVideoSnapshot,
                         ControlCaptureIntentZeroShutterLag,
                         ControlCaptureIntentManual},               {      ControlSceneModeDisabled,
                                                                           ControlSceneModeFacePriority,
                                                                           ControlSceneModeAction,
                                                                           ControlSceneModePortrait,
                                                                           ControlSceneModeLandscape,
                                                                           ControlSceneModeNight,
                                                                           ControlSceneModeNightPortrait,
                                                                           ControlSceneModeTheatre,
                                                                           ControlSceneModeBeach,
                                                                           ControlSceneModeSnow,
                                                                           ControlSceneModeSunset,
                                                                           ControlSceneModeSteadyphoto,
                                                                           ControlSceneModeFireworks,
                                                                           ControlSceneModeSports,
                                                                           ControlSceneModeParty,
                                                                           ControlSceneModeCandlelight,
                                                                           ControlSceneModeBarcode,
                                                                           ControlSceneModeHighSpeedVideo,
                                                                           ControlSceneModeFacePriorityLowLight, },    { noiseReductionmodeAll },     {0,
                                                                                                                                                       CustomVendorTagBurst },             0
    };

    m_FDGKeysMap.push_back(keys);

    keys =
    {
        /*******************************************************************************************************************************/
        /*************************************************B2Y FeatureGraph**************************************************************/
        /*******************************************************************************************************************************/
        // FGD name    // CameraId          // CaptureIntent                         // SceneMode     // NoiseReduction   // vendortag   // OpsMode
          Bayer2YUVFeatureGraphDescriptor.pFeatureGraphName,
            MULTI_CAMERA,      { ControlCaptureIntentStillCapture,
                                 ControlCaptureIntentVideoSnapshot,
                                 ControlCaptureIntentZeroShutterLag,
                                 ControlCaptureIntentManual },         { ControlSceneModeFacePriority,
                                                                         ControlSceneModeAction,
                                                                         ControlSceneModePortrait,
                                                                         ControlSceneModeLandscape,
                                                                         ControlSceneModeNight,
                                                                         ControlSceneModeNightPortrait,
                                                                         ControlSceneModeTheatre,
                                                                         ControlSceneModeBeach,
                                                                         ControlSceneModeSnow,
                                                                         ControlSceneModeSunset,
                                                                         ControlSceneModeSteadyphoto,
                                                                         ControlSceneModeFireworks,
                                                                         ControlSceneModeSports,
                                                                         ControlSceneModeParty,
                                                                         ControlSceneModeCandlelight,
                                                                         ControlSceneModeBarcode },       { NoiseReductionModeFast },             { 0 },             0
    };
    m_FDGKeysMap.push_back(keys);

    Dump(GetGraphDescriptorTables());
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiFeature2GraphSelector::Initialize
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ChiFeature2GraphSelector::Initialize(
    FeatureGraphManagerConfig* pConfig)
{
    m_pFeature2GraphDescs.reserve(MaxNumberOfGraphs);
    m_stageSequenceHint.reserve(DefaultStageSequences);

    m_pCameraInfo     = pConfig->pCameraInfo;
    m_inputOutputType = pConfig->inputOutputType;
    m_isQCFAUsecase   = FALSE;
    m_remosaicType    = CHIREMOSAICTYPE::UnKnown;

    ExtensionModule::GetInstance()->GetVendorTagOps(&m_vendorTagOps);

    return CDKResultSuccess;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiFeature2GraphSelector::Destroy
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID ChiFeature2GraphSelector::Destroy()
{
    for (auto& keyToGraphDescMapItem : m_clonedFeatureGraphDescriptorsMap)
    {
        FreeGraphDescResources(keyToGraphDescMapItem.second);
    }
    m_clonedFeatureGraphDescriptorsMap.clear();
    m_pFeature2GraphDescs.clear();
    m_stageSequenceHint.clear();

    m_cameraIdMap.clear();

    for (auto& pruneKeyToGraphDescMapItem : m_prunedFeatureGraphDescriptorsMap)
    {
        FreeGraphDescResources(pruneKeyToGraphDescMapItem.second);
        ChxUtils::Free(pruneKeyToGraphDescMapItem.first);
    }
    m_prunedFeatureGraphDescriptorsMap.clear();
    CHX_DELETE this;
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiFeature2GraphSelector::SearchFeatureGraphinTable
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
ChiFeature2GraphDesc* ChiFeature2GraphSelector::SearchFeatureGraphinTable(
    FGDKeysForCaptureRequest*     pKeysForRequest,
    std::vector<FGDKeysForTable>* FeatureGraphDesc)
{
    ChiFeature2GraphDesc* pChiFeature2GraphDesc = NULL;

    // Find FGD based on the above keys
    BOOL descriptorFound = FALSE;
    BOOL cameraIdFound   = FALSE;

    for (auto &it : *FeatureGraphDesc)
    {
        CHX_LOG_INFO("descriptorName:%s", it.pDescriptorName);
        for (auto &cameraMapIt : m_cameraIdMap)
        {
            std::set<UINT> cameraIdSet = cameraMapIt.first;

            if ((cameraIdSet.find(pKeysForRequest->cameraId) != cameraIdSet.end()) && (it.cameraId == cameraMapIt.second))
            {
                cameraIdFound = TRUE;
            }
        }
        if (TRUE == cameraIdFound)
        {
            if (m_pCameraInfo->numPhysicalCameras > 1)
            {
                ChiFeature2FGDKeysForClonedMap phyCamDesc = {0};
                phyCamDesc.bMultiCamera = TRUE;
                phyCamDesc.cameraId     = m_pCameraInfo->cameraId;
                phyCamDesc.featureFlags.value             = pKeysForRequest->flags.value;
                if (m_pCameraInfo->logicalCameraType == LogicalCameraType::LogicalCameraType_SAT)
                {
                    phyCamDesc.pDescriptorName =MultiCameraFusionFeatureSuperGraphDescriptor.pFeatureGraphName;
                }
                else
                {
                    phyCamDesc.pDescriptorName =MultiCameraBokehFeatureSuperGraphDescriptor.pFeatureGraphName;
                }
                auto mapIterator = m_clonedFeatureGraphDescriptorsMap.find(phyCamDesc);
                if (mapIterator != m_clonedFeatureGraphDescriptorsMap.end())
                {
                    pChiFeature2GraphDesc = GetPrunedFeatureGraph(mapIterator->second, pKeysForRequest);
                }

                if (NULL != pChiFeature2GraphDesc)
                {
                    return pChiFeature2GraphDesc;
                }
            }
            if (it.captureIntent.find(pKeysForRequest->captureIntent) != it.captureIntent.end())
            {
                if (it.customVendorTag.find(pKeysForRequest->customVendorTag) != it.customVendorTag.end())
                {
                    if (it.noiseReductionMode.find(pKeysForRequest->noiseReductionMode) != it.noiseReductionMode.end())
                    {
                        if (it.sceneMode.find(pKeysForRequest->sceneMode) != it.sceneMode.end())
                        {
                            ChiFeature2FGDKeysForClonedMap phyCamDesc = {0};

                            phyCamDesc.pDescriptorName                = it.pDescriptorName;
                            phyCamDesc.cameraId                       = pKeysForRequest->physicalCameraId;
                            phyCamDesc.featureFlags.value             = pKeysForRequest->flags.value;

                            if ((pKeysForRequest->pMCCResult->isValid) &&
                                (TRUE == pKeysForRequest->pMCCResult->isSnapshotFusion))
                            {
                                phyCamDesc.bMultiCamera = TRUE;
                                phyCamDesc.cameraId     = m_pCameraInfo->cameraId;
                                CHX_LOG_INFO("FusionMode:cameraid:%d, descriptor:%s",
                                    phyCamDesc.cameraId,
                                    phyCamDesc.pDescriptorName);
                            }
                            else
                            {
                                phyCamDesc.bMultiCamera  = FALSE;
                                phyCamDesc.cameraId      = pKeysForRequest->pMCCResult->masterCameraId;
                                CHX_LOG_INFO("SingleZone: cameraid:%d, descriptor:%s",
                                    phyCamDesc.cameraId,
                                    phyCamDesc.pDescriptorName);
                            }

                            // Force SAT use super graph, super graph will be pruned into different graph based on key
                            // such as noise level, scene mode, mcc etc.
                            if (m_pCameraInfo->logicalCameraType == LogicalCameraType::LogicalCameraType_SAT)
                            {
                                phyCamDesc.bMultiCamera = TRUE;
                                phyCamDesc.cameraId     = m_pCameraInfo->cameraId;
                            }

                            // For now, map BPS camera only in non-MultiCamera Features.
                            if ((FALSE == phyCamDesc.bMultiCamera) &&
                                (RealtimeEngineType_BPS == m_pCameraInfo->ppDeviceInfo[0]->pDeviceConfig->realtimeEngine))
                            {
                                phyCamDesc.featureFlags.isBPSCamera = TRUE;
                            }

                            if ((pKeysForRequest->captureIntent == ControlCaptureIntentStillCapture) &&
                                (TRUE == m_isJpegSnapshot) &&
                                (TRUE == ExtensionModule::GetInstance()->IsSWMFEnabled()))
                            {
                                if (TRUE == IsBurstShot())
                                {
                                    phyCamDesc.pDescriptorName = RTBayer2YUVJPEGFeatureGraphDescriptor.pFeatureGraphName;
                                }
                                else
                                {
                                    phyCamDesc.pDescriptorName = RTBayer2YUVSWMFJPEGFeatureGraphDescriptor.pFeatureGraphName;
                                }

                                auto mapIterator           = m_clonedFeatureGraphDescriptorsMap.find(phyCamDesc);
                                if (mapIterator != m_clonedFeatureGraphDescriptorsMap.end())
                                {
                                    pChiFeature2GraphDesc = mapIterator->second;
                                }
                            }
                            else
                            {
                                if (SINGLE_CAMERA == it.cameraId)
                                {
                                    if ((ControlCaptureIntentPreview == pKeysForRequest->captureIntent) ||
                                        (TRUE == IsCaptureIntentForSnapshot(pKeysForRequest)))
                                    {
                                        if (TRUE == m_is4KYUVOut)
                                        {
                                            phyCamDesc.pDescriptorName = RTBayer2YUVFeatureGraphDescriptor.pFeatureGraphName;
                                            if (NoiseReductionModeHighQuality == pKeysForRequest->noiseReductionMode)
                                            {
                                                phyCamDesc.pDescriptorName =
                                                    RTMFSRYUVFeatureGraphDescriptor.pFeatureGraphName;
                                            }
                                        }
                                        else if (TRUE == m_isRawInput)
                                        {
                                            phyCamDesc.pDescriptorName = Bayer2YUVFeatureGraphDescriptor.pFeatureGraphName;
                                        }
                                    }
                                }
                                auto mapIterator = m_clonedFeatureGraphDescriptorsMap.find(phyCamDesc);
                                if (mapIterator != m_clonedFeatureGraphDescriptorsMap.end())
                                {
                                    if (pKeysForRequest->pMCCResult->isValid)
                                    {
                                        if (m_pCameraInfo->logicalCameraType == LogicalCameraType::LogicalCameraType_SAT ||
                                            m_pCameraInfo->logicalCameraType == LogicalCameraType::LogicalCameraType_RTB)
                                        {
                                            pChiFeature2GraphDesc = GetPrunedFeatureGraph(mapIterator->second,
                                                                                          pKeysForRequest);
                                        }

                                        if (NULL == pChiFeature2GraphDesc)
                                        {
                                            pChiFeature2GraphDesc = mapIterator->second;
                                        }
                                    }
                                    else
                                    {
                                        pChiFeature2GraphDesc = mapIterator->second;
                                    }

                                    descriptorFound                = TRUE;

                                    CHX_LOG_INFO("MatchFound:isMulti:%d physicalCameraId:%d ---> %s, flags:%x, pDesc:%p",
                                                 phyCamDesc.bMultiCamera,
                                                 phyCamDesc.cameraId,
                                                 it.pDescriptorName,
                                                 phyCamDesc.featureFlags.value,
                                                 pChiFeature2GraphDesc);
                                    break;
                                }
                                else
                                {
                                    for (auto item = m_clonedFeatureGraphDescriptorsMap.begin();
                                         item != m_clonedFeatureGraphDescriptorsMap.end(); item++)
                                    {
                                        CHX_LOG_INFO("phyCamId:%d, ->%s, isMulti:%d pDesc:%p, camid:%d,%s,isMulti:%d",
                                                     item->first.cameraId,
                                                     item->first.pDescriptorName,
                                                     item->first.bMultiCamera,
                                                     item->second,
                                                     phyCamDesc.cameraId,
                                                     phyCamDesc.pDescriptorName,
                                                     phyCamDesc.bMultiCamera);
                                    }
                                }
                            }
                        }
                        else
                        {
                            CHX_LOG_INFO("Not match SceneMode:%x", pKeysForRequest->sceneMode);
                        }
                    }
                    else
                    {
                        CHX_LOG_INFO("Not match NoiseMode:%x", pKeysForRequest->noiseReductionMode);
                    }
                }
                else
                {
                    CHX_LOG_INFO("Not match customVendorTag:%x", pKeysForRequest->customVendorTag);
                }
            }
            else
            {
                CHX_LOG_INFO("Not match capture intent:%d", pKeysForRequest->captureIntent);
            }
        }
        else
        {
            CHX_LOG_INFO("Not match cameraID:%d", pKeysForRequest->cameraId);
        }
    }

    if (NULL == pChiFeature2GraphDesc)
    {
        CHX_LOG_ERROR("no descriptor found!");
    }
    return pChiFeature2GraphDesc;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiFeature2GraphSelector::PruneInternalGraphLinks
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
ChiFeature2GraphDesc* ChiFeature2GraphSelector::PruneInternalGraphLinks(
    const ChiFeature2GraphDesc*                   pFeatureGraphDescriptor,
    const std::vector<ChiFeature2PruneRule>&      rPruneRules)
{
    CDKResult result = CDKResultSuccess;

    std::vector<ChiFeature2GraphInternalLinkDesc> internalLinkDescriptors;
    std::set<ChiFeature2InstanceDesc*>            featureInstanceDescriptors;

    ChiFeature2GraphDesc* pFinalGraphDesc = static_cast<ChiFeature2GraphDesc*>(CHX_CALLOC(sizeof (ChiFeature2GraphDesc)));
    if (NULL != pFinalGraphDesc)
    {
        for (UINT linkIdx = 0; linkIdx < pFeatureGraphDescriptor->numInternalLinks; linkIdx++)
        {
            ChiFeature2GraphInternalLinkDesc internalLink = pFeatureGraphDescriptor->pInternalLinks[linkIdx];

            // Save internal feature for given prune rule
            for (const ChiFeature2PruneRule& pruneRule : rPruneRules)
            {
                if (pruneRule.linkType != ChiFeature2GraphLinkType::Internal)
                {
                    continue;
                }
                if (((pruneRule.cameraId == internalLink.srcPortId.instanceProps.cameraId) &&
                    (FALSE == IsMultiCameraFeature(reinterpret_cast<ChiFeature2Type&>(
                    internalLink.srcPortId.featureId)))) ||
                    ((pruneRule.cameraId == internalLink.sinkPortId.instanceProps.cameraId) &&
                    (FALSE == IsMultiCameraFeature(reinterpret_cast<ChiFeature2Type&>(
                    internalLink.sinkPortId.featureId)))))
                {
                    if (pruneRule.prunableVariant == internalLink.pruneVariant)
                    {
                        CHX_LOG_VERBOSE("InternalLink{(%d,%d,%d,%d,%d)->(%d,%d,%d,%d,%d)} is saved."
                                        "pruneRule{%d, %d, %d}",
                                        internalLink.srcPortId.featureId,
                                        internalLink.srcPortId.instanceProps.instanceId,
                                        internalLink.srcPortId.portId.port,
                                        internalLink.srcPortId.portId.portDirectionType,
                                        internalLink.srcPortId.portId.portType,
                                        internalLink.sinkPortId.featureId,
                                        internalLink.sinkPortId.instanceProps.instanceId,
                                        internalLink.sinkPortId.portId.port,
                                        internalLink.sinkPortId.portId.portDirectionType,
                                        internalLink.sinkPortId.portId.portType,
                                        pruneRule.cameraId,
                                        pruneRule.prunableVariant.pruneGroup,
                                        pruneRule.prunableVariant.pruneVariantType);

                        internalLinkDescriptors.push_back(internalLink);
                    }
                    else
                    {
                        CHX_LOG_VERBOSE("InternalLink{(%d,%d,%d,%d,%d)->(%d,%d,%d,%d,%d)} is discard!"
                                        "pruneRule{%d, %d, %d}",
                                        internalLink.srcPortId.featureId,
                                        internalLink.srcPortId.instanceProps.instanceId,
                                        internalLink.srcPortId.portId.port,
                                        internalLink.srcPortId.portId.portDirectionType,
                                        internalLink.srcPortId.portId.portType,
                                        internalLink.sinkPortId.featureId,
                                        internalLink.sinkPortId.instanceProps.instanceId,
                                        internalLink.sinkPortId.portId.port,
                                        internalLink.sinkPortId.portId.portDirectionType,
                                        internalLink.sinkPortId.portId.portType,
                                        pruneRule.cameraId,
                                        pruneRule.prunableVariant.pruneGroup,
                                        pruneRule.prunableVariant.pruneVariantType);

                    }
                }
                else
                {
                    CHX_LOG_VERBOSE("InternalLink{(%d,%d,%d,%d,%d)->(%d,%d,%d,%d,%d)} is discard!"
                                    "pruneRule{%d, %d, %d}",
                                    internalLink.srcPortId.featureId,
                                    internalLink.srcPortId.instanceProps.instanceId,
                                    internalLink.srcPortId.portId.port,
                                    internalLink.srcPortId.portId.portDirectionType,
                                    internalLink.srcPortId.portId.portType,
                                    internalLink.sinkPortId.featureId,
                                    internalLink.sinkPortId.instanceProps.instanceId,
                                    internalLink.sinkPortId.portId.port,
                                    internalLink.sinkPortId.portId.portDirectionType,
                                    internalLink.sinkPortId.portId.portType,
                                    pruneRule.cameraId,
                                    pruneRule.prunableVariant.pruneGroup,
                                    pruneRule.prunableVariant.pruneVariantType);
                }
            }
        }

        if (TRUE == internalLinkDescriptors.empty())
        {
            CHX_LOG_ERROR("No matching internal links are found");
            result = CDKResultEInvalidArg;
        }
        else
        {
            // Save pruned internal links into the feature graph descriptor
            pFinalGraphDesc->numInternalLinks = internalLinkDescriptors.size();
            pFinalGraphDesc->pInternalLinks =
                    static_cast<ChiFeature2GraphInternalLinkDesc*>(CHX_CALLOC(pFinalGraphDesc->numInternalLinks *
                                                                   sizeof (ChiFeature2GraphInternalLinkDesc)));
            if (NULL != pFinalGraphDesc->pInternalLinks)
            {
                std::copy(internalLinkDescriptors.begin(),
                          internalLinkDescriptors.end(),
                          &pFinalGraphDesc->pInternalLinks[0]);
            }
            else
            {
                CHX_LOG_ERROR("Calloc pInternalLinks failed!");
                result = CDKResultENoMemory;
            }
        }

        if (CDKResultSuccess == result)
        {
            // Get feature instance from sink and source ports of enabled internal links and save it only once
            for (auto internalLink : internalLinkDescriptors)
            {
                for (UINT8 featureIdx = 0; featureIdx < pFeatureGraphDescriptor->numFeatureInstances; featureIdx++)
                {
                    ChiFeature2InstanceDesc* pFeatrurInstance = &pFeatureGraphDescriptor->pFeatureInstances[featureIdx];
                    if ((internalLink.srcPortId.featureId == pFeatrurInstance->pFeatureDesc->featureId) &&
                        (internalLink.srcPortId.instanceProps == *pFeatrurInstance->pInstanceProps))
                    {
                        CHX_LOG_INFO("saving feature for cameraid = %d featureid = %d",
                                internalLink.srcPortId.instanceProps.cameraId,
                                internalLink.srcPortId.featureId);
                        featureInstanceDescriptors.insert(pFeatrurInstance);
                    }
                    if ((internalLink.sinkPortId.featureId == pFeatrurInstance->pFeatureDesc->featureId) &&
                            (internalLink.sinkPortId.instanceProps == *pFeatrurInstance->pInstanceProps))
                    {
                        CHX_LOG_INFO("saving sink feature for cameraid = %d featureid = %d",
                                internalLink.srcPortId.instanceProps.cameraId,
                                internalLink.srcPortId.featureId);
                        featureInstanceDescriptors.insert(pFeatrurInstance);
                    }

                }
            }

            if (0 == featureInstanceDescriptors.size())
            {
                CHX_LOG_INFO("No feature instances to saved!");
                result = CDKResultEInvalidArg;
            }
            else
            {
                pFinalGraphDesc->numFeatureInstances = featureInstanceDescriptors.size();
                pFinalGraphDesc->pFeatureInstances   = static_cast<ChiFeature2InstanceDesc*>(
                        CHX_CALLOC(pFinalGraphDesc->numFeatureInstances * sizeof (ChiFeature2InstanceDesc)));

                if (NULL != pFinalGraphDesc->pFeatureInstances)
                {
                    UINT featureIdx = 0;
                    for (auto featureInstanceDescItem : featureInstanceDescriptors)
                    {
                        // Save pruned feature instances
                        ChiFeature2InstanceDesc* pChiFeature2InstanceDesc    =
                                &pFinalGraphDesc->pFeatureInstances[featureIdx++];
                        ChiFeature2InstanceDesc* pSourceFeature2InstanceDesc = featureInstanceDescItem;

                        result = CloneFeature2InstanceDesc(pSourceFeature2InstanceDesc, pChiFeature2InstanceDesc);
                    }
                }
                else
                {
                    result = CDKResultENoMemory;
                    CHX_LOG_ERROR("No memory available while allocating for feature instances");
                }
            }
        }
    }
    else
    {
        result = CDKResultENoMemory;
        CHX_LOG_ERROR("Allocation failed for pClonedGraphDesc");
    }

    if (CDKResultSuccess != result)
    {
        if (NULL != pFinalGraphDesc)
        {
            FreeGraphDescResources(pFinalGraphDesc);
            pFinalGraphDesc = NULL;
        }
    }
    return pFinalGraphDesc;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiFeature2GraphSelector::PruneSinkGraphLinks
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ChiFeature2GraphSelector::PruneSinkGraphLinks(
    const ChiFeature2GraphDesc*                   pSrcFeatureGraphDescriptor,
    ChiFeature2GraphDesc*                         pDestFeatureGraphDescriptor,
    const std::vector<ChiFeature2PruneRule>&      rPruneRules)
{
    CDKResult result = CDKResultSuccess;
    if ((NULL != pSrcFeatureGraphDescriptor) && (NULL != pDestFeatureGraphDescriptor))
    {
        std::vector<ChiFeature2GraphExtSinkLinkDesc> sinkLinkDescriptors;
        for (UINT32 linkIdx = 0 ; linkIdx < pSrcFeatureGraphDescriptor->numExtSinkLinks; ++linkIdx)
        {
            ChiFeature2GraphExtSinkLinkDesc sinkLink = pSrcFeatureGraphDescriptor->pExtSinkLinks[linkIdx];
            for (const ChiFeature2PruneRule& pruneRule: rPruneRules)
            {
                if (pruneRule.linkType != ChiFeature2GraphLinkType::ExternalSink)
                {
                    continue;
                }
                if ((pruneRule.cameraId == sinkLink.portId.instanceProps.cameraId) &&
                    (pruneRule.prunableVariant == sinkLink.pruneVariant))
                {
                    CHX_LOG_INFO("SinkLink{%d,%d,%d,%d} is saved, pruneRule:{%d,%d,%d}",
                        sinkLink.portId.featureId,
                        sinkLink.portId.instanceProps.cameraId,
                        sinkLink.portId.instanceProps.instanceId,
                        sinkLink.portId.portId.port,
                        pruneRule.cameraId,
                        pruneRule.prunableVariant.pruneGroup,
                        pruneRule.prunableVariant.pruneVariantType);
                    sinkLinkDescriptors.push_back(sinkLink);
                }
                else
                {
                    CHX_LOG_VERBOSE("SinkLink{%d,%d,%d,%d} {%d,%d}is discard, pruneRule:{%d,%d,%d}",
                        sinkLink.portId.featureId,
                        sinkLink.portId.instanceProps.cameraId,
                        sinkLink.portId.instanceProps.instanceId,
                        sinkLink.portId.portId.port,
                        sinkLink.pruneVariant.pruneGroup,
                        sinkLink.pruneVariant.pruneVariantType,
                        pruneRule.cameraId,
                        pruneRule.prunableVariant.pruneGroup,
                        pruneRule.prunableVariant.pruneVariantType);
                }
            }
        }

        if (sinkLinkDescriptors.size() == 0)
        {
            CHX_LOG_ERROR("Invalid prune rule! No sink link is saved");
            result = CDKResultEInvalidArg;
        }
        else
        {
            pDestFeatureGraphDescriptor->numExtSinkLinks = sinkLinkDescriptors.size();
            pDestFeatureGraphDescriptor->pExtSinkLinks = static_cast<ChiFeature2GraphExtSinkLinkDesc*>(CHX_CALLOC(
                pDestFeatureGraphDescriptor->numExtSinkLinks * sizeof(ChiFeature2GraphExtSinkLinkDesc)));

            if (NULL != pDestFeatureGraphDescriptor->pExtSinkLinks)
            {
                std::copy(sinkLinkDescriptors.begin(), sinkLinkDescriptors.end(),
                    pDestFeatureGraphDescriptor->pExtSinkLinks);
            }
            else
            {
                result = CDKResultENoMemory;
                CHX_LOG_ERROR("No Memory! Allocate pExtSinkLink failed!");
            }
        }
    }
    else
    {
        CHX_LOG_ERROR("Invalid Arg! pSrcGraphDesc=%p, pDestFeatureDesc=%p",
            pSrcFeatureGraphDescriptor,
            pDestFeatureGraphDescriptor);
        result = CDKResultEInvalidArg;
    }
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiFeature2GraphSelector::PruneSourceGraphLinks
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ChiFeature2GraphSelector::PruneSourceGraphLinks(
    const ChiFeature2GraphDesc*                   pSrcFeatureGraphDescriptor,
    ChiFeature2GraphDesc*                         pDestFeatureGraphDescriptor,
    const std::vector<ChiFeature2PruneRule>&      rPruneRules)
{
    CDK_UNREFERENCED_PARAM(rPruneRules);
    CDKResult result = CDKResultSuccess;
    pDestFeatureGraphDescriptor->numExtSrcLinks = pSrcFeatureGraphDescriptor->numExtSrcLinks;
    pDestFeatureGraphDescriptor->pExtSrcLinks  =
            static_cast<ChiFeature2GraphExtSrcLinkDesc*>(
                    CHX_CALLOC(pSrcFeatureGraphDescriptor->numExtSrcLinks * sizeof (ChiFeature2GraphExtSrcLinkDesc)));
    if (NULL != pDestFeatureGraphDescriptor->pExtSrcLinks)
    {
        for (UINT linkIdx = 0; linkIdx < pSrcFeatureGraphDescriptor->numExtSrcLinks; linkIdx++)
        {
            ChiFeature2GraphExtSrcLinkDesc* pChiFeature2GraphExtSrcLinkDesc = &pDestFeatureGraphDescriptor->pExtSrcLinks[linkIdx];
            ChxUtils::Memcpy(pChiFeature2GraphExtSrcLinkDesc,
                             &pSrcFeatureGraphDescriptor->pExtSrcLinks[linkIdx],
                             sizeof(ChiFeature2GraphExtSrcLinkDesc));
        }
    }
    else
    {
        result = CDKResultENoMemory;
        CHX_LOG_ERROR("Allocation failed for pClonedGraphDesc->pExtSrcLinks");
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiFeature2GraphSelector::FreeGraphDescResources
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID ChiFeature2GraphSelector::FreeGraphDescResources(
        ChiFeature2GraphDesc* pGraphDesc)
{
    if (NULL != pGraphDesc)
    {
        if (NULL != pGraphDesc->pFeatureInstances)
        {
            for (UINT featureIdx = 0; featureIdx < pGraphDesc->numFeatureInstances; featureIdx++)
            {
                ChiFeature2InstanceDesc* pChiFeature2InstanceDesc = &pGraphDesc->pFeatureInstances[featureIdx];

                if (NULL != pChiFeature2InstanceDesc->pInstanceProps)
                {
                    CHX_FREE(pChiFeature2InstanceDesc->pInstanceProps);
                    pChiFeature2InstanceDesc->pInstanceProps = NULL;
                }
            }

            ChiFeature2InstanceDesc* pChiFeature2InstanceDesc = pGraphDesc->pFeatureInstances;
            CHX_FREE(pChiFeature2InstanceDesc);
            pChiFeature2InstanceDesc = NULL;
        }

        if (NULL != pGraphDesc->pInternalLinks)
        {
            ChiFeature2GraphInternalLinkDesc* pChiFeature2GraphInternalLinkDesc = pGraphDesc->pInternalLinks;

            CHX_FREE(pChiFeature2GraphInternalLinkDesc);
            pChiFeature2GraphInternalLinkDesc = NULL;
        }

        if (NULL != pGraphDesc->pExtSinkLinks)
        {
            ChiFeature2GraphExtSinkLinkDesc* pChiFeature2GraphExtSinkLinkDesc = pGraphDesc->pExtSinkLinks;

            CHX_FREE(pChiFeature2GraphExtSinkLinkDesc);
            pChiFeature2GraphExtSinkLinkDesc = NULL;
        }

        if (NULL != pGraphDesc->pExtSrcLinks)
        {
            ChiFeature2GraphExtSrcLinkDesc* pChiFeature2GraphExtSrcLinkDesc = pGraphDesc->pExtSrcLinks;

            CHX_FREE(pChiFeature2GraphExtSrcLinkDesc);
            pChiFeature2GraphExtSrcLinkDesc = NULL;
        }
        CHX_FREE(pGraphDesc);
        pGraphDesc = NULL;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiFeature2GraphSelector::BuildInternalLinkPruneRule
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ChiFeature2GraphSelector::BuildInternalLinkPruneRule(
    const FGDKeysForCaptureRequest* pKeysForRequest,
    UINT8                           physicalCameraIndex,
    ChiFeature2PruneRule&           rPruneRule)
{
    CDKResult result = CDKResultSuccess;
    if (TRUE == pKeysForRequest->pMCCResult->isValid)
    {
        if (TRUE == pKeysForRequest->pMCCResult->isSnapshotFusion)
        {
            if (TRUE == ChxUtils::IsBitSet(pKeysForRequest->pMCCResult->activeMap, physicalCameraIndex))
            {
                rPruneRule.cameraId = physicalCameraIndex;
                rPruneRule.linkType = ChiFeature2GraphLinkType::Internal;
                if (pKeysForRequest->pMCCResult->masterCameraId ==
                    m_pCameraInfo->ppDeviceInfo[physicalCameraIndex]->cameraId)
                {

                    if (ControlSceneModeHDR == pKeysForRequest->sceneMode)
                    {
                        if (TRUE == IsBurstShot())
                        {
                            rPruneRule.prunableVariant.pruneVariantType = ChiFeature2Type::B2Y;
                        }
                        else
                        {
                            if (NoiseReductionModeHighQuality == pKeysForRequest->noiseReductionMode)
                            {
                                rPruneRule.prunableVariant.pruneVariantType = ChiFeature2Type::MFNR_HDR;
                            }
                            else
                            {
                                rPruneRule.prunableVariant.pruneVariantType = ChiFeature2Type::HDR;
                            }
                        }
                    }
                    else if (NoiseReductionModeHighQuality == pKeysForRequest->noiseReductionMode)
                    {
                        if ((CustomVendorTagValues::CustomVendorTagMFNR != pKeysForRequest->customVendorTag) &&
                            (NoiseReductionModeHighQuality == pKeysForRequest->noiseReductionMode))
                        {
                            rPruneRule.prunableVariant.pruneVariantType = ChiFeature2Type::B2Y;
                        }
                        else
                        {
                            rPruneRule.prunableVariant.pruneVariantType = ChiFeature2Type::MFSR;
                        }
                    }
                    else
                    {
                        rPruneRule.prunableVariant.pruneVariantType = ChiFeature2Type::B2Y;
                    }
                }
                else
                {
                    rPruneRule.prunableVariant.pruneVariantType = ChiFeature2Type::B2Y;
                }

                // For Snapshot YUV callback mode, no bokeh/fusion feature is included
                // We define all internal/sink links in singlezone
                if (static_cast<UINT32>(InputOutputType::YUV_OUT) == m_inputOutputType)
                {
                    rPruneRule.prunableVariant.pruneGroup       = ChiFeature2PruneGroup::SingleZone;
                }
                else
                {
                    rPruneRule.prunableVariant.pruneGroup       = ChiFeature2PruneGroup::DualZone;
                }
            }
            else
            {
                CHX_LOG_INFO("Skip prune for physicalCamera:%d", physicalCameraIndex);
                result = CDKResultEInvalidArg;
            }
        }
        else
        {
            rPruneRule.prunableVariant.pruneGroup       = ChiFeature2PruneGroup::SingleZone;
            if (pKeysForRequest->pMCCResult->masterCameraId ==
                m_pCameraInfo->ppDeviceInfo[physicalCameraIndex]->cameraId)
            {
                if (ControlSceneModeHDR == pKeysForRequest->sceneMode)
                {
                    if (TRUE == IsBurstShot())
                    {
                        rPruneRule.prunableVariant.pruneVariantType = ChiFeature2Type::B2Y;
                    }
                    else
                    {
                        if (NoiseReductionModeHighQuality == pKeysForRequest->noiseReductionMode)
                        {
                            rPruneRule.prunableVariant.pruneVariantType = ChiFeature2Type::MFNR_HDR;
                        }
                        else
                        {
                            rPruneRule.prunableVariant.pruneVariantType = ChiFeature2Type::HDR;
                        }
                    }
                }
                else if (NoiseReductionModeHighQuality == pKeysForRequest->noiseReductionMode)
                {
                    if ((CustomVendorTagValues::CustomVendorTagMFNR != pKeysForRequest->customVendorTag) &&
                        (NoiseReductionModeHighQuality == pKeysForRequest->noiseReductionMode))
                    {
                        rPruneRule.prunableVariant.pruneVariantType = ChiFeature2Type::B2Y;
                    }
                    else
                    {
                        rPruneRule.prunableVariant.pruneVariantType = ChiFeature2Type::MFSR;
                    }
                }
                else
                {
                    rPruneRule.prunableVariant.pruneVariantType = ChiFeature2Type::B2Y;
                }

                rPruneRule.cameraId = physicalCameraIndex;
                rPruneRule.linkType = ChiFeature2GraphLinkType::Internal;
            }
            else
            {
                CHX_LOG_INFO("Skip prune for physicalCamera:%d", physicalCameraIndex);
                result = CDKResultEInvalidArg;
            }
        }
    }
    else
    {
        CHX_LOG_ERROR("Invalid mcc result!");
        result = CDKResultEInvalidArg;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiFeature2GraphSelector::BuildSinkLinkPruneRule
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ChiFeature2GraphSelector::BuildSinkLinkPruneRule(
    const FGDKeysForCaptureRequest* pKeysForRequest,
    UINT8                           physicalCameraIndex,
    ChiFeature2PruneRule&           rPruneRule)
{
    CDKResult result = CDKResultSuccess;
    if (TRUE == pKeysForRequest->pMCCResult->isValid)
    {
        if (TRUE == pKeysForRequest->pMCCResult->isSnapshotFusion)
        {
            rPruneRule.cameraId = physicalCameraIndex;
            rPruneRule.linkType = ChiFeature2GraphLinkType::ExternalSink;

            if (static_cast<UINT32>(InputOutputType::YUV_OUT) == m_inputOutputType)
            {
                if (TRUE == ChxUtils::IsBitSet(pKeysForRequest->pMCCResult->activeMap, physicalCameraIndex))
                {
                    if (pKeysForRequest->pMCCResult->masterCameraId ==
                        m_pCameraInfo->ppDeviceInfo[physicalCameraIndex]->cameraId)
                    {
                        rPruneRule.prunableVariant.pruneGroup       = ChiFeature2PruneGroup::SingleZone;
                        if (ControlSceneModeHDR == pKeysForRequest->sceneMode)
                        {
                            if (TRUE == IsBurstShot())
                            {
                                rPruneRule.prunableVariant.pruneVariantType = ChiFeature2Type::B2Y;
                            }
                            else
                            {
                                if (NoiseReductionModeHighQuality == pKeysForRequest->noiseReductionMode)
                                {
                                    rPruneRule.prunableVariant.pruneVariantType = ChiFeature2Type::MFNR_HDR;
                                }
                                else
                                {
                                    rPruneRule.prunableVariant.pruneVariantType = ChiFeature2Type::HDR;
                                }
                            }
                        }
                        else if (NoiseReductionModeHighQuality == pKeysForRequest->noiseReductionMode)
                        {
                            if ((CustomVendorTagValues::CustomVendorTagMFNR != pKeysForRequest->customVendorTag) &&
                                (NoiseReductionModeHighQuality == pKeysForRequest->noiseReductionMode))
                            {
                                rPruneRule.prunableVariant.pruneVariantType = ChiFeature2Type::B2Y;
                            }
                            else
                            {
                                rPruneRule.prunableVariant.pruneVariantType = ChiFeature2Type::MFSR;
                            }
                        }
                        else
                        {
                            rPruneRule.prunableVariant.pruneVariantType = ChiFeature2Type::B2Y;
                        }
                    }
                    else
                    {
                        rPruneRule.prunableVariant.pruneVariantType = ChiFeature2Type::B2Y;
                    }
                }
                else
                {
                    CHX_LOG_INFO("Skip prune for physicalCamera:%d", physicalCameraIndex);
                    result = CDKResultEInvalidArg;
                }
            }
            else
            {
                if (m_pCameraInfo->logicalCameraType == LogicalCameraType::LogicalCameraType_SAT)
                {
                    rPruneRule.prunableVariant.pruneVariantType = ChiFeature2Type::FUSION;
                }
                else if (m_pCameraInfo->logicalCameraType == LogicalCameraType::LogicalCameraType_RTB)
                {
                    rPruneRule.prunableVariant.pruneVariantType = ChiFeature2Type::BOKEH;
                }
                rPruneRule.prunableVariant.pruneGroup       = ChiFeature2PruneGroup::DualZone;

                if (physicalCameraIndex != 0)
                {
                    CHX_LOG_INFO("Skip prune for physicalCamera:%d", physicalCameraIndex);
                    result = CDKResultEInvalidArg;
                }
            }
        }
        else
        {
            rPruneRule.prunableVariant.pruneGroup       = ChiFeature2PruneGroup::SingleZone;
            if (pKeysForRequest->pMCCResult->masterCameraId ==
                m_pCameraInfo->ppDeviceInfo[physicalCameraIndex]->cameraId)
            {
                if (ControlSceneModeHDR == pKeysForRequest->sceneMode)
                {
                    if (TRUE == IsBurstShot())
                    {
                        rPruneRule.prunableVariant.pruneVariantType = ChiFeature2Type::B2Y;
                    }
                    else
                    {
                        if (NoiseReductionModeHighQuality == pKeysForRequest->noiseReductionMode)
                        {
                            rPruneRule.prunableVariant.pruneVariantType = ChiFeature2Type::MFNR_HDR;
                        }
                        else
                        {
                            rPruneRule.prunableVariant.pruneVariantType = ChiFeature2Type::HDR;
                        }
                    }
                }
                else if (NoiseReductionModeHighQuality == pKeysForRequest->noiseReductionMode)
                {
                    if ((CustomVendorTagValues::CustomVendorTagMFNR != pKeysForRequest->customVendorTag) &&
                        (NoiseReductionModeHighQuality == pKeysForRequest->noiseReductionMode))
                    {
                        rPruneRule.prunableVariant.pruneVariantType = ChiFeature2Type::B2Y;
                    }
                    else
                    {
                        rPruneRule.prunableVariant.pruneVariantType = ChiFeature2Type::MFSR;
                    }

                }
                else
                {
                    rPruneRule.prunableVariant.pruneVariantType = ChiFeature2Type::B2Y;
                }
                rPruneRule.cameraId = physicalCameraIndex;
                rPruneRule.linkType = ChiFeature2GraphLinkType::ExternalSink;
            }
            else
            {
                CHX_LOG_INFO("Skip prune for physicalCamera:%d", physicalCameraIndex);
                result = CDKResultEInvalidArg;
            }
        }
    }
    else
    {
        CHX_LOG_ERROR("Invalid mcc result!");
        result = CDKResultEInvalidArg;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiFeature2GraphSelector::GetPruneRulesAndIdentifier
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ChiFeature2GraphSelector::GetPruneRulesAndIdentifier(
        const FGDKeysForCaptureRequest*               pKeysForRequest,
        std::vector<ChiFeature2PruneRule>&            rOutPruneRules,
        CHAR*                                         pOutPruneString)
{
    CDKResult result                                  = CDKResultSuccess;
    CHAR perCameraPruneIdentifier[MaxStringLength256] = { 0 };

    switch (m_pCameraInfo->logicalCameraType)
    {
        case LogicalCameraType::LogicalCameraType_SAT:
        case LogicalCameraType::LogicalCameraType_RTB:
        {
            if (TRUE == pKeysForRequest->pMCCResult->isValid)
            {
                for (UINT32 i = 0; i < m_pCameraInfo->numPhysicalCameras; ++i)
                {
                    ChiFeature2PruneRule pruneRule;
                    CDKResult buildPruneResult = CDKResultSuccess;
                    buildPruneResult = BuildInternalLinkPruneRule(pKeysForRequest, i, pruneRule);
                    if (CDKResultSuccess == buildPruneResult)
                    {
                        // Create unique identifiers to use as a key for pruned graph map
                        CdkUtils::SNPrintF(perCameraPruneIdentifier, sizeof(perCameraPruneIdentifier), "PRUNE:%d%d%d%d",
                                           pruneRule.cameraId,
                                           pruneRule.linkType,
                                           pruneRule.prunableVariant.pruneGroup,
                                           pruneRule.prunableVariant.pruneVariantType);
                        CdkUtils::StrLCat(pOutPruneString, perCameraPruneIdentifier, MaxStringLength256);

                        CHX_LOG_INFO("rOutPrun:%s", pOutPruneString);
                        rOutPruneRules.push_back(pruneRule);
                    }

                    buildPruneResult = BuildSinkLinkPruneRule(pKeysForRequest, i, pruneRule);
                    if (CDKResultSuccess == buildPruneResult)
                    {
                        // Create unique identifiers to use as a key for pruned graph map
                        CdkUtils::SNPrintF(perCameraPruneIdentifier, sizeof(perCameraPruneIdentifier), "PRUNE:%d%d%d%d",
                                           pruneRule.cameraId,
                                           pruneRule.linkType,
                                           pruneRule.prunableVariant.pruneGroup,
                                           pruneRule.prunableVariant.pruneVariantType);
                        CdkUtils::StrLCat(pOutPruneString, perCameraPruneIdentifier, MaxStringLength256);

                        CHX_LOG_INFO("rOutPrun:%s", pOutPruneString);
                        rOutPruneRules.push_back(pruneRule);
                    }

                }
            }
            else
            {
                CHX_LOG_ERROR("No pruning need to be done");
                result = CDKResultEInvalidArg;
            }
        }
            CHX_LOG_INFO("pruningIdentifierString %s", pOutPruneString);
            break;
        default:
            CHX_LOG_INFO("Pruning is not needed for logical camera id = %d", m_pCameraInfo->cameraId);
            result = CDKResultEInvalidArg;
            break;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiFeature2GraphSelector::GetPrunedFeatureGraph
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
ChiFeature2GraphDesc* ChiFeature2GraphSelector::GetPrunedFeatureGraph(
        const ChiFeature2GraphDesc*     pFeatureGraphDescriptor,
        const FGDKeysForCaptureRequest* pKeysForRequest)
{

    CDKResult result                      = CDKResultSuccess;
    ChiFeature2GraphDesc* pFinalGraphDesc = NULL;

    if ((NULL == pFeatureGraphDescriptor) || (NULL == pKeysForRequest))
    {
        CHX_LOG_ERROR("Graph descriptor or key for request is null");
        result = CDKResultEFailed;
    }

    if (CDKResultSuccess == result)
    {
        std::vector<ChiFeature2PruneRule> pruneRules;
        CHAR pruneIDString[MaxStringLength256] = { 0 };

        result = GetPruneRulesAndIdentifier(pKeysForRequest, pruneRules, pruneIDString);

        // Search pruned graph descriptor in cache first
        if ((CDKResultSuccess == result) && (0 != CdkUtils::StrLen(pruneIDString)))
        {
            auto prunedGraphDescIt = m_prunedFeatureGraphDescriptorsMap.find(pruneIDString);
            if (prunedGraphDescIt != m_prunedFeatureGraphDescriptorsMap.end())
            {
                pFinalGraphDesc = prunedGraphDescIt->second;
                if (NULL == pFinalGraphDesc)
                {
                    CHX_LOG_ERROR("Prune internal graph link failed! Pruning is failed or cannot be done");
                    result = CDKResultEFailed;
                }
                else
                {
                    CHX_LOG_INFO("Feature descriptor found in prune map %s, %s",
                        (NULL != pFinalGraphDesc->pFeatureGraphName) ? pFinalGraphDesc->pFeatureGraphName : "NULL", pruneIDString);
                }
            }
            else
            {
                if ((CDKResultSuccess == result) && (FALSE == pruneRules.empty()))
                {
                    // Prune only internal links and features associated with them
                    pFinalGraphDesc = PruneInternalGraphLinks(pFeatureGraphDescriptor, pruneRules);
                    if (NULL != pFinalGraphDesc)
                    {
                        result = PruneSinkGraphLinks(pFeatureGraphDescriptor, pFinalGraphDesc, pruneRules);
                        if (CDKResultSuccess == result)
                        {
                            // clone source links.
                            pFinalGraphDesc->pFeatureGraphName = pFeatureGraphDescriptor->pFeatureGraphName;

                            result = PruneSourceGraphLinks(pFeatureGraphDescriptor, pFinalGraphDesc, pruneRules);
                        }
                    }
                    else
                    {
                        CHX_LOG_ERROR("Prune internal graph link failed!");
                        result = CDKResultEFailed;
                    }
                }

                if (CDKResultSuccess != result)
                {
                    FreeGraphDescResources(pFinalGraphDesc);
                    pFinalGraphDesc = NULL;
                }
                else if (NULL != pFinalGraphDesc)
                {
                    CHAR* pPruneIDString = static_cast<CHAR*>(ChxUtils::Calloc(MaxStringLength256 * sizeof(CHAR)));
                    CdkUtils::StrLCpy(pPruneIDString, pruneIDString, (MaxStringLength256 * sizeof(CHAR)));

                    CHX_LOG_INFO("InsertPruning graph:%s, %s", pPruneIDString,
                        (NULL != pFinalGraphDesc->pFeatureGraphName) ? pFinalGraphDesc->pFeatureGraphName : "NULL");
                    m_prunedFeatureGraphDescriptorsMap.insert({pPruneIDString, pFinalGraphDesc});
                }
                else
                {
                    CHX_LOG_ERROR("Prune internal graph link failed!");
                    result = CDKResultEFailed;
                }
            }
        }
        else
        {
            result = CDKResultEFailed;
            CHX_LOG_ERROR("Pruning is failed or cannot be done");
        }
    }

    return pFinalGraphDesc;

}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiFeature2GraphSelector::GetFeatureHint
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID ChiFeature2GraphSelector::GetFeatureHint(
    ChiFeature2Type                  featureId,
    ChiFeature2UsecaseRequestObject* pRequestObject,
    ChiFeature2Hint*                 pCustomHints)
{
    std::map<UINT32, ChiMetadata*> metadataFrameNumberMap = pRequestObject->GetMetadataMap();
    ChiMetadata*                   pOutputMeta            = NULL;

    if (TRUE == pRequestObject->IsReprocessRequest())
    {
        pOutputMeta = pRequestObject->GetStatusMetadata();
    }
    else
    {
        for (auto mapIterator = metadataFrameNumberMap.rbegin(); mapIterator != metadataFrameNumberMap.rend(); mapIterator++)
        {
            if (NULL != mapIterator->second)
            {
                pOutputMeta = mapIterator->second;
                break;
            }
        }
    }

    camera_metadata_entry_t entry = { 0 };
    INT32 status = -1;
    entry.tag = ANDROID_CONTROL_AE_MODE;

    if (NULL != pOutputMeta)
    {
        status = pOutputMeta->FindTag(entry.tag, &entry);
        UINT32 controlAEMode = ANDROID_CONTROL_AE_MODE_OFF;

        INT* pFlashMode = static_cast<INT*>(pOutputMeta->GetTag(ANDROID_FLASH_MODE));
        UINT8 flashMode = ANDROID_FLASH_MODE_OFF;
        if (NULL != pFlashMode)
        {
            flashMode = *pFlashMode;
            CHX_LOG_INFO("FlashMode %d", flashMode);
        }

        if (0 == status)
        {
            INT controlAEMode = static_cast<INT32>(*(entry.data.i32));
            CHX_LOG_INFO("controlAEMode %d ", controlAEMode);
            if ((ANDROID_CONTROL_AE_MODE_ON_ALWAYS_FLASH == controlAEMode) ||
                ((ANDROID_CONTROL_AE_MODE_ON == controlAEMode) && (ANDROID_FLASH_MODE_OFF != flashMode)))
            {
                pCustomHints->captureMode.u.Manual = 1;
            }
        }

        INT32* pFlashRequired = static_cast<INT32*>(pOutputMeta->GetTag("com.qti.stats_control", "is_flash_snapshot"));
        if (NULL != pFlashRequired)
        {
            CHX_LOG_INFO("isFlashRequired = %d", *pFlashRequired);
            if (1 == *pFlashRequired)
            {
                pCustomHints->captureMode.u.Manual = 1;

                // APP will require flash turn off when burst snapshot in auto flash mode,
                // output metadata may be from previous frame metadata. Use current flash mode to update.

                ChiMetadata* pAppSetting = pRequestObject->GetAppSettings();
                INT*         pFlash_Mode = static_cast<INT*>(pAppSetting->GetTag(ANDROID_FLASH_MODE));
                INT*         pAe_Mode    = static_cast<INT*>(pAppSetting->GetTag(ANDROID_CONTROL_AE_MODE));

                if (NULL != pFlash_Mode) 
                {
                    if ((ANDROID_FLASH_MODE_OFF == *pFlash_Mode) &&
                        ((ANDROID_CONTROL_AE_MODE_OFF == *pAe_Mode) ||
                        (ANDROID_CONTROL_AE_MODE_ON == *pAe_Mode)))
                    {
                        pCustomHints->captureMode.u.Manual = 0;
                        CHX_LOG_INFO("Flash is required turn off");
                    }
                }
            }
        }

        if (SelectInSensorHDR3ExpUsecase::InSensorHDR3ExpSeamlessSnapshot ==
            ExtensionModule::GetInstance()->SelectInSensorHDR3ExpUsecase())
        {
            AECFrameControl* pAECFrameControl = static_cast<AECFrameControl*>(pOutputMeta->GetTag(
                                                        "org.quic.camera2.statsconfigs", "AECFrameControl"));

            if ((NULL != pAECFrameControl)                                                  &&
                (TRUE == pAECFrameControl->inSensorHDR3ExpTriggerOutput.isTriggerInfoValid) &&
                (TRUE == pAECFrameControl->isInSensorHDR3ExpSnapshot))
            {
                pCustomHints->captureMode.u.InSensorHDR3Exp = 1;
                pCustomHints->captureMode.u.Manual          = 1;
            }
        }
    }

    if (SelectInSensorHDR3ExpUsecase::InSensorHDR3ExpForceSeamlessSnapshot ==
        ExtensionModule::GetInstance()->SelectInSensorHDR3ExpUsecase())
    {
        pCustomHints->captureMode.u.InSensorHDR3Exp = 1;
        pCustomHints->captureMode.u.Manual          = 1;
    }

    pCustomHints->numEarlyPCRs = ExtensionModule::GetInstance()->GetNumPCRsBeforeStreamOn();

    switch (featureId)
    {
        case ChiFeature2Type::HDR:
        case ChiFeature2Type::RAWHDR:
        {
            pCustomHints->numFrames = 3;

            camera_metadata_entry_t entry = { 0 };
            INT32 status = -1;
            INT32 captureIntent = -1;

            entry.tag = ANDROID_CONTROL_AE_EXPOSURE_COMPENSATION;
            if (NULL != pOutputMeta)
            {
                status = pOutputMeta->FindTag(entry.tag, &entry);

                if (0 == status)
                {
                    INT exposureValue = static_cast<INT32>(*(entry.data.i32));
                    CHX_LOG_INFO("Exposure value %d", exposureValue);
                }
            }

            // Set the stage hint; using default of 1 sequence per stage
            m_stageSequenceHint.clear();
            m_stageSequenceHint.push_back(pCustomHints->numFrames);
            VerifyAndSetStageSequenceHint(pCustomHints);
        }
        break;

        case ChiFeature2Type::MFNR:
        case ChiFeature2Type::MFSR:
        case ChiFeature2Type::SWMF:
        case ChiFeature2Type::ANCHORSYNC:
        {
            if (ChiFeature2Type::SWMF == featureId)
            {
                pCustomHints->numFrames = 8;
            }
            else if (ChiFeature2Type::MFSR == featureId)
            {
                pCustomHints->numFrames = 3;
            }

            if (NULL != pOutputMeta)
            {
                AECFrameControl* pFrameControl = NULL;

                pFrameControl = static_cast<AECFrameControl*>(pOutputMeta->GetTag("org.quic.camera2.statsconfigs",
                                                                                "AECFrameControl"));
                if (NULL != pFrameControl)
                {
                    FLOAT realGain = pFrameControl->exposureInfo[ExposureIndexSafe].linearGain;

                    if (realGain <= 2.0f)
                    {
                        pCustomHints->numFrames = 3;
                    }
                    else if (realGain <= 4.0f)
                    {
                        pCustomHints->numFrames = 4;
                    }
                    else if (realGain <= 8.0f)
                    {
                        pCustomHints->numFrames = 5;
                    }
                    else if (realGain <= 16.0f)
                    {
                        pCustomHints->numFrames = 6;
                    }
                    else if (realGain <= 32.0f)
                    {
                        pCustomHints->numFrames = 7;
                    }
                    else
                    {
                        pCustomHints->numFrames = 8;
                    }
                    CHX_LOG_INFO("AEC Gain received = %f, number of frames %d", realGain, pCustomHints->numFrames);
                }
                else
                {
                    CHX_LOG_ERROR("Not able to obtain AEC gain for calculation of Total MFNR frames");
                }

                if (ChiFeature2Type::SWMF == featureId)
                {
                    if (0 != ExtensionModule::GetInstance()->ForceSWMFFixedNumOfFrames())
                    {
                        pCustomHints->numFrames = ExtensionModule::GetInstance()->ForceSWMFFixedNumOfFrames();
                        if (pCustomHints->numFrames < 3)
                        {
                            pCustomHints->numFrames = 3;
                        }
                        CHX_LOG_INFO("Force Num of frames for SWMF to : %d, camxsettings: %d", pCustomHints->numFrames, ExtensionModule::GetInstance()->ForceSWMFFixedNumOfFrames());
                    }
                }
                else if (ChiFeature2Type::MFSR == featureId)
                {
                    if (0 != ExtensionModule::GetInstance()->ForceHWMFFixedNumOfFrames())
                    {
                        pCustomHints->numFrames = ExtensionModule::GetInstance()->ForceHWMFFixedNumOfFrames();
                        if (pCustomHints->numFrames < 3)
                        {
                            pCustomHints->numFrames = 3;
                        }
                        CHX_LOG_INFO("Force Num of frames for HWMF %d", pCustomHints->numFrames);
                    }
                }

                INT32* pLLSNeeded = static_cast<INT32*>(pOutputMeta->GetTag("com.qti.stats_control", "is_lls_needed"));
                if (NULL != pLLSNeeded)
                {
                    CHX_LOG_INFO("LLSMode = %d", *pLLSNeeded);
                    pCustomHints->captureMode.u.LowLight = *pLLSNeeded;
                }
            }

            // Set the stage hint; using default of 1 sequence per stage
            m_stageSequenceHint.clear();
            m_stageSequenceHint.push_back(pCustomHints->numFrames);
            VerifyAndSetStageSequenceHint(pCustomHints);
            break;
        }

        case ChiFeature2Type::FRAME_SELECT:
        {
            if (ChiFeature2Type::FRAME_SELECT == featureId)
            {
                pCustomHints->numFrames = 8;
            }
            break;
        }

        case ChiFeature2Type::REALTIME:
        {
            if (NULL != pOutputMeta)
            {
                INT32* pLLSNeeded = static_cast<INT32*>(pOutputMeta->GetTag("com.qti.stats_control", "is_lls_needed"));
                if (NULL != pLLSNeeded)
                {
                    CHX_LOG_INFO("LLSMode = %d", *pLLSNeeded);
                    pCustomHints->captureMode.u.LowLight = *pLLSNeeded;
                }
            }
            break;
        }

        default:
            break;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiFeature2GraphSelector::SelectFeatureGraphforRequestFromTable
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
ChiFeature2GraphDesc* ChiFeature2GraphSelector::SelectFeatureGraphforRequestFromTable(
    ChiFeature2UsecaseRequestObject* pFeature2RequestObject,
    std::map<UINT32, ChiMetadata*>   pMetadataFrameNumberMap,
    GraphDescriptorTables*           pGraphDescriptorTables)
{
    ChiMetadata*                pAppSettings          = pFeature2RequestObject->GetAppSettings();
    const camera_metadata_t*    pMetaSettings         = pFeature2RequestObject->GetMetaSettings();
    ChiMetadata*                pResultMeta           = NULL;
    ChiFeature2GraphDesc*       pChiFeature2GraphDesc = NULL;
    CHICAPTUREREQUEST*          pChiRequest           = NULL;

    FGDKeysForCaptureRequest keysForRequest = {0};

    keysForRequest.cameraId           = m_pCameraInfo->cameraId;
    pChiRequest                       = pFeature2RequestObject->GetChiCaptureRequest();
    keysForRequest.captureIntent      = GetCaptureIntentForRequest(pAppSettings, pChiRequest);
    keysForRequest.noiseReductionMode = GetNoiseReductionMode(pAppSettings);
    keysForRequest.sceneMode          = GetSceneMode(pAppSettings);
    keysForRequest.pMCCResult         = pFeature2RequestObject->GetMCCResult();

    if (ANDROID_CONTROL_CAPTURE_INTENT_STILL_CAPTURE == keysForRequest.captureIntent)
    {
        for (auto mapIterator = pMetadataFrameNumberMap.rbegin();
             mapIterator != pMetadataFrameNumberMap.rend(); mapIterator++)
        {
            if (NULL != mapIterator->second)
            {
                pResultMeta = mapIterator->second;
                break;
            }
        }
        keysForRequest.flags = GetNZSLSnapshotFlag(pAppSettings, pResultMeta);
    }

    keysForRequest.customVendorTag = GetCustomVendorTag(pAppSettings, &keysForRequest);

    m_is4KYUVOut     = FALSE;
    m_isJpegSnapshot = FALSE;
    for (UINT i = 0; i < pChiRequest->numOutputs; i++)
    {
        if (TRUE == Is4KYUVOut(pChiRequest->pOutputBuffers[i].pStream))
        {
            m_is4KYUVOut = TRUE;
        }
        if (TRUE == UsecaseSelector::IsJPEGSnapshotStream(
            reinterpret_cast<camera3_stream_t*>(pChiRequest->pOutputBuffers[i].pStream)))
        {
            m_isJpegSnapshot = TRUE;
        }
    }

    m_isRawInput = FALSE;
    if (NULL != pChiRequest->pInputBuffers)
    {
        if (TRUE == UsecaseSelector::IsRawInputStream(
            reinterpret_cast<camera3_stream_t*>(pChiRequest->pInputBuffers->pStream)))
        {
            m_isRawInput = TRUE;
        }
    }

    if ((NULL != pGraphDescriptorTables) && (NULL == pChiFeature2GraphDesc))
    {
        // Search first in OEM keysMap
        pChiFeature2GraphDesc = SearchFeatureGraphinTable(&keysForRequest, pGraphDescriptorTables->pFeatureGraphDescKeysMap);
    }

    if (NULL == pChiFeature2GraphDesc)
    {
        GraphDescriptorTables* pGraphDescriptorTables = GetGraphDescriptorTables();
        // Search in base class
        pChiFeature2GraphDesc = SearchFeatureGraphinTable(&keysForRequest, pGraphDescriptorTables->pFeatureGraphDescKeysMap);
    }

    if (NULL != pChiFeature2GraphDesc)
    {
        CHX_LOG_INFO("Selected feature graph : %s", pChiFeature2GraphDesc->pFeatureGraphName);
    }
    else
    {
        CHX_LOG_ERROR("Failed to select feature graph");
    }

    return pChiFeature2GraphDesc;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiFeature2GraphSelector::VerifyAndSetStageSequenceHint
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID ChiFeature2GraphSelector::VerifyAndSetStageSequenceHint(
    ChiFeature2Hint* pHint)
{
    UINT8 runningFrameTotal = 0;

    for (UINT8 sequence: m_stageSequenceHint)
    {
        runningFrameTotal += sequence;
    }

    if (NULL != pHint)
    {
        if (runningFrameTotal != pHint->numFrames)
        {
            CHX_LOG_ERROR("pFramesPerSequence Array contains:%d frames but total number of frames is:%d, not setting hint!!",
                          runningFrameTotal,
                          pHint->numFrames);
            m_stageSequenceHint.clear();
        }

        pHint->stageSequenceInfo = m_stageSequenceHint;
    }
    else
    {
        CHX_LOG_ERROR("Hint is NULL, cannot set stage sequence hint!");
    }

}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiFeature2GraphSelector::SelectFeatureGraphforRequest
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
ChiFeature2GraphDesc* ChiFeature2GraphSelector::SelectFeatureGraphforRequest(
    ChiFeature2UsecaseRequestObject* pChiUsecaseRequestObject,
    std::map<UINT32, ChiMetadata*>   pMetadataFrameNumberMap)
{
    return SelectFeatureGraphforRequestFromTable(pChiUsecaseRequestObject, pMetadataFrameNumberMap, NULL);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiFeature2GraphSelector::UpdateInstancePropsForGraph
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID ChiFeature2GraphSelector::UpdateInstancePropsForGraph(
    ChiFeature2GraphDesc* pFeatureGraphDesc,
    UINT32 instanceId,
    UINT32 cameraid)
{
    CDK_UNUSED_PARAM(instanceId);

    UINT numFeatureInstances = pFeatureGraphDesc->numFeatureInstances;

    for (UINT featureIdx = 0; featureIdx < numFeatureInstances; featureIdx++)
    {
        ChiFeature2InstanceDesc* pChiFeature2InstanceDesc = &pFeatureGraphDesc->pFeatureInstances[featureIdx];
        pChiFeature2InstanceDesc->pInstanceProps->cameraId   = cameraid;
    }

    for (UINT linkIdx = 0; linkIdx < pFeatureGraphDesc->numInternalLinks; linkIdx++)
    {
        ChiFeature2GraphInternalLinkDesc* pChiFeature2GraphInternalLinkDesc = &pFeatureGraphDesc->pInternalLinks[linkIdx];
        pChiFeature2GraphInternalLinkDesc->srcPortId.instanceProps.cameraId   = cameraid;
        pChiFeature2GraphInternalLinkDesc->sinkPortId.instanceProps.cameraId  = cameraid;
    }

    for (UINT linkIdx = 0; linkIdx < pFeatureGraphDesc->numExtSinkLinks; linkIdx++)
    {
        ChiFeature2GraphExtSinkLinkDesc* pChiFeature2GraphExtSinkLinkDesc = &pFeatureGraphDesc->pExtSinkLinks[linkIdx];
        pChiFeature2GraphExtSinkLinkDesc->portId.instanceProps.cameraId   = cameraid;
    }

    for (UINT linkIdx = 0; linkIdx < pFeatureGraphDesc->numExtSrcLinks; linkIdx++)
    {
        ChiFeature2GraphExtSrcLinkDesc* pChiFeature2GraphExtSrcLinkDesc = &pFeatureGraphDesc->pExtSrcLinks[linkIdx];
        pChiFeature2GraphExtSrcLinkDesc->portId.instanceProps.cameraId   = cameraid;
    }

}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiFeature2GraphSelector::GetAllFeatureGraphDescriptors
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID ChiFeature2GraphSelector::GetAllFeatureGraphDescriptors(
    FeatureGraphManagerConfig*             pConfig,
    GraphDescriptorTables*                 pGraphDescriptorTables,
    ChiFeature2NameMap&                    rFeatureNameToOpsMap)
{
    BOOL skipGraph              = FALSE;
    UINT cameraId               = pConfig->pCameraInfo->cameraId;

    for (UINT i = 0; i < pConfig->pCameraStreamConfig->numStreams; i++)
    {
        ChiStream* pStream = pConfig->pCameraStreamConfig->pChiStreams[i];
        camera3_stream_t* pCamera3Stream = reinterpret_cast<camera3_stream_t*>(pConfig->pCameraStreamConfig->pChiStreams[i]);

        if (TRUE == UsecaseSelector::IsRawInputStream(pCamera3Stream))
        {
            m_isRawInput = TRUE;
        }
    }

    // Find the cameraId in the cameraId set and get the corresponding list of FGDs
    for (auto &it : *pGraphDescriptorTables->pCameraIdDescriptorNameSet)
    {
        for (auto &cameraMapIt : m_cameraIdMap)
        {
            std::set<UINT> cameraIdSet = cameraMapIt.first;

            if ((cameraIdSet.find(cameraId) != cameraIdSet.end()) && (it.first == cameraMapIt.second))
            {
                std::set<const CHAR*> DescriptorNameSet = it.second;
                for (auto &descriptorName : DescriptorNameSet)
                {
                    // Clone graph descriptor for single camera graph
                    ChiFeature2GraphDesc featureGraphDesc =
                        pGraphDescriptorTables->pFeatureGraphDescriptorsMap->at(descriptorName);

                    if (FALSE == featureGraphDesc.isMultiCameraGraph)
                    {
                        for (UINT8 i = 0; i < m_pCameraInfo->numPhysicalCameras; i++)
                        {
                            ChiFeature2FGDKeysForClonedMap phyCamDesc;
                            phyCamDesc.pDescriptorName    = descriptorName;
                            phyCamDesc.cameraId           = m_pCameraInfo->ppDeviceInfo[i]->cameraId;
                            phyCamDesc.bMultiCamera       = FALSE;
                            phyCamDesc.featureFlags.value = 0;

                            for (UINT i = 0; i < featureGraphDesc.numFeatureInstances; i++)
                            {
                                const CHAR* pFeatureName = featureGraphDesc.pFeatureInstances[i].pFeatureDesc->pFeatureName;
                                for (auto it = rFeatureNameToOpsMap.begin(); it != rFeatureNameToOpsMap.end(); ++it)
                                {
                                    skipGraph = TRUE;
                                    if (!CdkUtils::StrCmp(it->first, pFeatureName))
                                    {
                                        skipGraph = FALSE;
                                        break;
                                    }
                                }
                            }
                            if (FALSE == skipGraph)
                            {
                                CHX_LOG_INFO("Insert single graph:cameraID %d->%s",
                                             phyCamDesc.cameraId,
                                             phyCamDesc.pDescriptorName);

                                // As BPS camera RealTime Feature will always be used for the specified cameraID
                                // (outside of QCFA scenarios) there is no need to add an additional GraphDescriptor
                                // to the selector map.
                                if (RealtimeEngineType_BPS == m_pCameraInfo->ppDeviceInfo[i]->pDeviceConfig->realtimeEngine)
                                {
                                    phyCamDesc.featureFlags.isBPSCamera = TRUE;
                                }

                                CallCloneGraphDescriptor(phyCamDesc, featureGraphDesc);
                                UpdateInstancePropsForGraph(m_clonedFeatureGraphDescriptorsMap[phyCamDesc], i, i);
                                if (TRUE == IsNonZSLSnapshotGraphNeeded(pConfig, descriptorName, i, &phyCamDesc.featureFlags))
                                {
                                    CHX_LOG_INFO("Insert single non-zsl graph: cameraID %d->%s, flags:%x",
                                        phyCamDesc.cameraId,
                                        phyCamDesc.pDescriptorName,
                                        phyCamDesc.featureFlags.value);
                                    CallCloneGraphDescriptor(phyCamDesc, featureGraphDesc);

                                    if ((TRUE == phyCamDesc.featureFlags.isNZSLSnapshot) &&
                                        (TRUE == phyCamDesc.featureFlags.isSWRemosaicSnapshot))
                                    {
                                        CHX_LOG_VERBOSE("Update RT feature descritor for sw remosaic snapshot");

                                        ChiFeature2GraphDesc* pGraphDesc = m_clonedFeatureGraphDescriptorsMap[phyCamDesc];

                                        for (UINT featureIdx = 0; featureIdx < pGraphDesc->numFeatureInstances; featureIdx++)
                                        {
                                            if (static_cast<UINT32>(ChiFeature2Type::REALTIME) ==
                                                pGraphDesc->pFeatureInstances[featureIdx].pFeatureDesc->featureId)
                                            {
                                                pGraphDesc->pFeatureInstances[featureIdx].pFeatureDesc =
                                                    &RealTimeFeatureWithSWRemosaicDescriptor;
                                            }
                                        }
                                    }
                                }

                                if (TRUE == m_isRawInput)
                                {
                                    ChiFeature2GraphDesc featureGraphDesc = Bayer2YUVFeatureGraphDescriptor;
                                    phyCamDesc.pDescriptorName = featureGraphDesc.pFeatureGraphName;
                                    CallCloneGraphDescriptor(phyCamDesc, featureGraphDesc);
                                }
                            }
                            else
                            {
                                CHX_LOG_INFO("Removing feature graph %s", featureGraphDesc.pFeatureGraphName);
                            }
                        }
                    }
                    else
                    {
                        CHX_LOG_INFO("Multi camera graph:%s", descriptorName);

                        ChiFeature2FGDKeysForClonedMap phyCamDesc;
                        phyCamDesc.pDescriptorName    = descriptorName;
                        phyCamDesc.cameraId           = m_pCameraInfo->cameraId;
                        phyCamDesc.bMultiCamera       = TRUE;
                        phyCamDesc.featureFlags.value = 0;

                        CHX_LOG_INFO("Insert multi graph:cameraID %d->%s",
                                     phyCamDesc.cameraId,
                                     phyCamDesc.pDescriptorName);

                        CallCloneGraphDescriptor(phyCamDesc, featureGraphDesc);
                    }
                }
            }
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiFeature2GraphSelector::GetFeatureGraphMapforConfig
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
keysToCloneDescMap ChiFeature2GraphSelector::GetFeatureGraphMapforConfig(
    FeatureGraphManagerConfig*  pConfig,
    FeatureGraphSelectorConfig& rSelectorOutput,
    GraphDescriptorTables*      pGraphDescriptorTables)
{
    if (NULL != pGraphDescriptorTables)
    {
        GetAllFeatureGraphDescriptors(pConfig, pGraphDescriptorTables, rSelectorOutput.featureNameToOpsMap);
    }

    rSelectorOutput.numFeatureGraphDescriptors = m_clonedFeatureGraphDescriptorsMap.size();

    return m_clonedFeatureGraphDescriptorsMap;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiFeature2GraphSelector::CloneExtSrsExtSinkLinkDesc
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ChiFeature2GraphSelector::CloneExtSrcExtSinkLinkDesc(
    const ChiFeature2GraphDesc* pSourceGraphDescriptor,
    ChiFeature2GraphDesc*       pDestGraphDesc)
{
    CDKResult result                = CDKResultSuccess;
    pDestGraphDesc->numExtSrcLinks  = pSourceGraphDescriptor->numExtSrcLinks;

    pDestGraphDesc->numExtSinkLinks = pSourceGraphDescriptor->numExtSinkLinks;
    pDestGraphDesc->pExtSinkLinks   =
            static_cast<ChiFeature2GraphExtSinkLinkDesc*>(
                    CHX_CALLOC(pSourceGraphDescriptor->numExtSinkLinks * sizeof (ChiFeature2GraphExtSinkLinkDesc)));

    if (NULL != pDestGraphDesc->pExtSinkLinks)
    {
        for (UINT linkIdx = 0; linkIdx < pSourceGraphDescriptor->numExtSinkLinks; linkIdx++)
        {
            ChiFeature2GraphExtSinkLinkDesc* pChiFeature2GraphExtSinkLinkDesc = &pDestGraphDesc->pExtSinkLinks[linkIdx];
            ChxUtils::Memcpy(pChiFeature2GraphExtSinkLinkDesc,
                             &pSourceGraphDescriptor->pExtSinkLinks[linkIdx],
                             sizeof(ChiFeature2GraphExtSinkLinkDesc));
        }
    }
    else
    {
        result = CDKResultENoMemory;
        CHX_LOG_ERROR("Allocation failed for pClonedGraphDesc->pInternalLinks");
    }

    pDestGraphDesc->pExtSrcLinks =
            static_cast<ChiFeature2GraphExtSrcLinkDesc*>(
                    CHX_CALLOC(pSourceGraphDescriptor->numExtSrcLinks * sizeof (ChiFeature2GraphExtSrcLinkDesc)));
    if (NULL != pDestGraphDesc->pExtSrcLinks)
    {
        for (UINT linkIdx = 0; linkIdx < pSourceGraphDescriptor->numExtSrcLinks; linkIdx++)
        {
            ChiFeature2GraphExtSrcLinkDesc* pChiFeature2GraphExtSrcLinkDesc = &pDestGraphDesc->pExtSrcLinks[linkIdx];
            ChxUtils::Memcpy(pChiFeature2GraphExtSrcLinkDesc,
                             &pSourceGraphDescriptor->pExtSrcLinks[linkIdx],
                             sizeof(ChiFeature2GraphExtSrcLinkDesc));
        }
    }
    else
    {
        result = CDKResultENoMemory;
        CHX_LOG_ERROR("Allocation failed for pClonedGraphDesc->pExtSrcLinks");
    }
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiFeature2GraphSelector::CloneFeatureGraphDescriptor
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
ChiFeature2GraphDesc* ChiFeature2GraphSelector::CloneFeatureGraphDescriptor(
    const ChiFeature2GraphDesc* pFeatureGraphDescriptor)
{
    CDKResult result = CDKResultSuccess;

    ChiFeature2GraphDesc* pClonedGraphDesc = static_cast<ChiFeature2GraphDesc*>(
        CHX_CALLOC(sizeof(ChiFeature2GraphDesc)));

    if (NULL != pClonedGraphDesc)
    {
        pClonedGraphDesc->numFeatureInstances = pFeatureGraphDescriptor->numFeatureInstances;
        pClonedGraphDesc->numInternalLinks    = pFeatureGraphDescriptor->numInternalLinks;

        pClonedGraphDesc->pFeatureInstances = static_cast<ChiFeature2InstanceDesc*>(
            CHX_CALLOC(pFeatureGraphDescriptor->numFeatureInstances * sizeof(ChiFeature2InstanceDesc)));

        if (NULL != pClonedGraphDesc->pFeatureInstances)
        {
            for (UINT featureIdx = 0; featureIdx < pFeatureGraphDescriptor->numFeatureInstances; featureIdx++)
            {
                ChiFeature2InstanceDesc* pChiFeature2InstanceDesc = &pClonedGraphDesc->pFeatureInstances[featureIdx];
                result = CloneFeature2InstanceDesc(&pFeatureGraphDescriptor->pFeatureInstances[featureIdx],
                                          pChiFeature2InstanceDesc);
                if (CDKResultSuccess != result)
                {
                    break;
                }
            }
        }
        else
        {
            result = CDKResultENoMemory;
            CHX_LOG_ERROR("Allocation failed for FeatureInstances");
        }

        if (CDKResultSuccess == result)
        {
            pClonedGraphDesc->pInternalLinks = static_cast<ChiFeature2GraphInternalLinkDesc*>(
                CHX_CALLOC(pFeatureGraphDescriptor->numInternalLinks * sizeof(ChiFeature2GraphInternalLinkDesc)));

            if (NULL != pClonedGraphDesc->pInternalLinks)
            {
                for (UINT linkIdx = 0; linkIdx < pFeatureGraphDescriptor->numInternalLinks; linkIdx++)
                {
                    ChiFeature2GraphInternalLinkDesc* pChiFeature2GraphInternalLinkDesc =
                        &pClonedGraphDesc->pInternalLinks[linkIdx];

                    ChxUtils::Memcpy(pChiFeature2GraphInternalLinkDesc,
                                     &pFeatureGraphDescriptor->pInternalLinks[linkIdx],
                                     sizeof(ChiFeature2GraphInternalLinkDesc));
                }
            }
            else
            {
                result = CDKResultENoMemory;
                CHX_LOG_ERROR("Allocation failed for pClonedGraphDesc->pInternalLinks");
            }

            if (CDKResultSuccess == result)
            {
                pClonedGraphDesc->pFeatureGraphName = pFeatureGraphDescriptor->pFeatureGraphName;
                result = CloneExtSrcExtSinkLinkDesc(pFeatureGraphDescriptor, pClonedGraphDesc);
            }
        }
    }
    else
    {
        result = CDKResultENoMemory;
        CHX_LOG_ERROR("Allocation failed for pClonedGraphDesc");
    }

    if (CDKResultSuccess != result)
    {
        FreeGraphDescResources(pClonedGraphDesc);
        pClonedGraphDesc = NULL;
    }

    // We can update some items in the featureInstanceProps, right after cloning the graph descriptor, like camera id.
    // It's better not to modify instanceProps(at lease static part of it) out side of this.

    return pClonedGraphDesc;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiFeature2GraphSelector::CloneFeature2InstanceDesc
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ChiFeature2GraphSelector::CloneFeature2InstanceDesc(
    ChiFeature2InstanceDesc*    pSourceFeature2InstanceDesc,
    ChiFeature2InstanceDesc*    pDestFeature2InstanceDesc)
{
    CDKResult result = CDKResultSuccess;
    if ((NULL != pSourceFeature2InstanceDesc) && (NULL != pDestFeature2InstanceDesc))
    {
        ChxUtils::Memcpy(pDestFeature2InstanceDesc,
                         pSourceFeature2InstanceDesc,
                         sizeof(ChiFeature2InstanceDesc));

        // Clone the instance props here also, incase same graph descriptors being used more than once.
        pDestFeature2InstanceDesc->pInstanceProps =
            static_cast<ChiFeature2InstanceProps*>(CHX_CALLOC(sizeof(ChiFeature2InstanceProps)));

        if (NULL != pDestFeature2InstanceDesc->pInstanceProps)
        {
            ChxUtils::Memcpy(pDestFeature2InstanceDesc->pInstanceProps,
                             pSourceFeature2InstanceDesc->pInstanceProps,
                             sizeof(ChiFeature2InstanceProps));
        }
        else
        {
            CHX_LOG_ERROR("Allocate pInstanceProps failed!");
            result = CDKResultENoMemory;
        }
    }
    else
    {
        CHX_LOG_ERROR("Invalid Arg! pSourceFeature2InstanceDesc=%p, pDestFeature2InstanceDesc=%p",
                      pSourceFeature2InstanceDesc, pDestFeature2InstanceDesc);
        result = CDKResultEInvalidArg;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiFeature2GraphSelector::AddCustomFeatureGraphNodeHints
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID ChiFeature2GraphSelector::AddCustomFeatureGraphNodeHints(
    ChiFeature2UsecaseRequestObject*             pRequestObject,
    std::vector<ChiFeature2InstanceRequestInfo>& rFeatureInstanceReqInfoList)
{
    ChiFeature2Hint featureHint         = { 0 };
    UINT            numFeatureInstances = rFeatureInstanceReqInfoList.size();

    for (UINT8 featureIndex = 0; featureIndex < numFeatureInstances; featureIndex++)
    {
        UINT32          featureId   = rFeatureInstanceReqInfoList[featureIndex].pFeatureBase->GetFeatureId();
        ChiFeature2Type featureType = static_cast<ChiFeature2Type>(featureId);

        const CHAR* pFeatureName = rFeatureInstanceReqInfoList[featureIndex].pFeatureBase->GetFeatureName();

        ChxUtils::Memset(&(rFeatureInstanceReqInfoList[featureIndex].featureHint),
                         0,
                         sizeof(rFeatureInstanceReqInfoList[featureIndex].featureHint));
        GetFeatureHint(featureType, pRequestObject, &(rFeatureInstanceReqInfoList[featureIndex].featureHint));
        CHX_LOG_INFO("Feature hint for %s: %d is %d",
                     (NULL != pFeatureName) ? pFeatureName : "NULL", featureId,
                     rFeatureInstanceReqInfoList[featureIndex].featureHint.numFrames);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiFeature2GraphSelector::IsCaptureIntentForSnapshot
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL ChiFeature2GraphSelector::IsCaptureIntentForSnapshot(
    const FGDKeysForCaptureRequest* pKeysForRequest)
{
    BOOL snapshotIntent = FALSE;

    if ((ControlCaptureIntentZeroShutterLag == pKeysForRequest->captureIntent) ||
        (ControlCaptureIntentStillCapture   == pKeysForRequest->captureIntent) ||
        (ControlCaptureIntentManual         == pKeysForRequest->captureIntent) ||
        (ControlCaptureIntentVideoSnapshot  == pKeysForRequest->captureIntent))
    {
        snapshotIntent = TRUE;
    }

    return snapshotIntent;
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiFeature2GraphSelector::GetCustomVendorTag
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UINT32 ChiFeature2GraphSelector::GetCustomVendorTag(
    ChiMetadata* pAppSettings,
    const FGDKeysForCaptureRequest* pKeysForRequest)
{
    BOOL   enableRawHDR           = FALSE;
    UINT32 customTagValue         = 0;
    UINT32 customNoiseReduction   = 0;
    UINT32 burstShotFps           = 0;
    camera_metadata_entry_t entry = { 0 };
    INT32 status                  = -1;
    m_isBurstShotEnabled          = FALSE;

    if (TRUE == IsCaptureIntentForSnapshot(pKeysForRequest))
    {
        enableRawHDR = ExtensionModule::GetInstance()->EnableRawHDRSnapshot();

        if ((TRUE == enableRawHDR) && (ControlSceneModeHDR == GetSceneMode(pAppSettings)))
        {
            customTagValue = CustomVendorTagValues::CustomVendorTagRawReprocessing;
        }
        else
        {
            entry.tag = ExtensionModule::GetInstance()->GetVendorTagId(VendorTag::CustomNoiseReduction);
            status = pAppSettings->FindTag(entry.tag, &entry);
            if (0 == status)
            {
                customNoiseReduction = static_cast<UINT32>(*(entry.data.u8));
                customTagValue = CustomVendorTagValues::CustomVendorTagMFNR;
                CHX_LOG_INFO("Custom Noise Reduction %d", customNoiseReduction);
            }
        }

        entry.tag = ExtensionModule::GetInstance()->GetVendorTagId(VendorTag::BurstFps);
        status = pAppSettings->FindTag(entry.tag, &entry);
        if (0 == status)
        {
            burstShotFps = static_cast<UINT32>(*(entry.data.u8));
            if (1 == burstShotFps)
            {
                m_isBurstShotEnabled = TRUE;
            }
            CHX_LOG_INFO("Burst mode selected %d", burstShotFps);
        }

        CHX_LOG_INFO("custom vendor tag %d", customTagValue);
    }

    return customTagValue;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiFeature2GraphSelector::GetCaptureIntentForRequest
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UINT32 ChiFeature2GraphSelector::GetCaptureIntentForRequest(
    ChiMetadata*       pAppSettings,
    CHICAPTUREREQUEST* pChiRequest)
{
    camera_metadata_entry_t entry = { 0 };
    INT32 status        = -1;
    INT32 captureIntent = -1;

    entry.tag = ANDROID_CONTROL_CAPTURE_INTENT;
    status = pAppSettings->FindTag(entry.tag, &entry);

    if (0 == status)
    {
        INT captureIntentValue = static_cast<INT32>(*(entry.data.i32));
        CHX_LOG_INFO("Capture Intent value %d", captureIntentValue);

        // Fallback to StillCapture, as we are not handling CustomIntent
        if (ControlCaptureIntentCustom == captureIntentValue)
        {
            CHX_LOG_INFO("Overriding capture intent : %d with %d",
                captureIntentValue, ControlCaptureIntentStillCapture);
            captureIntentValue = ControlCaptureIntentStillCapture;
        }


        if (CaptureIntentAll.find(captureIntentValue) != CaptureIntentAll.end())
        {
            captureIntent = captureIntentValue;
            // For pure snapshot(blob) request in config stream capture intent should be still capture.
            if ((captureIntent == ControlCaptureIntentPreview) ||
                (captureIntent == ControlCaptureIntentZeroShutterLag))
            {
                for (UINT i = 0; i < pChiRequest->numOutputs; i++)
                {
                    if (pChiRequest->pOutputBuffers[i].pStream->format == ChiStreamFormatBlob)
                    {
                        captureIntent = ControlCaptureIntentStillCapture;
                        CHX_LOG_INFO("overriding capture intent: previous %d with %d ",
                            ControlCaptureIntentZeroShutterLag, captureIntent);
                        break;
                    }
                }
            }
            else if ((captureIntent == ControlCaptureIntentStillCapture) ||
                     (captureIntent == ControlCaptureIntentVideoSnapshot))
            {
                BOOL isRealtimeStream = FALSE;
                BOOL isOfflineStream  = FALSE;
                for (UINT streamIndex = 0; streamIndex < pChiRequest->numOutputs; ++streamIndex)
                {
                    ChiStream*        pStream   = pChiRequest->pOutputBuffers[streamIndex].pStream;
                    camera3_stream_t* pFWStream = reinterpret_cast<camera3_stream_t*>(pStream);

                    if ((TRUE == UsecaseSelector::IsYUVSnapshotStream(pFWStream) &&
                        (TRUE == Is4KYUVOut(pStream))) ||
                        (TRUE == UsecaseSelector::IsHEIFStream(pFWStream)) ||
                        (TRUE == UsecaseSelector::IsJPEGSnapshotStream(pFWStream)))
                    {
                        isOfflineStream = TRUE;
                    }

                    if ((TRUE == UsecaseSelector::IsPreviewStream(pFWStream)) ||
                        (TRUE == UsecaseSelector::IsVideoStream(pFWStream)) ||
                        (TRUE == UsecaseSelector::IsRawStream(pFWStream)) ||
                        (TRUE == UsecaseSelector::IsYUVSnapshotStream(pFWStream) &&
                        (FALSE == Is4KYUVOut(pStream))))
                    {
                        isRealtimeStream = TRUE;
                    }
                }

                if ((FALSE == isOfflineStream) && (TRUE == isRealtimeStream))
                {
                    captureIntent = ControlCaptureIntentPreview;
                }
            }
        }
        else
        {
            CHX_LOG_WARN("Failed to find capture Intent");
        }
    }
    CHX_LOG_INFO("Capture Intent value returned %d", captureIntent);
    return captureIntent;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiFeature2GraphSelector::GetNoiseReductionMode
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UINT32 ChiFeature2GraphSelector::GetNoiseReductionMode(
    ChiMetadata* pAppSettings)
{
    camera_metadata_entry_t entry = { 0 };
    INT32 status = -1;
    INT32 controlNoiseReductionMode = -1;

    entry.tag = ANDROID_NOISE_REDUCTION_MODE;
    status = pAppSettings->FindTag(entry.tag, &entry);

    if (0 == status)
    {
        INT32 controlNoiseReductionModeValue = static_cast<INT32>(*(entry.data.i32));
        CHX_LOG_INFO("Noise Reduction Mode %d", controlNoiseReductionModeValue);

        if (noiseReductionmodeAll.find(controlNoiseReductionModeValue) != noiseReductionmodeAll.end())
        {
            controlNoiseReductionMode = controlNoiseReductionModeValue;
        }
        else
        {
            CHX_LOG_WARN("Failed to find a noise mode");
        }
    }

    return controlNoiseReductionMode;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiFeature2GraphSelector::GetSceneMode
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UINT32 ChiFeature2GraphSelector::GetSceneMode(
    ChiMetadata* pAppSettings)
{
    camera_metadata_entry_t entry = { 0 };
    INT32 status = -1;
    INT32 controlSceneMode = -1;

    entry.tag = ANDROID_CONTROL_SCENE_MODE;
    status = pAppSettings->FindTag(entry.tag, &entry);

    if (0 == status)
    {
        INT32 controlSceneModeValue = static_cast<INT32>(*(entry.data.i32));
        CHX_LOG_INFO("Scene Mode %d", controlSceneModeValue);

        if (SceneModeAll.find(controlSceneModeValue) != SceneModeAll.end())
        {
            controlSceneMode = controlSceneModeValue;
        }
        else
        {
            CHX_LOG_WARN("Failed to find scene mode");
        }

        // if controlSceneModeValue is zero, assign default value
        if (0 == controlSceneMode)
        {
            controlSceneMode = ControlSceneModeFacePriority;
        }
    }

    return controlSceneMode;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiFeature2GraphSelector::Dump
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID ChiFeature2GraphSelector::Dump(
    GraphDescriptorTables* pGraphDescriptorTables)
{
    for (auto &it : *pGraphDescriptorTables->pCameraIdDescriptorNameSet)
    {
        std::set<const CHAR *> descriptorNameSet = it.second;
        for (auto nameIt : descriptorNameSet)
        {
            CHX_LOG_INFO("%pK: m_cameraIdDescriptorNameSet Descriptor Name : %s", pGraphDescriptorTables, nameIt);
        }
    }

    for (auto &it : *pGraphDescriptorTables->pFeatureGraphDescriptorsMap)
    {
        CHX_LOG_INFO("%pK : m_featureGraphDescriptorsMap Descriptor Name : %s", pGraphDescriptorTables,  it.first);
    }

    for (auto &it : *pGraphDescriptorTables->pFeatureGraphDescKeysMap)
    {
        CHX_LOG_INFO("%pK : m_FDGKeysMap Descriptor Name : %s", pGraphDescriptorTables, it.pDescriptorName);
    }

}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiFeature2GraphSelector::GetNZSLSnapshotFlag
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
ChiFeature2InstanceFlags ChiFeature2GraphSelector::GetNZSLSnapshotFlag(
    ChiMetadata* pAppSetting,
    ChiMetadata* pResultMeta)
{
    CDK_UNREFERENCED_PARAM(pAppSetting);
    const FLOAT AECGainThreshold = ExtensionModule::GetInstance()->AECGainThresholdForQCFA();
    ChiFeature2InstanceFlags flags              = {{0}};

    // Check AEC gain for qcfa usecase, if aec gain is less than threshold, trigger non-zsl snapshot
    if (TRUE == m_isQCFAUsecase)
    {
        if (NULL != pResultMeta)
        {
            AECFrameControl* pFrameControl =
                static_cast<AECFrameControl*>(pResultMeta->GetTag("org.quic.camera2.statsconfigs",
                                                                  "AECFrameControl"));
            if (NULL != pFrameControl)
            {
                FLOAT curAecGain = pFrameControl->exposureInfo[ExposureIndexSafe].linearGain;

                CHX_LOG_VERBOSE("QCFA fullsize snapshot AECGainThreshold:%f, curAecGain:%f",
                   AECGainThreshold, curAecGain);
                if (AECGainThreshold > curAecGain)
                {
                    flags.isNZSLSnapshot = TRUE;

                    if (CHIREMOSAICTYPE::HWRemosaic == m_remosaicType)
                    {
                        flags.isHWRemosaicSnapshot = TRUE;
                    }
                    else if (CHIREMOSAICTYPE::SWRemosaic == m_remosaicType)
                    {
                        flags.isSWRemosaicSnapshot = TRUE;
                    }
                }

                CHX_LOG_INFO("AEC Gain received: %f, nzsl flags: %x", curAecGain, flags.value);
            }
            else
            {
                CHX_LOG_WARN("Can't get AECFrameControl from metadata");
            }
        }
        else
        {
            CHX_LOG_WARN("pResultMeta is NULL");
        }
    }

    return flags;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiFeature2GraphSelector::IsNonZSLSnapshotGraphNeeded
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL ChiFeature2GraphSelector::IsNonZSLSnapshotGraphNeeded(
    const FeatureGraphManagerConfig* pConfig,
    const CHAR*                      pGraphName,
    UINT32                           phyCamIdx,
    ChiFeature2InstanceFlags*        pInstanceFlags)
{
    BOOL                     needNonZSLSnapshot = FALSE;
    ChiFeature2InstanceFlags flags              = {{0}};

    if ((NULL == pConfig) || (NULL == pGraphName))
    {
        CHX_LOG_ERROR("Invalid input parameters.");
    }
    else if (!CdkUtils::StrCmp(pGraphName, "RealtimeFG"))
    {
        // always return false for preview feature graph
        CHX_LOG_VERBOSE("Skip check for preview graph");
    }
    else
    {
        // For quadcfa sensor, if the requested snapshot size (jpeg or snapshot yuv) is larger than sensor's binning size,
        // then non-zsl snapshot is required, with HWRemosaic or SWRemosaic type.
        if ((LogicalCameraType_Default == pConfig->pCameraInfo->logicalCameraType) &&
            (TRUE == UsecaseSelector::IsQuadCFASensor(pConfig->pCameraInfo, &m_remosaicType)))
        {
            camera3_stream_configuration_t streamConfig = { 0 };
            streamConfig.num_streams    = pConfig->pCameraStreamConfig->numStreams;
            streamConfig.streams        = reinterpret_cast<camera3_stream_t**>(pConfig->pCameraStreamConfig->pChiStreams);
            streamConfig.operation_mode = pConfig->pCameraStreamConfig->operationMode;

            if (TRUE == UsecaseSelector::QuadCFAMatchingUsecase(pConfig->pCameraInfo, &streamConfig))
            {
                needNonZSLSnapshot = TRUE;
                m_isQCFAUsecase    = TRUE;

                flags.isNZSLSnapshot = TRUE;

                if (CHIREMOSAICTYPE::HWRemosaic == m_remosaicType)
                {
                    flags.isHWRemosaicSnapshot = TRUE;
                }
                else if (CHIREMOSAICTYPE::SWRemosaic == m_remosaicType)
                {
                    flags.isSWRemosaicSnapshot = TRUE;
                }

                if (RealtimeEngineType_BPS == m_pCameraInfo->ppDeviceInfo[0]->pDeviceConfig->realtimeEngine)
                {
                    flags.isBPSCamera = TRUE;
                }

                CHX_LOG_VERBOSE("Graph:%s, remosaicType: %d, physical cameraId: %d, flags: %x",
                    pGraphName, m_remosaicType, pConfig->pCameraInfo->ppDeviceInfo[phyCamIdx]->cameraId, flags.value);
            }
        }
    }

    if ((TRUE == needNonZSLSnapshot) && (NULL != pInstanceFlags))
    {
        *pInstanceFlags = flags;
    }

    return needNonZSLSnapshot;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiFeature2GraphSelector::~ChiFeature2GraphSelector
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
ChiFeature2GraphSelector::~ChiFeature2GraphSelector()
{

}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiFeature2GraphSelector::BuildCameraIdSet
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID ChiFeature2GraphSelector::BuildCameraIdSet()
{
    const LogicalCameraInfo* pLogicalCameraInfo = ExtensionModule::GetInstance()->GetAllLogicalCameraInfo();
    UINT numLogicalCameraInfo                   = ExtensionModule::GetInstance()->GetNumberOfLogicalConfig();

    std::set<UINT> cameraIdSetSingle = {};
    std::set<UINT> cameraIdSetMulti  = {};
    std::set<UINT> cameraIdSetBokeh  = {};
    std::set<UINT> cameraIdSetFusion = {};

    for (UINT i = 0; i < numLogicalCameraInfo; i++)
    {
        switch (pLogicalCameraInfo[i].logicalCameraType)
        {
            case LogicalCameraType_Default:
                cameraIdSetSingle.insert(pLogicalCameraInfo[i].cameraId);
                break;
            case LogicalCameraType_RTB:
            case LogicalCameraType_VR:
            case LogicalCameraType_SAT:
            case LogicalCameraType_BAYERMONO:
            case LogicalCameraType_DualApp:
                if (LogicalCameraType_SAT == pLogicalCameraInfo[i].logicalCameraType)
                {
                    cameraIdSetFusion.insert(pLogicalCameraInfo[i].cameraId);
                }

                if (LogicalCameraType_RTB == pLogicalCameraInfo[i].logicalCameraType)
                {
                    cameraIdSetBokeh.insert(pLogicalCameraInfo[i].cameraId);
                }

                cameraIdSetMulti.insert(pLogicalCameraInfo[i].cameraId);
                break;
            default:
                CHX_LOG_ERROR("Cannot map logical camera type %d", pLogicalCameraInfo[i].logicalCameraType);
                break;
        }
    }

    m_cameraIdMap.insert({ { cameraIdSetSingle },  SINGLE_CAMERA });
    m_cameraIdMap.insert({ { cameraIdSetMulti },   MULTI_CAMERA });
    m_cameraIdMap.insert({ { cameraIdSetBokeh },   BOKEH_CAMERA });
    m_cameraIdMap.insert({ { cameraIdSetFusion },  FUSION_CAMERA });

    for (auto &cameraMapIt : m_cameraIdMap)
    {
        std::set<UINT> cameraIdSet = cameraMapIt.first;
        for (auto &cameraId : cameraIdSet)
        {
            CHX_LOG_INFO("cameraId %d, set %d", cameraId, cameraMapIt.second);
        }
    }

}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CreateFeatureGraphSelector
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
ChiFeature2GraphSelector* CreateFeatureGraphSelector(
    FeatureGraphManagerConfig* pCreateInputInfo,
    std::set<const CHAR *>&    rFeatureDescNameSet)
{
    ChiFeature2GraphSelector* pFeatureGraphSelector = ChiFeature2GraphSelectorOEM::Create(pCreateInputInfo,
                                                                                          rFeatureDescNameSet);
    return pFeatureGraphSelector;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// DoQueryCaps
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID* DoQueryCaps(
    VOID* pPrivateData)
{
    CDK_UNUSED_PARAM(pPrivateData);

    return NULL;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// GetFeatureGraphMapforConfig
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
keysToCloneDescMap GetFeatureGraphMapforConfig(
    FeatureGraphManagerConfig*        pConfig,
    FeatureGraphSelectorConfig& rSelectorOutput,
    VOID*                             pPrivateData)
{
    ChiFeature2GraphSelector* pSelector = static_cast<ChiFeature2GraphSelector*>(pPrivateData);
    return pSelector->GetFeatureGraphMapforConfig(pConfig,
                                                  rSelectorOutput,
                                                  pSelector->GetGraphDescriptorTables());
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// SelectFeatureGraphforRequest
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
ChiFeature2GraphDesc* SelectFeatureGraphforRequest(
    ChiFeature2UsecaseRequestObject* pChiUsecaseRequestObject,
    std::map<UINT32, ChiMetadata*>   pMetadataFrameNumberMap,
    VOID*                            pPrivateData)
{
    ChiFeature2GraphSelector* pSelector = static_cast<ChiFeature2GraphSelector*>(pPrivateData);
    return pSelector->SelectFeatureGraphforRequest(pChiUsecaseRequestObject, pMetadataFrameNumberMap);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// AddCustomFeatureGraphNodeHints
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID AddCustomFeatureGraphNodeHints(
    ChiFeature2UsecaseRequestObject*             pRequestObject,
    std::vector<ChiFeature2InstanceRequestInfo>& rFeatureInstanceReqInfoList,
    VOID*                                        pPrivateData)
{
    ChiFeature2GraphSelector* pSelector = static_cast<ChiFeature2GraphSelector*>(pPrivateData);
    pSelector->AddCustomFeatureGraphNodeHints(pRequestObject, rFeatureInstanceReqInfoList);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Destroy
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID Destroy(
    VOID* pPrivateData)
{
    ChiFeature2GraphSelector* pSelector = static_cast<ChiFeature2GraphSelector*>(pPrivateData);
    pSelector->Destroy();
    pSelector = NULL;
}

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiFeature2GraphSelectorOpsEntry
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID ChiFeature2GraphSelectorOpsEntry(
    CHIFEATURE2GRAPHSELECTOROPS* pChiFeature2GraphSelectorOps)
{
    if (NULL != pChiFeature2GraphSelectorOps)
    {
        pChiFeature2GraphSelectorOps->size                 = sizeof(CHIFEATURE2GRAPHSELECTOROPS);
        pChiFeature2GraphSelectorOps->pCreate              = CreateFeatureGraphSelector;
        pChiFeature2GraphSelectorOps->pGetFGDListForConfig = GetFeatureGraphMapforConfig;
        pChiFeature2GraphSelectorOps->pSelectFGD           = SelectFeatureGraphforRequest;
        pChiFeature2GraphSelectorOps->pAddCustomHints      = AddCustomFeatureGraphNodeHints;
        pChiFeature2GraphSelectorOps->pQueryCaps           = DoQueryCaps;
        pChiFeature2GraphSelectorOps->pDestroy             = Destroy;
    }
}
#ifdef __cplusplus
}
#endif // __cplusplus
