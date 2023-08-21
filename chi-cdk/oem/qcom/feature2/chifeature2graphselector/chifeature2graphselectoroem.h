////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  chifeature2graphselectoroem.h
/// @brief CHI feature graph selector base class declarations
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#ifndef CHIFEATURE2GRAPHSELECTOROEM_H
#define CHIFEATURE2GRAPHSELECTOROEM_H

#include "chifeature2graphselector.h"

extern const ChiFeature2GraphDesc RTRawHDRBayer2YUVJPEGFeatureGraphDescriptor;
extern const ChiFeature2GraphDesc MultiCameraBokehFeatureLitoSuperGraphDescriptor;
extern const ChiFeature2GraphDesc MultiCameraFusionFeatureLitoSuperGraphDescriptor;

// NOWHINE FILE CP006:  Need whiner update: std::vector allowed in exceptional cases

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief graph selector derived class
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class ChiFeature2GraphSelectorOEM : public ChiFeature2GraphSelector
{
public:
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// Create
    ///
    /// @brief  Create feature graph selector object
    ///
    /// @param  pConfig             Config Information required to create a feature graph.
    /// @param  rFeatureDescNameSet feature descriptor name set
    ///
    /// @return ChiFeature2GraphSelector object on successful creation
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static ChiFeature2GraphSelectorOEM* Create(
        FeatureGraphManagerConfig* pConfig,
        std::set<const CHAR*>&     rFeatureDescNameSet);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetFeatureGraphMapforConfig
    ///
    /// @brief  method to get a feature graph on a request basis
    ///
    /// @param  pConfig                 Config Information to get list of feature graph.
    /// @param  rSelectorOutput         feature graph selector config data
    /// @param  pGraphDescriptorTables  All the graph tables required to pick graph descriptors
    ///
    /// @return ChiFeature2GraphDesc Map of feature graphs supported for this config
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual keysToCloneDescMap GetFeatureGraphMapforConfig(
        FeatureGraphManagerConfig*  pConfig,
        FeatureGraphSelectorConfig& rSelectorOutput,
        GraphDescriptorTables*      pGraphDescriptorTables);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// PopulateAllTablesOEM
    ///
    /// @brief  Method to populate the keys map, <cameraid, descriptorName>.
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID PopulateAllTablesOEM();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// SelectFeatureGraphforRequest
    ///
    /// @brief  Method to create an instance of the feature graph descriptor per physical device.
    ///
    /// @param  pChiUsecaseRequestObject Usecase request object
    /// @param  pMetadataFrameNumberMap  result metadata map for each frame number
    ///
    /// @return CDKResultSuccess if successful or CDK error values in case of error.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual ChiFeature2GraphDesc* SelectFeatureGraphforRequest(
        ChiFeature2UsecaseRequestObject* pChiUsecaseRequestObject,
        std::map<UINT32, ChiMetadata*>   pMetadataFrameNumberMap);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ChiFeature2GraphSelectorOEM
    ///
    /// @brief  Default constructor.
    ///
    /// @return None.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    ChiFeature2GraphSelectorOEM() = default;

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ~ChiFeature2GraphSelectorOEM
    ///
    /// @brief  Destructor.
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    ~ChiFeature2GraphSelectorOEM();

private:
    std::set<const CHAR*>                        m_featureDescNameSet;                     ///< List of feature Descriptors

    ChiFeature2GraphSelectorOEM(const ChiFeature2GraphSelectorOEM&) = delete;              ///< Disallow the copy constructor
    ChiFeature2GraphSelectorOEM& operator= (const ChiFeature2GraphSelectorOEM&) = delete;  ///< Disallow assignment operator
};

#endif // CHIFEATURE2GRAPHSELECTOROEM_H
