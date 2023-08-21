////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  chifeature2fusion.h
/// @brief CHI fusion feature derived class declarations
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef CHIFEATURE2FUSION_H
#define CHIFEATURE2FUSION_H

#include "chifeature2base.h"

// NOWHINE FILE CP006:  Need whiner update: std::vector allowed in exceptional cases

static CHAR physicalCameraIdName[MaxCameras][MaxStringLength64] = {};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief Feature derived class
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class CDK_VISIBILITY_PUBLIC ChiFeature2Fusion : public ChiFeature2Base
{
public:

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// Create
    ///
    /// @brief  Static function to create RealTime feature
    ///
    /// @param  pCreateInputInfo   Pointer to create input info for RealTime feature
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static ChiFeature2Fusion* Create(
        ChiFeature2CreateInputInfo* pCreateInputInfo);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// Destroy
    ///
    /// @brief  Destructor for fusion feature.
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual VOID Destroy();

protected:
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// OnPrepareRequest
    ///
    /// @brief  Virtual method to prepare request for Fusion feature.
    ///
    /// @param  pRequestObject  Feature request object instance.
    ///
    /// @return CDKResultSuccess if successful or CDK error values in case of error.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual CDKResult OnPrepareRequest(
        ChiFeature2RequestObject* pRequestObject) const;

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// OnPopulateDependency
    ///
    /// @brief  populate dependency as per stage descriptor
    ///         Derived features can override this to set its own feature setting and populating logic
    ///
    /// @param  pRequestObject  Feature request object instance.
    ///
    /// @return CDKResultSuccess if successful or CDK error values in case of error.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual CDKResult OnPopulateDependency(
        ChiFeature2RequestObject* pRequestObject) const;

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// DoCleanupRequest
    ///
    /// @brief  Virtual method to cleanup request for Fusion feature.
    ///
    /// @param  pRequestObject  Feature request object instance.
    ///
    /// @return CDKResultSuccess if successful or CDK error values in case of error.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual CDKResult DoCleanupRequest(
        ChiFeature2RequestObject* pRequestObject) const;

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// DoFlush
    ///
    /// @brief  Virtual method to flush request for Fusion feature.
    ///
    /// @return CDKResultSuccess if successful or CDK error values in case of error.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual CDKResult DoFlush();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// OnSelectFlowToExecuteRequest
    ///
    /// @brief  Virtual method to select base provided request flow
    ///
    /// @param  pRequestObject  Feature request object instance.
    ///
    /// @return Selected RequestFlowType
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual ChiFeature2RequestFlowType OnSelectFlowToExecuteRequest(
        ChiFeature2RequestObject* pRequestObject) const;

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// OnPopulateDependencySettings
    ///
    /// @brief  populate Feature Settings
    ///
    /// @param  pRequestObject    Feature request object instance.
    /// @param  dependencyIndex   Dependency index
    /// @param  pSettingPortId    Metadata port id
    /// @param  pFeatureSettings  Metadata Setting
    ///
    /// @return CDKResultSuccess if successful or CDK error values in case of error.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual CDKResult OnPopulateDependencySettings(
        ChiFeature2RequestObject*     pRequestObject,
        UINT8                         dependencyIndex,
        const ChiFeature2Identifier*  pSettingPortId,
        ChiMetadata*                  pFeatureSettings
        ) const;

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// OnPipelineCreate
    ///
    /// @brief  Function to publish pipeline created.
    ///         Derived features can override this to update pipeline data structures.
    ///
    /// @param  pKey Pipeline global Id
    ///
    /// @return CDKResultSuccess if successful or CDK error values in case of error.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CDKResult OnPipelineCreate(
        ChiFeature2Identifier* pKey);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// OnPortCreate
    ///
    /// @brief  Function assign to max buffers for port
    ///
    /// @param  pKey   Key pointer
    ///
    /// @return CDKResultSuccess if successful or CDK error values in case of error
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CDKResult OnPortCreate(
        ChiFeature2Identifier* pKey);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// OnPopulateConfiguration
    ///
    /// @brief  The base feature implementation populates request object configuration for current stage
    ///         Derived class can override this function
    ///
    /// @param  pRequestObject      Feature request object containing the configuration information for request submission.
    ///
    /// @return CDKResultSuccess if successful or CDK error values in case of error.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CDKResult OnPopulateConfiguration(
        ChiFeature2RequestObject* pRequestObject
        ) const;


    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// OnPopulateConfigurationSettings
    ///
    /// @brief  The base feature implementation populates request object configuration for current stage
    ///         Derived class can override this.
    ///
    /// @param  pRequestObject      Feature request object containing the configuration information for request submission.
    /// @param  pMetadataPortId     Metadata portId
    /// @param  pInputMetadata      Input metadata setting
    ///
    /// @return CDKResultSuccess if successful or CDK error values in case of error.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CDKResult OnPopulateConfigurationSettings(
        ChiFeature2RequestObject*     pRequestObject,
        const ChiFeature2Identifier*  pMetadataPortId,
        ChiMetadata*                  pInputMetadata
        ) const;


    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// OnPrepareInputSettings
    ///
    /// @brief  The base feature implementation which prepares input settings from
    ///         upstream output metadata.
    ///
    /// @param  pInputMetadata              Input metadata setting
    /// @param  pUpstreamResultMetadata     Upstream feature output settings
    ///
    /// @return CDKResultSuccess if successful or CDK error values in case of error.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual CDKResult OnPrepareInputSettings(
        ChiMetadata*                  pInputMetadata,
        ChiMetadata*                  pUpstreamResultMetadata
        ) const;

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// OnPreparePipelineCreate
    ///
    /// @brief  Function updates pipeline specific datastructures before creating camx pipeline
    ///
    /// @param  pKey Pipeline global Id
    ///
    /// @return CDKResultSuccess if successful or CDK error values in case of error.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual CDKResult OnPreparePipelineCreate(
        ChiFeature2Identifier* pKey);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// MergeSnapshotMetadata
    ///
    /// @brief  Merge multicamera dualzone snapshot Metadata
    ///
    /// @param  pRequestObject      Feature request object instance.
    /// @param  hMetadata0          Input metadata 0 handle
    /// @param  hMetadata1          Input metadata 1 handle
    ///
    /// @return CDKResultSuccess upon successful execution, CDKResultEFailed otherwise
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CDKResult MergeSnapshotMetadata(
        ChiFeature2RequestObject* pRequestObject,
        CHIMETAHANDLE             hMetadata0,
        CHIMETAHANDLE             hMetadata1) const;

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// FillSATOfflinePipelineInputMetadata
    ///
    /// @brief  Function to fill the offline metadata
    ///
    /// @param  pMultiCamResultMetadata      Multi camera result metadata
    /// @param  pOfflineMetadata             fill offline result metadata
    ///
    /// @return CDKResultSuccess upon successful execution, CDKResultEFailed otherwise
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CDKResult FillSATOfflinePipelineInputMetadata(
        MulticamResultMetadata*     pMultiCamResultMetadata,
        ChiMetadata*                pOfflineMetadata
        ) const;

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// FillRTBOfflinePipelineInputMetadata
    ///
    /// @brief  Function to fill the offline metadata
    ///
    /// @param  pMultiCamResultMetadata      Multi camera result metadata
    /// @param  pOfflineMetadata             fill offline result metadata
    ///
    /// @return CDKResultSuccess upon successful execution, CDKResultEFailed otherwise
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CDKResult FillRTBOfflinePipelineInputMetadata(
        MulticamResultMetadata*     pMultiCamResultMetadata,
        ChiMetadata*                pOfflineMetadata
        ) const;

private:
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ChiFeature2Fusion
    ///
    /// @brief  Deafault constructor.
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    ChiFeature2Fusion() = default;

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ~ChiFeature2Fusion
    ///
    /// @brief  Virtual destructor.
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual ~ChiFeature2Fusion();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// PopulateDependencyPortsBasedOnMCC
    ///
    /// @brief  Populate port dependency based on MCC result
    ///
    /// @param  pRequestObject    Feature request object instance.
    /// @param  dependencyIndex   Dependency index
    /// @param  pInputDependency  Input dependency port info.
    ///
    /// @return CDKResultSuccess if populate successfully
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CDKResult PopulateDependencyPortsBasedOnMCC(
        ChiFeature2RequestObject*         pRequestObject,
        UINT8                             dependencyIndex,
        const ChiFeature2InputDependency* pInputDependency) const;

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// BuildInputPortListBasedOnMCC
    ///
    /// @brief  Build input port list based on MCC
    ///
    /// @param  pRequestObject    Feature request object instance.
    /// @param  stageId           Stage Id
    /// @param  pInputPortList    Pointer of input port list.
    /// @param  rEnabledPortList  Reference of enabled port list
    ///
    /// @return CDKResultSuccess if populate successfully
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CDKResult BuildInputPortListBasedOnMCC(
        ChiFeature2RequestObject*           pRequestObject,
        UINT8                               stageId,
        ChiFeature2PortIdList*              pInputPortList,
        std::vector<ChiFeature2Identifier>& rEnabledPortList) const;

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ExtractCameraMetadata
    ///
    /// @brief  Function to extract the subset of camera metadata from the result metadata
    ///
    /// @param  pResultMetadata           Pointer to result metadata
    /// @param  pExtractedCameraMetadata  Pointer to extracted camera metadata
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID ExtractCameraMetadata(
        ChiMetadata*    pResultMetadata,
        CameraMetadata* pExtractedCameraMetadata) const;

    ChiFeature2Fusion(const ChiFeature2Fusion&)            = delete;                ///< Disallow the copy constructor
    ChiFeature2Fusion& operator= (const ChiFeature2Fusion&) = delete;               ///< Disallow assignment operator
};

#endif // CHXFEATURE2GENERIC_H
