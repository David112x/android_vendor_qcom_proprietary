////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2019-2020 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  chifeature2graphselectoroem.cpp
/// @brief CHI feature2 derived graph selector implementation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "chifeature2graphselectoroem.h"

// NOWHINE FILE NC009:  CHI files will start with CHI
// NOWHINE FILE CP006:  Need whiner update: std::vector allowed in exceptional cases
// NOWHINE ENTIRE FILE - Temporarily bypassing for existing CHI files, required for table

extern std::set<UINT> CaptureIntentAll;
extern std::set<UINT> SceneModeAll;
extern std::set<UINT> noiseReductionmodeAll;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiFeature2graphSelectorlito::Create
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
ChiFeature2GraphSelectorOEM* ChiFeature2GraphSelectorOEM::Create(
    FeatureGraphManagerConfig* pConfig,
    std::set<const CHAR*>&     rFeatureDescNameSet)
{
    ChiFeature2GraphSelectorOEM* pFeatureGraphSelector = CHX_NEW(ChiFeature2GraphSelectorOEM);

    pFeatureGraphSelector->PopulateAllTablesOEM();

    rFeatureDescNameSet = pFeatureGraphSelector->m_featureDescNameSet;

    pFeatureGraphSelector->Initialize(pConfig);

    return pFeatureGraphSelector;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiFeature2GraphSelector::GetFeatureGraphMapforConfig
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
keysToCloneDescMap ChiFeature2GraphSelectorOEM::GetFeatureGraphMapforConfig(
    FeatureGraphManagerConfig*  pConfig,
    FeatureGraphSelectorConfig& rSelectorOutput,
    GraphDescriptorTables*      pGraphDescriptorTables)
{
    CDK_UNUSED_PARAM(pGraphDescriptorTables);

    GraphDescriptorTables* pGraphDescriptorTablesOEM = GetGraphDescriptorTables();

    return ChiFeature2GraphSelector::GetFeatureGraphMapforConfig(pConfig,
                                                                 rSelectorOutput,
                                                                 pGraphDescriptorTablesOEM);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiFeature2GraphSelectorOEM::PopulateAllTablesOEM
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID ChiFeature2GraphSelectorOEM::PopulateAllTablesOEM()
{
    /// @brief set of cameraIds and its corresponding list of Feature graph Descriptors
    GraphDescriptorTables* pGraphDescriptorTables = GetGraphDescriptorTables();

    if (NULL != pGraphDescriptorTables)
    {
        *pGraphDescriptorTables->pCameraIdDescriptorNameSet =
        {
        };

        /// @brief mapping feature graph descriptor name to feature graph descriptor
        *pGraphDescriptorTables->pFeatureGraphDescriptorsMap =
        {
        };

        /// @brief Keys to Feature Graph Descriptor map table
        *pGraphDescriptorTables->pFeatureGraphDescKeysMap =
        {

        };
    }

    Dump(GetGraphDescriptorTables());

    PopulateAllTables();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiFeature2GraphSelectorOEM::SelectFeatureGraphforRequest
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
ChiFeature2GraphDesc* ChiFeature2GraphSelectorOEM::SelectFeatureGraphforRequest(
    ChiFeature2UsecaseRequestObject* pChiUsecaseRequestObject,
    std::map<UINT32, ChiMetadata*>   pMetadataFrameNumberMap)
{
    GraphDescriptorTables* pGraphDescriptorTables = GetGraphDescriptorTables();
    return ChiFeature2GraphSelector::SelectFeatureGraphforRequestFromTable(pChiUsecaseRequestObject,
                                                                           pMetadataFrameNumberMap,
                                                                           pGraphDescriptorTables);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiFeature2GraphSelectorOEM::~ChiFeature2GraphSelectorOEM
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
ChiFeature2GraphSelectorOEM::~ChiFeature2GraphSelectorOEM()
{

}
