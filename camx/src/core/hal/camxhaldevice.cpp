////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2017-2020 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  camxhaldevice.cpp
/// @brief Definitions for HALDevice class
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include <system/camera_metadata.h>

#include "camxchitypes.h"
#include "camxchi.h"
#include "camxdebug.h"
#include "camxhal3entry.h"  /// @todo (CAMX-351) Remove this header
#include "camxhal3metadatautil.h"
#include "camxhal3defaultrequest.h"
#include "camxhal3module.h"
#include "camxhaldevice.h"
#include "camxhwcontext.h"
#include "camxhwfactory.h"
#include "camxmem.h"
#include "camxmemspy.h"
#include "camxosutils.h"
#include "camxpipeline.h"
#include "camxsession.h"
#include "camxtrace.h"
#include "camxsensornode.h"

#include "chioverride.h"

CAMX_NAMESPACE_BEGIN

// Forward declaration
struct SubDeviceProperty;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Constants
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static const BOOL AllowStreamReuse = FALSE;         ///< Flag used to enable stream reuse

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// HALDevice::~HALDevice
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
HALDevice::~HALDevice()
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// HALDevice::Create
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
HALDevice* HALDevice::Create(
    const HwModule* pHwModule,
    UINT32          cameraId,
    UINT32          frameworkId)
{
    CamxResult result     = CamxResultENoMemory;
    HALDevice* pHALDevice = CAMX_NEW HALDevice;

    if (NULL != pHALDevice)
    {
        pHALDevice->m_fwId = frameworkId;

        result = pHALDevice->Initialize(pHwModule, cameraId);

        if (CamxResultSuccess != result)
        {
            pHALDevice->Destroy();

            pHALDevice = NULL;
        }
    }

    return pHALDevice;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// HALDevice::Destroy
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID HALDevice::Destroy()
{
    ThermalManager* pThermalManager = HAL3Module::GetInstance()->GetThermalManager();
    if (NULL != pThermalManager)
    {
        pThermalManager->UnregisterHALDevice(this);
    }

    if (NULL != m_ppHAL3Streams)
    {
        for (UINT32 stream = 0; stream < m_numStreams; stream++)
        {
            if (NULL != m_ppHAL3Streams[stream])
            {
                CAMX_DELETE m_ppHAL3Streams[stream];
                m_ppHAL3Streams[stream] = NULL;
            }
        }

        CAMX_FREE(m_ppHAL3Streams);
        m_ppHAL3Streams = NULL;
    }

    if (NULL != m_pResultMetadata)
    {
        HAL3MetadataUtil::FreeMetadata(m_pResultMetadata);
        m_pResultMetadata = NULL;
    }

    for (UINT i = 0; i < RequestTemplateCount; i++)
    {
        if (NULL != m_pDefaultRequestMetadata[i])
        {
            HAL3MetadataUtil::FreeMetadata(m_pDefaultRequestMetadata[i]);
        }
    }
    CAMX_DELETE this;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// HALDevice::Initialize
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult HALDevice::Initialize(
    const HwModule* pHwModule,
    UINT32          cameraId)
{
    CamxResult result = CamxResultSuccess;

    m_cameraId = cameraId;

    if (CamxResultSuccess == result)
    {
        m_camera3Device.hwDevice.tag     = HARDWARE_DEVICE_TAG; /// @todo (CAMX-351) Get from local macro

#if ((CAMX_ANDROID_API) && (CAMX_ANDROID_API >= 28)) // Android-P or better
        m_camera3Device.hwDevice.version = CAMERA_DEVICE_API_VERSION_3_5;
#else
        m_camera3Device.hwDevice.version = CAMERA_DEVICE_API_VERSION_3_3;
#endif // ((CAMX_ANDROID_API) && (CAMX_ANDROID_API >= 28))

        m_camera3Device.hwDevice.close   = reinterpret_cast<CloseFunc>(GetHwDeviceCloseFunc());
        m_camera3Device.pDeviceOps       = reinterpret_cast<Camera3DeviceOps*>(GetCamera3DeviceOps());
        m_camera3Device.pPrivateData     = this;
        // NOWHINE CP036a: Need exception here
        m_camera3Device.hwDevice.pModule = const_cast<HwModule*>(pHwModule);

        m_HALCallbacks.process_capture_result = ProcessCaptureResult;
        m_HALCallbacks.notify_result          = Notify;
    }

    ClearFrameworkRequestBuffer();

    SIZE_T                entryCapacity;
    SIZE_T                dataSize;
    HAL3MetadataUtil::CalculateSizeAllMeta(&entryCapacity, &dataSize, TagSectionVisibleToFramework);

    m_pResultMetadata = HAL3MetadataUtil::CreateMetadata(
            entryCapacity,
            dataSize);

    for (UINT i = RequestTemplatePreview; i < RequestTemplateCount; i++)
    {
        if (NULL == m_pDefaultRequestMetadata[i])
        {
            ConstructDefaultRequestSettings(static_cast<Camera3RequestTemplate>(i));
        }
    }

    const StaticSettings* pStaticSettings = HwEnvironment::GetInstance()->GetStaticSettings();

    m_numPartialResult = pStaticSettings->numMetadataResults;

    /* We will increment the Partial result count by 1 if CHI also has its own implementation */
    if (CHIPartialDataSeparate == pStaticSettings->enableCHIPartialData)
    {
        m_numPartialResult++;
    }

    m_tracingZoom = FALSE;

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// HALDevice::ProcessCaptureResult
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID HALDevice::ProcessCaptureResult(
    const camera3_device_t*         pCamera3Device,
    const camera3_capture_result_t* pCamera3_CaptureResult)
{
    const Camera3CaptureResult* pCamera3CaptureResult = reinterpret_cast<const Camera3CaptureResult*>(pCamera3_CaptureResult);
    HALDevice*                  pHALDevice            = static_cast<HALDevice*>(pCamera3Device->priv);
    CamxResult                  result                = CamxResultSuccess;
    const StaticSettings*       pSettings             = HwEnvironment::GetInstance()->GetStaticSettings();
    BOOL                        updateFramework       = TRUE;

    // Keep track of information related to request for error conditions
    FrameworkRequestData* pFrameworkRequest = pHALDevice->GetFrameworkRequestData(
        pCamera3CaptureResult->frameworkFrameNum, FALSE);

    if (pFrameworkRequest->frameworkNum != pCamera3CaptureResult->frameworkFrameNum)
    {
        updateFramework = FALSE;
        CAMX_LOG_WARN(CamxLogGroupCore, "Unexpected frame number: %d, Can't send result to FWK"
            " Num Output Buffers: %d, num partial metadata: %d, ResultMetdata: %p",
            pCamera3CaptureResult->frameworkFrameNum,
            pCamera3CaptureResult->numOutputBuffers,
            pCamera3CaptureResult->numPartialMetadata,
            pCamera3CaptureResult->pResultMetadata);
        pHALDevice->LogErrorAndAssert(UnexpectedFrameNumber);
    }

    // if Request error TRUE, do not send metadata or buffer
    if ((TRUE == pFrameworkRequest->requestStatus.notifyRequestError) &&
        (pFrameworkRequest->numBuffers == pFrameworkRequest->numBuffersReceived))
    {
        updateFramework = FALSE;
        CAMX_LOG_WARN(CamxLogGroupCore, "Received metadata or buffer after sending ERROR_REQUEST for Frame %u",
            pFrameworkRequest->frameworkNum);
        pHALDevice->LogRequest(pFrameworkRequest, -1, FALSE, StringFormat);
    }
    // if Result error TRUE, if result has metadata
    if (TRUE == pFrameworkRequest->requestStatus.notifyResultError)
    {
        if (NULL != pCamera3CaptureResult->pResultMetadata)
        {
            updateFramework = FALSE;
            CAMX_LOG_WARN(CamxLogGroupCore, "Received metadata after sending ERROR_RESULT for Frame %u",
                pFrameworkRequest->frameworkNum);
            pHALDevice->LogRequest(pFrameworkRequest, -1, FALSE, StringFormat);
            pHALDevice->LogErrorAndAssert(UnexpectedMetadata);
        }
    }

    if ((0 != pCamera3CaptureResult->numOutputBuffers) &&
        (pFrameworkRequest->numBuffersReceived == pFrameworkRequest->numBuffers))
    {
        updateFramework = FALSE;
        CAMX_LOG_WARN(CamxLogGroupCore, "Receiving unexpected buffer for Frame %u", pFrameworkRequest->frameworkNum);
        pHALDevice->LogRequest(pFrameworkRequest, -1, FALSE, StringFormat);
        pHALDevice->LogErrorAndAssert(UnexpectedOutputBuffer);
    }

    if (TRUE == updateFramework)
    {
        if (pCamera3CaptureResult->numOutputBuffers > 0)
        {
            for (UINT i = 0; i < pCamera3CaptureResult->numOutputBuffers; i++)
            {
                Camera3StreamBuffer* pStreamBuffer =
                    // NOWHINE CP036a: Google API requires const type
                    const_cast<Camera3StreamBuffer*>(&pCamera3CaptureResult->pOutputBuffers[i]);
                pStreamBuffer->releaseFence = -1;
                CAMX_LOG_INFO(CamxLogGroupHAL,
                    "Returning framework result Frame: %d, Metadata: %p, Stream %p, Fmt: %d Width: %d Height: %d",
                    pCamera3CaptureResult->frameworkFrameNum,
                    pCamera3CaptureResult->pResultMetadata,
                    pCamera3CaptureResult->pOutputBuffers[i].pStream,
                    pCamera3CaptureResult->pOutputBuffers[i].pStream->format,
                    pCamera3CaptureResult->pOutputBuffers[i].pStream->width,
                    pCamera3CaptureResult->pOutputBuffers[i].pStream->height,
                    pCamera3CaptureResult->pOutputBuffers[i].releaseFence,
                    pCamera3CaptureResult->pOutputBuffers[i].bufferStatus);
            }
        }

        if (NULL != pCamera3CaptureResult->pResultMetadata)
        {
            CAMX_LOG_INFO(CamxLogGroupHAL,
                "Returning framework result metadata only for frame: %d, Metadata: %p",
                pCamera3CaptureResult->frameworkFrameNum, pCamera3CaptureResult->pResultMetadata);
        }

        pHALDevice->UpdateFrameworkRequestBufferResult(pCamera3CaptureResult, pFrameworkRequest);
        pHALDevice->GetCallbackOps()->process_capture_result(pHALDevice->GetCallbackOps(), pCamera3_CaptureResult);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// HALDevice::Notify
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID HALDevice::Notify(
    const camera3_device_t*     pCamera3Device,
    const camera3_notify_msg_t* pNotifyMessage)
{
    const Camera3NotifyMessage* pCamera3NotifyMessage = reinterpret_cast<const Camera3NotifyMessage*>(pNotifyMessage);
    HALDevice*                  pHALDevice            = static_cast<HALDevice*>(pCamera3Device->priv);
    BOOL                        updateFramework       = TRUE;
    CDKResult                   result                = CDKResultSuccess;

    UINT32                frameNumber       = 0;
    FrameworkRequestData* pFrameworkRequest = NULL;

    if (NULL == pCamera3NotifyMessage)
    {
        updateFramework = FALSE;
        result          = CDKResultEFailed;
    }
    else if (MessageTypeShutter == pCamera3NotifyMessage->messageType)
    {
        frameNumber = pCamera3NotifyMessage->message.shutterMessage.frameworkFrameNum;
    }
    else if (MessageTypeError == pCamera3NotifyMessage->messageType)
    {
        frameNumber = pCamera3NotifyMessage->message.errorMessage.frameworkFrameNum;
    }
    else
    {
        updateFramework = FALSE;
        result          = CDKResultEFailed;
        CAMX_LOG_ERROR(CamxLogGroupHAL, "Unknown message type: %u", pCamera3NotifyMessage->messageType);
    }

    if (CDKResultSuccess == result)
    {
        pFrameworkRequest = pHALDevice->GetFrameworkRequestData(frameNumber, FALSE);

        if (TRUE == pFrameworkRequest->requestStatus.notifyRequestError)
        {
            // if Result error || Buffer error, if Request error TRUE, Invalid error
            if ((MessageTypeError == pCamera3NotifyMessage->messageType) &&
                 (MessageCodeResult == pCamera3NotifyMessage->message.errorMessage.errorMessageCode ||
                  MessageCodeBuffer == pCamera3NotifyMessage->message.errorMessage.errorMessageCode))
            {
                updateFramework = FALSE;

                CAMX_LOG_WARN(CamxLogGroupCore, "Received ERROR_RESULT or ERROR_BUFFER after sending ERROR_REQUEST "
                    "for Frame %u and error code = %d",
                    pFrameworkRequest->frameworkNum,
                    pCamera3NotifyMessage->message.errorMessage.errorMessageCode);
                pHALDevice->LogRequest(pFrameworkRequest, -1, FALSE, StringFormat);
                ErrorType errorType = ((MessageCodeResult == pCamera3NotifyMessage->message.errorMessage.errorMessageCode) ?
                    UnexpectedMetadataErrorNotification : UnexpectedBufferErrorNotification);
                pHALDevice->LogErrorAndAssert(errorType);
            }
            else if (MessageTypeShutter == pCamera3NotifyMessage->messageType)
            {
                updateFramework = FALSE;
                CAMX_LOG_WARN(CamxLogGroupCore, "Received Shutter Message after sending ERROR_REQUEST for Frame %u",
                    pFrameworkRequest->frameworkNum);
                pHALDevice->LogRequest(pFrameworkRequest, -1, FALSE, StringFormat);
                pHALDevice->LogErrorAndAssert(UnexpectedShutter);
            }
        }

        if ((MessageTypeShutter == pCamera3NotifyMessage->messageType) &&
            (TRUE == pFrameworkRequest->requestStatus.notifyShutter))
        {
            updateFramework = FALSE;
            CAMX_LOG_WARN(CamxLogGroupCore, "Shutter notification already received for Frame %u, so dropping notification",
                pFrameworkRequest->frameworkNum);
            pHALDevice->LogRequest(pFrameworkRequest, -1, FALSE, StringFormat);
            pHALDevice->LogErrorAndAssert(UnexpectedShutter);
        }

        if ((MessageTypeError == pCamera3NotifyMessage->messageType) &&
            (MessageCodeBuffer == pCamera3NotifyMessage->message.errorMessage.errorMessageCode))
        {
            UINT streamId = pHALDevice->GetStreamId(pCamera3NotifyMessage->message.errorMessage.pErrorStream);
            if (TRUE == pFrameworkRequest->buffers[streamId].bufferFlags.notifyBufferError)
            {
                updateFramework = FALSE;
                CAMX_LOG_WARN(CamxLogGroupCore, "Received ERROR_BUFFER notification 2nd time for the same stream %p"
                    " of Frame %u", pCamera3NotifyMessage->message.errorMessage.pErrorStream,
                    pFrameworkRequest->frameworkNum);
                pHALDevice->LogRequest(pFrameworkRequest, -1, FALSE, StringFormat);
                pHALDevice->LogErrorAndAssert(UnexpectedBufferErrorNotification);
            }
            else if (TRUE == pFrameworkRequest->buffers[streamId].bufferFlags.bufferStatusOK)
            {
                updateFramework = FALSE;
                CAMX_LOG_WARN(CamxLogGroupCore, "Received ERROR_BUFFER notification for the same stream %p whose buffer "
                    "status is OK for Frame %u ",
                    pCamera3NotifyMessage->message.errorMessage.pErrorStream,
                    pFrameworkRequest->frameworkNum);
                pHALDevice->LogRequest(pFrameworkRequest, -1, FALSE, StringFormat);
                pHALDevice->LogErrorAndAssert(UnexpectedBufferErrorNotification);
            }
        }

        if (TRUE == updateFramework)
        {
            switch (pCamera3NotifyMessage->messageType)
            {
                case MessageTypeError:
                    if (FALSE == pHALDevice->m_bFlushEnabled)
                    {
                        CAMX_LOG_ERROR(CamxLogGroupHAL, "type %08x, frame_number %d, error_code %08x, error_stream %p",
                                       pCamera3NotifyMessage->messageType,
                                       pCamera3NotifyMessage->message.errorMessage.frameworkFrameNum,
                                       pCamera3NotifyMessage->message.errorMessage.errorMessageCode,
                                       pCamera3NotifyMessage->message.errorMessage.pErrorStream);
                    }
                    else
                    {
                        CAMX_LOG_INFO(CamxLogGroupHAL, "type %08x, frame_number %d, error_code %08x, error_stream %p",
                                       pCamera3NotifyMessage->messageType,
                                       pCamera3NotifyMessage->message.errorMessage.frameworkFrameNum,
                                       pCamera3NotifyMessage->message.errorMessage.errorMessageCode,
                                       pCamera3NotifyMessage->message.errorMessage.pErrorStream);
                    }
                    break;
                case MessageTypeShutter:
                    CAMX_LOG_CONFIG(CamxLogGroupHAL, "type %08x, frame_number %d, timestamp %llu",
                                  pCamera3NotifyMessage->messageType,
                                  pCamera3NotifyMessage->message.shutterMessage.frameworkFrameNum,
                                  pCamera3NotifyMessage->message.shutterMessage.timestamp);
                    CAMX_TRACE_ASYNC_END_F(CamxLogGroupHAL, pCamera3NotifyMessage->message.shutterMessage.frameworkFrameNum,
                        "SHUTTERLAG frameID: %d", pCamera3NotifyMessage->message.shutterMessage.frameworkFrameNum);
                    break;
                default:
                    CAMX_LOG_INFO(CamxLogGroupHAL, "Unknown message type");
                    break;
            }
            // Keep track of information related to request for error conditions
            pHALDevice->UpdateFrameworkRequestBufferNotify(pCamera3NotifyMessage, pFrameworkRequest);
            pHALDevice->GetCallbackOps()->notify(pHALDevice->GetCallbackOps(), pNotifyMessage);
        }
    }
    else
    {
        CAMX_LOG_ERROR(CamxLogGroupHAL, "pCamera3NotifyMessage is NULL");
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// HALDevice::NotifyMessage
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID HALDevice::NotifyMessage(
    HALDevice*                  pHALDevice,
    const Camera3NotifyMessage* pCamera3NotifyMessage)
{
    const camera3_callback_ops_t* pCamera3CbOps = NULL;

    pCamera3CbOps = pHALDevice->GetCallbackOps();
    if (NULL != pCamera3CbOps && NULL != pCamera3CbOps->notify)
    {
        pCamera3CbOps->notify(pHALDevice->GetCallbackOps(),
                              reinterpret_cast<const camera3_notify_msg_t*>(pCamera3NotifyMessage));
    }
    else
    {
        CAMX_LOG_ERROR(CamxLogGroupHAL, "Callback is not initialized!");
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// HALDevice::TraceZoom
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID HALDevice::TraceZoom(
    camera3_capture_request_t* pRequest)
{
    camera_metadata_entry_t entry = { 0 };

    // NOWHINE CP036a: Since the function does not take a const, had to add the const_cast
    if (0 == find_camera_metadata_entry(const_cast<camera_metadata_t*>(pRequest->settings),
        ANDROID_SCALER_CROP_REGION, &entry))
    {
        if (m_tracingZoom == FALSE)
        {
            // Get Initial Crop Value
            m_zoomLevel.left   = entry.data.i32[0];
            m_zoomLevel.top    = entry.data.i32[1];
            m_zoomLevel.width  = entry.data.i32[2];
            m_zoomLevel.height = entry.data.i32[3];
            m_tracingZoom      = TRUE;
        }
        else
        {
            // Check to see if Crop Value changes
            if ((static_cast<INT>(m_zoomLevel.left)   != entry.data.i32[0] ||
                 static_cast<INT>(m_zoomLevel.top)    != entry.data.i32[1] ||
                 static_cast<INT>(m_zoomLevel.width)  != entry.data.i32[2] ||
                 static_cast<INT>(m_zoomLevel.height) != entry.data.i32[3]))
            {
                // Start Trace
                CAMX_TRACE_MESSAGE_F(CamxLogGroupCore, "Zoom Frame Request: %d", pRequest->frame_number);

                m_zoomLevel.left   = entry.data.i32[0];
                m_zoomLevel.top    = entry.data.i32[1];
                m_zoomLevel.width  = entry.data.i32[2];
                m_zoomLevel.height = entry.data.i32[3];
            }
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// HALDevice::CheckValidStreamConfig
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult HALDevice::CheckValidStreamConfig(
    Camera3StreamConfig* pStreamConfigs
    ) const
{

    HwCameraInfo                  cameraInfo;
    const HwEnvironment*          pHWEnvironment  = HwEnvironment::GetInstance();
    const StaticSettings*         pSettings       = HwEnvironment::GetInstance()->GetStaticSettings();
    const SensorModuleStaticCaps* pSensorCaps     = NULL;

    CamxResult  result            = CamxResultSuccess;
    UINT32      numOutputStreams  = 0;
    UINT32      numInputStreams   = 0;
    UINT32      logicalCameraId   = GetCHIAppCallbacks()->chi_remap_camera_id(m_fwId, IdRemapTorch);

    result = pHWEnvironment->GetCameraInfo(logicalCameraId, &cameraInfo);

    if (CamxResultSuccess == result)
    {
        pSensorCaps = cameraInfo.pSensorCaps;

        if ((StreamConfigModeConstrainedHighSpeed == pStreamConfigs->operationMode) ||
            (StreamConfigModeSuperSlowMotionFRC == pStreamConfigs->operationMode))
        {
            BOOL isConstrainedHighSpeedSupported = FALSE;
            BOOL isJPEGSnapshotStream            = FALSE;

            for (UINT i = 0; i < cameraInfo.pPlatformCaps->numRequestCaps; i++)
            {
                if (RequestAvailableCapabilitiesConstrainedHighSpeedVideo == cameraInfo.pPlatformCaps->requestCaps[i])
                {
                    isConstrainedHighSpeedSupported = TRUE;
                    break;
                }
            }

            if (FALSE == isConstrainedHighSpeedSupported)
            {
                result = CamxResultEInvalidArg;
                CAMX_LOG_ERROR(CamxLogGroupHAL, "Invalid operationMode: %d", pStreamConfigs->operationMode);
            }

            // @todo (CAMX-4248) Remove it when we start to support live snapshot in the HFR mode
            // Check if we have any JPEG stream. So far we don`t support live snapshot in the HFR mode.
            for (UINT32 stream = 0; stream < pStreamConfigs->numStreams; stream++)
            {
                if ((StreamTypeOutput   == pStreamConfigs->ppStreams[stream]->streamType) &&
                    (HALPixelFormatBlob == pStreamConfigs->ppStreams[stream]->format)     &&
                    ((HALDataspaceJFIF  == pStreamConfigs->ppStreams[stream]->dataspace)  ||
                    (HALDataspaceV0JFIF == pStreamConfigs->ppStreams[stream]->dataspace)))
                {
                    isJPEGSnapshotStream = TRUE;
                    break;
                }
            }

            if (TRUE == isJPEGSnapshotStream)
            {
                result = CamxResultEUnsupported;
                CAMX_LOG_ERROR(CamxLogGroupHAL, "Non-supporting snapshot in operationMode: %d", pStreamConfigs->operationMode);
            }
        }
    }

    if (CamxResultSuccess == result)
    {
        const UINT                  numAvailableConfigs     = cameraInfo.pHwEnvironmentCaps->numStreamConfigs;
        const ScalerStreamConfig*   pAvailableConfigs       = cameraInfo.pHwEnvironmentCaps->streamConfigs;
        const UINT                  numInternalPixelFormat  = cameraInfo.pPlatformCaps->numInternalPixelFormats;
        const HALPixelFormat*       pInternalFormats        = cameraInfo.pPlatformCaps->internalPixelFormats;
        BOOL                        isAvailableStream       = FALSE;
        UINT                        numMatchedInputOutputConfigs;

        // Count the number of input and output streams
        for (UINT32 stream = 0; stream < pStreamConfigs->numStreams; stream++)
        {
            isAvailableStream               = FALSE;
            numMatchedInputOutputConfigs    = 0;

            // Check if the stream has a valid rotation
            if ((pStreamConfigs->ppStreams[stream]->rotation != StreamRotationCCW0)   &&
                (pStreamConfigs->ppStreams[stream]->rotation != StreamRotationCCW90)  &&
                (pStreamConfigs->ppStreams[stream]->rotation != StreamRotationCCW180) &&
                (pStreamConfigs->ppStreams[stream]->rotation != StreamRotationCCW270))
            {
                result = CamxResultEInvalidArg;
                CAMX_LOG_ERROR(CamxLogGroupHAL,
                    "Stream has an invalid rotation: %d",
                    pStreamConfigs->ppStreams[stream]->rotation);
                break;
            }

            // Check if stream has invalid format
            if (pStreamConfigs->ppStreams[stream]->format <= 0)
            {
                result = CamxResultEInvalidArg;
                CAMX_LOG_ERROR(CamxLogGroupHAL,
                    "Stream has an invalid format: %d",
                    pStreamConfigs->ppStreams[stream]->format);
                break;
            }

            switch (pStreamConfigs->ppStreams[stream]->streamType)
            {
                case StreamTypeOutput:
                    numOutputStreams++;
                    break;

                case StreamTypeInput:
                    numInputStreams++;
                    break;

                case StreamTypeBidirectional:
                    numOutputStreams++;
                    numInputStreams++;
                    break;

                default:
                    CAMX_ASSERT_ALWAYS_MESSAGE("Invalid streamStype: %d", pStreamConfigs->ppStreams[stream]->streamType);
                    result = CamxResultEInvalidArg;
                    break;
            }

            // Check if device support the stream configuration.
            if ((TRUE == pSettings->multiCameraVREnable) || (TRUE == pSettings->multiCameraEnable))
            {
                logicalCameraId   = GetCHIAppCallbacks()->chi_remap_camera_id(m_fwId, IdRemapCamera);
                isAvailableStream = CheckSupportedResolution(logicalCameraId, pStreamConfigs->ppStreams[stream]->format,
                                   pStreamConfigs->ppStreams[stream]->width , pStreamConfigs->ppStreams[stream]->height ,
                                   pStreamConfigs->ppStreams[stream]->streamType);
            }
            else
            {
                for (UINT32 i = 0; i < numAvailableConfigs; i++)
                {
                    if ((pStreamConfigs->ppStreams[stream]->format == pAvailableConfigs[i].format) &&
                        (pStreamConfigs->ppStreams[stream]->width  == pAvailableConfigs[i].width) &&
                        (pStreamConfigs->ppStreams[stream]->height == pAvailableConfigs[i].height))
                    {
                        if (pStreamConfigs->ppStreams[stream]->streamType == pAvailableConfigs[i].type)
                        {
                            isAvailableStream = TRUE;
                            break;
                        }
                        else if ((StreamTypeBidirectional == pStreamConfigs->ppStreams[stream]->streamType) &&
                                 ((ScalerAvailableStreamConfigurationsOutput == pAvailableConfigs[i].type) ||
                                 (ScalerAvailableStreamConfigurationsInput  == pAvailableConfigs[i].type)))
                        {
                                 // For StreamTypeBidirectional, both input and output stream configuration need to be supported
                                 // by device.
                            numMatchedInputOutputConfigs++;
                            if (ScalerAvailableStreamConfigurationsEnd == numMatchedInputOutputConfigs)
                            {
                                isAvailableStream = TRUE;
                                break;
                            }
                        }
                    }
                }
            }
            // Check if driver allow non-standard Android scaler stream format. e.g. HALPixelFormatY8
            if ((FALSE == isAvailableStream) && (TRUE == pSettings->enableInternalHALPixelStreamConfig))
            {
                for (UINT32 i = 0; i < numInternalPixelFormat; i++)
                {
                    if (pStreamConfigs->ppStreams[stream]->format == pInternalFormats[i])
                    {
                        isAvailableStream = TRUE;
                        break;
                    }
                }
            }

            // For Quad CFA sensor, if only expose quarter size to all apps (OEM app, 3rd app or CTS),
            // but OEM app will still config full size snapshot, need to accept it.
            if ((FALSE == isAvailableStream) &&
                (TRUE  == pSensorCaps->isQuadCFASensor) &&
                (FALSE == pSettings->exposeFullSizeForQCFA))
            {
                if ((StreamTypeOutput == pStreamConfigs->ppStreams[stream]->streamType) &&
                    (pStreamConfigs->ppStreams[stream]->width  > static_cast<UINT32>(pSensorCaps->QuadCFADim.width  / 2)) &&
                    (pStreamConfigs->ppStreams[stream]->height > static_cast<UINT32>(pSensorCaps->QuadCFADim.height / 2)) &&
                    (pStreamConfigs->ppStreams[stream]->width  <= static_cast<UINT32>(pSensorCaps->QuadCFADim.width)) &&
                    (pStreamConfigs->ppStreams[stream]->height <= static_cast<UINT32>(pSensorCaps->QuadCFADim.height)) &&
                    (pStreamConfigs->ppStreams[stream]->width  > pAvailableConfigs[0].width) &&
                    (pStreamConfigs->ppStreams[stream]->height > pAvailableConfigs[0].height))
                {
                    CAMX_LOG_INFO(CamxLogGroupHAL,
                                   "Accept size (%d x %d) for Quad CFA sensor",
                                   pStreamConfigs->ppStreams[stream]->width,
                                   pStreamConfigs->ppStreams[stream]->height);

                    isAvailableStream = TRUE;
                }
            }
            // Check if it is HEIF Blob which is not available in regular stream configuration map
            // The resolution should be (size x 1)
            if ((FALSE == isAvailableStream) &&
                (StreamTypeOutput            == pStreamConfigs->ppStreams[stream]->streamType) &&
                (HALPixelFormatBlob          == pStreamConfigs->ppStreams[stream]->format) &&
                (1                           == pStreamConfigs->ppStreams[stream]->height) &&
                (HALDataspaceJPEGAPPSegments == pStreamConfigs->ppStreams[stream]->dataspace))
            {
                isAvailableStream = TRUE;
                CAMX_LOG_INFO(CamxLogGroupHAL,
                              "Accept size (%u x %u) for HEIF Blob",
                              pStreamConfigs->ppStreams[stream]->width,
                              pStreamConfigs->ppStreams[stream]->height);
                break;
            }

            if (FALSE == isAvailableStream)
            {
                result = CamxResultEInvalidArg;
                CAMX_LOG_WARN(CamxLogGroupHAL,
                               "Invalid streamStype: %d, format: %d, width: %d, height: %d",
                               pStreamConfigs->ppStreams[stream]->streamType,
                               pStreamConfigs->ppStreams[stream]->format,
                               pStreamConfigs->ppStreams[stream]->width,
                               pStreamConfigs->ppStreams[stream]->height);
                break;
            }
        }
    }

    if (CamxResultSuccess == result)
    {
        // We allow 0 up to MaxNumOutputBuffers output streams. Zero output streams are allowed, despite the HAL3 specification,
        // in order to support metadata-only requests.
        if (numOutputStreams > MaxNumOutputBuffers)
        {
            CAMX_LOG_ERROR(CamxLogGroupHAL,
                           "Invalid number of output streams (including bi-directional): %d",
                           numOutputStreams);
            result = CamxResultEInvalidArg;
        }

        // We allow 0 up to MaxNumInputBuffers input streams
        if (numInputStreams > MaxNumInputBuffers)
        {
            CAMX_LOG_ERROR(CamxLogGroupHAL,
                           "Invalid number of input streams (including bi-directional): %d",
                           numInputStreams);
            result = CamxResultEInvalidArg;
        }
    }
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// HALDevice::CheckSupportedResolution
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL HALDevice::CheckSupportedResolution(
    INT32 cameraId,
    INT32 format,
    UINT32 width,
    UINT32 height,
    INT32 streamType
    ) const
{
    CameraInfo info;

    if (0 != GetCHIAppCallbacks()->chi_get_camera_info(cameraId, reinterpret_cast<struct camera_info*>(&info)))
    {
        CAMX_LOG_INFO(CamxLogGroupHAL, "Can't get camera info for camera id: %d", cameraId);
        return FALSE;
    }
    // NOWHINE CP036a: The 'getters' for metadata because of being a sorted hashmap can modify the object
    Metadata* pStaticCameraInfo = const_cast<Metadata*>(info.pStaticCameraInfo);
    INT32* pAvailStreamConfig   = NULL;
    INT32 val = HAL3MetadataUtil::GetMetadata(pStaticCameraInfo, ScalerAvailableStreamConfigurations,
            reinterpret_cast<VOID**>(&pAvailStreamConfig));

    if (val != CamxResultSuccess)
    {
        CAMX_LOG_INFO(CamxLogGroupHAL, "Can't find the metadata entry for ANDROID_SCALER_AVAILABLE_STREAM_CONFIGURATIONS");
        return FALSE;
    }

    if (NULL == pAvailStreamConfig)
    {
        CAMX_LOG_INFO(CamxLogGroupHAL,
                      "Value of the metadata entry for ANDROID_SCALER_AVAILABLE_STREAM_CONFIGURATIONS is NULL");
        return FALSE;
    }

    SIZE_T entryCount = HAL3MetadataUtil::GetMetadataEntryCount(pStaticCameraInfo);

    INT32 matched = 0;

    if (streamType != StreamTypeBidirectional)
    {
        streamType = (streamType == StreamTypeOutput ? ScalerAvailableStreamConfigurationsOutput :
            (streamType == StreamTypeInput ? ScalerAvailableStreamConfigurationsInput : -1));

        if (-1 == streamType)
        {
            CAMX_LOG_WARN(CamxLogGroupHAL, "Invalid stream direction: %d", streamType);
            return FALSE;
        }
    }

    const INT32 STREAM_WIDTH_OFFSET       = 1;
    const INT32 STREAM_HEIGHT_OFFSET      = 2;
    const INT32 STREAM_IS_INPUT_OFFSET    = 3;
    const INT32 STREAM_CONFIGURATION_SIZE = 4;

    for (UINT32 i = 0; i < entryCount; i += STREAM_CONFIGURATION_SIZE)
    {
        /* This is a special case because, for bi-directional stream we need to ensure
         * given resolution is supported both as input and output */

        INT32 queriedFormat    = pAvailStreamConfig[i];
        UINT32 swidth          = pAvailStreamConfig[i + STREAM_WIDTH_OFFSET];
        UINT32 sheight         = pAvailStreamConfig[i + STREAM_HEIGHT_OFFSET];
        INT32 streamDirection  = pAvailStreamConfig[i + STREAM_IS_INPUT_OFFSET];

        if (StreamTypeBidirectional == streamType)
        {
            if ((ScalerAvailableStreamConfigurationsOutput == streamDirection) &&
                (format == queriedFormat))
            {
                if (swidth == width &&
                    sheight == height)
                {
                    matched++;
                }
            }
            if ((ScalerAvailableStreamConfigurationsInput == streamDirection) &&
                (format == queriedFormat))
            {
                if (swidth == width &&
                    sheight == height)
                {
                    matched++;
                }
            }
        }
        else
        {
            if (streamDirection == streamType && format == queriedFormat)
            {
                if (swidth == width &&
                    sheight == height)
                {
                    matched++;
                }
            }
        }
    }

    if ((StreamTypeBidirectional == streamType) && (matched == 2))
    {
        return FALSE;
    }
    if ((StreamTypeBidirectional != streamType) && (matched == 1))
    {
        return TRUE;
    }
    return TRUE;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// HALDevice::InitializeHAL3Streams
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult HALDevice::InitializeHAL3Streams(
    Camera3StreamConfig* pStreamConfigs)
{
    CamxResult   result        = CamxResultSuccess;
    UINT32       stream        = 0;
    HAL3Stream*  pHAL3Stream   = NULL;
    HAL3Stream** ppHAL3Streams = NULL;

    ppHAL3Streams = static_cast<HAL3Stream**>(CAMX_CALLOC(sizeof(HAL3Stream*) * MaxNumOutputBuffers));

    if (NULL != ppHAL3Streams)
    {
        // Reset the existing list of streams, if any. This is done before checking to see if the stream can be reused since
        // the existing stream's flag will be updated directly.
        if (NULL != m_ppHAL3Streams)
        {
            for (stream = 0; (stream < MaxNumOutputBuffers) && (NULL != m_ppHAL3Streams[stream]); stream++)
            {
                m_ppHAL3Streams[stream]->SetStreamReused(FALSE);
            }
        }

        // For each incoming stream, check whether it matches an existing configuration that can be reused or if it is a new
        // configuration.
        for (stream = 0; stream < pStreamConfigs->numStreams; stream++)
        {
            CAMX_ENTRYEXIT_SCOPE_ID(CamxLogGroupHAL, SCOPEEventHAL3DeviceInitializeHAL3Stream, stream);

            Camera3Stream* pStream = pStreamConfigs->ppStreams[stream];

            // If the maximum number of inflight buffers allowed by this stream is > 0, we know the HAL has already seen this
            // stream configuration before since the buffer count comes from the HAL. Therefore, attempt to reuse it. If the
            // maximum number of buffers is 0, create a new HAL3 stream object.
            if ((pStream->maxNumBuffers > 0) && (TRUE == AllowStreamReuse))
            {
                pHAL3Stream = reinterpret_cast<HAL3Stream*>(pStream->pPrivateInfo);

                if ((NULL != pHAL3Stream) && (TRUE == pHAL3Stream->IsStreamConfigMatch(pStream)))
                {
                    pHAL3Stream->SetStreamReused(TRUE);
                    pHAL3Stream->SetStreamIndex(stream);
                    ppHAL3Streams[stream] = pHAL3Stream;
                }
                else
                {
                    CAMX_LOG_ERROR(CamxLogGroupHAL, "Invalid stream configuration!");
                    // HAL interface requires -EINVAL (EInvalidArg) for invalid arguments
                    result = CamxResultEInvalidArg;
                    ppHAL3Streams[stream] = NULL;
                    break;
                }
            }
            else
            {
                pHAL3Stream = CAMX_NEW HAL3Stream(pStream, stream, Format::RawMIPI);

                if (NULL != pHAL3Stream)
                {
                    ppHAL3Streams[stream] = pHAL3Stream;
                    pStream->pPrivateInfo = pHAL3Stream;
                }
                else
                {
                    CAMX_LOG_ERROR(CamxLogGroupHAL, "Cannot create HAL3Stream! Out of memory!");

                    result = CamxResultENoMemory;
                    ppHAL3Streams[stream] = NULL;
                    break;
                }
            }
        }

        // If stream reuse and creation successful, store configurations in m_ppHAL3Streams, otherwise clean up
        if (CamxResultSuccess == result)
        {
            DestroyUnusedStreams(m_ppHAL3Streams, m_numStreams);

            m_ppHAL3Streams = ppHAL3Streams;
            m_numStreams    = pStreamConfigs->numStreams;
        }
        else
        {
            DestroyUnusedStreams(ppHAL3Streams, pStreamConfigs->numStreams);
            ppHAL3Streams = NULL;
        }
    }
    else
    {
        CAMX_LOG_ERROR(CamxLogGroupHAL, "Creating ppHAL3Streams failed! Out of memory!");
        result = CamxResultENoMemory;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// HALDevice::DestroyUnusedStreams
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void HALDevice::DestroyUnusedStreams(
    HAL3Stream** ppHAL3Streams,
    UINT32       numStreams
    ) const
{
    if (NULL != ppHAL3Streams)
    {
        for (UINT32 stream = 0; stream < numStreams; stream++)
        {
            if ((NULL != ppHAL3Streams[stream]) && (FALSE == ppHAL3Streams[stream]->IsStreamReused()))
            {
                CAMX_DELETE ppHAL3Streams[stream];
                ppHAL3Streams[stream] = NULL;
            }
        }

        CAMX_FREE(ppHAL3Streams);
        ppHAL3Streams = NULL;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// HALDevice::GetCHIAppCallbacks
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CAMX_INLINE chi_hal_callback_ops_t* HALDevice::GetCHIAppCallbacks() const
{
    return (HAL3Module::GetInstance()->GetCHIAppCallbacks());
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// HALDevice::ConfigureStreams
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult HALDevice::ConfigureStreams(
    Camera3StreamConfig* pStreamConfigs)
{
    CamxResult result = CamxResultSuccess;

    // Validate the incoming stream configurations
    result = CheckValidStreamConfig(pStreamConfigs);

    if ((StreamConfigModeConstrainedHighSpeed == pStreamConfigs->operationMode) ||
        (StreamConfigModeSuperSlowMotionFRC == pStreamConfigs->operationMode))
    {
        SearchNumBatchedFrames (pStreamConfigs, &m_usecaseNumBatchedFrames, &m_FPSValue);
        CAMX_ASSERT(m_usecaseNumBatchedFrames > 1);
    }
    else
    {
        // Not a HFR usecase batch frames value need to set to 1.
        m_usecaseNumBatchedFrames = 1;
    }

    if (CamxResultSuccess == result)
    {
        if (TRUE == m_bCHIModuleInitialized)
        {
            GetCHIAppCallbacks()->chi_teardown_override_session(reinterpret_cast<camera3_device*>(&m_camera3Device), 0, NULL);
            ReleaseStreamConfig();
            DeInitRequestLogger();
        }

        m_bCHIModuleInitialized = CHIModuleInitialize(pStreamConfigs);

        ClearFrameworkRequestBuffer();

        if (FALSE == m_bCHIModuleInitialized)
        {
            CAMX_LOG_ERROR(CamxLogGroupHAL, "CHI Module failed to configure streams");
            result = CamxResultEFailed;
        }
        else
        {
            result = SaveStreamConfig(pStreamConfigs);
            result = InitializeRequestLogger();
            CAMX_LOG_VERBOSE(CamxLogGroupHAL, "CHI Module configured streams ... CHI is in control!");
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// HALDevice::CHIModuleInitialize
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL HALDevice::CHIModuleInitialize(
    Camera3StreamConfig* pStreamConfigs)
{
    BOOL isOverrideEnabled = FALSE;

    if (TRUE == HAL3Module::GetInstance()->IsCHIOverrideModulePresent())
    {
        /// @todo (CAMX-1518) Handle private data from Override module
        VOID*                   pPrivateData;
        chi_hal_callback_ops_t* pCHIAppCallbacks  = GetCHIAppCallbacks();

        pCHIAppCallbacks->chi_initialize_override_session(GetCameraId(),
                                                          reinterpret_cast<const camera3_device_t*>(&m_camera3Device),
                                                          &m_HALCallbacks,
                                                          reinterpret_cast<camera3_stream_configuration_t*>(pStreamConfigs),
                                                          &isOverrideEnabled,
                                                          &pPrivateData);
    }

    return isOverrideEnabled;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// HALDevice::SearchNumBatchedFrames
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID HALDevice::SearchNumBatchedFrames(
    Camera3StreamConfig* pStreamConfigs,
    UINT*                pBatchSize,
    UINT*                pFPSValue)
{
    INT32 width    = 0;
    INT32 height   = 0;

    // We will take the following steps -
    //  1) We will search SupportedHFRVideoSizes, for the matching Video/Preview stream.
    //     Note: For the use case of multiple output streams, application must select one unique size from this metadata
    //           to use (e.g., preview and recording streams must have the same size). Otherwise, the high speed capture
    //           session creation will fail
    //  2) If a single entry is found in SupportedHFRVideoSizes, we choose the batchsize from that entry
    //  3) Else (multiple entries are found), we see if PropertyIDUsecaseFPS is published
    //  4)  If published, we pick the batchsize from that entry which is closest to the published FPS
    //  5)  If not published, we pick the batchsize from the first entry

    for (UINT streamIndex = 0 ; streamIndex < pStreamConfigs->numStreams; streamIndex++)
    {
        if (StreamTypeOutput == pStreamConfigs->ppStreams[streamIndex]->streamType)
        {
            width  = pStreamConfigs->ppStreams[streamIndex]->width;
            height = pStreamConfigs->ppStreams[streamIndex]->height;
            break;
        }
    }

    const PlatformStaticCaps*       pCaps                     =
        HwEnvironment::GetInstance()->GetPlatformStaticCaps();

    const HFRConfigurationParams*   pHFRParams[MaxHFRConfigs] = { NULL };
    UINT                            numHFREntries             = 0;

    if ((0 != width) && (0 != height))
    {
        for (UINT i = 0; i < pCaps->numDefaultHFRVideoSizes; i++)
        {
            if ((pCaps->defaultHFRVideoSizes[i].width == width) && (pCaps->defaultHFRVideoSizes[i].height == height))
            {
                // Out of the pair of entries in the table, we would like to store the second entry
                pHFRParams[numHFREntries++] = &pCaps->defaultHFRVideoSizes[i + 1];
                // Make sure that we don't hit the other entry in the pair, again
                i++;
            }
        }

        if (numHFREntries >= 1)
        {
            *pBatchSize = pHFRParams[0]->batchSizeMax;
            *pFPSValue = pHFRParams[0]->maxFPS;
        }
        else
        {
            CAMX_LOG_ERROR(CamxLogGroupHAL, "Failed to find supported HFR entry!");
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// HALDevice::Close
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult HALDevice::Close()
{
    CamxResult              result            = CamxResultSuccess;
    chi_hal_callback_ops_t* pCHIAppCallbacks  = GetCHIAppCallbacks();

    INT fd          = -1;
    BOOL dumpToFile = FALSE;
    DumpFrameworkRequests(fd, dumpToFile, FALSE);

    if (TRUE == IsCHIModuleInitialized())
    {
        m_bCHIModuleInitialized = FALSE;
        pCHIAppCallbacks->chi_teardown_override_session(reinterpret_cast<camera3_device*>(&m_camera3Device), 0, NULL);
        ReleaseStreamConfig();
        DeInitRequestLogger();
    }

    // Move torch release to post Session Destroy as we might set error conditions accordingly.
    UINT32 logicalCameraId = pCHIAppCallbacks->chi_remap_camera_id(m_fwId, IdRemapTorch);

    HAL3Module::GetInstance()->ReleaseTorchForCamera(logicalCameraId, m_fwId);

    PrintErrorSummary();
    /// @todo (CAMX-1797) Add support for SetDebugBuffers(0);

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// HALDevice::GetPhysicalCameraIDs
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult HALDevice::GetPhysicalCameraIDs(
    UINT32                    cameraID,
    UINT32                    arraySize,
    CDKInfoPhysicalCameraIds* pCDKPhysCamIds)
{
    CamxResult result = CamxResultEFailed;

    if ((0 < arraySize) && (NULL != pCDKPhysCamIds))
    {
        CDKInfoNumCameras cdkNumPhysicalCams;
        CDKInfoCameraId   cdkCamId                = { cameraID };
        chi_hal_callback_ops_t* pCHIAppCallbacks  = GetCHIAppCallbacks();

        if (NULL != pCHIAppCallbacks)
        {
            result = pCHIAppCallbacks->chi_get_info(CDKGetInfoNumPhysicalCameras, &cdkCamId, &cdkNumPhysicalCams);

            CAMX_LOG_VERBOSE(CamxLogGroupHAL, "Logical CameraID %u has %u physical cameras",
                             cameraID, cdkNumPhysicalCams.numCameras);
        }

        if ((CamxResultSuccess == result)                &&
            (0 != cdkNumPhysicalCams.numCameras)         &&
            (arraySize >= cdkNumPhysicalCams.numCameras))
        {
            result = pCHIAppCallbacks->chi_get_info(CDKGetInfoPhysicalCameraIds, &cdkCamId, pCDKPhysCamIds);

            if (CamxResultSuccess == result)
            {
                CAMX_ASSERT(cdkNumPhysicalCams.numCameras == pCDKPhysCamIds->numCameras);

                for (UINT32 camIdx = 0; camIdx < pCDKPhysCamIds->numCameras; camIdx++)
                {
                    CAMX_LOG_VERBOSE(CamxLogGroupHAL, "Logical CameraID %u: physical cameraID %u (%u of %u)",
                                     cameraID, pCDKPhysCamIds->physicalCameraIds[camIdx],
                                     (camIdx+1), pCDKPhysCamIds->numCameras);
                }
            }
        }
    }

    if (CamxResultSuccess != result)
    {
        CAMX_LOG_ERROR(CamxLogGroupHAL, "Failed to get physical camera ids result=%u", result);
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// HALDevice::CloseCachedSensorHandles
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void HALDevice::CloseCachedSensorHandles(
    UINT32 cameraID)
{
    CDKInfoPhysicalCameraIds    cdkPhysCamIds;
    UINT32                      physicalCamIds[MaxNumImageSensors];

    CamxResult                  result          = CamxResultEFailed;
    const HwEnvironment*        pHWEnvironment  = HwEnvironment::GetInstance();

    if (NULL != pHWEnvironment)
    {
        cdkPhysCamIds.physicalCameraIds = &physicalCamIds[0];
        result                          = GetPhysicalCameraIDs(cameraID, MaxNumImageSensors, &cdkPhysCamIds);
    }

    if (CamxResultSuccess == result)
    {
        for (UINT32 camIdx = 0; camIdx < cdkPhysCamIds.numCameras; camIdx++)
        {
            HwCameraInfo    cameraInfo;
            UINT32          physicalCamId = cdkPhysCamIds.physicalCameraIds[camIdx];

            result = pHWEnvironment->GetCameraInfo(physicalCamId, &cameraInfo);

            if (CamxResultSuccess == result)
            {
                SensorSubDevicesCache* pSensorDevicesCache =
                    reinterpret_cast<SensorSubDevicesCache*>(cameraInfo.pSensorCaps->pSensorDeviceCache);

                if (NULL == pSensorDevicesCache)
                {
                    // Cannot release the opened and cached HW handles OR No handles cached to release
                    // This is not a fatal error, if the handles or cache is null that indicates there is no
                    // HW that was cached to begin with.
                    // If the HW environment is NULL, we would probably crash much earlier than this point
                    result = CamxResultEFailed;
                    CAMX_LOG_WARN(CamxLogGroupHAL, "pSensorDeviceCache=NULL for Logical CameraID=%u, PhysicalID=%u",
                                   cameraID, physicalCamId);
                }
                else
                {
                    CAMX_LOG_CONFIG(CamxLogGroupHAL, "Releasing resources for Logical CameraID=%u, PhysicalID=%u",
                                    cameraID, physicalCamId);
                    pSensorDevicesCache->ReleaseAllSubDevices(physicalCamId);

                    // NOWHINE CP036a: Need exception here
                    SensorModuleStaticCaps* pSensorCaps = const_cast<SensorModuleStaticCaps*>(cameraInfo.pSensorCaps);
                    for (UINT i = 0; i < MaxRTSessionHandles; i++)
                    {
                        pSensorCaps->hCSLSession[i] = CSLInvalidHandle;
                    }
                    result = CamxResultSuccess;

                    HwEnvironment::GetInstance()->InitializeSensorHwDeviceCache(
                        physicalCamId, NULL, CSLInvalidHandle, 0, NULL, NULL);
                }
            }
            else
            {
                result = CamxResultEFailed;
                CAMX_LOG_ERROR(CamxLogGroupHAL, "Failed to retrieve cameraInfo for Logical CameraID=%u, PhysicalID=%u",
                               cameraID, physicalCamId);
            }
        }
    }
    else
    {
        CAMX_LOG_ERROR(CamxLogGroupHAL, "Failed to physical cameraIDs for Logical cameraID=%u", cameraID);
    }
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// HALDevice::ProcessCaptureRequest
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult HALDevice::ProcessCaptureRequest(
    Camera3CaptureRequest* pRequest)
{
    CamxResult result = CamxResultEFailed;

    if (TRUE == IsCHIModuleInitialized())
    {
        // Keep track of information related to request for error conditions
        PopulateFrameworkRequestBuffer(pRequest);

        CAMX_LOG_INFO(CamxLogGroupHAL,
                      "CHIModule: Original framework framenumber %d contains %d output buffers",
                      pRequest->frameworkFrameNum,
                      pRequest->numOutputBuffers);

        result = GetCHIAppCallbacks()->chi_override_process_request(reinterpret_cast<const camera3_device*>(&m_camera3Device),
                                                                    reinterpret_cast<camera3_capture_request_t*>(pRequest),
                                                                    NULL);
        if (CamxResultSuccess != result)
        {
            // Remove the request from the framework data list if the request fails
            RemoveFrameworkRequestBuffer(pRequest);
        }
    }
    else
    {
        CAMX_LOG_ERROR(CamxLogGroupHAL, "CHIModule disabled, rejecting HAL request");
    }

    CAMX_ASSERT(CamxResultSuccess == result);

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// HALDevice::Dump
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID HALDevice::Dump(
    INT fd
    ) const
{
    CAMX_ENTRYEXIT(CamxLogGroupCore);

    CAMX_LOG_TO_FILE(fd, 2, "+------------------------------------------------------------------+\n  "
                            "+         HAL Dump                                                 +\n  "
                            "+------------------------------------------------------------------+");

    DumpFrameworkRequests(fd, TRUE, FALSE);

    if ((NULL != GetCHIAppCallbacks()) && (NULL != m_camera3Device.pDeviceOps))
    {
        GetCHIAppCallbacks()->chi_override_dump(reinterpret_cast<const camera3_device*>(&m_camera3Device), fd);
    }

    ChiDumpState(fd);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// HALDevice::Flush
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult HALDevice::Flush()
{
    CAMX_ENTRYEXIT(CamxLogGroupCore);

    CamxResult result = CamxResultSuccess;

    INT fd          = -1;
    BOOL dumpToFile = FALSE;
    DumpFrameworkRequests(fd, dumpToFile, FALSE);

    // if there is no pending request, return immediately
    if ((InvalidFrameIndex == m_frameworkRequests.lastFrameworkRequestIndex) &&
        (InvalidFrameIndex == m_secondaryFrameworkRequests.lastFrameworkRequestIndex))
    {
        CAMX_LOG_INFO(CamxLogGroupHAL, "No pending request, so flush is returned with success");
    }
    else if ((NULL != GetCHIAppCallbacks()) && (NULL != m_camera3Device.pDeviceOps))
    {
        m_bFlushEnabled = TRUE;
        result = GetCHIAppCallbacks()->chi_override_flush(reinterpret_cast<const camera3_device*>(&m_camera3Device));
        m_bFlushEnabled = FALSE;

        DumpFrameworkRequests(fd, dumpToFile, TRUE);
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// HALDevice::ConstructDefaultRequestSettings
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
const Metadata* HALDevice::ConstructDefaultRequestSettings(
    Camera3RequestTemplate requestTemplate)
{
    UINT requestTemplateIndex       = requestTemplate - 1; // Need to subtract 1 since the templates start at 0x1
    const Metadata* pMetadata       = NULL;

    if (NULL == m_pDefaultRequestMetadata[requestTemplateIndex])
    {
        const Metadata*          pOverrideMetadata  = NULL;
        const camera_metadata_t* pChiMetadata       = NULL;

        GetCHIAppCallbacks()->chi_get_default_request_settings(GetCameraId(), requestTemplate, &pChiMetadata);

        UINT32 logicalCameraId = GetCHIAppCallbacks()->chi_remap_camera_id(GetFwCameraId(), IdRemapTorch);

        pOverrideMetadata = reinterpret_cast<const Metadata*>(pChiMetadata);

        pMetadata = HAL3DefaultRequest::ConstructDefaultRequestSettings(logicalCameraId, requestTemplate);

        if ((NULL != pMetadata) && (NULL != pOverrideMetadata))
        {
            CamxResult result = CamxResultSuccess;

            // NOWHINE CP036a: Since google function is non-const, had to add the const_cast
            result = HAL3MetadataUtil::MergeMetadata(const_cast<Metadata*>(pMetadata), pOverrideMetadata);

            if (CamxResultSuccess == result)
            {
                CAMX_LOG_INFO(CamxLogGroupHAL, "Override specific tags added to construct default settings");

                const StaticSettings*   pSettings = HwEnvironment::GetInstance()->GetStaticSettings();
                UINT32 visibility = (TRUE == pSettings->MetadataVisibility) ?
                    TagSectionVisibility::TagSectionVisibleToFramework : TagSectionVisibility::TagSectionVisibleToAll;

                SIZE_T                requestEntryCapacity;
                SIZE_T                requestDataSize;

                HAL3MetadataUtil::CalculateSizeAllMeta(&requestEntryCapacity, &requestDataSize, visibility);

                m_pDefaultRequestMetadata[requestTemplateIndex] = HAL3MetadataUtil::CreateMetadata(
                    requestEntryCapacity,
                    requestDataSize);

                result = HAL3MetadataUtil::CopyMetadata(m_pDefaultRequestMetadata[requestTemplateIndex],
                    // NOWHINE CP036a: Need cast
                    const_cast<Metadata*>(pMetadata), visibility);

                if (CamxResultSuccess != result)
                {
                    // NOWHINE CP036a: Need cast
                    m_pDefaultRequestMetadata[requestTemplateIndex] = const_cast<Metadata*>(pMetadata);
                }
            }
            else
            {
                CAMX_LOG_ERROR(CamxLogGroupHAL, "Could not add override specific tags to construct default settings");
            }
        }
        else
        {
            if (NULL == pMetadata)
            {
                CAMX_LOG_ERROR(CamxLogGroupHAL, "Construct default settings failures");
            }
            if (NULL == pOverrideMetadata)
            {
                CAMX_LOG_INFO(CamxLogGroupHAL, "No override specific tags given by override");
            }
        }
    }

    return m_pDefaultRequestMetadata[requestTemplateIndex];
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// HALDevice::DumpFrameworkRequests
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID HALDevice::DumpFrameworkRequests(
    INT  fd,
    BOOL dumpToFile,
    BOOL includeCompletedRequests
    ) const
{
    LogFrameworkRequests(&m_frameworkRequests.requestData[0], m_frameworkRequests.lastFrameworkRequestIndex,
        MaxOutstandingRequests, fd, dumpToFile, includeCompletedRequests);
    LogFrameworkRequests(&m_secondaryFrameworkRequests.requestData[0], m_secondaryFrameworkRequests.lastFrameworkRequestIndex,
        MaxSecondaryOutstandingRequests, fd, dumpToFile, includeCompletedRequests);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// HALDevice::LogRequest
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID HALDevice::LogRequest(
    const FrameworkRequestData* pRequestData,
    INT                         fd,
    BOOL                        withHeader,
    LogType                     logType
    ) const
{
    if ((NULL != pRequestData) && (NULL != m_pStreamConfig))
    {
        CHAR requestString[450];

        requestString[0] = '\0';

        // print header if requested
        if (TRUE == withHeader)
        {
            if (DumpToFile == logType)
            {
                CAMX_LOG_TO_FILE(fd, 4, "%s%s", HeaderString, m_pStreamsHeader);
            }
            else
            {
                CAMX_LOG_DUMP(CamxLogGroupHAL, "%s%s", HeaderString, m_pStreamsHeader);
            }
        }

        if (StringFormat == logType)
        {
            OsUtils::SNPrintF(requestString, sizeof(requestString), "Frame: %u ReqErr: %s ResErr: %s BufErr: %s Shutter: %s "
                "Metadata[Rcvd/Req]: %d/%d Output[Rcvd/Total/InErr]: %d/%d(%d)  Input[Rcvd/Total]: %d/%d Streams: ",
                pRequestData->frameworkNum,
                Utils::BoolToString(pRequestData->requestStatus.notifyRequestError),
                Utils::BoolToString(pRequestData->requestStatus.notifyResultError),
                Utils::BoolToString(pRequestData->requestStatus.anyBufferErrorNotification),
                Utils::BoolToString(pRequestData->requestStatus.notifyShutter),
                pRequestData->numPartialMetadataReceived,
                pRequestData->numPartialRequest,
                pRequestData->numBuffersReceived,
                pRequestData->numBuffers,
                pRequestData->numBuffersInError,
                pRequestData->numInputBuffersReceived,
                pRequestData->numInputBuffers);
        }
        else
        {
            OsUtils::SNPrintF(requestString, sizeof(requestString), " %8u\t\t\t %s\t\t\t %s\t\t\t %s\t\t  %s\t\t\t %d/%d\t\t\t"
                "\t\t\t%d/%d(%d)\t\t\t   %d/%d    ",
                pRequestData->frameworkNum,
                Utils::BoolToString(pRequestData->requestStatus.notifyRequestError),
                Utils::BoolToString(pRequestData->requestStatus.notifyResultError),
                Utils::BoolToString(pRequestData->requestStatus.anyBufferErrorNotification),
                Utils::BoolToString(pRequestData->requestStatus.notifyShutter),
                pRequestData->numPartialMetadataReceived,
                pRequestData->numPartialRequest,
                pRequestData->numBuffersReceived,
                pRequestData->numBuffers,
                pRequestData->numBuffersInError,
                pRequestData->numInputBuffersReceived,
                pRequestData->numInputBuffers);
        }

        // add stream information to the requestString
        for (UINT index = 0; index < m_pStreamConfig->numStreams; index++)
        {
            CHAR streamStatusString[60];

            if (StringFormat == logType)
            {
                OsUtils::SNPrintF(streamStatusString, sizeof(streamStatusString), "S%d(%p)[Rcvd/StatusOK/BufErrNotify]:"
                    " %d/%d/%d ## ",
                    index,
                    m_pStreamConfig->ppStreams[index],
                    pRequestData->buffers[index].bufferFlags.bufferReceived,
                    pRequestData->buffers[index].bufferFlags.bufferStatusOK,
                    pRequestData->buffers[index].bufferFlags.notifyBufferError);
            }
            else
            {
                OsUtils::SNPrintF(streamStatusString, sizeof(streamStatusString), "          %d/%d/%d%36s",
                    pRequestData->buffers[index].bufferFlags.bufferReceived,
                    pRequestData->buffers[index].bufferFlags.bufferStatusOK,
                    pRequestData->buffers[index].bufferFlags.notifyBufferError,
                    "##");
            }
            OsUtils::StrLCat(requestString, streamStatusString, sizeof(requestString));
        }

        if (DumpToFile == logType)
        {
            CAMX_LOG_TO_FILE(fd, 4, "%s", requestString);
        }
        else
        {
            CAMX_LOG_INFO(CamxLogGroupHAL, "%s", requestString);
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// HALDevice::LogFrameworkRequests
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID HALDevice::LogFrameworkRequests(
    const FrameworkRequestData* pRequestDataArray,
    INT                         latestFrameworkRequestIndex,
    UINT32                      maxSize,
    INT                         fd,
    BOOL                        dumpToFile,
    BOOL                        includeCompletedRequests
    ) const
{
    BOOL withHeader = TRUE;

    if (InvalidFrameIndex != latestFrameworkRequestIndex)
    {
        UINT32 count = maxSize;
        while (count)
        {
            const FrameworkRequestData* pRequestData = &pRequestDataArray[latestFrameworkRequestIndex];
            count--;
            if ((TRUE == pRequestData->requestStatus.indexUsed) || ((TRUE == includeCompletedRequests) &&
                (TRUE == pRequestData->requestStatus.requestIdDone)))
            {
                // Check if framework request entry is in error state
                if ((TRUE == includeCompletedRequests) ||
                    (pRequestData->numBuffersReceived != pRequestData->numBuffers) ||
                    ((pRequestData->numPartialMetadataReceived != pRequestData->numPartialRequest) &&
                    (FALSE == pRequestData->requestStatus.notifyResultError)) ||
                    (TRUE == pRequestData->requestStatus.notifyRequestError) ||
                    (TRUE == pRequestData->requestStatus.notifyResultError) ||
                    (0    != pRequestData->numBuffersInError) ||
                    ((FALSE == pRequestData->requestStatus.notifyShutter) && (0 != pRequestData->numBuffers)))
                {
                    if (TRUE == dumpToFile)
                    {
                        LogRequest(pRequestData, fd, withHeader, DumpToFile);
                    }
                    else
                    {
                        LogRequest(pRequestData, -1, withHeader, PrintToLogcat);
                    }
                    withHeader = FALSE;
                }
            }

            if (0 == latestFrameworkRequestIndex)
            {
                latestFrameworkRequestIndex = maxSize - 1;
            }
            else
            {
                latestFrameworkRequestIndex--;
            }
        }
    }
    else
    {
        CAMX_LOG_INFO(CamxLogGroupHAL, "m_frameworkRequests is empty latestFrameworkRequestIndex: %d",
            latestFrameworkRequestIndex);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// HALDevice::ClearFrameworkRequestBuffer
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID HALDevice::ClearFrameworkRequestBuffer()
{
    Utils::Memset(&m_frameworkRequests, 0, sizeof(m_frameworkRequests));
    m_frameworkRequests.lastFrameworkRequestIndex = InvalidFrameIndex;

    Utils::Memset(&m_secondaryFrameworkRequests, 0, sizeof(m_secondaryFrameworkRequests));
    m_secondaryFrameworkRequests.lastFrameworkRequestIndex = InvalidFrameIndex;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// HALDevice::PopulateFrameworkRequestBuffer
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID HALDevice::PopulateFrameworkRequestBuffer(
    Camera3CaptureRequest* pRequest)
{
    FrameworkRequestData* pFrameworkRequest = GetFrameworkRequestData(pRequest->frameworkFrameNum, TRUE);

    // Index is still being used, so adding the request to the secondary list
    if (TRUE == pFrameworkRequest->requestStatus.indexUsed)
    {
        CAMX_LOG_INFO(CamxLogGroupHAL, "Copying frame number %u to secondary list", pRequest->frameworkFrameNum);
        LogRequest(pFrameworkRequest, -1, FALSE, StringFormat);
        CopyToSecondaryRequestData(pRequest->frameworkFrameNum);
    }

    Utils::Memset(pFrameworkRequest, 0, sizeof(FrameworkRequestData));

    pFrameworkRequest->frameworkNum      = pRequest->frameworkFrameNum;
    pFrameworkRequest->numBuffers        = pRequest->numOutputBuffers;
    pFrameworkRequest->numInputBuffers   = (NULL != pRequest->pInputBuffer) ? 1 : 0;
    pFrameworkRequest->numPartialRequest = m_numPartialResult;
    pFrameworkRequest->requestStatus.indexUsed = TRUE;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// HALDevice::RemoveFrameworkRequestBuffer
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID HALDevice::RemoveFrameworkRequestBuffer(
    Camera3CaptureRequest* pRequest)
{
    FrameworkRequestData* pFrameworkRequest = GetFrameworkRequestData(pRequest->frameworkFrameNum, FALSE);

    Utils::Memset(pFrameworkRequest, 0, sizeof(FrameworkRequestData));
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// HALDevice::UpdateFrameworkRequestBufferResult
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID HALDevice::UpdateFrameworkRequestBufferResult(
    const Camera3CaptureResult* pCamera3CaptureResult,
    FrameworkRequestData*       pFrameworkRequest)
{
    pFrameworkRequest->numBuffersReceived += pCamera3CaptureResult->numOutputBuffers;

    if (pFrameworkRequest->numPartialMetadataReceived < pCamera3CaptureResult->numPartialMetadata)
    {
        pFrameworkRequest->numPartialMetadataReceived = pCamera3CaptureResult->numPartialMetadata;
    }

    if (NULL != pCamera3CaptureResult->pInputBuffer)
    {
        pFrameworkRequest->numInputBuffersReceived += 1;
        UINT streamId = GetStreamId(pCamera3CaptureResult->pInputBuffer->pStream);
        pFrameworkRequest->buffers[streamId].bufferFlags.bufferReceived = TRUE;
        if (BufferStatusOK == pCamera3CaptureResult->pInputBuffer->bufferStatus)
        {
            pFrameworkRequest->buffers[streamId].bufferFlags.bufferStatusOK = TRUE;
        }
        else
        {
            CAMX_LOG_ERROR(CamxLogGroupHAL, "BufferStatusError not expected for input buffer with StreamId %d for frame %u",
                streamId,
                pFrameworkRequest->frameworkNum);
        }
    }

    for (UINT index = 0; index < pCamera3CaptureResult->numOutputBuffers; index++)
    {
        UINT streamId = GetStreamId(pCamera3CaptureResult->pOutputBuffers[index].pStream);
        pFrameworkRequest->buffers[streamId].bufferFlags.bufferReceived = TRUE;
        if (BufferStatusOK == pCamera3CaptureResult->pOutputBuffers[index].bufferStatus)
        {
            pFrameworkRequest->buffers[streamId].bufferFlags.bufferStatusOK = TRUE;
        }
        else
        {
            if ((TRUE == pFrameworkRequest->buffers[streamId].bufferFlags.notifyBufferError) ||
                (TRUE == pFrameworkRequest->requestStatus.notifyRequestError))
            {
                pFrameworkRequest->numBuffersInError++;
            }
            else
            {
                CAMX_LOG_ERROR(CamxLogGroupHAL, "Error notification not received for buffer with streamId %d for frame %u",
                    streamId,
                    pFrameworkRequest->frameworkNum);
            }
        }
    }

    if ((pFrameworkRequest->numBuffers == pFrameworkRequest->numBuffersReceived) &&
        (pFrameworkRequest->numInputBuffers == pFrameworkRequest->numInputBuffersReceived))
    {
        BOOL anyError      = FALSE;
        BOOL metadataError = FALSE;

        // Check whether there is any buffer error
        if (TRUE == pFrameworkRequest->numBuffersInError)
        {
            anyError = TRUE;
        }

        if ((TRUE == pFrameworkRequest->requestStatus.notifyResultError) ||
            (TRUE == pFrameworkRequest->requestStatus.notifyRequestError))
        {
            metadataError = TRUE;
            anyError      = TRUE;
        }

        if (((pFrameworkRequest->numPartialMetadataReceived == pFrameworkRequest->numPartialRequest) ||
            (TRUE == metadataError)) &&
            ((TRUE == pFrameworkRequest->requestStatus.notifyShutter) || (TRUE == anyError)))
        {
            pFrameworkRequest->requestStatus.requestIdDone = TRUE;
            pFrameworkRequest->requestStatus.indexUsed     = FALSE;
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// HALDevice::UpdateFrameworkRequestBufferNotify
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID HALDevice::UpdateFrameworkRequestBufferNotify(
    const Camera3NotifyMessage* pCamera3NotifyMessage,
    FrameworkRequestData*       pFrameworkRequest)
{
    if (MessageTypeShutter == pCamera3NotifyMessage->messageType)
    {
        pFrameworkRequest->requestStatus.notifyShutter = TRUE;
    }
    else if (MessageTypeError == pCamera3NotifyMessage->messageType)
    {
        UINT streamId;
        switch (pCamera3NotifyMessage->message.errorMessage.errorMessageCode)
        {
            case MessageCodeRequest:
                pFrameworkRequest->requestStatus.notifyRequestError = TRUE;
                break;
            case MessageCodeResult:
                pFrameworkRequest->requestStatus.notifyResultError = TRUE;
                break;
            case MessageCodeBuffer:
                streamId = GetStreamId(pCamera3NotifyMessage->message.errorMessage.pErrorStream);
                pFrameworkRequest->buffers[streamId].bufferFlags.notifyBufferError = TRUE;
                pFrameworkRequest->requestStatus.anyBufferErrorNotification = TRUE;
                break;
            default:
                CAMX_LOG_ERROR(CamxLogGroupHAL, "Unknown error type: %u, frame number: %u",
                               pCamera3NotifyMessage->message.errorMessage.errorMessageCode,
                               pFrameworkRequest->frameworkNum);
                break;
        }

    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// HALDevice::SaveStreamConfig
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult HALDevice::SaveStreamConfig(
    Camera3StreamConfig* pStreamConfig)
{
    CamxResult result = CamxResultSuccess;

    m_pStreamConfig = static_cast<Camera3StreamConfig*>(CAMX_CALLOC(sizeof(Camera3StreamConfig)));

    if (NULL != m_pStreamConfig)
    {
        m_pStreamConfig->ppStreams =
            static_cast<Camera3Stream**>(CAMX_CALLOC(sizeof(Camera3Stream*) * pStreamConfig->numStreams));
    }

    if ((NULL != m_pStreamConfig) && (NULL != m_pStreamConfig->ppStreams))
    {
        m_pStreamConfig->numStreams = pStreamConfig->numStreams;

        for (UINT32 index = 0; index < m_pStreamConfig->numStreams; index++)
        {
            m_pStreamConfig->ppStreams[index] = pStreamConfig->ppStreams[index];
        }

        m_pStreamConfig->operationMode = pStreamConfig->operationMode;
    }
    else
    {
        CAMX_LOG_ERROR(CamxLogGroupHAL, "Failed to save stream config, Out of memory!!!, m_pStreamConfig = %p ",
            m_pStreamConfig);
        result = CamxResultEFailed;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// HALDevice::ReleaseStreamConfig
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID HALDevice::ReleaseStreamConfig()
{
    if (NULL != m_pStreamConfig)
    {
        if (NULL != m_pStreamConfig->ppStreams)
        {
            CAMX_FREE(m_pStreamConfig->ppStreams);
            m_pStreamConfig->ppStreams = NULL;
        }
        CAMX_FREE(m_pStreamConfig);
        m_pStreamConfig = NULL;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// HALDevice::InitializeRequestLogger
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult HALDevice::InitializeRequestLogger()
{
    CamxResult result = CamxResultSuccess;

    CHAR stream[60];

    UINT size = sizeof(stream) * m_pStreamConfig->numStreams;

    m_pStreamsHeader = static_cast<CHAR*>(CAMX_CALLOC(size));

    if (NULL != m_pStreamsHeader)
    {
        m_pStreamsHeader[0] = '\0';
        for (UINT index = 0; index < m_pStreamConfig->numStreams; index++)
        {
            OsUtils::SNPrintF(stream, sizeof(stream), "S%d[Rcvd/StatusOK/BufErrNotify](%p)[%d] ## ",
                index,
                // reinterpret_cast<Camera3HalStream*>(m_pStreamConfig->ppStreams[index]->pReserved[0])->id,
                m_pStreamConfig->ppStreams[index],
                m_pStreamConfig->ppStreams[index]->streamType);
            OsUtils::StrLCat(m_pStreamsHeader, stream, size);
        }
        CAMX_LOG_INFO(CamxLogGroupHAL, "m_pStreamsHeader %s", m_pStreamsHeader);
    }
    else
    {
        CAMX_LOG_ERROR(CamxLogGroupHAL, "Failed to initialize request logger, Num Stream %d, Out of memory!!!",
            m_pStreamConfig->numStreams);
        result = CamxResultEFailed;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// HALDevice::DeInitRequestLogger
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID HALDevice::DeInitRequestLogger()
{
    if (NULL != m_pStreamsHeader)
    {
        CAMX_FREE(m_pStreamsHeader);
        m_pStreamsHeader = NULL;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// HALDevice::GetFrameworkRequestData
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
FrameworkRequestData* HALDevice::GetFrameworkRequestData(
    UINT32 frameNumber,
    BOOL   update)
{
    UINT32 frameIndex = frameNumber % MaxOutstandingRequests;
    FrameworkRequestData* pFrameworkRequest;

    pFrameworkRequest = &m_frameworkRequests.requestData[frameIndex];

    // Keep track of lastest index used in the buffer
    if (TRUE == update)
    {
        m_frameworkRequests.lastFrameworkRequestIndex = frameIndex;
    }
    // Check secondary request data array if frameNumber not found in primary list
    else if (pFrameworkRequest->frameworkNum != frameNumber)
    {
        BOOL matchFlound = FALSE;

        // Linear search. Array size should be choosen small.
        for (UINT index = 0; index < MaxSecondaryOutstandingRequests; index++)
        {
            pFrameworkRequest = &m_secondaryFrameworkRequests.requestData[index];
            if (pFrameworkRequest->frameworkNum == frameNumber)
            {
                matchFlound = TRUE;
                break;
            }
        }
        if (FALSE == matchFlound)
        {
            CAMX_LOG_ERROR(CamxLogGroupHAL, "No match found for frame: %d", frameNumber);
        }
    }

    return pFrameworkRequest;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// HALDevice::CopyToSecondaryRequestData
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID HALDevice::CopyToSecondaryRequestData(
    UINT32 frameNumber)
{
    UINT32 frameIndex = (frameNumber % MaxOutstandingRequests);
    if (InvalidFrameIndex == m_secondaryFrameworkRequests.lastFrameworkRequestIndex)
    {
        m_secondaryFrameworkRequests.lastFrameworkRequestIndex = 0;
    }

    UINT32 lastIndex = m_secondaryFrameworkRequests.lastFrameworkRequestIndex++;
    m_secondaryFrameworkRequests.lastFrameworkRequestIndex %= MaxSecondaryOutstandingRequests;

    FrameworkRequestData* pSecondaryFrameworkData = &m_secondaryFrameworkRequests.requestData[lastIndex];
    if (FALSE == pSecondaryFrameworkData->requestStatus.indexUsed)
    {
        m_secondaryFrameworkRequests.requestData[lastIndex] = m_frameworkRequests.requestData[frameIndex];
    }
    else
    {
        CAMX_LOG_ERROR(CamxLogGroupHAL, "FATAL Error, Frame number %u, still not serviced",
            pSecondaryFrameworkData->frameworkNum);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// HALDevice::LogErrorAndAssert
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID HALDevice::LogErrorAndAssert(
    ErrorType errorType)
{
    BOOL crossThreshold = FALSE;
    UINT32* pErrorCnt   = NULL;
    BOOL raiseSigabort  = TRUE;

    switch (errorType)
    {
        case UnexpectedFrameNumber:
            m_errorTraker.numUnexpectedFrameNumber++;
            pErrorCnt = &m_errorTraker.numUnexpectedFrameNumber;
            break;
        case UnexpectedShutter:
            m_errorTraker.numUnexpectedShutterNotification++;
            pErrorCnt = &m_errorTraker.numUnexpectedShutterNotification;
            break;
        case UnexpectedRequestErrorNotification:
            m_errorTraker.numUnexpectedRequestErrorNotification++;
            pErrorCnt = &m_errorTraker.numUnexpectedRequestErrorNotification;
            break;
        case UnexpectedBufferErrorNotification:
            m_errorTraker.numUnexpectedBufferErrorNotification++;
            pErrorCnt = &m_errorTraker.numUnexpectedBufferErrorNotification;
            // Forcefully disabling sigabort raise for buffer error notification
            //  until the fix for duplicate error notification is merged.
            raiseSigabort = FALSE;
            break;
        case UnexpectedMetadataErrorNotification:
            m_errorTraker.numUnexpectedMetadataNotification++;
            pErrorCnt = &m_errorTraker.numUnexpectedMetadataNotification;
            break;
        case UnexpectedOutputBuffer:
            m_errorTraker.numUnexpectedOutputBufferResult++;
            pErrorCnt = &m_errorTraker.numUnexpectedOutputBufferResult;
            break;
        case UnexpectedMetadata:
            m_errorTraker.numUnexpectedMetadataResult++;
            pErrorCnt = &m_errorTraker.numUnexpectedMetadataResult;
            break;
        default:
            CAMX_LOG_ERROR(CamxLogGroupHAL, "Unknown error type %d, not supported!!", errorType);
            break;
    }

    const StaticSettings* pStaticSettings = HwEnvironment::GetInstance()->GetStaticSettings();
    if ((NULL != pErrorCnt) && (*pErrorCnt >= pStaticSettings->maxNumberOfAcceptedErrors))
    {
        crossThreshold = TRUE;
    }

    if ((TRUE == pStaticSettings->raisesigabrt) && (TRUE == crossThreshold) && (TRUE == raiseSigabort))
    {
        CAMX_LOG_ERROR(CamxLogGroupCore, "FATAL ERROR: Raise SigAbort to debug"
            " the root cause of Unexpected Result/Notification");
        PrintErrorSummary();
        OsUtils::RaiseSignalAbort();
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// HALDevice::PrintErrorSummary
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID HALDevice::PrintErrorSummary()
{
    if (0 != m_errorTraker.numUnexpectedFrameNumber ||
        0 != m_errorTraker.numUnexpectedShutterNotification ||
        0 != m_errorTraker.numUnexpectedRequestErrorNotification ||
        0 != m_errorTraker.numUnexpectedBufferErrorNotification ||
        0 != m_errorTraker.numUnexpectedMetadataNotification ||
        0 != m_errorTraker.numUnexpectedOutputBufferResult ||
        0 != m_errorTraker.numUnexpectedMetadataResult)
    {
        CAMX_LOG_DUMP(CamxLogGroupHAL, "==================================================================================");
        CAMX_LOG_DUMP(CamxLogGroupHAL, "+ Num Unknown Frame Error                   : %u",
            m_errorTraker.numUnexpectedFrameNumber);
        CAMX_LOG_DUMP(CamxLogGroupHAL, "+ Num Unexpected Shutter                    : %u",
            m_errorTraker.numUnexpectedShutterNotification);
        CAMX_LOG_DUMP(CamxLogGroupHAL, "+ Num Unexpected Request Error Notification : %u",
            m_errorTraker.numUnexpectedRequestErrorNotification);
        CAMX_LOG_DUMP(CamxLogGroupHAL, "+ Num Unexpected Buffer Error Notification  : %u",
            m_errorTraker.numUnexpectedBufferErrorNotification);
        CAMX_LOG_DUMP(CamxLogGroupHAL, "+ Num Unexpected Metadata Error Notification: %u",
            m_errorTraker.numUnexpectedMetadataNotification);
        CAMX_LOG_DUMP(CamxLogGroupHAL, "+ Num Unexpected Output buffer result       : %u",
            m_errorTraker.numUnexpectedOutputBufferResult);
        CAMX_LOG_DUMP(CamxLogGroupHAL, "+ Num Unexpected metadata result            : %u",
            m_errorTraker.numUnexpectedMetadataResult);
        CAMX_LOG_DUMP(CamxLogGroupHAL, "==================================================================================");
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// HALDevice::GetStreamId
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UINT HALDevice::GetStreamId(
    Camera3Stream* pStream
    ) const
{
    UINT streamId   = 0;
    BOOL matchFound = FALSE;

    // look for matching stream in the m_pStreamConfig which was cloned at the time of configure stream
    for (UINT streamIndex = 0; streamIndex < m_pStreamConfig->numStreams; streamIndex++)
    {
        if (pStream == m_pStreamConfig->ppStreams[streamIndex])
        {
            streamId   = streamIndex;
            matchFound = TRUE;
            break;
        }
    }

    if (FALSE == matchFound)
    {
        CAMX_LOG_ERROR(CamxLogGroupHAL, "Matching stream not found!!");
    }

    if (streamId > MaxOutputBuffers)
    {
        CAMX_LOG_ERROR(CamxLogGroupHAL, "StreamId is out of bound!!");
    }

    return streamId;
}

CAMX_NAMESPACE_END
