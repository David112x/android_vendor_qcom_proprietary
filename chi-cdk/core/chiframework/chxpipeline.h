////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2017-2020 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  chxpipeline.h
/// @brief CHX pipeline class
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef CHXPIPELINE_H
#define CHXPIPELINE_H

#include <assert.h>

#include "chi.h"
#include "chioverride.h"
#include "camxcdktypes.h"
#include "chxextensionmodule.h"
#include "chxmetadata.h"

// NOWHINE ENTIRE FILE - Temporarily bypassing for existing CHI files

/// Constants
static const UINT64 InvalidPipeline = 0x0;
static const UINT   SensorNodeId    = 0;
static const UINT   TorchNodeId     = 10;

/// @brief Define the pipeline types
enum PipelineType
{
    Invalid             = 0,
    RealtimePreview     = 1,
    RealtimeZSL         = 2,
    OfflineZSLYUV       = 3,
    OfflineZSLJPEG      = 4,
    Default             = 5,
    MFNRPrefilter       = 6,
    MFNRBlend           = 7,
    MFNRPostfilter      = 8,
    MFNRScale           = 9,
    OfflineYUVJPEG      = 10,
    OfflineCustom       = 11,
    OfflinePreview      = 12,
    MFSRPrefilter       = 13,
    MFSRBlend           = 14,
    MFSRPostfilter      = 15,
    MaxPipelineTypes    = 16,
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief Pipeline class
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class CDK_VISIBILITY_PUBLIC Pipeline
{
public:
    static Pipeline* Create(
        UINT32                                cameraId,
        PipelineType                          type,
        const CHAR*                           pName = NULL);

    /// Destroy
    VOID Destroy();

    /// Set the output buffers
    VOID SetOutputBuffers(
        UINT                     numOutputs,
        CHIPORTBUFFERDESCRIPTOR* pOutputs);

    /// Set the input buffers
    VOID SetInputBuffers(
        UINT                     numInputs,
        CHIPORTBUFFERDESCRIPTOR* pInputs);

    /// Set the preview stream
    CDKResult SetPreviewStream(
        CHISTREAM* pStream);

    /// Set the RDI stream
    CDKResult SetRDIStream(
        CHISTREAM* pStream);

    /// Set the snapshot stream
    CDKResult SetSnapshotStream(
        CHISTREAM* pStream);

    /// Set the blend output stream
    CDKResult SetOutputStream(
        CHISTREAM* pStream);

    /// Set the input stream
    CDKResult SetInputStream(
        CHISTREAM* pStream);

    /// Set pipeline activate flag
    VOID SetPipelineActivateFlag()
    {
        m_pipelineActivated = TRUE;
    }

    /// Set pipeline deactivate
    VOID SetPipelineDeactivate()
    {
        m_pipelineActivated = FALSE;
    }

    /// Set the pipeline metadataclientid
    CHX_INLINE VOID SetMetadataClientId(UINT clientId)
    {
        m_metaDataClientId = clientId;
    }

    /// Get the pipeline handle
    CHIPIPELINEDESCRIPTOR GetPipelineHandle() const
    {
        return m_hPipelineHandle;
    }

    /// Return the input buffer descriptor
    CHIPORTBUFFERDESCRIPTOR* GetInputBufferDescriptors()
    {
        return ((FALSE == IsRealTime()) ? &m_pipelineInputBuffer[0] : NULL);
    }

    /// Get metadata client Id
    UINT GetMetadataClientId() const
    {
        return m_metaDataClientId;
    }

    /// Set the pipeline node/port info
    VOID SetPipelineNodePorts(
        const CHIPIPELINECREATEDESCRIPTOR* pCreateDesc)
    {
        m_pipelineDescriptor = *pCreateDesc;
    }

    /// Set the pipeline node/port info
    VOID SetPipelineName(
        const CHAR* const pPipelineName)
    {
        FreeName();
        m_pPipelineName = pPipelineName;
    }

    /// Set the pipeline name with an instance ID to append to the name's end
    VOID SetPipelineNameF(
        const CHAR* const pPipelineName,
        UINT              instanceId);

    /// Get the pipeline name
    const CHAR* GetPipelineName() const
    {
        return m_pPipelineName;
    }

    /// Set the sensor mode pick hint
    VOID SetSensorModePickHint(
        const CHISENSORMODEPICKHINT* pSensorModePickHint)
    {
        m_pSensorModePickhint = pSensorModePickHint;
    }

    /// Set android metadata contained within stream configuration
    CDKResult SetAndroidMetadata(
        camera3_stream_configuration* pStreamConfig);

    CDKResult SetVendorTag(
        UINT32       tagID,
        const VOID*  pData,
        UINT32       count);

    /// Set the pipeline resource policy
    VOID SetPipelineResourcePolicy(
        CHIRESOURCEPOLICY policy)
    {
        m_resourcePolicy = policy;
    }

    /// Get the pipeline resource policy
    CHIRESOURCEPOLICY GetPipelineResourcePolicy() const
    {
        return m_resourcePolicy;
    }

    /// Set the defer finalize pipeline flag
    VOID SetDeferFinalizeFlag(BOOL isDeferFinalizeNeeded)
    {
        m_isDeferFinalizeNeeded = isDeferFinalizeNeeded;
    }

    /// Get the defer finalize pipeline flag
    BOOL GetDeferFinalizeFlag() const
    {
        return m_isDeferFinalizeNeeded;
    }
    /// Get the input buffer requirements for this pipeline
    CHIPIPELINEINPUTOPTIONS* GetInputOptions()
    {
        return &m_pipelineInputOptions[0];
    }

    /// Is pipeline active
    BOOL IsPipelineActive() const
    {
        return m_pipelineActivated;
    }

    /// Is pipeline real time
    BOOL IsRealTime() const
    {
        return m_pipelineDescriptor.isRealTime;
    }

    /// Check if the pipeline is offline
    BOOL IsOffline()
    {
        return ((TRUE == IsRealTime()) ? FALSE : TRUE);
    }

    /// Get the pointer to the publish tag list
    CHX_INLINE UINT32* GetTagList()
    {
        return const_cast<UINT32* >(m_pPipelineMetadataInfo.publishTagArray);
    }

    /// Get publish tag count
    CHX_INLINE UINT32 GetTagCount()
    {
        return m_pPipelineMetadataInfo.publishTagCount;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetPartialTagCount
    ///
    /// @brief  Returns the Total partial Tags for the pipeline
    ///
    /// @return Count
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// Get Partial Tag count
    CHX_INLINE UINT32 GetPartialTagCount()
    {
        return m_pPipelineMetadataInfo.publishPartialTagCount;
    }

    /// return pipeline name
    CHX_INLINE const CHAR* GetPipelineName()
    {
        return m_pPipelineName;
    }

    /// gets the metadata buffer count
    CHX_INLINE UINT32 GetMetadataBufferCount()
    {
        return m_pPipelineMetadataInfo.maxNumMetaBuffers;
    }

    /// Get pointer to this Pipeline descriptor's metadata object.
    /// Tags published on this metadata instance can be queried via a CamX Pipeline's UsecasePool
    CHX_INLINE ChiMetadata* GetDescriptorMetadata()
    {
        return m_pPipelineDescriptorMetadata;
    }

    /// Queries the metadata tags
    CDKResult QueryMetadataTags(
       CHIHANDLE hSession);

    /// Create the pipeline descriptor
    CDKResult CreateDescriptor();

    /// Get the pipeline information
    const CHIPIPELINEINFO GetPipelineInfo() const;

    /// SetTuningUsecase : updating usecase tuning mode for usecasepool
    VOID SetTuningUsecase();

    /// Get the sensor mode index
    CHISENSORMODEINFO* GetSensorModeInfo()
    {
        return m_pSelectedSensorMode;
    }

    /// Get CameraId of the pipeline
    CHX_INLINE UINT32 GetCameraId()
    {
        return m_cameraId;
    }

    /// Does the pipeline have sensor node
    static BOOL HasSensorNode(
        const ChiPipelineCreateDescriptor* pCreateDesc);

    /// Get Number of IFEs used for this pipeline instance
    INT32 GetNumberOfIFEsUsed();

private:
    Pipeline() = default;
    ~Pipeline();

    /// Initialize the pipeline object
    CDKResult Initialize(
        UINT32                                cameraId,
        PipelineType                          type);

    /// Create the real time preview pipeline
    CDKResult CreateRealTimePreviewPipeline();

    /// Create the real time pipeline descriptor
    VOID SetupRealtimePreviewPipelineDescriptor();

    /// Finalize the real time pipeline
    CDKResult FinalizeRealTimeZSLPipeline();

    /// Create the real time ZSL pipeline descriptor used to create the pipeline
    VOID SetupRealTimeZSLPipelineDescriptor();

    /// Finalize the Offline ZSL pipeline
    CDKResult FinalizeOfflineZSLPipeline();

    /// Setup the offline ZSL JPEG pipeline descriptor
    VOID SetupOfflineZSLJPEGPipelineDescriptor();

    /// Setup the offline ZSL YUV pipeline descriptor
    VOID SetupOfflineZSLYUVPipelineDescriptor();

    /// Setup the offline Prefilter pipeline descriptor
    VOID SetupMFNRPrefilterPipelineDescriptor();

    /// Setup the offline Blend pipeline descriptor
    VOID SetupMFNRBlendPipelineDescriptor();

    /// Setup the offline Scale pipeline descriptor
    VOID SetupMFNRScalePipelineDescriptor();

    /// Setup the offline ZSL pipeline descriptor
    VOID SetupMFNRPostFilterPipelineDescriptor();

    /// Setup the offline MFSR Prefilter pipeline descriptor
    VOID SetupMFSRPrefilterPipelineDescriptor();

    /// Setup the offline MFSR Blend pipeline descriptor
    VOID SetupMFSRBlendPipelineDescriptor();

    /// Setup the offline MFSR ZSL pipeline descriptor
    VOID SetupMFSRPostFilterPipelineDescriptor();

    /// Frees memory allocated for this pipeline's name if allocated
    VOID FreeName()
    {
        if (TRUE == m_isNameAllocated)
        {
            ChxUtils::Free(const_cast<CHAR*>(m_pPipelineName));
            m_pPipelineName   = NULL;
            m_isNameAllocated = FALSE;
        }
    }

    // Do not implement the copy constructor or assignment operator
    Pipeline(const Pipeline& rPipeline) = delete;
    Pipeline& operator= (const Pipeline& rPipeline) = delete;

    PipelineType                m_type;                            ///< Pipeline type
    CHIPIPELINEDESCRIPTOR       m_hPipelineHandle;                 ///< Pipeline handle
    const CHAR*                 m_pPipelineName;                   ///< Name of the pipeline
    /// @todo Better way would be to not have fixed max size but as per need only
    CHIPIPELINECREATEDESCRIPTOR m_pipelineDescriptor;              ///< Pipeline descriptor
    CHINODE                     m_nodes[20];                       ///< List of nodes in the pipeline
    CHINODELINK                 m_links[20];                       ///< Links in the pipeline
    CHIINPUTPORTDESCRIPTOR      m_inputPorts[20];                  ///< Input ports
    CHIOUTPUTPORTDESCRIPTOR     m_outputPorts[20];                 ///< Output ports
    CHILINKNODEDESCRIPTOR       m_linkNodeDescriptors[20];         ///< Link node descriptors
    CHIPORTBUFFERDESCRIPTOR     m_pipelineOutputBuffer[16];        ///< Pipeline Output buffer descriptor
    UINT32                      m_numOutputBuffers;                ///< Number of output buffers of the pipeline
    CHIPORTBUFFERDESCRIPTOR     m_pipelineInputBuffer[16];         ///< Pipeline input buffers
    UINT32                      m_numInputBuffers;                 ///< Number of input buffers
    CHIPIPELINEINPUTOPTIONS     m_pipelineInputOptions[16];        ///< Input buffer requirements of the pipeline
    CHIPIPELINEINFO             m_pipelineInfo;                    ///< Pipeline input, output info
    UINT32                      m_cameraId;                        ///< Camera Id
    CHISENSORMODEINFO*          m_pSelectedSensorMode;             ///< Selected sensor mode for this pipeline
    BOOL                        m_pipelineActivated;               ///< flag to indicate if pipeline is active
    UINT32                      m_statsSkipPattern;                ///< 3A frame skip pattern for this pipeline
    UINT                        m_enableFOVC;                      ///< Enable/disable FOVC for this pipeline
    CHIRESOURCEPOLICY           m_resourcePolicy;                  ///< Resource Policy
    BOOL                        m_isDeferFinalizeNeeded;           ///< Flag to indicate if pipeline need to defer
                                                                   ///  finalize
    CHIPIPELINEMETADATAINFO     m_pPipelineMetadataInfo;           ///< Metadata information for the pipeline
    ChiMetadata*                m_pPipelineDescriptorMetadata;     ///< Metadata visible via the CamX Pipeline's UsecasePool
    UINT                        m_metaDataClientId;                ///< metadata client id for the pipeline

    const CHISENSORMODEPICKHINT* m_pSensorModePickhint;            ///< Extra flags used to select sensor mode
    BOOL                         m_isNameAllocated;                ///< True if name uses dynamic memory
    UINT32                       m_numInputOptions;                ///< Number of input Options
};

#endif // CHXPIPELINE_H
