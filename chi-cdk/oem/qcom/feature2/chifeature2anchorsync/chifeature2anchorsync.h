////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2020 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  chifeature2anchorsync.h
/// @brief CHI anchorsync feature derived class declarations
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef CHIFEATURE2ANCHORSYNC_H
#define CHIFEATURE2ANCHORSYNC_H


#include "chifeature2base.h"
#include "chxextensionmodule.h"

// NOWHINE FILE CP006:  used standard libraries for performance improvements

// Max input ports per physical camera: RDI/FD/Metadata
static const UINT8 MaxInputPortsPerCamera = 3;

enum class ChiFeature2AnchorFrameSelectionAlgorithm
{
    None,                                            ///< As per current implementation
    Fixed,                                           ///< Apply fixed known order
    NonFixed                                         ///< Apply non-fixed order based on AnchorFrameSelectionMode
};

enum class ChiFeature2AnchorFrameSelectionMode
{
    TimeStamp,                                       ///< Based on Latest RDI Buffer
    Sharpness,                                       ///< Process images in order of decreasing sharpness (focus value)
    Lighting                                         ///< Process images with similar lighting first
};

struct ChiFeature2AnchorFrameSelectionData
{
    FLOAT                                   focusValue[BufferQueueDepth];              ///< input buffer array
    UINT32*                                 pHistogram[BufferQueueDepth];              ///< input buffer array
    UINT32                                  minHistrogramBin;                          ///< input buffer array
    UINT32                                  maxHistrogramBin;                          ///< input buffer array
    UINT64                                  timestamps[BufferQueueDepth];              ///< input buffer array
    UINT32                                  numOfImagesToBlend;                        ///< input buffer array
    ChiFeature2AnchorFrameSelectionMode     anchorFrameSelectionMode;                  ///< input buffer array
    UINT32                                  desiredAnchorFrameIndex;                   ///< input buffer array
    UINT64                                  anchorFrameTimeRange;                      ///< input buffer array
    UINT32                                  brightnessTolerance;                       ///< input buffer array
    BOOL                                    removeExpectedBadImages;                   ///< input buffer array
    UINT32                                  numImagesAllowedAsAnchor;                  ///< input buffer array
    UINT32                                  numSharpnessImages;                        ///< input buffer array
    UINT32                                  sharpnessBlockSize;                        ///< input buffer array
    float                                   sharpnessRankValue;                        ///< input buffer array
};


/// @brief buffer information
struct ChiFeature2PortBufferInfo
{
    ChiFeature2PortType bufferType;        ///< Buffer type, ImageBuffer or Metadata
    union
    {
        CHISTREAMBUFFER   streamBuffer;    ///< Stream buffer that includes buffer handle and stream information
        CHIMETADATAHANDLE hMetaHandle;     ///< Metadata Handle
    } Buffer;
};

/// @brief anchor pick output information
struct ChiFeature2AnchorPickOutputInfo
{
    std::vector<UINT8> outputOrder;                   ///< output order
};

/// @brief anchor pick input information
struct ChiFeature2AnchorPickInputInfo
{
    std::vector<ChiFeature2PortBufferInfo> inputBufferArray[MaxInputPortsPerCamera];  ///< input buffer array
    UINT8 maxInputPorts;                                                              ///< max input ports
    ChiFeature2AnchorFrameSelectionData    anchorFrameSelectionData;                  ///< Anchor Selection params
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief Feature derived class
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class ChiFeature2AnchorSync : public ChiFeature2Base
{
public:
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// Create
    ///
    /// @brief  Static function to create AnchorSync feature
    ///
    /// @param  pCreateInputInfo   Pointer to create input info for AnchorSync feature
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static ChiFeature2AnchorSync* Create(
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
    /// DoAnchorPick
    ///
    /// @brief  virtual method to do anchor pick.
    ///
    /// @param  pRequestObject        Feature request object instance.
    /// @param  pAnchorPickInputInfo  the pointer of input information for anchor pick algorithm.
    /// @param  pAnchorPickOutputInfo the pointer of output information after do ancho pick.
    ///
    /// @return CDKResultSuccess if successful or CDK error values in case of errors
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual CDKResult DoAnchorPick(
        ChiFeature2RequestObject*        pRequestObject,
        ChiFeature2AnchorPickInputInfo*  pAnchorPickInputInfo,
        ChiFeature2AnchorPickOutputInfo* pAnchorPickOutputInfo) const;

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// AnchorPickSharpnessBased
    ///
    /// @brief  virtual method to do anchor pick.
    ///
    /// @param  pAnchorPickInputInfo  the pointer of input information for anchor pick algorithm.
    /// @param  pAnchorPickOutputInfo the pointer of output information after do ancho pick.
    ///
    /// @return CDKResultSuccess if successful or CDK error values in case of errors
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CDKResult AnchorPickSharpnessBased(
        ChiFeature2AnchorPickInputInfo*  pAnchorPickInputInfo,
        ChiFeature2AnchorPickOutputInfo* pAnchorPickOutputInfo) const;

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// CalculateSharpness
    ///
    /// @brief  virtual method to do anchor pick.
    ///
    /// @param  pInputFDBuffer      the pointer of FD buffer.
    /// @param  sharpnessBlockSize  sharpnessBlockSize.
    /// @param  sharpnessRankValue  sharpnessRankValue.
    ///
    /// @return CDKResultSuccess if successful or CDK error values in case of errors
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    FLOAT CalculateSharpness(
        CHISTREAMBUFFER* pInputFDBuffer,
        UINT32           sharpnessBlockSize,
        float            sharpnessRankValue) const;


    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// IsCandidateForAnchor
    ///
    /// @brief  virtual method to do anchor pick.
    ///
    /// @param  numImagesAllowedAsAnchor      total images used for anchor.
    /// @param  imageIndex                    image index.
    ///
    /// @return false/true
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CHX_INLINE BOOL IsCandidateForAnchor(
        UINT32 numImagesAllowedAsAnchor,
        UINT32 imageIndex
        ) const
    {
        return imageIndex < numImagesAllowedAsAnchor;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// FillDefaultAnchorParamsData
    ///
    /// @brief  virtual method to do anchor pick.
    ///
    /// @param  pAnchorPickInputInfo  the pointer of input information for anchor pick algorithm.
    ///
    /// @return CDKResultSuccess if successful or CDK error values in case of errors
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CDKResult FillDefaultAnchorParamsData(
        ChiFeature2AnchorPickInputInfo*     pAnchorPickInputInfo) const;

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// OnInitialize
    ///
    /// @brief  Function creates Sessions and Pipelines based on input descriptor.
    ///         Derived features can override this to create sessions and pipelines or for virtual camx impl.
    ///
    /// @param  pCreateInputInfo  Feature create input information.
    ///
    /// @return CDKResultSuccess if successful or CDK error values in case of error.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual CDKResult OnInitialize(
        ChiFeature2CreateInputInfo* pCreateInputInfo);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// OnPrepareRequest
    ///
    /// @brief  Virtual method to prepare request for anchorsync feature.
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
    /// OnExecuteProcessRequest
    ///
    /// @brief  Virtual method to execute request for anchorsync feature.
    ///
    /// @param  pRequestObject  Feature request object instance.
    ///
    /// @return CDKResultSuccess if successful or CDK error values in case of error.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual CDKResult OnExecuteProcessRequest(
        ChiFeature2RequestObject* pRequestObject) const;

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// DoCleanupRequest
    ///
    /// @brief  Virtual method to cleanup request for AnchorSync feature.
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
    /// @brief  Virtual method to flush request for AnchorSync feature.
    ///
    /// @return CDKResultSuccess if successful or CDK error values in case of error.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual CDKResult DoFlush();

private:
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// IsFDBufferAvailable
    ///
    /// @brief  Check if FD buffer is available for anchor pick algo.
    ///         Ideally this should come from realtime feature, use non-zsl flag for now.
    ///
    /// @return TRUE if fd buffer is available, otherwise FALSE.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CHX_INLINE BOOL IsFDBufferAvailable() const
    {
        BOOL isFDBufferAvailable = TRUE;

        if ((NULL != GetInstanceProps()) &&
            (TRUE == GetInstanceProps()->instanceFlags.isNZSLSnapshot))
        {
            isFDBufferAvailable = FALSE;
        }

        return isFDBufferAvailable;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ChiFeature2AnchorSync
    ///
    /// @brief  Deafault constructor.
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    ChiFeature2AnchorSync() = default;

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ~ChiFeature2AnchorSync
    ///
    /// @brief  Virtual destructor.
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual ~ChiFeature2AnchorSync() = default;

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ReleasePrivateInfo
    ///
    /// @brief  private method to release private resource of feature context.
    ///
    /// @param  pRequestObject        Feature request object instance.
    ///
    /// @return CDKResultSuccess if successful or CDK error values in case of error.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CDKResult ReleasePrivateInfo(
        ChiFeature2RequestObject* pRequestObject) const;

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// HandleCollectInputInfo
    ///
    /// @brief  private method to collect required information for anchorpick.
    ///
    /// @param  pRequestObject        Feature request object instance.
    /// @param  ppAnchorPickInputInfo The pointer of input information for anchor pick.
    /// @param  stageId               Current stage Id
    ///
    /// @return CDKResultSuccess if successful or CDK error values in case of error.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CDKResult HandleCollectInputInfo(
        ChiFeature2RequestObject*        pRequestObject,
        ChiFeature2AnchorPickInputInfo** ppAnchorPickInputInfo,
        UINT8                            stageId) const;

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// BuildAndSubmitOutputInfo
    ///
    /// @brief  private method to build output data and notify downstream feature.
    ///
    /// @param  pRequestObject        Feature request object instance.
    /// @param  ppAnchorPickInputInfo The pointer of input information for anchor pick.
    /// @param  pAnchorPickOutputInfo The pointer of output information after do ancho pick.
    /// @param  stageId               Current stage Id
    ///
    /// @return CDKResultSuccess if successful or CDK error values in case of error.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CDKResult BuildAndSubmitOutputInfo(
        ChiFeature2RequestObject*        pRequestObject,
        ChiFeature2AnchorPickInputInfo** ppAnchorPickInputInfo,
        ChiFeature2AnchorPickOutputInfo* pAnchorPickOutputInfo,
        UINT8                            stageId) const;

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ReleaseInputDependencyResource
    ///
    /// @brief  private method to release input dependency.
    ///
    /// @param  pRequestObject        Feature request object instance.
    /// @param  requestId             RequestId to release resource

    /// @return CDKResultSuccess if successful or CDK error values in case of error.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CDKResult ReleaseInputDependencyResource(
        ChiFeature2RequestObject*  pRequestObject,
        UINT8                      requestId
        ) const;

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
    /// GetOutputInputMap
    ///
    /// @brief  Get the input port index which is mapped to output index
    ///
    /// @param  outputPortIdx     Output port index.
    ///
    /// @return input port index which mapped to output port
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    UINT8 GetOutputInputMap(
        UINT8 outputPortIdx) const;

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// IsPortActive
    ///
    /// @brief  To check if the port need to be enabled
    ///
    /// @param  pMCCResult     The pointer of mcc result
    /// @param  cameraIndex    The camera index
    ///
    /// @return TRUE if this port is required
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    BOOL IsPortActive(
        const Feature2ControllerResult* pMCCResult,
        UINT32                          cameraIndex) const;

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// IsAnchorPickRequired
    ///
    /// @brief  To check if anchor pick algorithm is required
    ///
    /// @param  pRequestObject The pointer of request object
    ///
    /// @return TRUE if anchor sync algorithm is required
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    BOOL IsAnchorPickRequired(
        ChiFeature2RequestObject* pRequestObject) const;

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// IsFDBufferRequired
    ///
    /// @brief  To check if FD buffer is required
    ///
    /// @param  pRequestObject The pointer of request object
    ///
    /// @return TRUE if FD buffer is required
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    BOOL IsFDBufferRequired(
        ChiFeature2RequestObject* pRequestObject) const;

    ChiFeature2AnchorSync(const ChiFeature2AnchorSync&) = delete;                       ///< Disallow the copy constructor
    ChiFeature2AnchorSync& operator= (const ChiFeature2AnchorSync&) = delete;               ///< Disallow assignment operator
};

#endif // CHIFEATURE2ANCHORSYNC_H
