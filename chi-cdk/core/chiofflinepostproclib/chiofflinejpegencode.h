////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2019-2020 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  chiofflinejpegencode.h
/// @brief Declarations for offline jpeg encode
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef CHIOFFLINEJPEGENCODE_H
#define CHIOFFLINEJPEGENCODE_H

#include "chiofflinepostprocbase.h"
#include "chiofflinepostprocencode.h"

// NOWHINE FILE CP023: Base classes are not Interfaces alone

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief Class for offline jpeg encode
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class ChiOfflineJpegEncode : protected ChiFeature2PostProcBase, protected ChiOfflinePostprocEncode
{
public:

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// Create
    ///
    /// @brief  Function to create encoder instance
    ///
    /// @param  pParams     Params to create encoder instance
    ///
    /// @return Encoder pointer
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static ChiFeature2PostProcBase* Create(
        PostProcCreateParams* pParams);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// Destroy
    ///
    /// @brief  Function to destroy encoder instance
    ///
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID Destroy() override;

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// PostProcess
    ///
    /// @brief  Function to encode the input bistream
    ///
    /// @param  pSessionParams  Ptr to SessionParam Structure
    ///
    /// @return Encoder pointer
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    PostProcResultInfo PostProcess(
        PostProcSessionParams*   pSessionParams) override;

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ReleaseResources
    ///
    /// @brief  Function to release resources of the encoder instance
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID ReleaseResources() override
    {
        ReleasePipeline();
    };

private:
    ChiOfflineJpegEncode()                                                = default;
    virtual ~ChiOfflineJpegEncode()                                       = default;
    ChiOfflineJpegEncode(const ChiOfflineJpegEncode& rOther)              = delete;
    ChiOfflineJpegEncode(const ChiOfflineJpegEncode&& rrOther)            = delete;
    ChiOfflineJpegEncode& operator=(const ChiOfflineJpegEncode& rOther)   = delete;
    ChiOfflineJpegEncode& operator=(const ChiOfflineJpegEncode&& rrOther) = delete;

    CHITARGETBUFFERINFOHANDLE   m_hInputImageBuffer;            ///< Input handle Buffer
    CHITARGETBUFFERINFOHANDLE   m_hOutputImageBuffer;           ///< Output Image Buffer
    CHITARGETBUFFERINFOHANDLE   m_hMetaBuffer;                  ///< Input Metadata Buffer
    CHISTREAM                   m_yuv2JpegStreamsInput;         ///< Input stream info
    CHISTREAM                   m_yuv2JpegStreamsOutput;        ///< Output stream info
    CHISTREAMCONFIGINFO         m_yuv2JpegStreamConfigInfo;     ///< Stream Config Info
    CHISTREAM*                  m_pJPEGStreams[2];              ///< Array of Streams
    ChiFeature2InstanceProps    m_instanceProps;                ///< Instance properties
    LogicalCameraInfo           m_logicalCameraInfo;            ///< Logical camera info
    DeviceInfo                  m_logicalDeviceInfo[2];         ///< Logical Device info
    CHITargetBufferManager*     m_pInputImageTBM;               ///< Input Image TBM Handle
    CHITargetBufferManager*     m_pOutputImageTBM;              ///< Output Image TBM Handle
    CHITargetBufferManager*     m_pMetadataTBM;                 ///< Metadata TBM Handle
    CHISTREAMBUFFER             m_inputBuffer;                  ///< Input buffer info
    CHISTREAMBUFFER             m_outputBuffer;                 ///< Output buffer info
    const native_handle_t*      m_phInput;                      ///< Input native handle
    const native_handle_t*      m_phOutput;                     ///< Output native handle
    UINT32                      m_GPURotation;                  ///< GPU rotation angle
    UINT8                       m_overrideFlag;                 ///< Flag to indicate IPE override status

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// InitializeStreamParams
    ///
    /// @brief  Function to Initialize JPEG Stream Params
    ///
    /// @param  pParams     Pointer to Create Parameters
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID InitializeStreamParams(
        PostProcCreateParams* pParams);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ProcessExtraSettings
    ///
    /// @brief  Function to parse extra settings and create metadata accordingly
    ///
    /// @param  pParams     Pointer to Create Parameters
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID ProcessExtraSettings(
        PostProcCreateParams* pParams);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// UpdateSessionParams
    ///
    /// @brief  Function to update session Parameters
    ///
    /// @param  pSessionParams  Pointer to session Parameters
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID UpdateSessionParams(
        PostProcSessionParams*  pSessionParams);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// PrepareChiBuffer
    ///
    /// @brief  Function to prepare chi buffers from native handle
    ///
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID PrepareChiBuffer();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetImageSize
    ///
    /// @brief  Function to calculate image size
    ///
    /// @param  pChiBuffer  Pointer to Chi Buffer
    ///
    /// @return Image Size
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    UINT32 GetImageSize(
        CHISTREAMBUFFER* pChiBuffer);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ValidateOutputBufferHandle
    ///
    /// @brief  Function to validate Output Buffer Handle
    ///
    /// @param  pTargetBuffer   Pointer to Target buffer
    ///
    /// @return TRUE or FALSE
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    UINT8 ValidateOutputBufferHandle(
        CHISTREAMBUFFER* pTargetBuffer);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ProcessFeature2Message
    ///
    /// @brief  Static ProcessMessage function will call this virtual function to process the response
    ///
    /// @param  pFeatureRequestObj  Feature request object containing context
    /// @param  pMessages           The message to process
    ///
    /// @return CDKResultSuccess if successful; error code otherwise
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CDKResult ProcessFeature2Message(
        ChiFeature2RequestObject*   pFeatureRequestObj,
        ChiFeature2Messages*        pMessages) override;

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ProcessGetInputDependencyMessage
    ///
    /// @brief  Function to update input dependent parameters
    ///
    /// @param  pFeatureRequestObj  Pointer to Feature2 Obj
    /// @param  pMessages           Pointer to Message Struct
    ///
    /// @return Feature2Base class pointer
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CDKResult ProcessGetInputDependencyMessage(
        ChiFeature2RequestObject*   pFeatureRequestObj,
        ChiFeature2Messages*        pMessages);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ProcessResultNotificationMessage
    ///
    /// @brief  Function to process result from feature2
    ///
    /// @param  pFeatureRequestObj  Pointer to Feature2 Req object
    /// @param  pMessages           Pointer to Nessage info structure
    ///
    /// @return Feature2Base class pointer
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CDKResult ProcessResultNotificationMessage(
        ChiFeature2RequestObject*   pFeatureRequestObj,
        ChiFeature2Messages*        pMessages);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ProcessReleaseInputDependencyMessage
    ///
    /// @brief  Function to release input dependencies
    ///
    /// @param  pFeatureRequestObj  Pointer to Feature2 Req object
    /// @param  pMessages           Pointer to Nessage info structure
    ///
    /// @return Feature2Base class pointer
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CDKResult ProcessReleaseInputDependencyMessage(
        ChiFeature2RequestObject*   pFeatureRequestObj,
        ChiFeature2Messages*        pMessages);


    // Interface functions from Feature2Encode class

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// InitializeTargetBufferManagers
    ///
    /// @brief  Function to initialize Target Buffer Managers for input and output
    ///
    ///
    /// @return CDKResultSuccess if success or appropriate error code.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CDKResult InitializeTargetBufferManagers() override;

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetGenericFeature2Descriptor
    ///
    /// @brief  Function to update Input Info structure
    ///
    /// @param  pFeature2CreateInputInfoOut  Pointer to Input Info structure
    ///
    /// @return CDKResultSuccess if success or appropriate error code.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CDKResult GetGenericFeature2Descriptor(
        ChiFeature2CreateInputInfo* pFeature2CreateInputInfoOut) override;

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetInputFeature2RequestObject
    ///
    /// @brief  Static Function to create Feature2 Request object
    ///
    /// @param  pFeature2Base               Pointer to Feature2Base class
    /// @param  pMetadata                   Pointer to Metadata
    /// @param  ppFeature2RequestObjectOut  Double Pointer to Feature2 Req Object
    /// @param  pPrivateData                Private data provided Feature2Req Object
    ///
    /// @return CDKResultSuccess or appropriate error code
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CDKResult GetInputFeature2RequestObject(
        ChiFeature2Base*            pFeature2Base,
        ChiMetadata*                pMetadata,
        ChiFeature2RequestObject**  ppFeature2RequestObjectOut,
        VOID*                       pPrivateData) override;

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// CreateFeature2
    ///
    /// @brief  Function to create Feature2Base class
    ///
    /// @param  pFeature2CreateInputInfo    Pointer to Feature2 Create Info
    ///
    /// @return Feature2Base class pointer
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    ChiFeature2Base* CreateFeature2(
        ChiFeature2CreateInputInfo* pFeature2CreateInputInfo) override;
};

#endif // CHIOFFLINEJPEGENCODE_H
