////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2019-2020 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  chiofflinejpegencode.cpp
/// @brief Implementations for offline jpeg encode related functions
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "chifeature2generic.h"
#include "chiofflinejpegencode.h"

// NOWHINE FILE CP006: STL keyword used for vector
// NOWHINE FILE GR022: c-style cast needed for pointer to UINT64 conversion

extern const ChiFeature2Descriptor JPEGFeatureDescriptorGPU;
extern const ChiFeature2Descriptor JPEGFeatureDescriptor;

const UINT32 CameraMetadataEntries  = 1024;   ///< Camera Metadata entry count
const UINT32 MaxNumBuffersNeeded    = 8;      ///< Max Buffers given in stream Config
const UINT32 MaxValidOutputPorts    = 2;      ///< Max valid output ports (currently JPEG and Meta are supported)
const UINT32 MinJPEGImagesize       = 262144; ///< Min JPEG Image Size value (256Kb)
const UINT32 YCRCB_420_SP           = 0X11;   ///< Pixel Format enum value used for NV21

/// @brief Transport header for compressed JPEG buffers in output streams. This structure redefines the camera3_jpeg_blob
///        defined in hardware/camera3.h. Additional information can be found there.
struct ChiJPEGBlob
{
    UINT16  JPEGBlobId;     ///< The JPEG blob ID field must be set to CAMERA3_JPEG_BLOB_ID
    UINT32  JPEGBlobSize;   ///< The JPEG sizein bytes
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///  ChiOfflineJpegEncode::InitializeStreamParams
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID ChiOfflineJpegEncode::InitializeStreamParams(
    PostProcCreateParams* pParams)
{
    m_pInputImageTBM                    = NULL;
    m_pOutputImageTBM                   = NULL;
    m_pMetadataTBM                      = NULL;
    m_hInputImageBuffer                 = NULL;
    m_hMetaBuffer                       = NULL;
    m_hOutputImageBuffer                = NULL;
    m_logicalCameraInfo.ppDeviceInfo    = NULL;
    m_pCameraMetadata                   = NULL;
    m_GPURotation                       = 0;
    m_overrideFlag                      = FALSE;

    ChxUtils::Memset(&m_logicalCameraInfo, 0, sizeof(m_logicalCameraInfo));
    ChxUtils::Memset(&m_yuv2JpegStreamsInput,  0, sizeof(m_yuv2JpegStreamsInput));
    ChxUtils::Memset(&m_yuv2JpegStreamsOutput, 0, sizeof(m_yuv2JpegStreamsOutput));
    ChxUtils::Memset(&m_yuv2JpegStreamConfigInfo, 0, sizeof(m_yuv2JpegStreamConfigInfo));

    m_createParams                       = *pParams;

    m_yuv2JpegStreamsInput.streamType    = ChiStreamTypeInput;
    m_yuv2JpegStreamsInput.width         = pParams->inBuffer.width;
    m_yuv2JpegStreamsInput.height        = pParams->inBuffer.height;
    m_yuv2JpegStreamsInput.format        = static_cast<ChiStreamFormat>(pParams->inBuffer.format);
    m_yuv2JpegStreamsInput.maxNumBuffers = MaxNumBuffersNeeded;
    m_yuv2JpegStreamsInput.rotation      = StreamRotationCCW0;
    m_yuv2JpegStreamsInput.grallocUsage  = GRALLOC1_PRODUCER_USAGE_CAMERA   |
                                           GRALLOC1_PRODUCER_USAGE_CPU_READ |
                                           GRALLOC1_PRODUCER_USAGE_CPU_WRITE;

    m_yuv2JpegStreamsOutput.streamType    = ChiStreamTypeOutput;
    m_yuv2JpegStreamsOutput.width         = pParams->outBuffer.width;
    m_yuv2JpegStreamsOutput.height        = pParams->outBuffer.height;
    m_yuv2JpegStreamsOutput.format        = static_cast<ChiStreamFormat>(pParams->outBuffer.format);
    m_yuv2JpegStreamsOutput.maxNumBuffers = MaxNumBuffersNeeded;
    m_yuv2JpegStreamsOutput.rotation      = StreamRotationCCW0;
    m_yuv2JpegStreamsOutput.grallocUsage  = GRALLOC1_PRODUCER_USAGE_CAMERA   |
                                            GRALLOC1_PRODUCER_USAGE_CPU_READ |
                                            GRALLOC1_PRODUCER_USAGE_CPU_WRITE;

    if ((ChiStreamFormatYCrCb420_SP == m_yuv2JpegStreamsInput.format) ||
        (YCRCB_420_SP == pParams->inBuffer.format))
    {
        m_yuv2JpegStreamsInput.grallocUsage |= GrallocUsageSwReadOften;
        m_yuv2JpegStreamsInput.format        = ChiStreamFormatYCbCr420_888;
    }

    m_pJPEGStreams[0] = &m_yuv2JpegStreamsInput;
    m_pJPEGStreams[1] = &m_yuv2JpegStreamsOutput;

    m_yuv2JpegStreamConfigInfo.numStreams  = 2;
    m_yuv2JpegStreamConfigInfo.pChiStreams = m_pJPEGStreams;

}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///  ChiOfflineJpegEncode::Create
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
ChiFeature2PostProcBase* ChiOfflineJpegEncode::Create(
    PostProcCreateParams* pParams)
{
    ChiOfflineJpegEncode* pEncoder = CHX_NEW ChiOfflineJpegEncode;

    if (NULL != pEncoder)
    {
        // Parent class setup
        CDKResult result = pEncoder->Setup();

        if (CDKResultSuccess == result)
        {
            // Initialize Stream and TBM params
            pEncoder->InitializeStreamParams(pParams);

            result = pEncoder->InitializeTargetBufferManagers();
        }

        if (CDKResultSuccess != result)
        {
            CHX_LOG_ERROR("Setup failed, return NULL");
            pEncoder->Destroy();
            pEncoder = NULL;
        }
    }
    else
    {
        CHX_LOG_ERROR("pEncoder is NULL, malloc failed");
    }

    return static_cast<ChiFeature2PostProcBase*>(pEncoder);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///  ChiOfflineJpegEncode::Destroy
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID ChiOfflineJpegEncode::Destroy()
{
    // Release Internal TBM firstly, and Metadata TBM
    // will use parent class's MatadataManager, so release
    // child TBM firstly.
    if (NULL != m_pInputImageTBM)
    {
        m_pInputImageTBM->Destroy();
        m_pInputImageTBM = NULL;
    }
    if (NULL != m_pOutputImageTBM)
    {
        m_pOutputImageTBM->Destroy();
        m_pOutputImageTBM = NULL;
    }

    if (NULL != m_pMetadataTBM)
    {
        m_pMetadataTBM->Destroy();
        m_pMetadataTBM = NULL;
    }

    // Reset all variables in parent class
    ChiOfflinePostprocEncode::Teardown();

    if (NULL != m_logicalCameraInfo.ppDeviceInfo)
    {
        CHX_DELETE m_logicalCameraInfo.ppDeviceInfo;
        m_logicalCameraInfo.ppDeviceInfo = NULL;
    }

    CHX_DELETE this;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//  ChiOfflineJpegEncode::PostProcess
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
PostProcResultInfo ChiOfflineJpegEncode::PostProcess(
    PostProcSessionParams* pSessionParams)
{
    CDKResult result;
    UINT32    rotation      = 0;
    BOOL      overrideScale = FALSE;

    // Reset output structure
    m_output.size    = 0;
    m_output.result  = POSTPROCFAILED;

    if (NULL != pSessionParams->pMetadata)
    {
        camera_metadata_entry_t entry = { 0 };

        if (0 == find_camera_metadata_entry(pSessionParams->pMetadata, ANDROID_JPEG_ORIENTATION, &entry))
        {
            rotation = *(entry.data.i32);
        }

        if (0 == find_camera_metadata_entry(pSessionParams->pMetadata,
                                            ANDROID_NOISE_REDUCTION_AVAILABLE_NOISE_REDUCTION_MODES, &entry))
        {
            INT count = static_cast<INT>(entry.count);
            for (INT index = 0; index < count; ++index)
            {
                if (ANDROID_NOISE_REDUCTION_MODE_HIGH_QUALITY == (entry.data.u8[index]))
                {
                    overrideScale = TRUE;
                    break;
                }
            }
        }

        // Check if ANDROID_NOISE_REDUCTION_MODE is directly set
        if (FALSE == overrideScale)
        {
            if (0 == find_camera_metadata_entry(pSessionParams->pMetadata,
                                                ANDROID_NOISE_REDUCTION_MODE, &entry))
            {
                UINT8 mode = *(entry.data.u8);;

                if (ANDROID_NOISE_REDUCTION_MODE_HIGH_QUALITY == mode)
                {
                    overrideScale = TRUE;
                }
            }
        }
    }

    if ((pSessionParams->inHandle[0].width != m_yuv2JpegStreamsInput.width) ||
        (pSessionParams->inHandle[0].height != m_yuv2JpegStreamsInput.height) ||
        (m_GPURotation != rotation) ||
        (m_overrideFlag != overrideScale))
    {
        CHX_LOG_INFO("Updated Session Params are received, rotation %d, overrideScale %d, WxH %dx%d,"
                     " grallocUsage 0x%x, chistream format 0x%x",
                     rotation, overrideScale, pSessionParams->inHandle[0].width,
                     pSessionParams->inHandle[0].height,
                     m_yuv2JpegStreamsInput.grallocUsage, m_yuv2JpegStreamsInput.format);
        DestroyFeature2Object();
        FreeCameraMetadata();
        m_GPURotation   = rotation;
        m_overrideFlag  = overrideScale;
        UpdateSessionParams(pSessionParams);
    }
    else if (NULL == GetBasePtr())
    {
        CHX_LOG_INFO("Base is already destroyed, Re-init");
        FreeCameraMetadata();
        UpdateSessionParams(pSessionParams);
    }

    m_frameNumber   = pSessionParams->frameNum;
    m_phInput       = pSessionParams->inHandle[0].phHandle;;
    m_phOutput      = pSessionParams->outHandle[0].phHandle;

    PrepareChiBuffer();

    ChiMetadata* pMetadata = m_pMetadataManager->Get(m_genericMetadataClientId, 0);

    if (NULL != pMetadata)
    {
        pMetadata->SetAndroidMetadata(pSessionParams->pMetadata);

        // Allocate camera metadata to pass extra session parameters
        if ((NULL == m_pCameraMetadata) &&
            ((0 != m_overrideFlag) || (0 != m_GPURotation) || (NULL != pSessionParams->pMetadata)))
        {
            m_pCameraMetadata = allocate_camera_metadata(CameraMetadataEntries,
                                                         CameraMetadataEntries * sizeof(UINT64));

            // Set override IPE Scale flag to TRUE
            if (0 != m_overrideFlag)
            {
                result = pMetadata->SetTag("org.quic.camera.overrideIPEScaleProfile",
                                           "OverrideIPEScaleProfile", static_cast<UINT8*>(&m_overrideFlag), 1);
            }

            // Set override GPU rotation flag to TRUE
            if (0 != m_GPURotation)
            {
                UINT8 rotation = 1;
                result = pMetadata->SetTag("org.quic.camera.overrideGPURotationUsecase",
                                           "OverrideGPURotationUsecase", &rotation, 1);
            }

            pMetadata->TranslateToCameraMetadata(m_pCameraMetadata, FALSE);

            // Provide android metadata to StreamConfig
            m_yuv2JpegStreamConfigInfo.pSessionSettings = m_pCameraMetadata;
            CHX_LOG_INFO("android metadata pointer is %p, m_overrideFlag %d, rotation %d",
                         m_pCameraMetadata, m_overrideFlag, m_GPURotation);
        }
        else
        {
            pMetadata->TranslateToCameraMetadata(m_pCameraMetadata, FALSE);
        }

        UpdateDebugMetadata(pMetadata);
        DoEncode();
    }
    else
    {
        CHX_LOG_ERROR("pMetadata is NULL");
    }

    return m_output;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//  ChiOfflineJpegEncode::PrepareChiBuffer
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID ChiOfflineJpegEncode::PrepareChiBuffer()
{
    m_inputBuffer.size                  = sizeof(CHISTREAMBUFFER);
    m_inputBuffer.pStream               = &m_yuv2JpegStreamsInput;
    m_inputBuffer.bufferInfo.bufferType = HALGralloc;
    m_inputBuffer.bufferInfo.phBuffer   = reinterpret_cast<CHIBUFFERHANDLE>(&m_phInput);
    m_inputBuffer.bufferStatus          = CAMERA3_BUFFER_STATUS_OK;
    m_inputBuffer.acquireFence.valid    = FALSE;
    m_inputBuffer.releaseFence.valid    = FALSE;

    m_outputBuffer.size                  = sizeof(CHISTREAMBUFFER);
    m_outputBuffer.pStream               = &m_yuv2JpegStreamsOutput;
    m_outputBuffer.bufferInfo.bufferType = HALGralloc;
    m_outputBuffer.bufferInfo.phBuffer   = reinterpret_cast<CHIBUFFERHANDLE>(&m_phOutput);
    m_outputBuffer.bufferStatus          = CAMERA3_BUFFER_STATUS_OK;
    m_outputBuffer.acquireFence.valid    = FALSE;
    m_outputBuffer.releaseFence.valid    = FALSE;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//  ChiOfflineJpegEncode::UpdateSessionParams
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID ChiOfflineJpegEncode::UpdateSessionParams(
    PostProcSessionParams* pSessionParams)
{
    ChxUtils::Memset(&m_yuv2JpegStreamsInput,  0, sizeof(m_yuv2JpegStreamsInput));
    ChxUtils::Memset(&m_yuv2JpegStreamsOutput, 0, sizeof(m_yuv2JpegStreamsOutput));

    m_yuv2JpegStreamsInput.streamType    = ChiStreamTypeInput;
    m_yuv2JpegStreamsInput.width         = pSessionParams->inHandle[0].width;
    m_yuv2JpegStreamsInput.height        = pSessionParams->inHandle[0].height;
    m_yuv2JpegStreamsInput.format        = static_cast<ChiStreamFormat>(pSessionParams->inHandle[0].format);
    m_yuv2JpegStreamsInput.maxNumBuffers = MaxNumBuffersNeeded;
    m_yuv2JpegStreamsInput.rotation      = StreamRotationCCW0;
    m_yuv2JpegStreamsInput.grallocUsage  = GRALLOC1_PRODUCER_USAGE_CAMERA   |
                                           GRALLOC1_PRODUCER_USAGE_CPU_READ |
                                           GRALLOC1_PRODUCER_USAGE_CPU_WRITE;

    m_yuv2JpegStreamsOutput.streamType    = ChiStreamTypeOutput;
    m_yuv2JpegStreamsOutput.width         = pSessionParams->outHandle[0].width;
    m_yuv2JpegStreamsOutput.height        = pSessionParams->outHandle[0].height;
    m_yuv2JpegStreamsOutput.format        = static_cast<ChiStreamFormat>(pSessionParams->outHandle[0].format);
    m_yuv2JpegStreamsOutput.maxNumBuffers = MaxNumBuffersNeeded;
    m_yuv2JpegStreamsOutput.rotation      = StreamRotationCCW0;
    m_yuv2JpegStreamsOutput.grallocUsage  = GRALLOC1_PRODUCER_USAGE_CAMERA   |
                                            GRALLOC1_PRODUCER_USAGE_CPU_READ |
                                            GRALLOC1_PRODUCER_USAGE_CPU_WRITE;

    if ((ChiStreamFormatYCrCb420_SP == m_yuv2JpegStreamsInput.format) ||
        (YCRCB_420_SP == pSessionParams->inHandle[0].format))
    {
        m_yuv2JpegStreamsInput.grallocUsage |= GrallocUsageSwReadOften;
        m_yuv2JpegStreamsInput.format        = ChiStreamFormatYCbCr420_888;
    }

    // Re-init TBM with new dimensions
    InitializeTargetBufferManagers();
    return;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//  ChiOfflineJpegEncode::GetImageSize
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UINT32 ChiOfflineJpegEncode::GetImageSize(
    CHISTREAMBUFFER* pChiBuffer)
{
    CHISTREAM*  pResultStream  = pChiBuffer->pStream;

    UINT32 width      = pResultStream->width;
    UINT32 height     = pResultStream->height;
    UINT32 format     = pResultStream->format;
    UINT32 dataspace  = pResultStream->dataspace;
    UINT32 imageSize  = 0;

    switch (format)
    {
        case HAL_PIXEL_FORMAT_BLOB:
        {
            UINT32       kMinJpegSize = MinJPEGImagesize + sizeof(ChiJPEGBlob);
            UINT32       bufSize      = width * height;
            CHAR*        pBuf         = static_cast<CHAR*>(ChxUtils::MemMap(m_phOutput->data[0], bufSize, 0));
            ChiJPEGBlob* pBlobPtr     = NULL;

            if (NULL != pBuf)
            {
                pBlobPtr = reinterpret_cast<ChiJPEGBlob*>(pBuf + bufSize - sizeof(ChiJPEGBlob));
                imageSize = pBlobPtr->JPEGBlobSize;
            }
            else
            {
                CHX_LOG_ERROR("pBuf is NULL, thus pBlobPtr is NULL | imageSize: %u", imageSize);
            }

            if (kMinJpegSize > imageSize)
            {
                if ((HAL_DATASPACE_V0_JFIF == dataspace) ||
                    (HAL_DATASPACE_JFIF    == dataspace))
                {
                    imageSize = (width * height * 4);
                }
                else
                {
                    imageSize = ((width * height * 2) / 3);
                }
            }

            if (NULL != pBlobPtr)
            {
                CHX_LOG_VERBOSE("blob structure info Id %d, JPEGBlobSize %d, bufSize %u, imageSize %u, kMinJpegSize %u",
                    pBlobPtr->JPEGBlobId, pBlobPtr->JPEGBlobSize, bufSize, imageSize, kMinJpegSize);
            }

            break;
        }

        case HAL_PIXEL_FORMAT_RAW_OPAQUE:
            imageSize = ((width * height * 3) / 2);
            break;

        case HAL_PIXEL_FORMAT_RAW10:
            // 4 pixels packed into 5 bytes, width must be multiple of 4 pixels
            imageSize = static_cast<UINT32>(ceil(width / 4.0) * 5 * height);
            break;

        case HAL_PIXEL_FORMAT_Y8:
            // 1 pixel packed into 1 byte (8 bits), width must be multiple of 2 pixels
            imageSize = static_cast<UINT32>(ceil(width / 2.0) * 2 * height);
            break;

        case HAL_PIXEL_FORMAT_Y16:
            // Same as Y8, but double the bits per pixel (1 pixel packed into 2 bytes)
            imageSize = static_cast<UINT32>(ceil(width / 2.0) * 4 * height);
            break;

        default:
            imageSize = (width * height);
            break;
    }

    m_output.size    = imageSize;
    m_output.result  = POSTPROCSUCCESS;

    return imageSize;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//  ChiOfflineJpegEncode::ValidateOutputBufferHandle
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UINT8 ChiOfflineJpegEncode::ValidateOutputBufferHandle(
    CHISTREAMBUFFER* pTargetBuffer)
{
    UINT8 valid = false;

    if (m_outputBuffer.bufferInfo.phBuffer == pTargetBuffer->bufferInfo.phBuffer)
    {
        valid = true;
        GetImageSize(pTargetBuffer);
    }

    return valid;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//  ChiOfflineJpegEncode::InitializeTargetBufferManagers
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ChiOfflineJpegEncode::InitializeTargetBufferManagers()
{
    CDKResult result = CDKResultSuccess;

    ChiTargetBufferManagerCreateData inputImageTBMCreateData    = { 0 };
    ChiTargetBufferManagerCreateData outputImageTBMCreateData   = { 0 };
    ChiTargetBufferManagerCreateData metadataTBMCreateData      = { 0 };

    const CHAR* pInputImageTargetBufferName  = "inYUVTarget";
    const CHAR* pOutputImageTargetBufferName = "outJpegTarget";

    if ( NULL != m_pInputImageTBM)
    {
        m_pInputImageTBM->Destroy();
    }

    if ( NULL != m_pOutputImageTBM)
    {
        m_pOutputImageTBM->Destroy();
    }

    if ( NULL != m_pMetadataTBM)
    {
        m_pMetadataTBM->Destroy();
    }

    // Create input image target buffer manager
    inputImageTBMCreateData.pTargetBufferName          = pInputImageTargetBufferName;
    inputImageTBMCreateData.numOfMetadataBuffers       = 0;
    inputImageTBMCreateData.numOfInternalStreamBuffers = 0;
    inputImageTBMCreateData.numOfExternalStreamBuffers = 1;
    inputImageTBMCreateData.externalStreamIds[0]       = (UINT64)&m_yuv2JpegStreamsInput;
    m_pInputImageTBM = CHITargetBufferManager::Create(&inputImageTBMCreateData);

    // Create output image target buffer manager
    outputImageTBMCreateData.pTargetBufferName           = pOutputImageTargetBufferName;
    outputImageTBMCreateData.numOfMetadataBuffers        = 0;
    outputImageTBMCreateData.numOfInternalStreamBuffers  = 0;
    outputImageTBMCreateData.numOfExternalStreamBuffers  = 1;
    outputImageTBMCreateData.externalStreamIds[0]        = (UINT64)&m_yuv2JpegStreamsOutput;
    m_pOutputImageTBM = CHITargetBufferManager::Create(&outputImageTBMCreateData);

    // Create metadata target buffer manager
    metadataTBMCreateData.pTargetBufferName             = "MetadataTarget";
    metadataTBMCreateData.numOfMetadataBuffers          = 1;
    metadataTBMCreateData.numOfInternalStreamBuffers    = 0;
    metadataTBMCreateData.numOfExternalStreamBuffers    = 0;
    metadataTBMCreateData.pChiMetadataManager           = GetMetadataManager();
    metadataTBMCreateData.metadataIds[0]                = GetMetadataClientId();;
    m_pMetadataTBM = CHITargetBufferManager::Create(&metadataTBMCreateData);

    if ((NULL == m_pInputImageTBM) || (NULL == m_pMetadataTBM))
    {
        result = CDKResultEFailed;
        CHX_LOG_ERROR("TBM creation failed!");
    }
    else
    {
        CHX_LOG_VERBOSE("TBM creation success!");
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//  ChiOfflineJpegEncode::GetGenericFeature2Descriptor
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ChiOfflineJpegEncode::GetGenericFeature2Descriptor(
    ChiFeature2CreateInputInfo* pFeature2CreateInputInfoOut)
{
    CDKResult                    result             = CDKResultSuccess;
    const ChiFeature2Descriptor* pFeatureDescriptor = NULL;

    switch (m_createParams.processMode)
    {
        case YUVToJPEG:
        {
            pFeatureDescriptor = &JPEGFeatureDescriptorGPU;
        }
        break;

        default:
            CHX_LOG_ERROR("Invalid test ID! (%d)", m_createParams.processMode);
            result = CDKResultEInvalidArg;
            break;
    }

    m_instanceProps          = {0};
    m_instanceProps.cameraId = m_selectCam;

    if (NULL == m_logicalCameraInfo.ppDeviceInfo)
    {
        m_logicalCameraInfo                     = {0};
        m_logicalDeviceInfo[0]                  = {0};
        m_logicalCameraInfo.cameraId            = m_selectCam;
        m_logicalCameraInfo.numPhysicalCameras  = 1;
        m_logicalDeviceInfo[0].cameraId         = m_selectCam;
        m_logicalCameraInfo.ppDeviceInfo        = CHX_NEW (DeviceInfo*);
        m_logicalCameraInfo.ppDeviceInfo[0]     = &m_logicalDeviceInfo[0];
        // get it from here :CHXExtensionModule::GetPhysicalCameraInfo(0);

        if (NULL == m_logicalCameraInfo.ppDeviceInfo)
        {
            CHX_LOG_ERROR("ppDeviceInfo malloc failed");
            result = CDKResultENoMemory;
        }
    }

    if (CDKResultSuccess == result)
    {
        pFeature2CreateInputInfoOut->pInstanceProps     = &m_instanceProps;
        pFeature2CreateInputInfoOut->pFeatureDescriptor = static_cast<const ChiFeature2Descriptor*>(pFeatureDescriptor);
        // const_cast<LogicalCameraInfo*> (ExtensionModule::GetInstance()->GetPhysicalCameraInfo(0));
        pFeature2CreateInputInfoOut->pCameraInfo        = &m_logicalCameraInfo;
        pFeature2CreateInputInfoOut->pUsecaseDescriptor = g_pUsecaseZSL;
        pFeature2CreateInputInfoOut->pStreamConfig      = &m_yuv2JpegStreamConfigInfo;
        CHX_LOG_VERBOSE("Result: GetFeature2Descriptor out numPhysicalCameras %d",
                        pFeature2CreateInputInfoOut->pCameraInfo->numPhysicalCameras);
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//  ChiOfflineJpegEncode::CreateFeature2
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
ChiFeature2Base* ChiOfflineJpegEncode::CreateFeature2(
    ChiFeature2CreateInputInfo* pFeature2CreateInputInfo)
{
    ChiFeature2Base* pFeature2Base = ChiFeature2Generic::Create(pFeature2CreateInputInfo);
    return pFeature2Base;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//  ChiOfflineJpegEncode::GetInputFeature2RequestObject
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ChiOfflineJpegEncode::GetInputFeature2RequestObject(
    ChiFeature2Base*            pFeature2Base,
    ChiMetadata*                pMetadata,
    ChiFeature2RequestObject**  ppFeature2RequestObjectOut,
    VOID*                       pPrivateData)
{
    CDKResult result                 = CDKResultSuccess;
    UINT8     numExternalOutputPorts = 0;

    ChiFeature2RequestObjectCreateInputInfo feature2RequestObjInputInfo                   = { 0 };
    ChiFeature2RequestOutputInfo            featureRequestOutputInfo[MaxValidOutputPorts] = { { 0 } };

    // Create usecaseRequestObject with frame number
    camera3_capture_request_t halRequest    = { 0 };
    halRequest.frame_number                 = m_frameNumber;

    ChiFeature2UsecaseRequestObjectCreateInputInfo usecaseRequestObjectCreateInputInfo = { 0 };

    usecaseRequestObjectCreateInputInfo.pRequest        = &halRequest;
    usecaseRequestObjectCreateInputInfo.pAppSettings    = pMetadata;
    ChiFeature2UsecaseRequestObject* pUsecaseRequestObj =
        ChiFeature2UsecaseRequestObject::Create(&usecaseRequestObjectCreateInputInfo);

    feature2RequestObjInputInfo.pUsecaseRequestObj  = pUsecaseRequestObj;
    feature2RequestObjInputInfo.pGraphPrivateData   = pPrivateData;
    feature2RequestObjInputInfo.pFeatureBase        = pFeature2Base;

    // Update GPU Rotation info in URO
    if ((0 != m_GPURotation) && (NULL != pUsecaseRequestObj))
    {
        pUsecaseRequestObj->SetGPURotationFlag(TRUE);
    }

    // Count external ports
    std::vector<ChiFeature2Identifier> identifiers = pFeature2Base->GetExternalGlobalPortIdList();
    for (UINT8 portIndex = 0; portIndex < identifiers.size(); portIndex++)
    {
        if (ChiFeature2PortDirectionType::ExternalOutput == identifiers[portIndex].portDirectionType)
        {
            numExternalOutputPorts++;
        }
    }

    INT externalIndex = 0;

    std::shared_ptr<ChiFeature2RequestOutputInfo> pRequestOutputs(CHX_NEW ChiFeature2RequestOutputInfo[numExternalOutputPorts]);

    // Setup output port info
    for (UINT8 portIndex = 0; portIndex < identifiers.size(); portIndex++)
    {
        INT validEntry = 0;
        INT validIndex = 0;

        if (ChiFeature2PortDirectionType::ExternalOutput == identifiers[portIndex].portDirectionType)
        {
            const ChiFeature2PortDescriptor* pPortDescriptor =
                pFeature2Base->GetPortDescriptorFromPortId((&identifiers[portIndex]));

            if (NULL != pPortDescriptor)
            {
                // Setup output image buffers
                if ((ChiFeature2PortType::ImageBuffer == identifiers[portIndex].portType) &&
                    (0 == m_GPURotation) &&
                    (!CdkUtils::StrCmp("JPEG_Out", pPortDescriptor->pPortName)))
                {
                    validIndex = TRUE;
                    validEntry = TRUE;
                }
                else if ((ChiFeature2PortType::ImageBuffer == identifiers[portIndex].portType) &&
                         (0 != m_GPURotation) &&
                         (!CdkUtils::StrCmp("JPEG_Out2", pPortDescriptor->pPortName)))
                {
                    validIndex = TRUE;
                    validEntry = TRUE;
                }

                if ((ChiFeature2PortType::MetaData == identifiers[portIndex].portType))
                {
                    validEntry = TRUE;
                    featureRequestOutputInfo[externalIndex].pPortDescriptor = pPortDescriptor;
                }

                CHX_LOG_VERBOSE("portId session %d, pipline %d, port %d, type %d, validEntry %d, name %s",
                                identifiers[portIndex].session, identifiers[portIndex].pipeline,
                                identifiers[portIndex].port, identifiers[portIndex].portType,
                                validEntry, pPortDescriptor->pPortName);
            }

            if (TRUE == validIndex)
            {
                ChiFeature2BufferMetadataInfo bufferMetaInfo = { 0 };
                UINT64                        targetId       = m_frameNumber;

                featureRequestOutputInfo[externalIndex].pPortDescriptor = pPortDescriptor;

                // Import output image buffer
                result = m_pOutputImageTBM->ImportExternalTargetBuffer(m_frameNumber,
                                                                       (UINT64)(&m_yuv2JpegStreamsOutput),
                                                                       &m_outputBuffer);
                if (CDKResultSuccess == result)
                {
                    m_hOutputImageBuffer = m_pOutputImageTBM->SetupTargetBuffer(targetId);
                }
                else
                {
                    m_hOutputImageBuffer = NULL;
                    CHX_LOG_ERROR("Output image buffer import failed!");
                }

                bufferMetaInfo.hBuffer = m_hOutputImageBuffer;
                bufferMetaInfo.key = (UINT64)(&m_yuv2JpegStreamsOutput);
                featureRequestOutputInfo[externalIndex].bufferMetadataInfo = bufferMetaInfo;

                CHX_LOG_INFO("outputimagebuffer %p, frame num %d, m_yuv2JpegStreamsOutput %p",
                             m_hOutputImageBuffer, m_frameNumber, &m_yuv2JpegStreamsOutput);
            }

            if (TRUE == validEntry)
            {
                pRequestOutputs.get()[externalIndex] = featureRequestOutputInfo[externalIndex];
                externalIndex++;
            }
        }
    }

    std::shared_ptr<ChiFeature2RequestMap> pRequestTable (CHX_NEW ChiFeature2RequestMap);
    // Hardcode number of Request as 1
    feature2RequestObjInputInfo.numRequests     = 1;
    feature2RequestObjInputInfo.pRequestTable   = pRequestTable.get();
    // Since there is only 1 request we are directly indexing as 0
    pRequestTable->requestIndex                 = 0;
    pRequestTable->numRequestOutputs            = externalIndex;
    pRequestTable->pRequestOutputs              = pRequestOutputs.get();

    if (CDKResultSuccess == result)
    {
        // Create Feature2 request object
        *ppFeature2RequestObjectOut = ChiFeature2RequestObject::Create(&feature2RequestObjInputInfo);

        if (NULL == *ppFeature2RequestObjectOut)
        {
            CHX_LOG_ERROR("ppFeature2RequestObjectOut Create Failed");
            result = CDKResultENoMemory;
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//  ChiOfflineJpegEncode::ProcessFeature2Message
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ChiOfflineJpegEncode::ProcessFeature2Message(
    ChiFeature2RequestObject*   pFeatureRequestObj,
    ChiFeature2Messages*        pMessages)
{
    CDKResult result = CDKResultSuccess;

    if (NULL == pMessages->pFeatureMessages)
    {
        CHX_LOG_ERROR("NULL pFeatureMessages pointer %p", pMessages->pFeatureMessages);
        result = CDKResultEInvalidArg;
    }

    if (CDKResultSuccess == result)
    {
        switch (pMessages->pFeatureMessages->messageType)
        {
            case ChiFeature2MessageType::GetInputDependency:
            {
                result = ProcessGetInputDependencyMessage(pFeatureRequestObj, pMessages);

                if (CDKResultSuccess != result)
                {
                    CHX_LOG_ERROR("Failed to process get input dependency message!");
                }
                break;
            }

            case ChiFeature2MessageType::ResultNotification:
            case ChiFeature2MessageType::MetadataNotification:
            {
                result = ProcessResultNotificationMessage(pFeatureRequestObj, pMessages);

                if (CDKResultSuccess != result)
                {
                    CHX_LOG_ERROR("Failed to process result notification message!");
                }
                break;
            }

            case ChiFeature2MessageType::ReleaseInputDependency:
            {
                result = ProcessReleaseInputDependencyMessage(pFeatureRequestObj, pMessages);

                if (CDKResultSuccess != result)
                {
                    CHX_LOG_ERROR("Failed to process release input dependency message!");
                }
                break;
            }

            case ChiFeature2MessageType::PartialMetadataNotification:
            {
                CHX_LOG_VERBOSE("PartialMetadataNotification is ignored");
                break;
            }

            // SubmitRequestNotification is handled by base class
            case ChiFeature2MessageType::SubmitRequestNotification:
            {
                break;
            }

            default:
                CHX_LOG_WARN("Unknown ChiFeature2MessageType %d", pMessages->pFeatureMessages->messageType);
                break;
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//  ChiOfflineJpegEncode::ProcessGetInputDependencyMessage
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ChiOfflineJpegEncode::ProcessGetInputDependencyMessage(
    ChiFeature2RequestObject* pFeatureRequestObj,
    ChiFeature2Messages*      pMessages)
{
    CDKResult result = CDKResultSuccess;

    ChiFeature2PortIdList portIds =
    {
        pMessages->pFeatureMessages->message.getDependencyData.pBatches[0].pDependencies[0].numPorts,
        pMessages->pFeatureMessages->message.getDependencyData.pBatches[0].pDependencies[0].pPorts
    };

    UINT32       clientId  = GetMetadataClientId();
    ChiMetadata* pMetadata = GetMetadataManager()->Get(clientId, 0);

    CHX_LOG_VERBOSE("GetInputDependency number of input ports: %u", portIds.numPorts);

    // Iterate through all ports, satisfying their dependencies
    for (UINT32 portIndex = 0; portIndex < portIds.numPorts; portIndex++)
    {
        ChiFeature2Identifier portIdentifier = portIds.pPorts[portIndex];
        CHX_LOG_VERBOSE("portId session %d, pipline %d, port %d, type %d", portIdentifier.session,
                portIdentifier.pipeline, portIdentifier.port, portIdentifier.portType);

        ChiFeature2BufferMetadataInfo bufferMetaInfo = { 0 };
        UINT64 seqId = m_frameNumber;

        switch (portIdentifier.portType)
        {
            case ChiFeature2PortType::ImageBuffer:
            {
                if (((0 == m_GPURotation) && (0 == portIdentifier.port)) ||
                    ((0 != m_GPURotation) && (0 != portIdentifier.port)))
                {
                    // Import input image buffer
                    result = m_pInputImageTBM->ImportExternalTargetBuffer(seqId,
                                                                          (UINT64)(&m_yuv2JpegStreamsInput),
                                                                          &m_inputBuffer);

                    if (CDKResultSuccess == result)
                    {
                        m_hInputImageBuffer = m_pInputImageTBM->SetupTargetBuffer(seqId);
                    }
                    else
                    {
                        m_hInputImageBuffer = NULL;
                        CHX_LOG_ERROR("Input image buffer import failed!");
                    }

                    bufferMetaInfo.hBuffer = m_hInputImageBuffer;
                    bufferMetaInfo.key     = (UINT64)(&m_yuv2JpegStreamsInput);
                    CHX_LOG_INFO("Rotation %d, port %d is updated with input handle",
                                 m_GPURotation, portIdentifier.port);
                }

                break;
            }

            case ChiFeature2PortType::MetaData:
            {
                // Set Android metadata before import for each URO
                if (NULL != pMetadata)
                {
                    pMetadata->SetAndroidMetadata(m_pCameraMetadata);
                }

                // Import input metadata buffer
                result = m_pMetadataTBM->ImportExternalTargetBuffer(seqId,
                                                                    static_cast<UINT64>(clientId),
                                                                    pMetadata);

                if (CDKResultSuccess == result)
                {
                    m_hMetaBuffer = m_pMetadataTBM->SetupTargetBuffer(seqId);
                }
                else
                {
                    m_hMetaBuffer = NULL;
                    CHX_LOG_ERROR("Metadata buffer import failed!");
                }

                bufferMetaInfo.hBuffer = m_hMetaBuffer;
                bufferMetaInfo.key = static_cast<UINT64>(clientId);
                break;
            }

            default:
                CHX_LOG_ERROR("Invalid port type! (%d)", portIdentifier.portType);
                break;
        }

        result = pFeatureRequestObj->SetBufferInfo(ChiFeature2RequestObjectOpsType::InputDependency,
                                                   &portIdentifier,
                                                   bufferMetaInfo.hBuffer,
                                                   bufferMetaInfo.key,
                                                   FALSE, 0, 0);

        if (CDKResultSuccess != result)
        {
            CHX_LOG_ERROR("Failed to set buffer info!");
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//  ChiOfflineJpegEncode::ProcessResultNotificationMessage
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ChiOfflineJpegEncode::ProcessResultNotificationMessage(
    ChiFeature2RequestObject*   pFeatureRequestObj,
    ChiFeature2Messages*        pMessages)
{
    CDKResult                      result                = CDKResultSuccess;
    ChiFeature2BufferMetadataInfo* pOutputBufferMetaInfo = NULL;
    CHISTREAMBUFFER*               pTargetBuffer         = NULL;

    CHX_LOG_VERBOSE("number of ouput ports %d", pMessages->pFeatureMessages->message.result.numPorts);

    // Iterate through all ports
    for (UINT i = 0; i < pMessages->pFeatureMessages->message.result.numPorts; ++i)
    {
        ChiFeature2Identifier portIdentifier = pMessages->pFeatureMessages->message.result.pPorts[i];

        result = pFeatureRequestObj->GetFinalBufferMetadataInfo(portIdentifier, &pOutputBufferMetaInfo, 0);

        CHITargetBufferManager* pTargetBufferManager =
            CHITargetBufferManager::GetTargetBufferManager(pOutputBufferMetaInfo->hBuffer);

        // Process image buffer
        if (ChiFeature2PortType::ImageBuffer == portIdentifier.portType)
        {
            if (NULL != pTargetBufferManager)
            {
                pTargetBuffer = static_cast<CHISTREAMBUFFER*>(pTargetBufferManager->GetTarget(
                    pOutputBufferMetaInfo->hBuffer, pOutputBufferMetaInfo->key));

                if (NULL != pTargetBuffer)
                {
                    if (true == ValidateOutputBufferHandle(pTargetBuffer))
                    {
                        CHX_LOG_VERBOSE("buffer info for handle is valid");
                    }
                    else
                    {
                        CHX_LOG_ERROR("buffer info for handle is not matching with class buf handle");
                        result = CDKResultEInvalidArg;
                    }
                }
                else
                {
                    CHX_LOG_ERROR("pTargetBuffer is NULL");
                    result = CDKResultENoSuch;
                }
            }
            else
            {
                CHX_LOG_ERROR("pTargetBufferManager is NULL for frameNumber %d", m_frameNumber);
                result = CDKResultEInvalidArg;
            }

            CHX_LOG_INFO("ImageBuffer result came for frameNumber %u, imageSize %u",
                         m_frameNumber, m_output.size);
        }

        CHX_LOG_VERBOSE("SetOutputNotifiedForPort called for port type %d", portIdentifier.portType);
        // Need to send ProcessRequest so base can release the buffers associated with this port
        pFeatureRequestObj->SetOutputNotifiedForPort(portIdentifier, 0);
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//  ChiOfflineJpegEncode::ProcessReleaseInputDependencyMessage
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ChiOfflineJpegEncode::ProcessReleaseInputDependencyMessage(
    ChiFeature2RequestObject*   pFeatureRequestObj,
    ChiFeature2Messages*        pMessages)
{
    CDK_UNUSED_PARAM(pFeatureRequestObj);
    CDKResult result = CDKResultSuccess;

    // Iterate through all dependencies and all input ports in message to mark the input resources for release
    UINT8 numDependencies = pMessages->pFeatureMessages->message.releaseDependencyData.numDependencies;
    ChiFeature2Dependency* pDependencies =
        pMessages->pFeatureMessages->message.releaseDependencyData.pDependencies;

    CHX_LOG_VERBOSE("ReleaseInputDependency numDependencies: %u", numDependencies);

    for (UINT8 dependencyIndex = 0; dependencyIndex < numDependencies; ++dependencyIndex)
    {
        UINT numInputPorts = pDependencies[dependencyIndex].numPorts;
        const ChiFeature2Identifier* pInputPorts = pDependencies[dependencyIndex].pPorts;

        // Iterate through all input ports in the message
        for (UINT inputPortIndex = 0; inputPortIndex < numInputPorts; inputPortIndex++)
        {
            switch (pInputPorts[inputPortIndex].portType)
            {
                case ChiFeature2PortType::ImageBuffer:
                    // Release the target input buffer
                    m_pInputImageTBM->ReleaseTargetBuffer(m_hInputImageBuffer);
                    break;
                case ChiFeature2PortType::MetaData:
                    // Release the target metadata buffer
                    m_pMetadataTBM->ReleaseTargetBuffer(m_hMetaBuffer);
                    break;
                default:
                    CHX_LOG_ERROR("Invalid port type! (%d)", pInputPorts[inputPortIndex].portType);
                    break;
            }
        }
    }

    return result;
}
