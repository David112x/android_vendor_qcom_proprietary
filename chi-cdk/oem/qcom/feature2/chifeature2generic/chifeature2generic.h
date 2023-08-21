////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  chifeature2generic.h
/// @brief CHI generic feature derived class declarations
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef CHIFEATURE2GENERIC_H
#define CHIFEATURE2GENERIC_H

#include "chifeature2base.h"

// NOWHINE FILE CP006:  used standard libraries for performance improvements

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief Feature derived class
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class CDK_VISIBILITY_PUBLIC ChiFeature2Generic : public ChiFeature2Base
{
public:
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// Create
    ///
    /// @brief  Static function to create feature instance
    ///
    /// @param  pCreateInputInfo   Pointer to create input info for feature
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static ChiFeature2Generic* Create(
        ChiFeature2CreateInputInfo* pCreateInputInfo);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// Destroy
    ///
    /// @brief  Virtual method to destroy.
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual VOID Destroy();

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
    virtual CDKResult OnPopulateDependencySettings (
        ChiFeature2RequestObject*     pRequestObject,
        UINT8                         dependencyIndex,
        const ChiFeature2Identifier*  pSettingPortId,
        ChiMetadata*                  pFeatureSettings
    ) const;

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// OnPopulateConfigurationSettings
    ///
    /// @brief  The base feature implementation populates request object configuration for current stage
    ///         Derived class can override this.
    ///
    /// @param  pRequestObject      Feature request object containing the configuration information for request submission.
    /// @param  pMetadataPortId     metadata portId
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
    /// OnPopulateConfiguration
    ///
    /// @brief  The base feature implementation populates request object configuration for current stage
    ///         Derived class can override this function
    ///
    /// @param  pRequestObject      Feature request object containing the configuration information for request submission.
    ///
    /// @return CDKResultSuccess if successful or CDK error values in case of error.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual CDKResult OnPopulateConfiguration(
        ChiFeature2RequestObject* pRequestObject
    ) const;

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// OnPopulateDependency
    ///
    /// @brief  Virtual function to populate dependency as per stage descriptor
    ///         Derived features can override this to set its own feature setting and populating logic
    ///
    /// @param  pRequestObject  Feature request object instance.
    ///
    /// @return CDKResultSuccess if successful or CDK error values in case of error.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual CDKResult OnPopulateDependency(
        ChiFeature2RequestObject* pRequestObject) const;


    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// OnPrepareRequest
    ///
    /// @brief  Virtual method to prepare request for Generic feature.
    ///
    /// @param  pRequestObject  Feature request object instance.
    ///
    /// @return CDKResultSuccess if successful or CDK error values in case of error.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual CDKResult OnPrepareRequest(
        ChiFeature2RequestObject* pRequestObject) const;

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// IsGPURotationNeeded
    ///
    /// @brief  method to check if GPU rotation is required
    ///
    /// @param  pRequestObject  Feature request object instance.
    ///
    /// @return TRUE or FALSE
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    BOOL IsGPURotationNeeded(
        ChiFeature2RequestObject* pRequestObject) const;

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
    /// OnSelectFlowToExecuteRequest
    ///
    /// @brief  Virtual method to select base provided request flow
    ///
    /// @param  pRequestObject  Feature request object instance.
    ///
    /// @return Selected ChiFeature2RequestFlowType
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual ChiFeature2RequestFlowType OnSelectFlowToExecuteRequest(
        ChiFeature2RequestObject* pRequestObject) const;

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
    /// DoCleanupRequest
    ///
    /// @brief  Virtual method to cleanup request for Generic feature.
    ///
    /// @param  pRequestObject  Feature request object instance.
    ///
    /// @return CDKResultSuccess if successful or CDK error values in case of error.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual CDKResult DoCleanupRequest(
        ChiFeature2RequestObject* pRequestObject) const;

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// OnMetadataResult
    ///
    /// @brief  Process metadata callback from CHI driver.
    ///
    /// @param  pRequestObject          Feature request object instance.
    /// @param  resultId                FRO batch requestId
    /// @param  pStageInfo              Stage info to which this callback belongs
    /// @param  pPortIdentifier         Port identifier on which the output is generated
    /// @param  pMetadata               Metadata from CamX on output image port
    /// @param  frameNumber             frameNumber for CamX result
    /// @param  pPrivateData            Private data of derived
    ///
    /// @return TRUE if the metadata should be sent to graph, FALSE otherwise
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    BOOL OnMetadataResult(
        ChiFeature2RequestObject*  pRequestObject,
        UINT8                      resultId,
        ChiFeature2StageInfo*      pStageInfo,
        ChiFeature2Identifier*     pPortIdentifier,
        ChiMetadata*               pMetadata,
        UINT32                     frameNumber,
        VOID*                      pPrivateData);

protected:
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// DoFlush
    ///
    /// @brief  Virtual method to flush request for Generic feature.
    ///
    /// @return CDKResultSuccess if successful or CDK error values in case of error.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual CDKResult DoFlush();

private:
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// OnPruneUsecaseDescriptor
    ///
    /// @brief  Function to select pipeline.
    ///         Derived features can override this to select the pipeline that needs to be created.
    ///
    /// @param  pCreateInputInfo   [IN] Feature create input information.
    /// @param  rPruneVariants    [OUT] Vector of prune properties
    ///
    /// @return CDKResultSuccess if successful or CDK error values in case of error.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual CDKResult OnPruneUsecaseDescriptor(
        const ChiFeature2CreateInputInfo*   pCreateInputInfo,
        std::vector<PruneVariant>&          rPruneVariants
        ) const;

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// OnPipelineSelect
    ///
    /// @brief  Function to select pipeline based on stream configuration.
    ///
    /// @param  pPipelineName        Pipeline name.
    /// @param  pCreateInputInfo     Feature create input information.
    ///
    /// @return CDKResultSuccess if successful or CDK error values in case of error.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual CDKResult OnPipelineSelect(
        const CHAR*                         pPipelineName,
        const ChiFeature2CreateInputInfo*   pCreateInputInfo) const;

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ChiFeature2Generic
    ///
    /// @brief  Default constructor.
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    ChiFeature2Generic() = default;

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ~ChiFeature2Generic
    ///
    /// @brief  Virtual destructor.
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual ~ChiFeature2Generic();

    BOOL  m_disableZoomCrop;                                                ///< is native resolution supported

    ChiFeature2Generic(const ChiFeature2Generic&)               = delete;   ///< Disallow the copy constructor
    ChiFeature2Generic& operator= (const ChiFeature2Generic&)   = delete;   ///< Disallow assignment operator
};

#endif // CHXFEATURE2GENERIC_H
