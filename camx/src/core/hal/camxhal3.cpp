////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2016-2020 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  camxhal3.cpp
/// @brief Landing methods for CamX implementation of HAL3
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// NOWHINE FILE NC010: Google Types
// NOWHINE FILE GR017: Google types
// NOWHINE FILE NC011: Google calls it 'init'

#include <system/camera_metadata.h>
#include "camera_metadata_hidden.h"
#include "camxchiofflinelogger.h"
#include "camxentry.h"
#include "camxhal3.h"
#include "camxhal3defaultrequest.h"
#include "camxhal3module.h"
#include "camxhaldevice.h"
#include "camxincs.h"
#include "camxvendortags.h"
#include "camxtrace.h"
#include "chi.h"
#include "chibinarylog.h"

CAMX_NAMESPACE_BEGIN

// Number of settings tokens
static const UINT32 NumExtendSettings = static_cast<UINT32>(ChxSettingsToken::CHX_SETTINGS_TOKEN_COUNT);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// FillTokenList
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void FillTokenList(
    CHISETTINGTOKEN* pTokens)
{
#define REGISTER_TOKEN(inToken, data)                                        \
    pTokens[static_cast<UINT>(inToken)].id    = static_cast<UINT>(inToken);  \
    pTokens[static_cast<UINT>(inToken)].size  = sizeof(data);

#define REGISTER_BIT_TOKEN(inToken, data)                                    \
    pTokens[static_cast<UINT>(inToken)].id    = static_cast<UINT>(inToken);  \
    pTokens[static_cast<UINT>(inToken)].size  = sizeof(VOID*);

    const StaticSettings* pStaticSettings = HwEnvironment::GetInstance()->GetStaticSettings();

    REGISTER_TOKEN(ChxSettingsToken::OverrideForceUsecaseId, pStaticSettings->overrideForceUsecaseId);
    REGISTER_TOKEN(ChxSettingsToken::OverrideDisableZSL, pStaticSettings->overrideDisableZSL);
    REGISTER_TOKEN(ChxSettingsToken::OverrideGPURotationUsecase, pStaticSettings->overrideGPURotationUsecase);
    REGISTER_TOKEN(ChxSettingsToken::OverrideEnableMFNR, pStaticSettings->overrideEnableMFNR);
    REGISTER_TOKEN(ChxSettingsToken::AnchorSelectionAlgoForMFNR, pStaticSettings->anchorSelectionAlgoForMFNR);
    REGISTER_TOKEN(ChxSettingsToken::OverrideHFRNo3AUseCase, pStaticSettings->overrideHFRNo3AUseCase);
    REGISTER_TOKEN(ChxSettingsToken::OverrideForceSensorMode, pStaticSettings->overrideForceSensorMode);
    REGISTER_TOKEN(ChxSettingsToken::DefaultMaxFPS, pStaticSettings->defaultMaxFPS);
    REGISTER_TOKEN(ChxSettingsToken::FovcEnable, pStaticSettings->fovcEnable);
    REGISTER_TOKEN(ChxSettingsToken::OverrideCameraClose, pStaticSettings->overrideCameraClose);
    REGISTER_TOKEN(ChxSettingsToken::OverrideCameraOpen, pStaticSettings->overrideCameraOpen);
    REGISTER_TOKEN(ChxSettingsToken::EISV2Enable, pStaticSettings->EISV2Enable);
    REGISTER_TOKEN(ChxSettingsToken::EISV3Enable, pStaticSettings->EISV3Enable);
    REGISTER_TOKEN(ChxSettingsToken::NumPCRsBeforeStreamOn, pStaticSettings->numPCRsBeforeStreamOn);
    REGISTER_TOKEN(ChxSettingsToken::StatsProcessingSkipFactor, pStaticSettings->statsProcessingSkipFactor);
    REGISTER_TOKEN(ChxSettingsToken::DumpDebugDataEveryProcessResult, pStaticSettings->dumpDebugDataEveryProcessResult);
    REGISTER_BIT_TOKEN(ChxSettingsToken::Enable3ADebugData, pStaticSettings->enable3ADebugData);
    REGISTER_BIT_TOKEN(ChxSettingsToken::EnableConcise3ADebugData, pStaticSettings->enableConcise3ADebugData);
    REGISTER_BIT_TOKEN(ChxSettingsToken::EnableTuningMetadata, pStaticSettings->enableTuningMetadata);
    REGISTER_TOKEN(ChxSettingsToken::DebugDataSizeAEC, pStaticSettings->debugDataSizeAEC);
    REGISTER_TOKEN(ChxSettingsToken::DebugDataSizeAWB, pStaticSettings->debugDataSizeAWB);
    REGISTER_TOKEN(ChxSettingsToken::DebugDataSizeAF, pStaticSettings->debugDataSizeAF);
    REGISTER_TOKEN(ChxSettingsToken::ConciseDebugDataSizeAEC, pStaticSettings->conciseDebugDataSizeAEC);
    REGISTER_TOKEN(ChxSettingsToken::ConciseDebugDataSizeAWB, pStaticSettings->conciseDebugDataSizeAWB);
    REGISTER_TOKEN(ChxSettingsToken::ConciseDebugDataSizeAF, pStaticSettings->conciseDebugDataSizeAF);
    REGISTER_TOKEN(ChxSettingsToken::TuningDumpDataSizeIFE, pStaticSettings->tuningDumpDataSizeIFE);
    REGISTER_TOKEN(ChxSettingsToken::TuningDumpDataSizeIPE, pStaticSettings->tuningDumpDataSizeIPE);
    REGISTER_TOKEN(ChxSettingsToken::TuningDumpDataSizeBPS, pStaticSettings->tuningDumpDataSizeBPS);
    REGISTER_TOKEN(ChxSettingsToken::MultiCameraVREnable, pStaticSettings->multiCameraVREnable);
    REGISTER_TOKEN(ChxSettingsToken::OverrideGPUDownscaleUsecase, pStaticSettings->overrideGPUDownscaleUsecase);
    REGISTER_TOKEN(ChxSettingsToken::AdvanceFeatureMask, pStaticSettings->advanceFeatureMask);
    REGISTER_TOKEN(ChxSettingsToken::DisableASDStatsProcessing, pStaticSettings->disableASDStatsProcessing);
    REGISTER_TOKEN(ChxSettingsToken::MultiCameraFrameSync, pStaticSettings->multiCameraFrameSync);
    REGISTER_TOKEN(ChxSettingsToken::OutputFormat, pStaticSettings->outputFormat);
    REGISTER_TOKEN(ChxSettingsToken::EnableCHIPartialData, pStaticSettings->enableCHIPartialData);
    REGISTER_TOKEN(ChxSettingsToken::EnableFDStreamInRealTime, pStaticSettings->enableFDStreamInRealTime);
    REGISTER_TOKEN(ChxSettingsToken::SelectInSensorHDR3ExpUsecase, pStaticSettings->selectInSensorHDR3ExpUsecase);
    REGISTER_TOKEN(ChxSettingsToken::EnableUnifiedBufferManager, pStaticSettings->enableUnifiedBufferManager);
    REGISTER_TOKEN(ChxSettingsToken::EnableCHIImageBufferLateBinding, pStaticSettings->enableCHIImageBufferLateBinding);
    REGISTER_TOKEN(ChxSettingsToken::EnableCHIPartialDataRecovery, pStaticSettings->enableCHIPartialDataRecovery);
    REGISTER_TOKEN(ChxSettingsToken::UseFeatureForQCFA, pStaticSettings->useFeatureForQCFA);
    REGISTER_TOKEN(ChxSettingsToken::AECGainThresholdForQCFA, pStaticSettings->AECGainThresholdForQCFA);
    REGISTER_TOKEN(ChxSettingsToken::EnableOfflineNoiseReprocess, pStaticSettings->enableOfflineNoiseReprocess);
    REGISTER_TOKEN(ChxSettingsToken::EnableAsciilog, pStaticSettings->enableAsciiLogging);
    REGISTER_TOKEN(ChxSettingsToken::EnableBinarylog, pStaticSettings->enableBinaryLogging);
    REGISTER_TOKEN(ChxSettingsToken::OverrideLogLevels, pStaticSettings->overrideLogLevels);
    REGISTER_TOKEN(ChxSettingsToken::EnableFeature2Dump, pStaticSettings->enableFeature2Dump);
    REGISTER_TOKEN(ChxSettingsToken::ForceHWMFFixedNumOfFrames, pStaticSettings->forceHWMFFixedNumOfFrames);
    REGISTER_TOKEN(ChxSettingsToken::ForceSWMFFixedNumOfFrames, pStaticSettings->forceSWMFFixedNumOfFrames);
    REGISTER_TOKEN(ChxSettingsToken::EnableTBMChiFence, pStaticSettings->enableTBMChiFence);
    REGISTER_TOKEN(ChxSettingsToken::EnableRawHDR, pStaticSettings->enableRawHDR);

    REGISTER_BIT_TOKEN(ChxSettingsToken::EnableRequestMapping, pStaticSettings->logRequestMapping);
    REGISTER_BIT_TOKEN(ChxSettingsToken::EnableSystemLogging, pStaticSettings->systemLogEnable);
    REGISTER_TOKEN(ChxSettingsToken::BPSRealtimeSensorId, pStaticSettings->bpsRealtimeSensorId);
    REGISTER_TOKEN(ChxSettingsToken::EnableMFSRChiFence, pStaticSettings->enableMFSRChiFence);
    REGISTER_TOKEN(ChxSettingsToken::MultiCameraJPEG, pStaticSettings->multiCameraJPEG);
    REGISTER_TOKEN(ChxSettingsToken::MaxHALRequests, pStaticSettings->maxHalRequests);
    REGISTER_TOKEN(ChxSettingsToken::MultiCameraHWSyncMask, pStaticSettings->multiCameraHWSyncMask);
    REGISTER_TOKEN(ChxSettingsToken::AnchorAlgoSelectionType, pStaticSettings->anchorAlgoSelectionType);
    REGISTER_TOKEN(ChxSettingsToken::EnableBLMClient, pStaticSettings->enableBLMClient);
    REGISTER_TOKEN(ChxSettingsToken::OverrideForceBurstShot, pStaticSettings->overrideForceBurstShot);
    REGISTER_BIT_TOKEN(ChxSettingsToken::ExposeFullSizeForQCFA, pStaticSettings->exposeFullSizeForQCFA);
    REGISTER_BIT_TOKEN(ChxSettingsToken::EnableScreenGrab, pStaticSettings->enableScreenGrab);
#undef REGISTER_SETTING
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// GetHALDevice
///
/// @brief  Extract the HAL device pointer from the camera3_device_t.
///         Precondition: pCamera3DeviceAPI has been checked for NULL
///
/// @param  pCamera3DeviceAPI The camera3_device_t pointer passed in from the application framework. Assumed that it has been
///                           checked against NULL.
///
/// @return A pointer to the HALSession object held in the opaque point in camera3_device_t.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static CAMX_INLINE HALDevice* GetHALDevice(
    const camera3_device_t* pCamera3DeviceAPI)
{
    CAMX_ASSERT(NULL != pCamera3DeviceAPI);

    return reinterpret_cast<HALDevice*>(pCamera3DeviceAPI->priv);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// FormatToString
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static const char* FormatToString(
    int format)
{
    const char* pFormatStr;

    switch (format)
    {
        case HAL_PIXEL_FORMAT_RGBA_8888:
            pFormatStr = "HAL_PIXEL_FORMAT_RGBA_8888";
            break;
        case HAL_PIXEL_FORMAT_RGBX_8888:
            pFormatStr = "HAL_PIXEL_FORMAT_RGBX_8888";
            break;
        case HAL_PIXEL_FORMAT_RGB_888:
            pFormatStr = "HAL_PIXEL_FORMAT_RGB_888";
            break;
        case HAL_PIXEL_FORMAT_RGB_565:
            pFormatStr = "HAL_PIXEL_FORMAT_RGB_565";
            break;
        case HAL_PIXEL_FORMAT_BGRA_8888:
            pFormatStr = "HAL_PIXEL_FORMAT_BGRA_8888";
            break;
        case HAL_PIXEL_FORMAT_YV12:
            pFormatStr = "HAL_PIXEL_FORMAT_YV12";
            break;
        case HAL_PIXEL_FORMAT_Y8:
            pFormatStr = "HAL_PIXEL_FORMAT_Y8";
            break;
        case HAL_PIXEL_FORMAT_Y16:
            pFormatStr = "HAL_PIXEL_FORMAT_Y16";
            break;
        case HAL_PIXEL_FORMAT_RAW16:
            pFormatStr = "HAL_PIXEL_FORMAT_RAW16";
            break;
        case HAL_PIXEL_FORMAT_RAW10:
            pFormatStr = "HAL_PIXEL_FORMAT_RAW10";
            break;
        case HAL_PIXEL_FORMAT_RAW12:
            pFormatStr = "HAL_PIXEL_FORMAT_RAW12";
            break;
        case HAL_PIXEL_FORMAT_RAW_OPAQUE:
            pFormatStr = "HAL_PIXEL_FORMAT_RAW_OPAQUE";
            break;
        case HAL_PIXEL_FORMAT_BLOB:
            pFormatStr = "HAL_PIXEL_FORMAT_BLOB";
            break;
        case HAL_PIXEL_FORMAT_IMPLEMENTATION_DEFINED:
            pFormatStr = "HAL_PIXEL_FORMAT_IMPLEMENTATION_DEFINED";
            break;
        case HAL_PIXEL_FORMAT_YCbCr_420_888:
            pFormatStr = "HAL_PIXEL_FORMAT_YCbCr_420_888";
            break;
        case HAL_PIXEL_FORMAT_YCbCr_422_888:
            pFormatStr = "HAL_PIXEL_FORMAT_YCbCr_422_888";
            break;
        case HAL_PIXEL_FORMAT_YCbCr_444_888:
            pFormatStr = "HAL_PIXEL_FORMAT_YCbCr_444_888";
            break;
        case HAL_PIXEL_FORMAT_FLEX_RGB_888:
            pFormatStr = "HAL_PIXEL_FORMAT_FLEX_RGB_888";
            break;
        case HAL_PIXEL_FORMAT_FLEX_RGBA_8888:
            pFormatStr = "HAL_PIXEL_FORMAT_FLEX_RGBA_8888";
            break;
        case HAL_PIXEL_FORMAT_YCbCr_422_SP:
            pFormatStr = "HAL_PIXEL_FORMAT_YCbCr_422_SP";
            break;
        case HAL_PIXEL_FORMAT_YCrCb_420_SP:
            pFormatStr = "HAL_PIXEL_FORMAT_YCrCb_420_SP";
            break;
        case HAL_PIXEL_FORMAT_YCbCr_422_I:
            pFormatStr = "HAL_PIXEL_FORMAT_YCbCr_422_I";
            break;
        case ChiStreamFormatNV12HEIF:
            pFormatStr = "HAL_PIXEL_FORMAT_NV12_HEIF";
            break;
        case ChiStreamFormatUBWCNV12:
            pFormatStr = "HAL_PIXEL_FORMAT_YCbCr_420_SP_VENUS_UBWC";
            break;
        case ChiStreamFormatUBWCTP10:
            pFormatStr = "HAL_PIXEL_FORMAT_YCbCr_420_TP10_UBWC";
            break;

        default:
            pFormatStr = "Unknown Format";
            break;
    }

    return pFormatStr;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// StreamTypeToString
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static const char* StreamTypeToString(
    int streamType)
{
    const char* pStreamTypeStr;

    switch (streamType)
    {
        case CAMERA3_STREAM_OUTPUT:
            pStreamTypeStr = "CAMERA3_STREAM_OUTPUT";
            break;
        case CAMERA3_STREAM_INPUT:
            pStreamTypeStr = "CAMERA3_STREAM_INPUT";
            break;
        case CAMERA3_STREAM_BIDIRECTIONAL:
            pStreamTypeStr = "CAMERA3_STREAM_BIDIRECTIONAL";
            break;
        default:
            pStreamTypeStr = "Unknown Stream Type";
            break;
    }

    return pStreamTypeStr;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// DataSpaceToString
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static const char* DataSpaceToString(
    android_dataspace_t dataSpace)
{
    const char* pDataSpaceStr;
    BOOL checkV1_2 = FALSE;
#if (CAMERA_MODULE_API_VERSION_CURRENT > CAMERA_MODULE_API_VERSION_2_4) // Check Camera Module Version
    android_dataspace_v1_2_t dataSpaceV1_2 = static_cast<android_dataspace_v1_2_t>(dataSpace);
#endif // Check Camera Module Version

    switch (dataSpace)
    {
        case HAL_DATASPACE_ARBITRARY:
            pDataSpaceStr = "HAL_DATASPACE_ARBITRARY";
            break;
        case HAL_DATASPACE_STANDARD_BT709:
            pDataSpaceStr = "HAL_DATASPACE_STANDARD_BT709";
            break;
        case HAL_DATASPACE_STANDARD_BT601_625:
            pDataSpaceStr = "HAL_DATASPACE_STANDARD_BT601_625";
            break;
        case HAL_DATASPACE_STANDARD_BT601_625_UNADJUSTED:
            pDataSpaceStr = "HAL_DATASPACE_STANDARD_BT601_625_UNADJUSTED";
            break;
        case HAL_DATASPACE_STANDARD_BT601_525:
            pDataSpaceStr = "HAL_DATASPACE_STANDARD_BT601_525";
            break;
        case HAL_DATASPACE_STANDARD_BT601_525_UNADJUSTED:
            pDataSpaceStr = "HAL_DATASPACE_STANDARD_BT601_525_UNADJUSTED";
            break;
        case HAL_DATASPACE_STANDARD_BT2020:
            pDataSpaceStr = "HAL_DATASPACE_STANDARD_BT2020";
            break;
        case HAL_DATASPACE_STANDARD_BT2020_CONSTANT_LUMINANCE:
            pDataSpaceStr = "HAL_DATASPACE_STANDARD_BT2020_CONSTANT_LUMINANCE";
            break;
        case HAL_DATASPACE_STANDARD_BT470M:
            pDataSpaceStr = "HAL_DATASPACE_STANDARD_BT470M";
            break;
        case HAL_DATASPACE_STANDARD_FILM:
            pDataSpaceStr = "HAL_DATASPACE_STANDARD_FILM";
            break;
        case HAL_DATASPACE_TRANSFER_LINEAR:
            pDataSpaceStr = "HAL_DATASPACE_TRANSFER_LINEAR";
            break;
        case HAL_DATASPACE_TRANSFER_SRGB:
            pDataSpaceStr = "HAL_DATASPACE_TRANSFER_SRGB";
            break;
        case HAL_DATASPACE_TRANSFER_SMPTE_170M:
            pDataSpaceStr = "HAL_DATASPACE_TRANSFER_SMPTE_170M";
            break;
        case HAL_DATASPACE_TRANSFER_GAMMA2_2:
            pDataSpaceStr = "HAL_DATASPACE_TRANSFER_GAMMA2_2";
            break;
        case HAL_DATASPACE_TRANSFER_GAMMA2_8:
            pDataSpaceStr = "HAL_DATASPACE_TRANSFER_GAMMA2_8";
            break;
        case HAL_DATASPACE_TRANSFER_ST2084:
            pDataSpaceStr = "HAL_DATASPACE_TRANSFER_ST2084";
            break;
        case HAL_DATASPACE_TRANSFER_HLG:
            pDataSpaceStr = "HAL_DATASPACE_TRANSFER_HLG";
            break;
        case HAL_DATASPACE_RANGE_FULL:
            pDataSpaceStr = "HAL_DATASPACE_RANGE_FULL";
            break;
        case HAL_DATASPACE_RANGE_LIMITED:
            pDataSpaceStr = "HAL_DATASPACE_RANGE_LIMITED";
            break;
        case HAL_DATASPACE_SRGB_LINEAR:
            pDataSpaceStr = "HAL_DATASPACE_SRGB_LINEAR";
            break;
        case HAL_DATASPACE_V0_SRGB_LINEAR:
            pDataSpaceStr = "HAL_DATASPACE_V0_SRGB_LINEAR";
            break;
        case HAL_DATASPACE_SRGB:
            pDataSpaceStr = "HAL_DATASPACE_SRGB";
            break;
        case HAL_DATASPACE_V0_SRGB:
            pDataSpaceStr = "HAL_DATASPACE_V0_SRGB";
            break;
        case HAL_DATASPACE_JFIF:
            pDataSpaceStr = "HAL_DATASPACE_JFIF";
            break;
        case HAL_DATASPACE_V0_JFIF:
            pDataSpaceStr = "HAL_DATASPACE_V0_JFIF";
            break;
        case HAL_DATASPACE_BT601_625:
            pDataSpaceStr = "HAL_DATASPACE_BT601_625";
            break;
        case HAL_DATASPACE_V0_BT601_625:
            pDataSpaceStr = "HAL_DATASPACE_V0_BT601_625";
            break;
        case HAL_DATASPACE_BT601_525:
            pDataSpaceStr = "HAL_DATASPACE_BT601_525";
            break;
        case HAL_DATASPACE_V0_BT601_525:
            pDataSpaceStr = "HAL_DATASPACE_V0_BT601_525";
            break;
        case HAL_DATASPACE_BT709:
            pDataSpaceStr = "HAL_DATASPACE_BT709";
            break;
        case HAL_DATASPACE_V0_BT709:
            pDataSpaceStr = "HAL_DATASPACE_V0_BT709";
            break;
        case HAL_DATASPACE_DEPTH:
            pDataSpaceStr = "HAL_DATASPACE_DEPTH";
            break;
        case HAL_DATASPACE_UNKNOWN:
            // case HAL_DATASPACE_STANDARD_UNSPECIFIED:
            // case HAL_DATASPACE_RANGE_UNSPECIFIED:
            // case HAL_DATASPACE_TRANSFER_UNSPECIFIED:
            // case HAL_DATASPACE_STANDARD_UNSPECIFIED:
        default:
            checkV1_2     = TRUE;
            pDataSpaceStr = "HAL_DATASPACE_UNKNOWN";
            break;
    }

#if (CAMERA_MODULE_API_VERSION_CURRENT > CAMERA_MODULE_API_VERSION_2_4) // Check Camera Module Version
    if (TRUE == checkV1_2)
    {
        switch (dataSpaceV1_2)
        {
            case HAL_DATASPACE_JPEG_APP_SEGMENTS:
                pDataSpaceStr = "HAL_DATASPACE_JPEG_APP_SEGMENTS";
                break;
            case HAL_DATASPACE_HEIF:
                pDataSpaceStr = "HAL_DATASPACE_HEIF";
                break;
            default:
                pDataSpaceStr = "HAL_DATASPACE_UNKNOWN";
                break;
        }
    }
#endif // Check Camera Module Version
    return pDataSpaceStr;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// RotationToString
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
const char* RotationToString(
    int rotation)
{
    const char* pRotationStr;

    switch (rotation)
    {
        case CAMERA3_STREAM_ROTATION_0:
            pRotationStr = "CAMERA3_STREAM_ROTATION_0";
            break;
        case CAMERA3_STREAM_ROTATION_90:
            pRotationStr = "CAMERA3_STREAM_ROTATION_90";
            break;
        case CAMERA3_STREAM_ROTATION_180:
            pRotationStr = "CAMERA3_STREAM_ROTATION_180";
            break;
        case CAMERA3_STREAM_ROTATION_270:
            pRotationStr = "CAMERA3_STREAM_ROTATION_270";
            break;
        default:
            pRotationStr = "Unknown Rotation Angle";
            break;
    }
    return pRotationStr;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// hw_module_methods_t entry points
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// GetCHIAppCallbacks
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static inline chi_hal_callback_ops_t* GetCHIAppCallbacks()
{
    return  HAL3Module::GetInstance()->GetCHIAppCallbacks();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// open
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static int open(
    const struct hw_module_t*   pHwModuleAPI,
    const char*                 pCameraIdAPI,
    struct hw_device_t**        ppHwDeviceAPI)
{
    CAMX_ENTRYEXIT_SCOPE(CamxLogGroupHAL, SCOPEEventHAL3Open);

    CamxResult result = CamxResultSuccess;
    CAMX_ASSERT(NULL != pHwModuleAPI);
    CAMX_ASSERT(NULL != pHwModuleAPI->id);
    CAMX_ASSERT(NULL != pHwModuleAPI->name);
    CAMX_ASSERT(NULL != pHwModuleAPI->author);
    CAMX_ASSERT(NULL != pHwModuleAPI->methods);
    CAMX_ASSERT('\0' != pCameraIdAPI[0]);
    CAMX_ASSERT(NULL != pCameraIdAPI);
    CAMX_ASSERT(NULL != ppHwDeviceAPI);

    if ((NULL != pHwModuleAPI) &&
        (NULL != pHwModuleAPI->id) &&
        (NULL != pHwModuleAPI->name) &&
        (NULL != pHwModuleAPI->author) &&
        (NULL != pHwModuleAPI->methods) &&
        (NULL != pCameraIdAPI) &&
        ('\0' != pCameraIdAPI[0]) &&
        (NULL != ppHwDeviceAPI))
    {
        CamX::OfflineLogger* pOfflineLoggerASCII = CamX::OfflineLogger::GetInstance(OfflineLoggerType::ASCII);
        if (NULL != pOfflineLoggerASCII)
        {
            pOfflineLoggerASCII->NotifyCameraOpen();
        }
        CamX::OfflineLogger* pOfflineLoggerBinary = CamX::OfflineLogger::GetInstance(OfflineLoggerType::BINARY);
        if (NULL != pOfflineLoggerBinary)
        {
            pOfflineLoggerBinary->NotifyCameraOpen();
        }

        UINT32  cameraId        = 0;
        UINT32  logicalCameraId = 0;
        CHAR*   pNameEnd    = NULL;

        cameraId = OsUtils::StrToUL(pCameraIdAPI, &pNameEnd, 10);
        if (*pNameEnd != '\0')
        {
            CAMX_LOG_ERROR(CamxLogGroupHAL, "Invalid camera id: %s", pCameraIdAPI);
            // HAL interface requires -EINVAL (EInvalidArg) for invalid arguments
            result = CamxResultEInvalidArg;
        }

        if (CamxResultSuccess == result)
        {
            // Framework camera ID should only be known to these static landing functions, and the remap function
            logicalCameraId = GetCHIAppCallbacks()->chi_remap_camera_id(cameraId, IdRemapCamera);

            // Reserve the Torch resource for camera.
            // If torch already switched on, then turn it off and reserve for camera.
            HAL3Module::GetInstance()->ReserveTorchForCamera(
                GetCHIAppCallbacks()->chi_remap_camera_id(cameraId, IdRemapTorch), cameraId);

            // Sample code to show how the VOID* can be used in ExtendOpen
            CHIEXTENDSETTINGS extend                       = { 0 };
            CHISETTINGTOKEN   tokenList[NumExtendSettings] = { { 0 } };
            extend.pTokens                                 = tokenList;

            GenerateExtendOpenData(NumExtendSettings, &extend);

            // Reserve the camera to detect if it is already open or too many concurrent are open
            CAMX_LOG_CONFIG(CamxLogGroupHAL, "HalOp: Begin OPEN, logicalCameraId: %d, cameraId: %d",
                            logicalCameraId, cameraId);
            result = HAL3Module::GetInstance()->ProcessCameraOpen(logicalCameraId, &extend);
        }

        if (CamxResultSuccess == result)
        {
            // Sample code to show how the VOID* can be used in ModifySettings
            ChiModifySettings setting[NumExtendSettings] = { { { 0 } } };
            GenerateModifySettingsData(setting);

            for (UINT i = 0; i < NumExtendSettings; i++)
            {
                GetCHIAppCallbacks()->chi_modify_settings(&setting[i]);
            }

            const StaticSettings* pStaticSettings = HwEnvironment::GetInstance()->GetStaticSettings();

            CAMX_LOG_INFO(CamxLogGroupHAL, "Open: overrideCameraClose is %d , overrideCameraOpen is %d ",
                pStaticSettings->overrideCameraClose, pStaticSettings->overrideCameraOpen);

            const HwModule* pHwModule  = reinterpret_cast<const HwModule*>(pHwModuleAPI);
            HALDevice*      pHALDevice = HALDevice::Create(pHwModule, logicalCameraId, cameraId);

            if (NULL != pHALDevice)
            {
                camera3_device_t* pCamera3Device = reinterpret_cast<camera3_device_t*>(pHALDevice->GetCameraDevice());
                camera3_device_t& rCamera3Device = *pCamera3Device;
                *ppHwDeviceAPI                   = &pCamera3Device->common;
                BINARY_LOG(LogEvent::HAL3_Open, rCamera3Device, logicalCameraId, cameraId);
            }
            else
            {
                // HAL interface requires -ENODEV (EFailed) for all other internal errors
                result = CamxResultEFailed;
                CAMX_LOG_ERROR(CamxLogGroupHAL, "Error while opening camera");

                CHIEXTENDSETTINGS extend                       = { 0 };
                CHISETTINGTOKEN   tokenList[NumExtendSettings] = { { 0 } };
                extend.pTokens = tokenList;

                GenerateExtendCloseData(NumExtendSettings, &extend);

                // Allow the camera to be reopened later
                HAL3Module::GetInstance()->ProcessCameraClose(logicalCameraId, &extend);

                ChiModifySettings setting[NumExtendSettings] = { { { 0 } } };
                GenerateModifySettingsData(setting);

                for (UINT i = 0; i < NumExtendSettings; i++)
                {
                    GetCHIAppCallbacks()->chi_modify_settings(&setting[i]);
                }
            }
        }

        if (CamxResultSuccess != result)
        {
            // If open fails, then release the Torch resource that we reserved.
            HAL3Module::GetInstance()->ReleaseTorchForCamera(
                GetCHIAppCallbacks()->chi_remap_camera_id(cameraId, IdRemapTorch), cameraId);
        }
        CAMX_LOG_CONFIG(CamxLogGroupHAL, "HalOp: End OPEN, logicalCameraId: %d, cameraId: %d",
                        logicalCameraId, cameraId);
    }
    else
    {
        CAMX_LOG_ERROR(CamxLogGroupHAL, "Invalid argument(s) for open()");
        // HAL interface requires -EINVAL (EInvalidArg) for invalid arguments
        result = CamxResultEInvalidArg;
    }

    return Utils::CamxResultToErrno(result);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// vendor_tag_ops_t entry points
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// get_tag_count
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static int get_tag_count(
    const vendor_tag_ops_t* pVendorTagOpsAPI)
{
    CAMX_ENTRYEXIT_SCOPE(CamxLogGroupHAL, SCOPEEventHAL3GetTagCount);

    // Per Google, this parameter is extraneous
    CAMX_UNREFERENCED_PARAM(pVendorTagOpsAPI);

    return static_cast<int>(VendorTagManager::GetTagCount(TagSectionVisibility::TagSectionVisibleToAll));
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// get_all_tags
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static void get_all_tags(
    const vendor_tag_ops_t* pVendorTagOpsAPI,
    uint32_t*               pTagArrayAPI)
{
    CAMX_ENTRYEXIT_SCOPE(CamxLogGroupHAL, SCOPEEventHAL3GetAllTags);

    // Per Google, this parameter is extraneous
    CAMX_UNREFERENCED_PARAM(pVendorTagOpsAPI);

    CAMX_STATIC_ASSERT(sizeof(pTagArrayAPI[0]) == sizeof(VendorTag));

    CAMX_ASSERT(NULL != pTagArrayAPI);
    if (NULL != pTagArrayAPI)
    {
        VendorTag* pVendorTags = static_cast<VendorTag*>(pTagArrayAPI);
        VendorTagManager::GetAllTags(pVendorTags, TagSectionVisibility::TagSectionVisibleToAll);
    }
    else
    {
        CAMX_LOG_ERROR(CamxLogGroupHAL, "Invalid argument 2 for get_all_tags()");
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// get_section_name
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static const char* get_section_name(
    const vendor_tag_ops_t* pVendorTagOpsAPI,
    uint32_t                tagAPI)
{
    CAMX_ENTRYEXIT_SCOPE(CamxLogGroupHAL, SCOPEEventHAL3GetSectionName);

    // Per Google, this parameter is extraneous
    CAMX_UNREFERENCED_PARAM(pVendorTagOpsAPI);

    CAMX_STATIC_ASSERT(sizeof(tagAPI) == sizeof(VendorTag));

    return VendorTagManager::GetSectionName(static_cast<VendorTag>(tagAPI));
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// get_tag_name
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static const char* get_tag_name(
    const vendor_tag_ops_t* pVendorTagOpsAPI,
    uint32_t                tagAPI)
{
    CAMX_ENTRYEXIT_SCOPE(CamxLogGroupHAL, SCOPEEventHAL3GetTagName);

    // Per Google, this parameter is extraneous
    CAMX_UNREFERENCED_PARAM(pVendorTagOpsAPI);

    CAMX_STATIC_ASSERT(sizeof(tagAPI) == sizeof(VendorTag));

    return VendorTagManager::GetTagName(static_cast<VendorTag>(tagAPI));
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// get_tag_type
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static int get_tag_type(
    const vendor_tag_ops_t* pVendorTagOpsAPI,
    uint32_t                tagAPI)
{
    CAMX_ENTRYEXIT_SCOPE(CamxLogGroupHAL, SCOPEEventHAL3GetTagType);

    // Per Google, this parameter is extraneous
    CAMX_UNREFERENCED_PARAM(pVendorTagOpsAPI);

    CAMX_STATIC_ASSERT(sizeof(tagAPI) == sizeof(VendorTag));

    VendorTagType vendorTagType = VendorTagManager::GetTagType(static_cast<VendorTag>(tagAPI));
    return static_cast<int>(vendorTagType);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// camera_module_t entry points
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// get_number_of_cameras
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static int get_number_of_cameras(void)
{
    CAMX_ENTRYEXIT_SCOPE(CamxLogGroupHAL, SCOPEEventHAL3GetNumberOfCameras);

    return static_cast<int>(HAL3Module::GetInstance()->GetNumCameras());
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// get_camera_info
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static int get_camera_info(
    int                 cameraIdAPI,
    struct camera_info* pInfoAPI)
{
    CAMX_ENTRYEXIT_SCOPE(CamxLogGroupHAL, SCOPEEventHAL3GetCameraInfo);

    CamxResult result = CamxResultSuccess;

    CAMX_ASSERT(NULL != pInfoAPI);

    if (NULL != pInfoAPI)
    {
        UINT32      cameraId        = static_cast<UINT32>(cameraIdAPI);
        UINT32      logicalCameraId = GetCHIAppCallbacks()->chi_remap_camera_id(cameraId, IdRemapCamera);

        CameraInfo* pInfo       = reinterpret_cast<CameraInfo*>(pInfoAPI);
        result = HAL3Module::GetInstance()->GetCameraInfo(logicalCameraId, pInfo);
        if (CamxResultSuccess != result)
        {
            CAMX_LOG_ERROR(CamxLogGroupHAL, "HAL3Module::GetCameraInfo() failed with result %s",
                           Utils::CamxResultToString(result));
            // HAL interface requires -ENODEV (EFailed) for all other failures
            result = CamxResultEFailed;
        }
    }
    else
    {
        CAMX_LOG_ERROR(CamxLogGroupHAL, "Invalid argument 2 for get_camera_info()");
        // HAL interface requires -EINVAL (EInvalidArg) for invalid arguments
        result = CamxResultEInvalidArg;
    }

    return Utils::CamxResultToErrno(result);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// set_callbacks
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static int set_callbacks(
    const camera_module_callbacks_t* pModuleCbsAPI)
{
    CAMX_ENTRYEXIT_SCOPE(CamxLogGroupHAL, SCOPEEventHAL3SetCallbacks);

    CamxResult result = CamxResultSuccess;

    result = HAL3Module::GetInstance()->SetCbs(pModuleCbsAPI);
    if (CamxResultSuccess != result)
    {
        CAMX_LOG_ERROR(CamxLogGroupHAL, "HAL3Module::SetCbs() failed with result %s", Utils::CamxResultToString(result));
        // HAL interface requires -ENODEV (EFailed) for all other failures
        result = CamxResultEFailed;
    }

    return Utils::CamxResultToErrno(result);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// get_vendor_tag_ops
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static void get_vendor_tag_ops(
    vendor_tag_ops_t* pVendorTagOpsAPI)
{
    CAMX_ENTRYEXIT_SCOPE(CamxLogGroupHAL, SCOPEEventHAL3GetVendorTagOps);

    CAMX_ASSERT(NULL != pVendorTagOpsAPI);

    if (NULL != pVendorTagOpsAPI)
    {
        pVendorTagOpsAPI->get_tag_count     = get_tag_count;
        pVendorTagOpsAPI->get_all_tags      = get_all_tags;
        pVendorTagOpsAPI->get_section_name  = get_section_name;
        pVendorTagOpsAPI->get_tag_name      = get_tag_name;
        pVendorTagOpsAPI->get_tag_type      = get_tag_type;
        pVendorTagOpsAPI->reserved[0]       = NULL;
    }

    /// @todo (CAMX-1223) Remove below and set the vendor tag ops in hal3test
    // This is the workaround for presil HAL3test on Windows
    // On Device, set_camera_metadata_vendor_ops will be call the set the
    // static vendor tag operation in camera_metadata.c
    //
    // On Windows side, theoretically hal3test should mimic what Android framework
    // does and call the set_camera_metadata_vendor_ops function in libcamxext library
    // However, in Windows, if both hal3test.exe and hal.dll link to libcamxext library,
    // there are two different instance of static varibles sit in different memory location.
    // Even if set_camera_metadata_vendor_ops is called in hal3test, when hal try to
    // access to vendor tag ops, it is still not set.
    set_camera_metadata_vendor_ops(pVendorTagOpsAPI);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// open_legacy
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static int open_legacy(
    const struct hw_module_t*   pHwModuleAPI,
    const char*                 pCameraIdAPI,
    uint32_t                    halVersion,
    struct hw_device_t**        ppHwDeviceAPI)
{
    CAMX_UNREFERENCED_PARAM(pHwModuleAPI);
    CAMX_UNREFERENCED_PARAM(pCameraIdAPI);
    CAMX_UNREFERENCED_PARAM(halVersion);
    CAMX_UNREFERENCED_PARAM(ppHwDeviceAPI);

    CAMX_ENTRYEXIT_SCOPE(CamxLogGroupHAL, SCOPEEventHAL3OpenLegacy);

    /// Intentionally do nothing. HAL interface requires -ENOSYS (ENotImplemented) result code.

    return Utils::CamxResultToErrno(CamxResultENotImplemented);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// set_torch_mode
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static int set_torch_mode(
    const char* pCameraIdAPI,
    bool        enabledAPI)
{
    CAMX_ENTRYEXIT_SCOPE(CamxLogGroupHAL, SCOPEEventHAL3SetTorchMode);

    CamxResult result = CamxResultSuccess;

    CAMX_ASSERT(NULL != pCameraIdAPI);
    CAMX_ASSERT('\0' != pCameraIdAPI[0]);

    if ((NULL != pCameraIdAPI) &&
        ('\0' != pCameraIdAPI[0]))
    {
        UINT32  cameraId    = 0;
        CHAR*   pNameEnd    = NULL;
        cameraId = OsUtils::StrToUL(pCameraIdAPI, &pNameEnd, 10);

        if (*pNameEnd != '\0')
        {
            CAMX_LOG_ERROR(CamxLogGroupHAL, "Invalid camera id: %s", pCameraIdAPI);
            // HAL interface requires -EINVAL (EInvalidArg) for invalid arguments
            result = CamxResultEInvalidArg;
        }

        if (CamxResultSuccess == result)
        {
            UINT32 logicalCameraId = GetCHIAppCallbacks()->chi_remap_camera_id(cameraId, IdRemapTorch);
            BOOL enableTorch = (TRUE == enabledAPI) ? TRUE : FALSE;

            result = HAL3Module::GetInstance()->SetTorchMode(logicalCameraId, cameraId, enableTorch);
        }
    }
    else
    {
        CAMX_LOG_ERROR(CamxLogGroupHAL, "Invalid argument 1 for set_torch_mode()");
        // HAL interface requires -EINVAL (EInvalidArg) for invalid arguments
        result = CamxResultEInvalidArg;
    }

    return Utils::CamxResultToErrno(result);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// init
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static int init()
{
    CAMX_ENTRYEXIT_SCOPE(CamxLogGroupHAL, SCOPEEventHAL3Init);

    /// Intentionally do nothing for now. Left as a placeholder for future use.
    /// @todo (CAMX-264) - Determine what, if any, CSL initialization should be done after module load

    return Utils::CamxResultToErrno(CamxResultSuccess);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// hw_device_t entry points
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// close
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static int close(
    struct hw_device_t* pHwDeviceAPI)
{
    CamxResult result = CamxResultSuccess;
    {
        CAMX_ENTRYEXIT_SCOPE(CamxLogGroupHAL, SCOPEEventHAL3Close);

        CAMX_ASSERT(NULL != pHwDeviceAPI);

        CAMX_LOG_INFO(CamxLogGroupHAL, "close(): %p", pHwDeviceAPI);

        if (NULL != pHwDeviceAPI)
        {
            camera3_device_t* pCamera3Device  = reinterpret_cast<camera3_device_t*>(pHwDeviceAPI);
            HALDevice*     pHALDevice      = static_cast<HALDevice*>(pCamera3Device->priv);

            CAMX_LOG_CONFIG(CamxLogGroupHAL, "HalOp: Begin CLOSE, logicalCameraId: %d, cameraId: %d",
                            pHALDevice->GetCameraId(), pHALDevice->GetFwCameraId());

            CAMX_ASSERT(NULL != pHALDevice);

            if (NULL != pHALDevice)
            {
                // Sample code to show how the VOID* can be used in ExtendOpen
                CHIEXTENDSETTINGS extend = { 0 };
                CHISETTINGTOKEN   tokenList[NumExtendSettings] = { { 0 } };
                extend.pTokens = tokenList;

                GenerateExtendCloseData(NumExtendSettings, &extend);

                // Allow the camera to be reopened
                HAL3Module::GetInstance()->ProcessCameraClose(pHALDevice->GetCameraId(), &extend);

                // Sample code to show how the VOID* can be used in ModifySettings
                ChiModifySettings setting[NumExtendSettings] = { { { 0 } } };
                GenerateModifySettingsData(setting);

                for (UINT i = 0; i < NumExtendSettings; i++)
                {
                    GetCHIAppCallbacks()->chi_modify_settings(&setting[i]);
                }
                const StaticSettings* pStaticSettings = HwEnvironment::GetInstance()->GetStaticSettings();
                CAMX_LOG_INFO(CamxLogGroupHAL, "Close: overrideCameraClose is %d , overrideCameraOpen is %d ",
                    pStaticSettings->overrideCameraClose, pStaticSettings->overrideCameraOpen);

                result = pHALDevice->Close();

                CAMX_LOG_VERBOSE(CamxLogGroupHAL, "Close done on cameraId %d", pHALDevice->GetCameraId());
                pHALDevice->CloseCachedSensorHandles(pHALDevice->GetCameraId());

                CAMX_LOG_CONFIG(CamxLogGroupHAL, "HalOp: End CLOSE, logicalCameraId: %d, cameraId: %d",
                                pHALDevice->GetCameraId(), pHALDevice->GetFwCameraId());

                CamX::OfflineLogger* pOfflineLoggerASCII = CamX::OfflineLogger::GetInstance(OfflineLoggerType::ASCII);
                if (NULL != pOfflineLoggerASCII)
                {
                    pOfflineLoggerASCII->NotifyCameraClose();
                }
                CamX::OfflineLogger* pOfflineLoggerBinary = CamX::OfflineLogger::GetInstance(OfflineLoggerType::BINARY);
                if (NULL != pOfflineLoggerBinary)
                {
                    pOfflineLoggerBinary->NotifyCameraClose();
                }

                // Unconditionally destroy the HALSession object
                pHALDevice->Destroy();
                pHALDevice = NULL;
            }
            else
            {
                CAMX_LOG_ERROR(CamxLogGroupHAL, "Invalid argument 1 for close()");
                result = CamxResultEInvalidArg;
            }
        }
        else
        {
            CAMX_LOG_ERROR(CamxLogGroupHAL, "Invalid argument 1 for close()");
            result = CamxResultEInvalidArg;
        }
    }

    return Utils::CamxResultToErrno(result);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// camera3_device_ops_t entry points
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// initialize
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static int initialize(
    const struct camera3_device*    pCamera3DeviceAPI,
    const camera3_callback_ops_t*   pCamera3CbOpsAPI)
{
    CAMX_ENTRYEXIT_SCOPE(CamxLogGroupHAL, SCOPEEventHAL3Initialize);

    CamxResult result = CamxResultSuccess;

    CAMX_ASSERT(NULL != pCamera3DeviceAPI);
    CAMX_ASSERT(NULL != pCamera3DeviceAPI->priv);

    CAMX_LOG_INFO(CamxLogGroupHAL, "initialize(): %p, %p", pCamera3DeviceAPI, pCamera3CbOpsAPI);

    if ((NULL != pCamera3DeviceAPI) &&
        (NULL != pCamera3DeviceAPI->priv))
    {
        HALDevice* pHALDevice = GetHALDevice(pCamera3DeviceAPI);
        pHALDevice->SetCallbackOps(pCamera3CbOpsAPI);

        // initialize thermal after hal callback is set
        const StaticSettings* pStaticSettings = HwEnvironment::GetInstance()->GetStaticSettings();

        if (TRUE == pStaticSettings->enableThermalMitigation)
        {
            ThermalManager* pThermalManager = HAL3Module::GetInstance()->GetThermalManager();
            if (NULL != pThermalManager)
            {
                CamxResult resultThermalReg = pThermalManager->RegisterHALDevice(pHALDevice);
                if (CamxResultEResource == resultThermalReg)
                {
                    result = resultThermalReg;
                }
                // else Ignore result even if it fails. We don't want camera to fail due to any issues with initializing the
                // thermal engine
            }
        }
    }
    else
    {
        CAMX_LOG_ERROR(CamxLogGroupHAL, "Invalid argument(s) for initialize()");
        // HAL interface requires -ENODEV (EFailed) if initialization fails for any reason, including invalid arguments.
        result = CamxResultEFailed;
    }

    return Utils::CamxResultToErrno(result);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// configure_streams
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static int configure_streams(
    const struct camera3_device*    pCamera3DeviceAPI,
    camera3_stream_configuration_t* pStreamConfigsAPI)
{
    CAMX_ENTRYEXIT_SCOPE(CamxLogGroupHAL, SCOPEEventHAL3ConfigureStreams);

    CamxResult result = CamxResultSuccess;

    CAMX_ASSERT(NULL != pCamera3DeviceAPI);
    CAMX_ASSERT(NULL != pCamera3DeviceAPI->priv);
    CAMX_ASSERT(NULL != pStreamConfigsAPI);
    CAMX_ASSERT(pStreamConfigsAPI->num_streams > 0);
    CAMX_ASSERT(NULL != pStreamConfigsAPI->streams);

    if ((NULL != pCamera3DeviceAPI)          &&
        (NULL != pCamera3DeviceAPI->priv)    &&
        (NULL != pStreamConfigsAPI)          &&
        (pStreamConfigsAPI->num_streams > 0) &&
        (NULL != pStreamConfigsAPI->streams))
    {
        CAMX_LOG_INFO(CamxLogGroupHAL, "Number of streams: %d", pStreamConfigsAPI->num_streams);

        HALDevice* pHALDevice = GetHALDevice(pCamera3DeviceAPI);

        CAMX_LOG_CONFIG(CamxLogGroupHAL, "HalOp: Begin CONFIG: %p, logicalCameraId: %d, cameraId: %d",
                        pCamera3DeviceAPI, pHALDevice->GetCameraId(), pHALDevice->GetFwCameraId());

        uint32_t numStreams      = pStreamConfigsAPI->num_streams;
        UINT32   logicalCameraId = pHALDevice->GetCameraId();
        UINT32   cameraId        = pHALDevice->GetFwCameraId();
        BINARY_LOG(LogEvent::HAL3_ConfigSetup, numStreams, logicalCameraId, cameraId);
        for (UINT32 stream = 0; stream < pStreamConfigsAPI->num_streams; stream++)
        {
            CAMX_ASSERT(NULL != pStreamConfigsAPI->streams[stream]);

            if (NULL == pStreamConfigsAPI->streams[stream])
            {
                CAMX_LOG_ERROR(CamxLogGroupHAL, "Invalid argument 2 for configure_streams()");
                // HAL interface requires -EINVAL (EInvalidArg) for invalid arguments
                result = CamxResultEInvalidArg;
                break;
            }
            else
            {
                camera3_stream_t& rConfigStream = *pStreamConfigsAPI->streams[stream];
                BINARY_LOG(LogEvent::HAL3_StreamInfo, rConfigStream);

                CAMX_LOG_INFO(CamxLogGroupHAL, "  stream[%d] = %p - info:", stream,
                    pStreamConfigsAPI->streams[stream]);
                CAMX_LOG_INFO(CamxLogGroupHAL, "            format       : %d, %s",
                    pStreamConfigsAPI->streams[stream]->format,
                    FormatToString(pStreamConfigsAPI->streams[stream]->format));
                CAMX_LOG_INFO(CamxLogGroupHAL, "            width        : %d",
                    pStreamConfigsAPI->streams[stream]->width);
                CAMX_LOG_INFO(CamxLogGroupHAL, "            height       : %d",
                    pStreamConfigsAPI->streams[stream]->height);
                CAMX_LOG_INFO(CamxLogGroupHAL, "            stream_type  : %08x, %s",
                    pStreamConfigsAPI->streams[stream]->stream_type,
                    StreamTypeToString(pStreamConfigsAPI->streams[stream]->stream_type));
                CAMX_LOG_INFO(CamxLogGroupHAL, "            usage        : %08x",
                    pStreamConfigsAPI->streams[stream]->usage);
                CAMX_LOG_INFO(CamxLogGroupHAL, "            max_buffers  : %d",
                    pStreamConfigsAPI->streams[stream]->max_buffers);
                CAMX_LOG_INFO(CamxLogGroupHAL, "            rotation     : %08x, %s",
                    pStreamConfigsAPI->streams[stream]->rotation,
                    RotationToString(pStreamConfigsAPI->streams[stream]->rotation));
                CAMX_LOG_INFO(CamxLogGroupHAL, "            data_space   : %08x, %s",
                    pStreamConfigsAPI->streams[stream]->data_space,
                    DataSpaceToString(pStreamConfigsAPI->streams[stream]->data_space));
                CAMX_LOG_INFO(CamxLogGroupHAL, "            priv         : %p",
                    pStreamConfigsAPI->streams[stream]->priv);
#if (defined(CAMX_ANDROID_API) && (CAMX_ANDROID_API >= 28)) // Android-P or better
                CAMX_LOG_INFO(CamxLogGroupHAL, "            physical_camera_id         : %s",
                    pStreamConfigsAPI->streams[stream]->physical_camera_id);
#endif // Android-P or better
                pStreamConfigsAPI->streams[stream]->reserved[0] = NULL;
                pStreamConfigsAPI->streams[stream]->reserved[1] = NULL;
            }
        }
        CAMX_LOG_INFO(CamxLogGroupHAL, "  operation_mode: %d", pStreamConfigsAPI->operation_mode);


        Camera3StreamConfig* pStreamConfigs = reinterpret_cast<Camera3StreamConfig*>(pStreamConfigsAPI);

        result = pHALDevice->ConfigureStreams(pStreamConfigs);

        if ((CamxResultSuccess != result) && (CamxResultEInvalidArg != result))
        {
            // HAL interface requires -ENODEV (EFailed) if a fatal error occurs
            result = CamxResultEFailed;
        }

        if (CamxResultSuccess == result)
        {
            for (UINT32 stream = 0; stream < pStreamConfigsAPI->num_streams; stream++)
            {
                CAMX_ASSERT(NULL != pStreamConfigsAPI->streams[stream]);

                if (NULL == pStreamConfigsAPI->streams[stream])
                {
                    CAMX_LOG_ERROR(CamxLogGroupHAL, "Invalid argument 2 for configure_streams()");
                    // HAL interface requires -EINVAL (EInvalidArg) for invalid arguments
                    result = CamxResultEInvalidArg;
                    break;
                }
                else
                {
                    CAMX_LOG_CONFIG(CamxLogGroupHAL, " FINAL stream[%d] = %p - info:", stream,
                        pStreamConfigsAPI->streams[stream]);
                    CAMX_LOG_CONFIG(CamxLogGroupHAL, "            format       : %d, %s",
                        pStreamConfigsAPI->streams[stream]->format,
                        FormatToString(pStreamConfigsAPI->streams[stream]->format));
                    CAMX_LOG_CONFIG(CamxLogGroupHAL, "            width        : %d",
                        pStreamConfigsAPI->streams[stream]->width);
                    CAMX_LOG_CONFIG(CamxLogGroupHAL, "            height       : %d",
                        pStreamConfigsAPI->streams[stream]->height);
                    CAMX_LOG_CONFIG(CamxLogGroupHAL, "            stream_type  : %08x, %s",
                        pStreamConfigsAPI->streams[stream]->stream_type,
                        StreamTypeToString(pStreamConfigsAPI->streams[stream]->stream_type));
                    CAMX_LOG_CONFIG(CamxLogGroupHAL, "            usage        : %08x",
                        pStreamConfigsAPI->streams[stream]->usage);
                    CAMX_LOG_CONFIG(CamxLogGroupHAL, "            max_buffers  : %d",
                        pStreamConfigsAPI->streams[stream]->max_buffers);
                    CAMX_LOG_CONFIG(CamxLogGroupHAL, "            rotation     : %08x, %s",
                        pStreamConfigsAPI->streams[stream]->rotation,
                        RotationToString(pStreamConfigsAPI->streams[stream]->rotation));
                    CAMX_LOG_CONFIG(CamxLogGroupHAL, "            data_space   : %08x, %s",
                        pStreamConfigsAPI->streams[stream]->data_space,
                        DataSpaceToString(pStreamConfigsAPI->streams[stream]->data_space));
                    CAMX_LOG_CONFIG(CamxLogGroupHAL, "            priv         : %p",
                        pStreamConfigsAPI->streams[stream]->priv);
                    CAMX_LOG_CONFIG(CamxLogGroupHAL, "            reserved[0]         : %p",
                        pStreamConfigsAPI->streams[stream]->reserved[0]);
                    CAMX_LOG_CONFIG(CamxLogGroupHAL, "            reserved[1]         : %p",
                        pStreamConfigsAPI->streams[stream]->reserved[1]);

                    Camera3HalStream* pHalStream =
                        reinterpret_cast<Camera3HalStream*>(pStreamConfigsAPI->streams[stream]->reserved[0]);
                    if (pHalStream != NULL)
                    {
                        if (TRUE == HwEnvironment::GetInstance()->GetStaticSettings()->enableHALFormatOverride)
                        {
                            pStreamConfigsAPI->streams[stream]->format =
                                static_cast<HALPixelFormat>(pHalStream->overrideFormat);
                        }
                        CAMX_LOG_CONFIG(CamxLogGroupHAL,
                            "   pHalStream: %p format : 0x%x, overrideFormat : 0x%x consumer usage: %llx,"
                            " producer usage: %llx",
                            pHalStream, pStreamConfigsAPI->streams[stream]->format,
                            pHalStream->overrideFormat, pHalStream->consumerUsage, pHalStream->producerUsage);
                    }
                }
            }
        }
        CAMX_LOG_CONFIG(CamxLogGroupHAL, "HalOp: End CONFIG, logicalCameraId: %d, cameraId: %d",
                        pHALDevice->GetCameraId(), pHALDevice->GetFwCameraId());
    }
    else
    {
        CAMX_LOG_ERROR(CamxLogGroupHAL, "Invalid argument(s) for configure_streams()");
        // HAL interface requires -EINVAL (EInvalidArg) for invalid arguments
        result = CamxResultEInvalidArg;
    }

    return Utils::CamxResultToErrno(result);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// construct_default_request_settings
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static const camera_metadata_t* construct_default_request_settings(
    const struct camera3_device*    pCamera3DeviceAPI,
    int                             requestTemplateAPI)
{
    CAMX_ENTRYEXIT_SCOPE(CamxLogGroupHAL, SCOPEEventHAL3ConstructDefaultRequestSettings);

    const Metadata* pMetadata = NULL;

    CAMX_ASSERT(NULL != pCamera3DeviceAPI);
    CAMX_ASSERT(NULL != pCamera3DeviceAPI->priv);

    if ((NULL != pCamera3DeviceAPI) && (NULL != pCamera3DeviceAPI->priv))
    {
        HALDevice*             pHALDevice        = GetHALDevice(pCamera3DeviceAPI);
        Camera3RequestTemplate requestTemplate   = static_cast<Camera3RequestTemplate>(requestTemplateAPI);

        pMetadata = pHALDevice->ConstructDefaultRequestSettings(requestTemplate);
    }
    else
    {
        CAMX_LOG_ERROR(CamxLogGroupHAL, "Invalid argument(s) for initialize()");
    }

    return reinterpret_cast<const camera_metadata_t*>(pMetadata);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// process_capture_request
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static int process_capture_request(
    const struct camera3_device*    pCamera3DeviceAPI,
    camera3_capture_request_t*      pCaptureRequestAPI)
{
    UINT64 frameworkFrameNum = 0;

    if (NULL != pCaptureRequestAPI)
    {
        frameworkFrameNum = pCaptureRequestAPI->frame_number;
    }

    CAMX_ENTRYEXIT_SCOPE_ID(CamxLogGroupHAL, SCOPEEventHAL3ProcessCaptureRequest, frameworkFrameNum);

    CAMX_TRACE_ASYNC_BEGIN_F(CamxLogGroupHAL, frameworkFrameNum, "HAL3: RequestTrace");

    CamxResult result = CamxResultSuccess;

    CAMX_ASSERT(NULL != pCamera3DeviceAPI);
    CAMX_ASSERT(NULL != pCamera3DeviceAPI->priv);
    CAMX_ASSERT(NULL != pCaptureRequestAPI);
    CAMX_ASSERT(pCaptureRequestAPI->num_output_buffers > 0);
    CAMX_ASSERT(NULL != pCaptureRequestAPI->output_buffers);

    if ((NULL != pCamera3DeviceAPI)                  &&
        (NULL != pCamera3DeviceAPI->priv)            &&
        (NULL != pCaptureRequestAPI)                 &&
        (pCaptureRequestAPI->num_output_buffers > 0) &&
        (NULL != pCaptureRequestAPI->output_buffers))
    {
        /// @todo (CAMX-337): Go deeper into camera3_capture_request_t struct for validation

        HALDevice*                 pHALDevice      = GetHALDevice(pCamera3DeviceAPI);
        Camera3CaptureRequest*     pRequest        = reinterpret_cast<Camera3CaptureRequest*>(pCaptureRequestAPI);
        camera3_capture_request_t& rCaptureRequest = *pCaptureRequestAPI;
        BINARY_LOG(LogEvent::HAL3_ProcessCaptureRequest, rCaptureRequest);

        CAMX_LOG_CONFIG(CamxLogGroupHAL, "frame_number %d, settings %p, num_output_buffers %d",
                      pCaptureRequestAPI->frame_number,
                      pCaptureRequestAPI->settings,
                      pCaptureRequestAPI->num_output_buffers);

        uint32_t frame_number      = rCaptureRequest.frame_number;
        BOOL     isCaptureBuffer   = FALSE;
        BOOL     isReprocessBuffer = FALSE;
        if (NULL != pCaptureRequestAPI->output_buffers)
        {
            for (UINT i = 0; i < pCaptureRequestAPI->num_output_buffers; i++)
            {
                const camera3_stream_buffer_t& rBuffer = pCaptureRequestAPI->output_buffers[i];
                isCaptureBuffer                        = TRUE;
                isReprocessBuffer                      = FALSE;
                BINARY_LOG(LogEvent::HAL3_BufferInfo, frame_number, rBuffer, isCaptureBuffer, isReprocessBuffer);
                CAMX_LOG_CONFIG(CamxLogGroupHAL, "    output_buffers[%d] : %p, buffer: %p, status: %08x, stream: %p",
                              i,
                              &pCaptureRequestAPI->output_buffers[i],
                              pCaptureRequestAPI->output_buffers[i].buffer,
                              pCaptureRequestAPI->output_buffers[i].status,
                              pCaptureRequestAPI->output_buffers[i].stream);
                if (HAL_PIXEL_FORMAT_BLOB == pCaptureRequestAPI->output_buffers[i].stream->format)
                {
                    CAMX_TRACE_ASYNC_BEGIN_F(CamxLogGroupHAL, pCaptureRequestAPI->frame_number, "SNAPSHOT frameID: %d",
                        pCaptureRequestAPI->frame_number);
                    CAMX_TRACE_ASYNC_BEGIN_F(CamxLogGroupHAL, pCaptureRequestAPI->frame_number, "SHUTTERLAG frameID: %d",
                        pCaptureRequestAPI->frame_number);
                }
            }
        }
        if (NULL != pCaptureRequestAPI->input_buffer)
        {

            const camera3_stream_buffer_t& rBuffer = *pCaptureRequestAPI->input_buffer;
            isCaptureBuffer                        = FALSE;
            isReprocessBuffer                      = TRUE;
            BINARY_LOG(LogEvent::HAL3_BufferInfo, frame_number, rBuffer, isCaptureBuffer, isReprocessBuffer);
            CAMX_LOG_CONFIG(CamxLogGroupHAL, "    input_buffer %p, buffer: %p, status: %08x, stream: %p",
                          pCaptureRequestAPI->input_buffer,
                          pCaptureRequestAPI->input_buffer->buffer,
                          pCaptureRequestAPI->input_buffer->status,
                          pCaptureRequestAPI->input_buffer->stream);
        }


        if (CAMX_IS_TRACE_ENABLED(CamxLogGroupCore))
        {
            pHALDevice->TraceZoom(pCaptureRequestAPI);
        }

        result = pHALDevice->ProcessCaptureRequest(pRequest);

        if ((CamxResultSuccess != result) && (CamxResultEInvalidArg != result))
        {
            // HAL interface requires -ENODEV (EFailed) if a fatal error occurs
            result = CamxResultEFailed;
        }
    }
    else
    {
        CAMX_LOG_ERROR(CamxLogGroupHAL, "Invalid argument(s) for process_capture_request()");
        // HAL interface requires -EINVAL (EInvalidArg) for invalid arguments
        result = CamxResultEInvalidArg;
    }

    return Utils::CamxResultToErrno(result);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// dump
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static void dump(
    const struct camera3_device*    pCamera3DeviceAPI,
    int                             fdAPI)
{
    CAMX_ENTRYEXIT_SCOPE(CamxLogGroupHAL, SCOPEEventHAL3Dump);

    if ((NULL != pCamera3DeviceAPI) &&
        (NULL != pCamera3DeviceAPI->priv))
    {
        HALDevice* pHALDevice = GetHALDevice(pCamera3DeviceAPI);

        CAMX_LOG_TO_FILE(fdAPI, 0, "###############  CameraId: %d Dump Start  ###############", pHALDevice->GetCameraId());

        HAL3Module::GetInstance()->Dump(fdAPI);
        pHALDevice->Dump(fdAPI);

        CAMX_LOG_TO_FILE(fdAPI, 0, "###############   CameraId: %d Dump End   ###############", pHALDevice->GetCameraId());
    }
    else
    {
        CAMX_LOG_TO_FILE(fdAPI, 2, "Invalid camera3_device pointer, cannot dump info");
    }

}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// flush
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static int flush(
    const struct camera3_device* pCamera3DeviceAPI)
{
    CAMX_ENTRYEXIT_SCOPE(CamxLogGroupHAL, SCOPEEventHAL3Flush);

    CamxResult result = CamxResultSuccess;

    CAMX_ASSERT(NULL != pCamera3DeviceAPI);
    CAMX_ASSERT(NULL != pCamera3DeviceAPI->priv);

    if ((NULL != pCamera3DeviceAPI) &&
        (NULL != pCamera3DeviceAPI->priv))
    {
        HALDevice* pHALDevice = GetHALDevice(pCamera3DeviceAPI);
        BOOL       isDone     = FALSE;
        BINARY_LOG(LogEvent::HAL3_FlushInfo, isDone);
        CAMX_LOG_CONFIG(CamxLogGroupHAL, "HalOp: Begin FLUSH: %p, logicalCameraId: %d, cameraId: %d",
                        pCamera3DeviceAPI, pHALDevice->GetCameraId(), pHALDevice->GetFwCameraId());

        result = pHALDevice->Flush();

        if ((CamxResultSuccess != result) && (CamxResultEInvalidArg != result))
        {
            // HAL interface requires -ENODEV (EFailed) if flush fails
            result = CamxResultEFailed;
        }
        isDone = TRUE;
        BINARY_LOG(LogEvent::HAL3_FlushInfo, isDone);
        CAMX_LOG_CONFIG(CamxLogGroupHAL, "HalOp: End FLUSH: %p with result %d, logicalCameraId: %d, cameraId: %d",
                        pCamera3DeviceAPI, result, pHALDevice->GetCameraId(), pHALDevice->GetFwCameraId());
    }
    else
    {
        CAMX_LOG_ERROR(CamxLogGroupHAL, "Invalid argument 1 for flush()");
        // HAL interface requires -EINVAL (EInvalidArg) if arguments are invalid
        result = CamxResultEInvalidArg;
    }

    return Utils::CamxResultToErrno(result);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// camera_module_callbacks_t exit points
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// camera_device_status_change
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static void camera_device_status_change(
    const struct camera_module_callbacks*   pModuleCbsAPI,
    int                                     cameraIdAPI,
    int                                     newStatusAPI)
{
    CAMX_ENTRYEXIT_SCOPE(CamxLogGroupHAL, SCOPEEventHAL3CameraDeviceStatusChange);

    pModuleCbsAPI->camera_device_status_change(pModuleCbsAPI, cameraIdAPI, newStatusAPI);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// torch_mode_status_change
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static void torch_mode_status_change(
    const struct camera_module_callbacks*   pModuleCbsAPI,
    const char*                             pCameraIdAPI,
    int                                     newStatusAPI)
{
    CAMX_ENTRYEXIT_SCOPE(CamxLogGroupHAL, SCOPEEventHAL3TorchModeStatusChange);

    if (NULL != pModuleCbsAPI)
    {
        pModuleCbsAPI->torch_mode_status_change(pModuleCbsAPI, pCameraIdAPI, newStatusAPI);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// camera3_callback_ops_t exit points
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// process_capture_result
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static void process_capture_result(
    const struct camera3_callback_ops*  pCamera3CbOpsAPI,
    const camera3_capture_result_t*     pCaptureResultAPI)
{
    UINT64 frameworkFrameNum = 0;

    if (NULL != pCaptureResultAPI)
    {

        const camera3_capture_result_t& rCaptureResult    = *pCaptureResultAPI;
        BOOL                            isCaptureBuffer    = FALSE;
        BOOL                            isReprocessBuffer  = FALSE;
        BINARY_LOG(LogEvent::HAL3_ProcessCaptureResult, rCaptureResult);

        frameworkFrameNum = pCaptureResultAPI->frame_number;

#if (defined(CAMX_ANDROID_API) && (CAMX_ANDROID_API >= 28)) // Android-P or better
        CAMX_LOG_CONFIG(CamxLogGroupHAL, "frame_number %d, partial_result %d, result %p, "
                      "num_physcam_metadata %d, num_output_buffers %d",
                      pCaptureResultAPI->frame_number,
                      pCaptureResultAPI->partial_result,
                      pCaptureResultAPI->result,
                      pCaptureResultAPI->num_physcam_metadata,
                      pCaptureResultAPI->num_output_buffers);
#else
        CAMX_LOG_CONFIG(CamxLogGroupHAL, "frame_number %d, partial_result %d, result %p, num_output_buffers %d",
                      pCaptureResultAPI->frame_number,
                      pCaptureResultAPI->partial_result,
                      pCaptureResultAPI->result,
                      pCaptureResultAPI->num_output_buffers);

#endif // Android-P or better

        uint32_t frame_number = rCaptureResult.frame_number;
        if (NULL != pCaptureResultAPI->output_buffers)
        {
            for (UINT i = 0; i < pCaptureResultAPI->num_output_buffers; i++)
            {
                CAMX_LOG_CONFIG(CamxLogGroupHAL, "    output_buffers[%d] : %p, buffer: %p, status: %08x, stream: %p",
                              i,
                              &pCaptureResultAPI->output_buffers[i],
                              pCaptureResultAPI->output_buffers[i].buffer,
                              pCaptureResultAPI->output_buffers[i].status,
                              pCaptureResultAPI->output_buffers[i].stream);

                if (HAL_PIXEL_FORMAT_BLOB == pCaptureResultAPI->output_buffers[i].stream->format)
                {
                    CAMX_TRACE_ASYNC_END_F(CamxLogGroupHAL, pCaptureResultAPI->frame_number, "SNAPSHOT frameID: %d",
                        pCaptureResultAPI->frame_number);
                }
                const camera3_stream_buffer_t& rBuffer = pCaptureResultAPI->output_buffers[i];
                isCaptureBuffer                        = TRUE;
                isReprocessBuffer                      = FALSE;
                BINARY_LOG(LogEvent::HAL3_BufferInfo, frame_number, rBuffer, isCaptureBuffer, isReprocessBuffer);
            }
        }

        if (NULL != pCaptureResultAPI->input_buffer)
        {
            CAMX_LOG_CONFIG(CamxLogGroupHAL, "    input_buffer %p, buffer: %p, status: %08x, stream: %p",
                          pCaptureResultAPI->input_buffer,
                          pCaptureResultAPI->input_buffer->buffer,
                          pCaptureResultAPI->input_buffer->status,
                          pCaptureResultAPI->input_buffer->stream);

            const camera3_stream_buffer_t& rBuffer = *pCaptureResultAPI->input_buffer;
            isCaptureBuffer                        = FALSE;
            isReprocessBuffer                      = TRUE;
            BINARY_LOG(LogEvent::HAL3_BufferInfo, frame_number, rBuffer, isCaptureBuffer, isReprocessBuffer);
        }
    }

    CAMX_ENTRYEXIT_SCOPE_ID(CamxLogGroupHAL, SCOPEEventHAL3ProcessCaptureResult, frameworkFrameNum);

    pCamera3CbOpsAPI->process_capture_result(pCamera3CbOpsAPI, pCaptureResultAPI);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// notify
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static void notify(
    const struct camera3_callback_ops*  pCamera3CbOpsAPI,
    const camera3_notify_msg_t*         pNotifyMessageAPI)
{
    BOOL bNeedNotify = TRUE;
    if (NULL != pNotifyMessageAPI)
    {
        const camera3_notify_msg_t& rNotifyMessage = *pNotifyMessageAPI;
        if (FALSE == (CAMERA3_MSG_ERROR == rNotifyMessage.type || CAMERA3_MSG_SHUTTER == rNotifyMessage.type))
        {
            bNeedNotify = FALSE;
        }
        BINARY_LOG(LogEvent::HAL3_Notify, rNotifyMessage);
        CAMX_TRACE_SYNC_BEGIN_F(CamxLogGroupHAL, "HAL3 Notify Type: %u", rNotifyMessage.type);
    }

    if (TRUE == bNeedNotify)
    {
        pCamera3CbOpsAPI->notify(pCamera3CbOpsAPI, pNotifyMessageAPI);
    }
    CAMX_TRACE_SYNC_END(CamxLogGroupHAL);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Jump table for HAL3
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
JumpTableHAL3 g_jumpTableHAL3 =
{
    open,
    get_number_of_cameras,
    get_camera_info,
    set_callbacks,
    get_vendor_tag_ops,
    open_legacy,
    set_torch_mode,
    init,
    get_tag_count,
    get_all_tags,
    get_section_name,
    get_tag_name,
    get_tag_type,
    close,
    initialize,
    configure_streams,
    construct_default_request_settings,
    process_capture_request,
    dump,
    flush,
    camera_device_status_change,
    torch_mode_status_change,
    process_capture_result,
    notify
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// GenerateExtendOpenData
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void GenerateExtendOpenData(
    UINT32              numTokens,
    CHIEXTENDSETTINGS*  pExtend)
{
    pExtend->numTokens         = numTokens;
    FillTokenList(pExtend->pTokens);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// GenerateExtendCloseData
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void GenerateExtendCloseData(
    UINT32             numTokens,
    CHIEXTENDSETTINGS* pExtend)
{
    pExtend->numTokens = numTokens;
    FillTokenList(pExtend->pTokens);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// GenerateModifySettingsData
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void GenerateModifySettingsData(
    ChiModifySettings* pSettings)
{
    const StaticSettings* pStaticSettings = HwEnvironment::GetInstance()->GetStaticSettings();

    // decltype()  returns the declaration type, the right hand side of the assignment in the code below is to let us use a
    // const cast in the macro, otherwise whiner whines and NOWHINEs are being ignored in the macro.
#define ADD_SETTING(inToken, data)                                                    \
    pSettings[static_cast<UINT>(inToken)].pData       = (decltype(data)*)(&(data));   \
    pSettings[static_cast<UINT>(inToken)].token.id    = static_cast<UINT32>(inToken); \
    pSettings[static_cast<UINT>(inToken)].token.size  = sizeof(data);

#define ADD_BIT_SETTING(inToken, data)                                                \
    pSettings[static_cast<UINT>(inToken)].pData       = (VOID*)(data == TRUE);        \
    pSettings[static_cast<UINT>(inToken)].token.id    = static_cast<UINT32>(inToken); \
    pSettings[static_cast<UINT>(inToken)].token.size  = sizeof(VOID*);


    ADD_SETTING(ChxSettingsToken::OverrideForceUsecaseId,          pStaticSettings->overrideForceUsecaseId);
    ADD_SETTING(ChxSettingsToken::OverrideDisableZSL,              pStaticSettings->overrideDisableZSL);
    ADD_SETTING(ChxSettingsToken::OverrideGPURotationUsecase,      pStaticSettings->overrideGPURotationUsecase);
    ADD_SETTING(ChxSettingsToken::OverrideEnableMFNR,              pStaticSettings->overrideEnableMFNR);
    ADD_SETTING(ChxSettingsToken::AnchorSelectionAlgoForMFNR,      pStaticSettings->anchorSelectionAlgoForMFNR);
    ADD_SETTING(ChxSettingsToken::OverrideHFRNo3AUseCase,          pStaticSettings->overrideHFRNo3AUseCase);
    ADD_SETTING(ChxSettingsToken::OverrideForceSensorMode,         pStaticSettings->overrideForceSensorMode);
    ADD_SETTING(ChxSettingsToken::DefaultMaxFPS,                   pStaticSettings->defaultMaxFPS);
    ADD_SETTING(ChxSettingsToken::FovcEnable,                      pStaticSettings->fovcEnable);
    ADD_SETTING(ChxSettingsToken::OverrideCameraClose,             pStaticSettings->overrideCameraClose);
    ADD_SETTING(ChxSettingsToken::OverrideCameraOpen,              pStaticSettings->overrideCameraOpen);
    ADD_SETTING(ChxSettingsToken::EISV2Enable,                     pStaticSettings->EISV2Enable);
    ADD_SETTING(ChxSettingsToken::EISV3Enable,                     pStaticSettings->EISV3Enable);
    ADD_SETTING(ChxSettingsToken::NumPCRsBeforeStreamOn,           pStaticSettings->numPCRsBeforeStreamOn);
    ADD_SETTING(ChxSettingsToken::StatsProcessingSkipFactor,       pStaticSettings->statsProcessingSkipFactor);
    ADD_SETTING(ChxSettingsToken::DumpDebugDataEveryProcessResult, pStaticSettings->dumpDebugDataEveryProcessResult);
    ADD_BIT_SETTING(ChxSettingsToken::EnableConcise3ADebugData,    pStaticSettings->enableConcise3ADebugData);
    ADD_BIT_SETTING(ChxSettingsToken::Enable3ADebugData,           pStaticSettings->enable3ADebugData);
    ADD_BIT_SETTING(ChxSettingsToken::EnableTuningMetadata,        pStaticSettings->enableTuningMetadata);
    ADD_SETTING(ChxSettingsToken::DebugDataSizeAEC,                pStaticSettings->debugDataSizeAEC);
    ADD_SETTING(ChxSettingsToken::DebugDataSizeAWB,                pStaticSettings->debugDataSizeAWB);
    ADD_SETTING(ChxSettingsToken::DebugDataSizeAF,                 pStaticSettings->debugDataSizeAF);
    ADD_SETTING(ChxSettingsToken::ConciseDebugDataSizeAEC,         pStaticSettings->conciseDebugDataSizeAEC);
    ADD_SETTING(ChxSettingsToken::ConciseDebugDataSizeAWB,         pStaticSettings->conciseDebugDataSizeAWB);
    ADD_SETTING(ChxSettingsToken::ConciseDebugDataSizeAF,          pStaticSettings->conciseDebugDataSizeAF);
    ADD_SETTING(ChxSettingsToken::TuningDumpDataSizeIFE,           pStaticSettings->tuningDumpDataSizeIFE);
    ADD_SETTING(ChxSettingsToken::TuningDumpDataSizeIPE,           pStaticSettings->tuningDumpDataSizeIPE);
    ADD_SETTING(ChxSettingsToken::TuningDumpDataSizeBPS,           pStaticSettings->tuningDumpDataSizeBPS);
    ADD_SETTING(ChxSettingsToken::MultiCameraVREnable,             pStaticSettings->multiCameraVREnable);
    ADD_SETTING(ChxSettingsToken::OverrideGPUDownscaleUsecase,     pStaticSettings->overrideGPUDownscaleUsecase);
    ADD_SETTING(ChxSettingsToken::AdvanceFeatureMask,              pStaticSettings->advanceFeatureMask);
    ADD_SETTING(ChxSettingsToken::DisableASDStatsProcessing,       pStaticSettings->disableASDStatsProcessing);
    ADD_SETTING(ChxSettingsToken::MultiCameraFrameSync,            pStaticSettings->multiCameraFrameSync);
    ADD_SETTING(ChxSettingsToken::OutputFormat,                    pStaticSettings->outputFormat);
    ADD_SETTING(ChxSettingsToken::EnableCHIPartialData,            pStaticSettings->enableCHIPartialData);
    ADD_SETTING(ChxSettingsToken::EnableFDStreamInRealTime,        pStaticSettings->enableFDStreamInRealTime);
    ADD_SETTING(ChxSettingsToken::SelectInSensorHDR3ExpUsecase,    pStaticSettings->selectInSensorHDR3ExpUsecase);
    ADD_SETTING(ChxSettingsToken::EnableUnifiedBufferManager,      pStaticSettings->enableUnifiedBufferManager);
    ADD_SETTING(ChxSettingsToken::EnableCHIImageBufferLateBinding, pStaticSettings->enableCHIImageBufferLateBinding);
    ADD_SETTING(ChxSettingsToken::EnableCHIPartialDataRecovery,    pStaticSettings->enableCHIPartialDataRecovery);
    ADD_SETTING(ChxSettingsToken::UseFeatureForQCFA,               pStaticSettings->useFeatureForQCFA);
    ADD_SETTING(ChxSettingsToken::AECGainThresholdForQCFA,         pStaticSettings->AECGainThresholdForQCFA);
    ADD_SETTING(ChxSettingsToken::EnableOfflineNoiseReprocess,     pStaticSettings->enableOfflineNoiseReprocess);
    ADD_SETTING(ChxSettingsToken::EnableAsciilog,                  pStaticSettings->enableAsciiLogging);
    ADD_SETTING(ChxSettingsToken::EnableBinarylog,                 pStaticSettings->enableBinaryLogging);
    ADD_SETTING(ChxSettingsToken::OverrideLogLevels,               pStaticSettings->overrideLogLevels);
    ADD_SETTING(ChxSettingsToken::EnableFeature2Dump,              pStaticSettings->enableFeature2Dump);
    ADD_SETTING(ChxSettingsToken::ForceHWMFFixedNumOfFrames,       pStaticSettings->forceHWMFFixedNumOfFrames);
    ADD_SETTING(ChxSettingsToken::ForceSWMFFixedNumOfFrames,       pStaticSettings->forceSWMFFixedNumOfFrames);
    ADD_SETTING(ChxSettingsToken::EnableTBMChiFence,               pStaticSettings->enableTBMChiFence);
    ADD_SETTING(ChxSettingsToken::EnableRawHDR,                    pStaticSettings->enableRawHDR);

    ADD_BIT_SETTING(ChxSettingsToken::EnableRequestMapping,        pStaticSettings->logRequestMapping);
    ADD_BIT_SETTING(ChxSettingsToken::EnableSystemLogging,         pStaticSettings->systemLogEnable);

    ADD_SETTING(ChxSettingsToken::BPSRealtimeSensorId,             pStaticSettings->bpsRealtimeSensorId);
    ADD_SETTING(ChxSettingsToken::EnableMFSRChiFence,              pStaticSettings->enableMFSRChiFence);
    ADD_SETTING(ChxSettingsToken::MultiCameraJPEG,                 pStaticSettings->multiCameraJPEG);
    ADD_SETTING(ChxSettingsToken::MaxHALRequests,                  pStaticSettings->maxHalRequests);
    ADD_SETTING(ChxSettingsToken::MultiCameraHWSyncMask,           pStaticSettings->multiCameraHWSyncMask);
    ADD_SETTING(ChxSettingsToken::AnchorAlgoSelectionType,         pStaticSettings->anchorAlgoSelectionType);
    ADD_SETTING(ChxSettingsToken::EnableBLMClient,                 pStaticSettings->enableBLMClient);
    ADD_SETTING(ChxSettingsToken::OverrideForceBurstShot,          pStaticSettings->overrideForceBurstShot);
    ADD_BIT_SETTING(ChxSettingsToken::ExposeFullSizeForQCFA,       pStaticSettings->exposeFullSizeForQCFA);
    ADD_BIT_SETTING(ChxSettingsToken::EnableScreenGrab,            pStaticSettings->enableScreenGrab);
#undef ADD_SETTING
#undef ADD_BIT_SETTING

}

CAMX_NAMESPACE_END
