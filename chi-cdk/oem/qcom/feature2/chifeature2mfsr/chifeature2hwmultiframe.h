////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  chifeature2hwmultiframe.h
/// @brief CHX generic feature derived class declarations
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef CHIFEATURE2HWMULTIFRAME_H
#define CHIFEATURE2HWMULTIFRAME_H

#include "chifeature2base.h"

// NOWHINE FILE CP006: Need vector to pass filtered port information

struct HWMFFeatureInfo;
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief Definitions of static data describing the HWMF Derived feature
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
extern const ChiFeature2PortDescriptor  MfsrPrefilterOutPutPortDescriptors[];
extern const ChiFeature2PortDescriptor  MfsrBlendOutPutPortDescriptors[];
extern const ChiFeature2PortDescriptor  MfsrPostFilterOutPutPortDescriptors[];
extern const ChiFeature2PortDescriptor  MfsrBlendInputPortDescriptors[];

/// @brief This structure encapsulates an final output dimension
struct ChiFeature2FinalOutputDimensions
{
    UINT32 width;   ///< Final yuv width
    UINT32 height;  ///< Final yuv height
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief Feature derived class
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class CDK_VISIBILITY_PUBLIC ChiFeature2HWmultiframe : public ChiFeature2Base
{
public:
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// Create
    ///
    /// @brief  Create feature
    ///
    /// @param  pCreateInputInfo  Input data for feature creation
    ///
    /// @return None.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static ChiFeature2HWmultiframe* Create(
        ChiFeature2CreateInputInfo* pCreateInputInfo);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// Destroy
    ///
    /// @brief  Virtual method to destroy.
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual VOID Destroy();

protected:
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// OnInitialize
    ///
    /// @brief  Initialize feature
    ///
    /// @param  pCreateInputInfo  Feature create input information.
    ///
    /// @return CDKResultSuccess if successful or CDK error values in case of error.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual CDKResult OnInitialize(
        ChiFeature2CreateInputInfo* pCreateInputInfo);

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
    /// OnPrepareRequest
    ///
    /// @brief  method to prepare processing request.
    ///         The feature is expected to set the request object's context once during this function.
    ///
    /// @param  pRequestObject  Feature request object instance.
    ///
    /// @return CDKResultSuccess if successful or CDK error values in case of error.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual CDKResult OnPrepareRequest(
        ChiFeature2RequestObject* pRequestObject
        ) const;

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// OnExecuteProcessRequest
    ///
    /// @brief  method to execute process request.
    ///
    /// @param  pRequestObject  Feature request object instance.
    ///
    /// @return None.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual CDKResult OnExecuteProcessRequest(
        ChiFeature2RequestObject* pRequestObject
        ) const;

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// OnBufferResult
    ///
    /// @brief  Virtual method to process buffer callback from CHI driver.
    ///
    /// @param  pRequestObject          Feature request object instance.
    /// @param  resultId                FRO batch requestId
    /// @param  pStageInfo              Stage info to which this callback belongs
    /// @param  pPortIdentifier         Port identifier on which the output is generated
    /// @param  pStreamBuffer           Buffer from CamX on output image port
    /// @param  frameNumber             frameNumber for CamX result
    /// @param  pPrivateData            Private data
    ///
    /// @return TRUE if the buffer should be sent to graph, FALSE otherwise
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual BOOL OnBufferResult(
        ChiFeature2RequestObject*  pRequestObject,
        UINT8                      resultId,
        ChiFeature2StageInfo*      pStageInfo,
        ChiFeature2Identifier*     pPortIdentifier,
        const CHISTREAMBUFFER*     pStreamBuffer,
        UINT32                     frameNumber,
        VOID*                      pPrivateData);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// OnReleaseDependencies
    ///
    /// @brief  Virtual method to process releasing dependencies
    ///
    ///         Base class will by default release all dependencies asked for a particular sequence. If this function
    ///         isoverriden, derived class will need to convey whether to release a particular buffer asked on given portId

    /// @param  pPortIdentifier          PortIdentifier on which to release dependency
    /// @param  dependencyIndex          Dependency Index for the given port
    /// @param  pStageInfo               Stage info to which this callback belongs
    /// @param  pRequestObject           Feature request object instance.

    /// @return TRUE if the given port should be released with the particular index
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual BOOL OnReleaseDependencies(
        const ChiFeature2Identifier*    pPortIdentifier,
        UINT8                           dependencyIndex,
        ChiFeature2StageInfo*           pStageInfo,
        ChiFeature2RequestObject*       pRequestObject
        ) const;

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// DoCleanupRequest
    ///
    /// @brief  method to cleanup processing request.
    ///
    /// @param  pRequestObject  Feature request object instance.
    ///
    /// @return CDKResultSuccess if successful or CDK error values in case of error.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual CDKResult DoCleanupRequest(
        ChiFeature2RequestObject* pRequestObject
        ) const;

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// DoFlush
    ///
    /// @brief  method to flush.
    ///
    /// @return CDKResultSuccess if successful or CDK error values in case of error.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual CDKResult DoFlush();

private:
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ChiFeature2HWmultiframe
    ///
    /// @brief  Deafault constructor.
    ///
    /// @return None.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    ChiFeature2HWmultiframe() = default;

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ~ChiFeature2HWmultiframe
    ///
    /// @brief  Destructor
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual ~ChiFeature2HWmultiframe();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// SelectOutputPorts
    ///
    /// @brief  Select output ports to enable
    ///
    /// @param  pAllOutputPorts   All output ports for this stage
    /// @param  pRequestObject    Feature request object.

    ///
    /// @return Filtered Output ports
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    std::vector<ChiFeature2Identifier>SelectOutputPorts(
        ChiFeature2PortIdList*      pAllOutputPorts,
        ChiFeature2RequestObject*   pRequestObject
        ) const;

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// SelectInputPorts
    ///
    /// @brief  Select input ports to enable
    ///
    /// @param  pAllInputPorts   All output ports for this stage
    /// @param  pRequestObject   Feature request object.
    ///
    /// @return Filtered Input ports
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    std::vector<ChiFeature2Identifier>SelectInputPorts(
        ChiFeature2PortIdList*      pAllInputPorts,
        ChiFeature2RequestObject*  pRequestObject
        ) const;

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ProcessHWMFPrefilterStage
    ///
    /// @brief  Process HWMF Prefilter stage
    ///
    /// @param  pRequestObject  Feature request object.
    /// @param  stageId         Prefilter StageId of HWMF.
    /// @param  stageSequenceId Prefilter SeqId of HWMF.
    /// @param  nextStageId     Next Stage of HWMF.
    ///
    /// @return CDKResultSuccess if successful, failure otherwise
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CDKResult ProcessHWMFPrefilterStage(
        ChiFeature2RequestObject * pRequestObject,
        UINT8                      stageId,
        UINT8                      stageSequenceId,
        UINT8                      nextStageId
        ) const;

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ProcessHWMFBlendInitStage
    ///
    /// @brief  Process HWMF BlendInit stage
    ///
    /// @param  pRequestObject  Feature request object.
    /// @param  stageId         BlendInit StageId of HWMF.
    /// @param  stageSequenceId BlendInit SeqId of HWMF.
    /// @param  nextStageId     Next Stage of HWMF.
    ///
    /// @return CDKResultSuccess if successful, failure otherwise
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CDKResult ProcessHWMFBlendInitStage(
        ChiFeature2RequestObject * pRequestObject,
        UINT8                      stageId,
        UINT8                      stageSequenceId,
        UINT8                      nextStageId
        ) const;

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ProcessHWMFBlendLoopStage
    ///
    /// @brief  Process HWMF Blend Loop stage
    ///
    /// @param  pRequestObject  Feature request object.
    /// @param  stageId         BlendLoop StageId of HWMF.
    /// @param  stageSequenceId BlendLoop SeqId of HWMF.
    /// @param  nextStageId     Next Stage of HWMF.
    ///
    /// @return CDKResultSuccess if successful, failure otherwise
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CDKResult ProcessHWMFBlendLoopStage(
        ChiFeature2RequestObject * pRequestObject,
        UINT8                      stageId,
        UINT8                      stageSequenceId,
        UINT8                      nextStageId
        ) const;

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ProcessHWMFPostfilterStage
    ///
    /// @brief  Process HWMF Postfilter stage
    ///
    /// @param  pRequestObject  Feature request object.
    /// @param  stageId         Postfilter StageId of HWMF.
    /// @param  stageSequenceId Postfilter SeqId of HWMF.
    /// @param  nextStageId     Next Stage of HWMF.
    ///
    /// @return CDKResultSuccess if successful, failure otherwise
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CDKResult ProcessHWMFPostfilterStage(
        ChiFeature2RequestObject * pRequestObject,
        UINT8                      stageId,
        UINT8                      stageSequenceId,
        UINT8                      nextStageId
        ) const;

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// IsRDIRequiredPostFilterStage
    ///
    /// @brief  Check whether RDI is needed for postfilter stage
    ///
    /// @return TRUE if RDI is needed for postfilter stage otherwise FALSE
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    BOOL IsRDIRequiredPostFilterStage() const;

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
    /// DumpInputMetaBuffer
    ///
    /// @brief  Dump input meta data buffer for prefilter stage
    ///
    /// @param  pInputMetaInfo  Input metadata info.
    /// @param  pBaseName       Base name to be used for determining the output dump file name
    /// @param  index           Multi frame sequence index.
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID DumpInputMetaBuffer(
        ChiFeature2BufferMetadataInfo* pInputMetaInfo,
        CHAR*                          pBaseName,
        UINT                           index
        ) const;

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// DumpInputImageBuffer
    ///
    /// @brief  Dump input raw image buffer for prefilter stage
    ///
    /// @param  pInputBufferInfo  Input raw image info.
    /// @param  pBaseName         Base name to be used for determining the output dump file name
    /// @param  index             Multi frame sequence index.
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID DumpInputImageBuffer(
        ChiFeature2BufferMetadataInfo* pInputBufferInfo,
        CHAR*                          pBaseName,
        UINT                           index
        ) const;

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
    /// GetUseCaseString
    ///
    /// @brief  Get a string corresponding to the use case passed in
    ///
    /// @param  useCase   Use case
    /// @param  output    Output string
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID GetUseCaseString(
        ChiModeUsecaseSubModeType useCase,
        std::string&              output
        ) const;

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetDumpFileBaseName
    ///
    /// @brief  Get the base name to be used for all the dump files
    ///
    /// @param  pRequestObject      Feature request object
    /// @param  pDumpFileBaseName   Output dump file base name
    /// @param  size                Size of the array containing the dump file base name
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID GetDumpFileBaseName(
        ChiFeature2RequestObject* pRequestObject,
        CHAR*                     pDumpFileBaseName,
        UINT                      size
        ) const;

    ChiFeature2HWmultiframe(const ChiFeature2HWmultiframe&)             = delete;   ///< Disallow the copy constructor
    ChiFeature2HWmultiframe& operator= (const ChiFeature2HWmultiframe&) = delete;   ///< Disallow assignment operator

    UINT32                              m_hwmfTotalNumFrames;       ///< HWMF total num of frames
    HWMFFeatureInfo*                    m_pHWMFFeatureInfo;         ///< HWMF feature data
    BOOL                                m_disableDS64Port;          ///< Decision flag to disable DS64 port
    ChiFeature2FinalOutputDimensions    m_hwmfFinalDimensions;      ///< Final Dimensions
    BOOL                                m_disableZoomCrop;          ///< Disable zoom crop in IPE
};

#endif // CHIFEATURE2HWMULTIFRAME_H
