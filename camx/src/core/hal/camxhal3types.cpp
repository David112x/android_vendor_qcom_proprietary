////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2016-2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  camxhal3types.cpp
/// @brief Contains static_asserts used to ensure binary compatibility between HAL3 API types and the corresponding CamX type.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include <cutils/native_handle.h>
#include <hardware/camera3.h>
#include <hardware/hardware.h>
#include <system/camera_metadata.h>
#include <system/camera_vendor_tags.h>

#include "camxincs.h"

#include "camxhal3types.h"

CAMX_NAMESPACE_BEGIN

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// system/camera_metadata.h Wrappers
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CAMX_STATIC_ASSERT(static_cast<INT>(VendorTagType::Byte) == static_cast<INT>(TYPE_BYTE));
CAMX_STATIC_ASSERT(static_cast<INT>(VendorTagType::Int32) == static_cast<INT>(TYPE_INT32));
CAMX_STATIC_ASSERT(static_cast<INT>(VendorTagType::Float) == static_cast<INT>(TYPE_FLOAT));
CAMX_STATIC_ASSERT(static_cast<INT>(VendorTagType::Int64) == static_cast<INT>(TYPE_INT64));
CAMX_STATIC_ASSERT(static_cast<INT>(VendorTagType::Double) == static_cast<INT>(TYPE_DOUBLE));
CAMX_STATIC_ASSERT(static_cast<INT>(VendorTagType::Rational) == static_cast<INT>(TYPE_RATIONAL));

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// hardware/hardware.h Wrappers
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CAMX_STATIC_ASSERT(sizeof(HwModule) == sizeof(hw_module_t));
CAMX_STATIC_ASSERT(sizeof(HwModule::reserved) == sizeof(hw_module_t::reserved));
CAMX_STATIC_ASSERT(offsetof(HwModule, tag) == offsetof(hw_module_t, tag));
CAMX_STATIC_ASSERT(offsetof(HwModule, moduleAPIVersion) == offsetof(hw_module_t, module_api_version));
CAMX_STATIC_ASSERT(offsetof(HwModule, HALAPIVersion) == offsetof(hw_module_t, hal_api_version));
CAMX_STATIC_ASSERT(offsetof(HwModule, pId) == offsetof(hw_module_t, id));
CAMX_STATIC_ASSERT(offsetof(HwModule, pName) == offsetof(hw_module_t, name));
CAMX_STATIC_ASSERT(offsetof(HwModule, pAuthor) == offsetof(hw_module_t, author));
CAMX_STATIC_ASSERT(offsetof(HwModule, pMethods) == offsetof(hw_module_t, methods));
CAMX_STATIC_ASSERT(offsetof(HwModule, pDSO) == offsetof(hw_module_t, dso));
CAMX_STATIC_ASSERT(offsetof(HwModule, reserved) == offsetof(hw_module_t, reserved));

CAMX_STATIC_ASSERT(sizeof(HwDevice) == sizeof(hw_device_t));
CAMX_STATIC_ASSERT(sizeof(HwDevice::reserved) == sizeof(hw_device_t::reserved));
CAMX_STATIC_ASSERT(offsetof(HwDevice, tag) == offsetof(hw_device_t, tag));
CAMX_STATIC_ASSERT(offsetof(HwDevice, version) == offsetof(hw_device_t, version));
CAMX_STATIC_ASSERT(offsetof(HwDevice, pModule) == offsetof(hw_device_t, module));
CAMX_STATIC_ASSERT(offsetof(HwDevice, reserved) == offsetof(hw_device_t, reserved));
CAMX_STATIC_ASSERT(offsetof(HwDevice, close) == offsetof(hw_device_t, close));

CAMX_STATIC_ASSERT(sizeof(HwModuleMethods) == sizeof(hw_module_methods_t));
CAMX_STATIC_ASSERT(offsetof(HwModuleMethods, Open) == offsetof(hw_module_methods_t, open));

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// system/graphics.h Wrappers
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CAMX_STATIC_ASSERT(static_cast<INT>(HALPixelFormatRGBA8888) == static_cast<INT>(HAL_PIXEL_FORMAT_RGBA_8888));
CAMX_STATIC_ASSERT(static_cast<INT>(HALPixelFormatRGBX8888) == static_cast<INT>(HAL_PIXEL_FORMAT_RGBX_8888));
CAMX_STATIC_ASSERT(static_cast<INT>(HALPixelFormatRGB888) == static_cast<INT>(HAL_PIXEL_FORMAT_RGB_888));
CAMX_STATIC_ASSERT(static_cast<INT>(HALPixelFormatRGB565) == static_cast<INT>(HAL_PIXEL_FORMAT_RGB_565));
CAMX_STATIC_ASSERT(static_cast<INT>(HALPixelFormatBGRA8888) == static_cast<INT>(HAL_PIXEL_FORMAT_BGRA_8888));
CAMX_STATIC_ASSERT(static_cast<INT>(HALPixelFormatYV12) == static_cast<INT>(HAL_PIXEL_FORMAT_YV12));
CAMX_STATIC_ASSERT(static_cast<INT>(HALPixelFormatY8) == static_cast<INT>(HAL_PIXEL_FORMAT_Y8));
CAMX_STATIC_ASSERT(static_cast<INT>(HALPixelFormatY16) == static_cast<INT>(HAL_PIXEL_FORMAT_Y16));
CAMX_STATIC_ASSERT(static_cast<INT>(HALPixelFormatRaw16) == static_cast<INT>(HAL_PIXEL_FORMAT_RAW16));
CAMX_STATIC_ASSERT(static_cast<INT>(HALPixelFormatRaw10) == static_cast<INT>(HAL_PIXEL_FORMAT_RAW10));
CAMX_STATIC_ASSERT(static_cast<INT>(HALPixelFormatRaw12) == static_cast<INT>(HAL_PIXEL_FORMAT_RAW12));
CAMX_STATIC_ASSERT(static_cast<INT>(HALPixelFormatRawOpaque) == static_cast<INT>(HAL_PIXEL_FORMAT_RAW_OPAQUE));
CAMX_STATIC_ASSERT(static_cast<INT>(HALPixelFormatBlob) == static_cast<INT>(HAL_PIXEL_FORMAT_BLOB));
CAMX_STATIC_ASSERT(static_cast<INT>(HALPixelFormatImplDefined) == static_cast<INT>(HAL_PIXEL_FORMAT_IMPLEMENTATION_DEFINED));
CAMX_STATIC_ASSERT(static_cast<INT>(HALPixelFormatYCbCr420_888) == static_cast<INT>(HAL_PIXEL_FORMAT_YCbCr_420_888));
CAMX_STATIC_ASSERT(static_cast<INT>(HALPixelFormatYCbCr422_888) == static_cast<INT>(HAL_PIXEL_FORMAT_YCbCr_422_888));
CAMX_STATIC_ASSERT(static_cast<INT>(HALPixelFormatYCbCr444_888) == static_cast<INT>(HAL_PIXEL_FORMAT_YCbCr_444_888));
CAMX_STATIC_ASSERT(static_cast<INT>(HALPixelFormatFlexRGB888) == static_cast<INT>(HAL_PIXEL_FORMAT_FLEX_RGB_888));
CAMX_STATIC_ASSERT(static_cast<INT>(HALPixelFormatFlexRGBA8888) == static_cast<INT>(HAL_PIXEL_FORMAT_FLEX_RGBA_8888));
CAMX_STATIC_ASSERT(static_cast<INT>(HALPixelFormatYCbCr422SP) == static_cast<INT>(HAL_PIXEL_FORMAT_YCbCr_422_SP));
CAMX_STATIC_ASSERT(static_cast<INT>(HALPixelFormatYCrCb420SP) == static_cast<INT>(HAL_PIXEL_FORMAT_YCrCb_420_SP));
CAMX_STATIC_ASSERT(static_cast<INT>(HALPixelFormatYCbCr422I) == static_cast<INT>(HAL_PIXEL_FORMAT_YCbCr_422_I));
CAMX_STATIC_ASSERT(static_cast<INT>(HALDataspaceUnknown) == static_cast<INT>(HAL_DATASPACE_UNKNOWN));
CAMX_STATIC_ASSERT(static_cast<INT>(HALDataspaceArbitrary) == static_cast<INT>(HAL_DATASPACE_ARBITRARY));
CAMX_STATIC_ASSERT(static_cast<INT>(HALDataspaceStandardShift) == static_cast<INT>(HAL_DATASPACE_STANDARD_SHIFT));
CAMX_STATIC_ASSERT(static_cast<INT>(HALDataspaceTransferShift) == static_cast<INT>(HAL_DATASPACE_TRANSFER_SHIFT));
CAMX_STATIC_ASSERT(static_cast<INT>(HALDataspaceRangeShift) == static_cast<INT>(HAL_DATASPACE_RANGE_SHIFT));
CAMX_STATIC_ASSERT(static_cast<INT>(HALDataspaceStandardBT601_625) == static_cast<INT>(HAL_DATASPACE_STANDARD_BT601_625));
CAMX_STATIC_ASSERT(static_cast<INT>(HALDataspaceTransferSmpte170M) == static_cast<INT>(HAL_DATASPACE_TRANSFER_SMPTE_170M));
CAMX_STATIC_ASSERT(static_cast<INT>(HALDataspaceRangeFull) == static_cast<INT>(HAL_DATASPACE_RANGE_FULL));
CAMX_STATIC_ASSERT(static_cast<INT>(HALDataspaceJFIF) == static_cast<INT>(HAL_DATASPACE_JFIF));
CAMX_STATIC_ASSERT(static_cast<INT>(HALDataspaceV0JFIF) == static_cast<INT>(HAL_DATASPACE_V0_JFIF));
CAMX_STATIC_ASSERT(static_cast<INT>(HALDataspaceBT601_625) == static_cast<INT>(HAL_DATASPACE_BT601_625));
CAMX_STATIC_ASSERT(static_cast<INT>(HALDataspaceBT601_525) == static_cast<INT>(HAL_DATASPACE_BT601_525));
CAMX_STATIC_ASSERT(static_cast<INT>(HALDataspaceBT709) == static_cast<INT>(HAL_DATASPACE_BT709));
CAMX_STATIC_ASSERT(static_cast<INT>(HALDataspaceSRGBLinear) == static_cast<INT>(HAL_DATASPACE_SRGB_LINEAR));
CAMX_STATIC_ASSERT(static_cast<INT>(HALDataspaceSRGB) == static_cast<INT>(HAL_DATASPACE_SRGB));
CAMX_STATIC_ASSERT(static_cast<INT>(HALDataspaceDepth) == static_cast<INT>(HAL_DATASPACE_DEPTH));
CAMX_STATIC_ASSERT(static_cast<INT>(HALDataspaceArbitrary) == static_cast<INT>(HAL_DATASPACE_ARBITRARY));
#if (CAMERA_MODULE_API_VERSION_CURRENT > CAMERA_MODULE_API_VERSION_2_4) // Check Camera Module Version
CAMX_STATIC_ASSERT(static_cast<INT>(HALDataspaceJPEGAPPSegments) == static_cast<INT>(HAL_DATASPACE_JPEG_APP_SEGMENTS));
CAMX_STATIC_ASSERT(static_cast<INT>(HALDataspaceHEIF) == static_cast<INT>(HAL_DATASPACE_HEIF));
#endif  // Check Camera Module Version

#if ANDROID
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// hardware/gralloc.h Wrappers
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CAMX_STATIC_ASSERT(GrallocUsageSwReadNever == GRALLOC_USAGE_SW_READ_NEVER);
CAMX_STATIC_ASSERT(GrallocUsageSwReadRarely == GRALLOC_USAGE_SW_READ_RARELY);
CAMX_STATIC_ASSERT(GrallocUsageSwReadOften == GRALLOC_USAGE_SW_READ_OFTEN);
CAMX_STATIC_ASSERT(GrallocUsageSwReadMask == GRALLOC_USAGE_SW_READ_MASK);
CAMX_STATIC_ASSERT(GrallocUsageSwWriteNever == GRALLOC_USAGE_SW_WRITE_NEVER);
CAMX_STATIC_ASSERT(GrallocUsageSwWriteRarely == GRALLOC_USAGE_SW_WRITE_RARELY);
CAMX_STATIC_ASSERT(GrallocUsageSwWriteOften == GRALLOC_USAGE_SW_WRITE_OFTEN);
CAMX_STATIC_ASSERT(GrallocUsageSwWriteMask == GRALLOC_USAGE_SW_WRITE_MASK);
CAMX_STATIC_ASSERT(GrallocUsageHwTexture == GRALLOC_USAGE_HW_TEXTURE);
CAMX_STATIC_ASSERT(GrallocUsageHwRender == GRALLOC_USAGE_HW_RENDER);
CAMX_STATIC_ASSERT(GrallocUsageHw2D == GRALLOC_USAGE_HW_2D);
CAMX_STATIC_ASSERT(GrallocUsageHwComposer == GRALLOC_USAGE_HW_COMPOSER);
CAMX_STATIC_ASSERT(GrallocUsageHwFrameBuffer == GRALLOC_USAGE_HW_FB);
CAMX_STATIC_ASSERT(GrallocUsageExternalDisplay == GRALLOC_USAGE_EXTERNAL_DISP);
CAMX_STATIC_ASSERT(GrallocUsageProtected == GRALLOC_USAGE_PROTECTED);
CAMX_STATIC_ASSERT(GrallocUsageCursor == GRALLOC_USAGE_CURSOR);
CAMX_STATIC_ASSERT(GrallocUsageHwVideoEncoder == GRALLOC_USAGE_HW_VIDEO_ENCODER);
CAMX_STATIC_ASSERT(GrallocUsageHwCameraWrite == GRALLOC_USAGE_HW_CAMERA_WRITE);
CAMX_STATIC_ASSERT(GrallocUsageHwCameraRead == GRALLOC_USAGE_HW_CAMERA_READ);
CAMX_STATIC_ASSERT(GrallocUsageHwCameraZSL == GRALLOC_USAGE_HW_CAMERA_ZSL);
CAMX_STATIC_ASSERT(GrallocUsageHwCameraMask == GRALLOC_USAGE_HW_CAMERA_MASK);
CAMX_STATIC_ASSERT(GrallocUsageHwMask == GRALLOC_USAGE_HW_MASK);
CAMX_STATIC_ASSERT(GrallocUsageRenderScript == GRALLOC_USAGE_RENDERSCRIPT);
CAMX_STATIC_ASSERT(GrallocUsageForeignBuffers == GRALLOC_USAGE_FOREIGN_BUFFERS);
CAMX_STATIC_ASSERT(GrallocUsageAllocMask == static_cast<UINT32>(GRALLOC_USAGE_ALLOC_MASK));
CAMX_STATIC_ASSERT(GrallocUsagePrivate0 == GRALLOC_USAGE_PRIVATE_0);
CAMX_STATIC_ASSERT(GrallocUsagePrivate1 == GRALLOC_USAGE_PRIVATE_1);
CAMX_STATIC_ASSERT(GrallocUsage10Bit == GRALLOC_USAGE_PRIVATE_2);
CAMX_STATIC_ASSERT(GrallocUsagePrivate3 == GRALLOC_USAGE_PRIVATE_3);
CAMX_STATIC_ASSERT(GrallocUsagePrivateMask == GRALLOC_USAGE_PRIVATE_MASK);
#if (CAMERA_MODULE_API_VERSION_CURRENT > CAMERA_MODULE_API_VERSION_2_4) // Check Camera Module Version
CAMX_STATIC_ASSERT(GrallocUsageHwImageEncoder == GRALLOC_USAGE_HW_IMAGE_ENCODER);
#endif // Check Camera Module Version
#endif // ANDROID
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// cutils/native_handle.h Wrappers
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CAMX_STATIC_ASSERT(sizeof(NativeHandle) == sizeof(native_handle_t));
CAMX_STATIC_ASSERT(offsetof(NativeHandle, version) == offsetof(native_handle_t, version));
CAMX_STATIC_ASSERT(offsetof(NativeHandle, numFDs) == offsetof(native_handle_t, numFds));
CAMX_STATIC_ASSERT(offsetof(NativeHandle, numInts) == offsetof(native_handle_t, numInts));

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// hardware/camera3.h Wrappers
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CAMX_STATIC_ASSERT(static_cast<INT>(StreamConfigModeNormal) == static_cast<INT>(CAMERA3_STREAM_CONFIGURATION_NORMAL_MODE));
CAMX_STATIC_ASSERT(static_cast<INT>(StreamConfigModeConstrainedHighSpeed) ==
                   static_cast<INT>(CAMERA3_STREAM_CONFIGURATION_CONSTRAINED_HIGH_SPEED_MODE));
CAMX_STATIC_ASSERT(static_cast<INT>(StreamConfigModeVendorStart) ==
                   static_cast<INT>(CAMERA3_VENDOR_STREAM_CONFIGURATION_MODE_START));

CAMX_STATIC_ASSERT(static_cast<INT>(RequestTemplatePreview) == static_cast<INT>(CAMERA3_TEMPLATE_PREVIEW));
CAMX_STATIC_ASSERT(static_cast<INT>(RequestTemplateStillCapture) == static_cast<INT>(CAMERA3_TEMPLATE_STILL_CAPTURE));
CAMX_STATIC_ASSERT(static_cast<INT>(RequestTemplateVideoRecord) == static_cast<INT>(CAMERA3_TEMPLATE_VIDEO_RECORD));
CAMX_STATIC_ASSERT(static_cast<INT>(RequestTemplateVideoSnapshot) == static_cast<INT>(CAMERA3_TEMPLATE_VIDEO_SNAPSHOT));
CAMX_STATIC_ASSERT(static_cast<INT>(RequestTemplateZeroShutterLag) == static_cast<INT>(CAMERA3_TEMPLATE_ZERO_SHUTTER_LAG));
CAMX_STATIC_ASSERT(static_cast<INT>(RequestTemplateManual) == static_cast<INT>(CAMERA3_TEMPLATE_MANUAL));
CAMX_STATIC_ASSERT(static_cast<INT>(RequestTemplateVendorStart) == static_cast<INT>(CAMERA3_VENDOR_TEMPLATE_START));

CAMX_STATIC_ASSERT(static_cast<INT>(StreamTypeOutput) == static_cast<INT>(CAMERA3_STREAM_OUTPUT));
CAMX_STATIC_ASSERT(static_cast<INT>(StreamTypeInput) == static_cast<INT>(CAMERA3_STREAM_INPUT));
CAMX_STATIC_ASSERT(static_cast<INT>(StreamTypeBidirectional) == static_cast<INT>(CAMERA3_STREAM_BIDIRECTIONAL));

CAMX_STATIC_ASSERT(static_cast<INT>(StreamRotationCCW0) == static_cast<INT>(CAMERA3_STREAM_ROTATION_0));
CAMX_STATIC_ASSERT(static_cast<INT>(StreamRotationCCW90) == static_cast<INT>(CAMERA3_STREAM_ROTATION_90));
CAMX_STATIC_ASSERT(static_cast<INT>(StreamRotationCCW180) == static_cast<INT>(CAMERA3_STREAM_ROTATION_180));
CAMX_STATIC_ASSERT(static_cast<INT>(StreamRotationCCW270) == static_cast<INT>(CAMERA3_STREAM_ROTATION_270));

CAMX_STATIC_ASSERT(static_cast<INT>(BufferStatusOK) == static_cast<INT>(CAMERA3_BUFFER_STATUS_OK));
CAMX_STATIC_ASSERT(static_cast<INT>(BufferStatusError) == static_cast<INT>(CAMERA3_BUFFER_STATUS_ERROR));

CAMX_STATIC_ASSERT(static_cast<INT>(MessageTypeError) == static_cast<INT>(CAMERA3_MSG_ERROR));
CAMX_STATIC_ASSERT(static_cast<INT>(MessageTypeShutter) == static_cast<INT>(CAMERA3_MSG_SHUTTER));

CAMX_STATIC_ASSERT(static_cast<INT>(MessageCodeDevice) == static_cast<INT>(CAMERA3_MSG_ERROR_DEVICE));
CAMX_STATIC_ASSERT(static_cast<INT>(MessageCodeRequest) == static_cast<INT>(CAMERA3_MSG_ERROR_REQUEST));
CAMX_STATIC_ASSERT(static_cast<INT>(MessageCodeResult) == static_cast<INT>(CAMERA3_MSG_ERROR_RESULT));
CAMX_STATIC_ASSERT(static_cast<INT>(MessageCodeBuffer) == static_cast<INT>(CAMERA3_MSG_ERROR_BUFFER));

CAMX_STATIC_ASSERT(sizeof(Camera3Stream) == sizeof(camera3_stream_t));
CAMX_STATIC_ASSERT(offsetof(Camera3Stream, streamType) == offsetof(camera3_stream_t, stream_type));
CAMX_STATIC_ASSERT(offsetof(Camera3Stream, width) == offsetof(camera3_stream_t, width));
CAMX_STATIC_ASSERT(offsetof(Camera3Stream, height) == offsetof(camera3_stream_t, height));
CAMX_STATIC_ASSERT(offsetof(Camera3Stream, format) == offsetof(camera3_stream_t, format));
CAMX_STATIC_ASSERT(offsetof(Camera3Stream, grallocUsage) == offsetof(camera3_stream_t, usage));
CAMX_STATIC_ASSERT(offsetof(Camera3Stream, maxNumBuffers) == offsetof(camera3_stream_t, max_buffers));
CAMX_STATIC_ASSERT(offsetof(Camera3Stream, pPrivateInfo) == offsetof(camera3_stream_t, priv));
CAMX_STATIC_ASSERT(offsetof(Camera3Stream, dataspace) == offsetof(camera3_stream_t, data_space));
CAMX_STATIC_ASSERT(offsetof(Camera3Stream, rotation) == offsetof(camera3_stream_t, rotation));

CAMX_STATIC_ASSERT(sizeof(Camera3StreamBuffer) == sizeof(camera3_stream_buffer_t));
CAMX_STATIC_ASSERT(offsetof(Camera3StreamBuffer, pStream) == offsetof(camera3_stream_buffer_t, stream));
CAMX_STATIC_ASSERT(offsetof(Camera3StreamBuffer, phBuffer) == offsetof(camera3_stream_buffer_t, buffer));
CAMX_STATIC_ASSERT(offsetof(Camera3StreamBuffer, bufferStatus) == offsetof(camera3_stream_buffer_t, status));
CAMX_STATIC_ASSERT(offsetof(Camera3StreamBuffer, acquireFence) == offsetof(camera3_stream_buffer_t, acquire_fence));
CAMX_STATIC_ASSERT(offsetof(Camera3StreamBuffer, releaseFence) == offsetof(camera3_stream_buffer_t, release_fence));

CAMX_STATIC_ASSERT(sizeof(Camera3CaptureResult) == sizeof(camera3_capture_result_t));
CAMX_STATIC_ASSERT(offsetof(Camera3CaptureResult, frameworkFrameNum) == offsetof(camera3_capture_result_t, frame_number));
CAMX_STATIC_ASSERT(offsetof(Camera3CaptureResult, pResultMetadata) == offsetof(camera3_capture_result_t, result));
CAMX_STATIC_ASSERT(offsetof(Camera3CaptureResult, numOutputBuffers) == offsetof(camera3_capture_result_t, num_output_buffers));
CAMX_STATIC_ASSERT(offsetof(Camera3CaptureResult, pOutputBuffers) == offsetof(camera3_capture_result_t, output_buffers));
CAMX_STATIC_ASSERT(offsetof(Camera3CaptureResult, pInputBuffer) == offsetof(camera3_capture_result_t, input_buffer));
CAMX_STATIC_ASSERT(offsetof(Camera3CaptureResult, numPartialMetadata) == offsetof(camera3_capture_result_t, partial_result));

CAMX_STATIC_ASSERT(sizeof(Camera3ErrorMessage) == sizeof(camera3_error_msg_t));
CAMX_STATIC_ASSERT(offsetof(Camera3ErrorMessage, frameworkFrameNum) == offsetof(camera3_error_msg_t, frame_number));
CAMX_STATIC_ASSERT(offsetof(Camera3ErrorMessage, pErrorStream) == offsetof(camera3_error_msg_t, error_stream));
CAMX_STATIC_ASSERT(offsetof(Camera3ErrorMessage, errorMessageCode) == offsetof(camera3_error_msg_t, error_code));

CAMX_STATIC_ASSERT(sizeof(Camera3ShutterMessage) == sizeof(camera3_shutter_msg_t));
CAMX_STATIC_ASSERT(offsetof(Camera3ShutterMessage, frameworkFrameNum) == offsetof(camera3_shutter_msg_t, frame_number));
CAMX_STATIC_ASSERT(offsetof(Camera3ShutterMessage, timestamp) == offsetof(camera3_shutter_msg_t, timestamp));

CAMX_STATIC_ASSERT(sizeof(Camera3NotifyMessage) == sizeof(camera3_notify_msg_t));
CAMX_STATIC_ASSERT(sizeof(Camera3NotifyMessage::message) == sizeof(camera3_notify_msg_t::message));
CAMX_STATIC_ASSERT(offsetof(Camera3NotifyMessage, messageType) == offsetof(camera3_notify_msg_t, type));
CAMX_STATIC_ASSERT(offsetof(Camera3NotifyMessage, message) == offsetof(camera3_notify_msg_t, message));

// Size is not matching with Hal structure. New parameter is added. There is no straight forward way to add that new
// variable only for latest android version as ANDROID_API version is not updated.
// For now, comment the check for size and revisit this logic later.
// CAMX_STATIC_ASSERT(sizeof(Camera3StreamConfig) == sizeof(camera3_stream_configuration_t));
CAMX_STATIC_ASSERT(offsetof(Camera3StreamConfig, numStreams) == offsetof(camera3_stream_configuration_t, num_streams));
CAMX_STATIC_ASSERT(offsetof(Camera3StreamConfig, ppStreams) == offsetof(camera3_stream_configuration_t, streams));
CAMX_STATIC_ASSERT(offsetof(Camera3StreamConfig, operationMode) == offsetof(camera3_stream_configuration_t, operation_mode));

CAMX_STATIC_ASSERT(sizeof(Camera3CaptureRequest) == sizeof(camera3_capture_request_t));
CAMX_STATIC_ASSERT(offsetof(Camera3CaptureRequest, frameworkFrameNum) == offsetof(camera3_capture_request_t, frame_number));
CAMX_STATIC_ASSERT(offsetof(Camera3CaptureRequest, pMetadata) == offsetof(camera3_capture_request_t, settings));
CAMX_STATIC_ASSERT(offsetof(Camera3CaptureRequest, pInputBuffer) == offsetof(camera3_capture_request_t, input_buffer));
CAMX_STATIC_ASSERT(offsetof(Camera3CaptureRequest, numOutputBuffers) ==
                   offsetof(camera3_capture_request_t, num_output_buffers));
CAMX_STATIC_ASSERT(offsetof(Camera3CaptureRequest, pOutputBuffers) == offsetof(camera3_capture_request_t, output_buffers));

CAMX_STATIC_ASSERT(sizeof(Camera3Device) == sizeof(camera3_device_t));
CAMX_STATIC_ASSERT(offsetof(Camera3Device, hwDevice) == offsetof(camera3_device_t, common));
CAMX_STATIC_ASSERT(offsetof(Camera3Device, pDeviceOps) == offsetof(camera3_device_t, ops));
CAMX_STATIC_ASSERT(offsetof(Camera3Device, pPrivateData) == offsetof(camera3_device_t, priv));

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// system/camera.h Wrappers
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CAMX_STATIC_ASSERT(static_cast<INT>(ImageSensorFacingBack) == static_cast<INT>(CAMERA_FACING_BACK));
CAMX_STATIC_ASSERT(static_cast<INT>(ImageSensorFacingFront) == static_cast<INT>(CAMERA_FACING_FRONT));
CAMX_STATIC_ASSERT(static_cast<INT>(ImageSensorFacingExternal) == static_cast<INT>(CAMERA_FACING_EXTERNAL));

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// hardware/camera_common.h Wrappers
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CAMX_STATIC_ASSERT(static_cast<INT>(CameraDeviceStatusNotPresent) == static_cast<INT>(CAMERA_DEVICE_STATUS_NOT_PRESENT));
CAMX_STATIC_ASSERT(static_cast<INT>(CameraDeviceStatusPresent) == static_cast<INT>(CAMERA_DEVICE_STATUS_PRESENT));
CAMX_STATIC_ASSERT(static_cast<INT>(CameraDeviceStatusEnumerating) == static_cast<INT>(CAMERA_DEVICE_STATUS_ENUMERATING));

CAMX_STATIC_ASSERT(static_cast<INT>(TorchModeStatusNotAvailable) == static_cast<INT>(TORCH_MODE_STATUS_NOT_AVAILABLE));
CAMX_STATIC_ASSERT(static_cast<INT>(TorchModeStatusAvailableOff) == static_cast<INT>(TORCH_MODE_STATUS_AVAILABLE_OFF));
CAMX_STATIC_ASSERT(static_cast<INT>(TorchModeStatusAvailableOn) == static_cast<INT>(TORCH_MODE_STATUS_AVAILABLE_ON));

CAMX_STATIC_ASSERT(sizeof(CameraInfo) == sizeof(camera_info_t));
CAMX_STATIC_ASSERT(offsetof(CameraInfo, imageSensorFacing) == offsetof(camera_info_t, facing));
CAMX_STATIC_ASSERT(offsetof(CameraInfo, imageOrientation) == offsetof(camera_info_t, orientation));
CAMX_STATIC_ASSERT(offsetof(CameraInfo, deviceVersion) == offsetof(camera_info_t, device_version));
CAMX_STATIC_ASSERT(offsetof(CameraInfo, pStaticCameraInfo) == offsetof(camera_info_t, static_camera_characteristics));
CAMX_STATIC_ASSERT(offsetof(CameraInfo, resourceCost) == offsetof(camera_info_t, resource_cost));
CAMX_STATIC_ASSERT(offsetof(CameraInfo, ppConflictingDevices) == offsetof(camera_info_t, conflicting_devices));
CAMX_STATIC_ASSERT(offsetof(CameraInfo, conflictingDevicesLength) == offsetof(camera_info_t, conflicting_devices_length));

CAMX_NAMESPACE_END
