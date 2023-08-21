////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2016-2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  camxjpegencnode.h
/// @brief JPEG Encoder Node class declarations
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/// @todo (CAMX-1988) Code clean up apply NC001 coding standard on entire project
/// @todo (CAMX-1989) Code clean up apply GR030 coding standard on entire project

#ifndef CAMXJPEGENCNODE_H
#define CAMXJPEGENCNODE_H

#include "camxmem.h"
#include "camxnode.h"
#include "camxcmdbuffermanager.h"
#include "camxhwcontext.h"
#include "camxjpegencconfig.h"
#include "camxcsljpegdefs.h"
#include "camxjpegutil.h"

CAMX_NAMESPACE_BEGIN

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief Class that implements the JPEG Encoder node base class
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class JPEGEncNode final : public Node
{
public:



    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// Create
    ///
    /// @brief  Static method to create JPEGEncNode Object.
    ///
    /// @param  pCreateInputData  Node create input data
    /// @param  pCreateOutputData Node create output data
    ///
    /// @return Pointer to the concrete JPEGEncNode object
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static JPEGEncNode* Create(
        const NodeCreateInputData* pCreateInputData,
        NodeCreateOutputData*      pCreateOutputData);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// Destroy
    ///
    /// @brief  This method destroys the derived instance of the interface
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual VOID Destroy();

protected:
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ProcessingNodeInitialize
    ///
    /// @brief  Initialize the hwl object
    ///
    /// @param  pCreateInputData  Node create input data
    /// @param  pCreateOutputData Node create output data
    ///
    /// @return CamxResultSuccess if successful
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual CamxResult ProcessingNodeInitialize(
        const NodeCreateInputData* pCreateInputData,
        NodeCreateOutputData*      pCreateOutputData);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// PostPipelineCreate
    ///
    /// @brief  virtual method to be called at NotifyTopologyCreated time; node should take care of updates and initialize
    ///         blocks that has dependency on other nodes in the topology at this time.
    ///
    /// @return CamxResultSuccess if successful
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual CamxResult PostPipelineCreate();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ExecuteProcessRequest
    ///
    /// @brief  Pure virtual method to trigger process request for the hwl node object.
    ///
    /// @param  pExecuteProcessRequestData Process request data
    ///
    /// @return CamxResultSuccess if successful and 0 dependencies, dependency information otherwise
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual CamxResult ExecuteProcessRequest(
        ExecuteProcessRequestData* pExecuteProcessRequestData);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ProcessingNodeFinalizeInputRequirement
    ///
    /// @brief  Determine its input buffer requirements based on all the output buffer requirements
    ///
    /// @param  pBufferNegotiationData  Negotiation data for all output ports of a node
    ///
    /// @return Success if the negotiation was successful, Failure otherwise
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual CamxResult ProcessingNodeFinalizeInputRequirement(
        BufferNegotiationData* pBufferNegotiationData);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// FinalizeBufferProperties
    ///
    /// @brief  Finalize the buffer properties of each output port
    ///
    /// @param  pBufferNegotiationData Buffer negotiation data
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual VOID FinalizeBufferProperties(
        BufferNegotiationData* pBufferNegotiationData);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// QueryMetadataPublishList
    ///
    /// @brief  Method to query the publish list from the node
    ///
    /// @param  pPublistTagList List of tags published by the node
    ///
    /// @return CamxResultSuccess if successful
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual CamxResult QueryMetadataPublishList(
        NodeMetadataList* pPublistTagList);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ~JPEGEncNode
    ///
    /// @brief  Destructor
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual ~JPEGEncNode();

private:
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// JPEGEncNode
    ///
    /// @brief  Constructor
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    JPEGEncNode();
    JPEGEncNode(const JPEGEncNode&) = delete;                 ///< Disallow the copy constructor.
    JPEGEncNode& operator=(const JPEGEncNode&) = delete;      ///< Disallow assignment operator.

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// PublishEncoderOutput
    ///
    /// @brief  Function to publish output params
    ///
    /// @param  requestId   requestId
    ///
    /// @return CamxResultSuccess if successful
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult PublishEncoderOutput(
        UINT64 requestId);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// SetupDeviceResource
    ///
    /// @brief  Setup List of the Required Resource
    ///
    /// @param  pDeviceInfo Pointer to device info
    /// @param  pResource   Pointer to an array of resource
    ///
    /// @return CamxResult  CamxResultSuccess when successful
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual CamxResult SetupDeviceResource(
        CSLJPEGAcquireDeviceInfo*  pDeviceInfo,
        CSLDeviceResource* pResource);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// FillJPEGCmdBuffer
    ///
    /// @brief  Fill JPEG command buffer
    ///
    /// @param  pCmdBuf        Pointer to command buffer
    /// @param  pModuleInput   Pointer to module input
    ///
    /// @return CamxResult  CamxResultSuccess when successful
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual CamxResult FillJPEGCmdBuffer(
        CmdBuffer*       pCmdBuf,
        JPEGInputData*   pModuleInput);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// UpdateCropForOutAspectRatio
    ///
    /// @brief  Adjust crop info if cropped aspect ratio different from output aspect ratio
    ///
    /// @param  pScaleCropReq  pointer to src. dst and crop dimensions
    /// @param  bCropWidth     if we should adjust crop width or height
    ///
    /// @return CamxResultSuccess if successful and 0 dependencies, dependency information otherwise
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual CamxResult UpdateCropForOutAspectRatio(
        JPEGEScaleConfig * pScaleCropReq,
        BOOL bCropWidth);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// SetDeviceIndex
    ///
    /// @brief  Set device index
    ///
    /// @param  idx Index
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    void SetDeviceIndex(
        INT32 idx);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// SetCmdBufResourceSize
    ///
    /// @brief  Set command buffer resource size
    ///
    /// @param  size Resource size
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    void SetCmdBufResourceSize(
        UINT size);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// AcquireDevice
    ///
    /// @brief  Helper method to acquire JPEG encoder device
    ///
    /// @return CamxResultSuccess
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult AcquireDevice();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ReleaseDevice
    ///
    /// @brief  Helper method to release JPEG encoder device
    ///
    /// @return CamxResultSuccess
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult ReleaseDevice();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// SetPropertyDependency
    ///
    /// @brief  Set the dependencey data.
    ///
    /// @param  pNodeRequestData   Pointer to the incoming NodeProcessRequestData
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID SetPropertyDependency(
        NodeProcessRequestData*  pNodeRequestData);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// SetDependencies
    ///
    /// @brief  Check if JPEG aggregator dependencies are met.
    ///
    /// @param  pExecuteProcessRequestData Process request data
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID SetDependencies(
        ExecuteProcessRequestData* pExecuteProcessRequestData);

    /// @brief All data that will be needed in the fence callback
    struct FenceCallbackData
    {
        volatile INT32* pBufferFilledSize;                          ///< Size filled by the encoder
    };

    JPEGEncConfig*          m_pJPEGEncConfigMod;                         ///< JPEG encoder config module
    CmdBufferManager*       m_pJPEGPacketManager;                        ///< Packet managers for JPEG Encoder node
    CmdBufferManager*       m_pJPEGCmdBufferManager;                     ///< Cmd buffer managers for JPEG Encoder node
    CmdBufferManager*       m_pInputParamCmdManager;                     ///< Cmd buffer managers for input params
    INT32                   m_deviceIndex;                               ///< JPEG Encoder device index
    UINT                    m_cmdBufResourceSize;                        ///< JPEG command resource size
    CSLDeviceHandle         m_hDevice;                                   ///< JPEG Encoder device handle
    CSLVersion              m_version;                                   ///< JPEG Encoder Hardware Revision
    FenceCallbackData       m_fenceCallbackData[MaxRequestQueueDepth];   ///< JPEG Encoder outut info
    JPEGQuantTable*         m_pQuantTables;                              ///< Pointer to Quantization tables
    UINT32                  m_quality;                                   ///< JPEG encoder quality
    BOOL                    m_rotationHandledGPU;                        ///< Is there a previous node that handles the rotation
    BOOL                    m_rotationHandledFCV;                        ///< Is there a previous node that handles the rotation
    UINT32                  m_rotatedDimensions[2];                      ///< JPEG input width[0] and height[1] after rotation
    BOOL                    m_bThumbnailEncode;                          ///< if JPEG node for Thumbnail
    UINT32                  m_thumbnailWidthMax;                         ///< Max Thumbnail Width
    UINT32                  m_thumbnailHeightMax;                        ///< Max Thumbnail Height
    UINT32                  m_thumbnailWidthMin;                         ///< Min Thumbnail Width
    UINT32                  m_thumbnailHeightMin;                        ///< Min Thumbnail Height
    UINT32                  m_imageWidthMax;                             ///< Max Main Image Width
    UINT32                  m_imageHeightMax;                            ///< Max Main Image Height
    UINT32                  m_prevModuleScaleFactorMax;                  ///< Max scaling needed from IPE
    BOOL                    m_bPrevModuleScaleOn;                        ///< IPE scaling on
    DimensionCap            m_curThumbnailDim;                           ///< thumbnail dimensions
    UINT                    m_JPEGCmdBlobCount;                          ///< Number of blob command buffers in circulation
};

CAMX_NAMESPACE_END

#endif // CAMXIFENODE_H
