////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2016-2020 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  camxhal3module.cpp
/// @brief Definitions for HAL3Module class. The purpose of the HAL3Device class is to abstract camera_module_t methods. For
///        further information on the corresponding methods in the HAL3 API, please refer to hardware/hardware.h and
///        hardware/camera3.h.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "camxchicontext.h"
#include "camxcsl.h"
#include "camxdisplayconfig.h"
#include "camxentry.h"
#include "camxhal3metadatautil.h"
#include "camxhaldevice.h"
#include "camxhal3module.h"
#include "camxhal3types.h"
#include "camxhaldevice.h"
#include "camxhwcontext.h"
#include "camxincs.h"
#include "camxutils.h"
#include "chi.h"
#include "chioverride.h"
#include "g_camxversion.h"
#include "camxhal3defaultrequest.h"

CAMX_NAMESPACE_BEGIN

// Number of settings tokens
static const UINT32 NumExtendSettings = static_cast<UINT32>(ChxSettingsToken::CHX_SETTINGS_TOKEN_COUNT);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Macros
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// These macros match the definitions found in hardware/camera_common.h and hardware/hardware.h
#define CAMERA_DEVICE_HALAPI_VERSION(major, minor) ((((major) & 0xFF) << 8) | ((minor) & 0xFF))
#define CAMERA_DEVICE_HALAPI_VERSION_3_3           CAMERA_DEVICE_HALAPI_VERSION(3, 3)


typedef VOID(*CHIHALOverrideEntry)(
    chi_hal_callback_ops_t* pCHIAppCallbacks);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// HAL3Module::GetInstance
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
HAL3Module* HAL3Module::GetInstance()
{
    static HAL3Module s_HAL3ModuleSingleton;
    return &s_HAL3ModuleSingleton;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// HAL3Module::SetCbs
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult HAL3Module::SetCbs(
    const camera_module_callbacks_t* pModuleCbs)
{
    CamxResult result = CamxResultSuccess;

    m_pModuleCbs    = pModuleCbs;
    m_dropCallbacks = FALSE;

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// HAL3Module::SetDropCallbacks
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID HAL3Module::SetDropCallbacks()
{
    CAMX_LOG_ERROR(CamxLogGroupHAL, "We are dropping the callbacks due to error conditions!");
    m_dropCallbacks = TRUE;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// HAL3Module::SetTorchMode
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult HAL3Module::SetTorchMode(
    UINT32  logicalCameraId,
    UINT32  fwNotificationId,
    BOOL    enableTorch)
{
    CamxResult result = CamxResultSuccess;

    if (m_numLogicalCameras <= logicalCameraId)
    {
        // CamxResultEInvalidArg : Camera ID is invalid
        CAMX_LOG_ERROR(CamxLogGroupHAL, "Invalid camera id: %d", logicalCameraId);
        result = CamxResultEInvalidArg;
    }
    // CamxResultENotImplemented : Camera ID does not have a torch
    /// @todo (CAMX-265) Make sure flash exists for this cameraId
    else if (TorchModeStatusNotAvailable == m_torchStatus[logicalCameraId])
    {
        // CamxResultEBusy : The camera ID was already reserved
        CAMX_LOG_ERROR(CamxLogGroupHAL, "Torch not available for camera id: %d", logicalCameraId);
        result = CamxResultEBusy;
    }
    else
    {
        TorchModeStatus torchStatus = (TRUE == enableTorch) ? TorchModeStatusAvailableOn : TorchModeStatusAvailableOff;
        HAL3Module::GetInstance()->SetTorchModeInternal(logicalCameraId, fwNotificationId, torchStatus, FALSE);
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// HAL3Module::ReserveTorchForCamera
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID HAL3Module::ReserveTorchForCamera(
    UINT32  logicalCameraId,
    UINT32  fwNotificationId)
{
    CAMX_ASSERT(logicalCameraId < m_numLogicalCameras);
    CAMX_UNREFERENCED_PARAM(fwNotificationId);

    // Make sure all flash lights are turned off to reserve the flash for camera Open
    for (UINT32 sensor = 0; sensor < m_numLogicalCameras; sensor++)
    {
        SetTorchModeInternal(sensor, sensor, TorchModeStatusNotAvailable, FALSE);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// HAL3Module::ReleaseTorchForCamera
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID HAL3Module::ReleaseTorchForCamera(
    UINT32  logicalCameraId,
    UINT32  fwNotificationId)
{
    CAMX_ASSERT(logicalCameraId < m_numLogicalCameras);
    CAMX_UNREFERENCED_PARAM(fwNotificationId);

    // Make sure all flash lights are released
    for (UINT32 sensor = 0; sensor < m_numLogicalCameras; sensor++)
    {
        CAMX_ASSERT(TorchModeStatusNotAvailable == m_torchStatus[sensor]);
        SetTorchModeInternal(sensor, sensor, TorchModeStatusAvailableOff, TRUE);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// HAL3Module::InitConfigurationForTorch
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID HAL3Module::InitConfigurationForTorch(
    UINT32               cameraId,
    Camera3StreamConfig* pStreamConfigs)
{
    CAMX_ASSERT(pStreamConfigs != NULL);

    chi_hal_callback_ops_t* pCHIAppCallbacks  = HAL3Module::GetInstance()->GetCHIAppCallbacks();

    /// @todo (CAMX-1518) Handle private data from Override module
    VOID*           pPrivateData;
    BOOL            isOverrideEnabled = FALSE;
    Camera3Stream*  pTotalStreams[1];
    CamxResult      result            = CamxResultSuccess;

    CHIEXTENDSETTINGS extend                       = { 0 };
    CHISETTINGTOKEN   tokenList[NumExtendSettings] = { { 0 } };
    extend.pTokens = tokenList;

    GenerateExtendOpenData(NumExtendSettings, &extend);

    // Reserve the camera to detect if it is already open or too many concurrent are open
    result = HAL3Module::GetInstance()->ProcessCameraOpen(cameraId, &extend);

    if (CamxResultSuccess == result)
    {

        // Sample code to show how the VOID* can be used in ModifySettings
        ChiModifySettings setting[NumExtendSettings] = { { { 0 } } };
        GenerateModifySettingsData(setting);

        /*
         * We are using setting 8 and 9 only because for torch mode usecase, we do not want other
         * override settings to be used, but just to modify the open and close status of the camera
         * or else torch mode will fail because other settings change the usecase selection logic
         */
        pCHIAppCallbacks->chi_modify_settings(&setting[8]); // overrride close
        pCHIAppCallbacks->chi_modify_settings(&setting[9]); // override open

        // Create a dummy streams configuration for Torch case.
        m_torchStream.streamType    = StreamTypeOutput;
        m_torchStream.width         = 640;
        m_torchStream.height        = 480;
        m_torchStream.format        = HALPixelFormatYCbCr420_888;
        m_torchStream.dataspace     = HALDataspaceUnknown;
        m_torchStream.rotation      = StreamRotationCCW0;
        m_torchStream.grallocUsage  = 0;
        m_torchStream.maxNumBuffers = 0;
        m_torchStream.pPrivateInfo  = NULL;

        pTotalStreams[0] = &m_torchStream;

        pStreamConfigs->numStreams    = 1;
        pStreamConfigs->operationMode = StreamConfigModeQTITorchWidget;
        pStreamConfigs->ppStreams     = reinterpret_cast<Camera3Stream **>(&pTotalStreams);
#if ((CAMX_ANDROID_API) && (CAMX_ANDROID_API >= 28)) // Android-P or better
        pStreamConfigs->pSessionParameters = NULL;
#endif // ((CAMX_ANDROID_API) && (CAMX_ANDROID_API >= 28))

        CamX::Utils::Memset(&m_camera3Device, 0, sizeof(m_camera3Device));

        pCHIAppCallbacks->chi_initialize_override_session(cameraId,
                                                          &m_camera3Device,
                                                          NULL,
                                                          reinterpret_cast<camera3_stream_configuration_t*>(pStreamConfigs),
                                                          &isOverrideEnabled,
                                                          &pPrivateData);
    }

    if (CamxResultSuccess == result)
    {
        // Create metadata needed for Torch.Need this to update the metadata while submitting the request.
        // NOWHINE CP036a: exception, as this is internal metadata.
        m_pMetadata = const_cast<Metadata*>
                      (HAL3DefaultRequest::ConstructDefaultRequestSettings(cameraId, RequestTemplatePreview));
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// HAL3Module::DeInitConfigurationForTorch
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID HAL3Module::DeInitConfigurationForTorch(
    UINT32 cameraId)
{
    CAMX_UNREFERENCED_PARAM(cameraId);

    CHIEXTENDSETTINGS extend                       = { 0 };
    CHISETTINGTOKEN   tokenList[NumExtendSettings] = { { 0 } };

    extend.pTokens = tokenList;
    GenerateExtendCloseData(NumExtendSettings, &extend);

    HAL3Module::GetInstance()->ProcessCameraClose(cameraId, &extend);

    // Sample code to show how the VOID* can be used in ModifySettings
    ChiModifySettings setting[NumExtendSettings] = { { { 0 } } };
    GenerateModifySettingsData(setting);
    /*
    * We are using setting 8 and 9 only because for torch mode usecase, we do not want other
    * override settings to be used, but just to modify the open and close status of the camera
    * or else torch mode will fail because other settings change the usecase selection logic
    */
    chi_hal_callback_ops_t* pCHIAppCallbacks  = HAL3Module::GetInstance()->GetCHIAppCallbacks();

    pCHIAppCallbacks->chi_modify_settings(&setting[8]); // overrride close
    pCHIAppCallbacks->chi_modify_settings(&setting[9]); // override open
    pCHIAppCallbacks->chi_teardown_override_session(&m_camera3Device, 0, NULL);
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// HAL3Module::SubmitRequestForTorch
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID HAL3Module::SubmitRequestForTorch(
    UINT32          cameraId,
    TorchModeStatus torchStatus)
{

    Camera3CaptureRequest       tempCaptureRequest;
    CamxResult                  result             = CamxResultSuccess;
    FlashModeValues             flashMode          = FlashModeOff;
    chi_hal_callback_ops_t*     pCHIAppCallbacks   = HAL3Module::GetInstance()->GetCHIAppCallbacks();

    CAMX_UNREFERENCED_PARAM(cameraId);

    // Assign frame number 1 for On, 2 for Off
    if (TorchModeStatusAvailableOn == torchStatus)
    {
        flashMode = FlashModeTorch;
        result    = HAL3MetadataUtil::SetMetadata(m_pMetadata, FlashMode,
                                                  reinterpret_cast<VOID*>(&(flashMode)),
                                                  1);
        tempCaptureRequest.frameworkFrameNum = 1;

    }
    else if (TorchModeStatusAvailableOff == torchStatus ||
             TorchModeStatusNotAvailable == torchStatus)
    {
        flashMode = FlashModeOff;
        result    = HAL3MetadataUtil::SetMetadata(m_pMetadata, FlashMode,
                                                  reinterpret_cast<VOID*>(&(flashMode)),
                                                  1);
        tempCaptureRequest.frameworkFrameNum = 2;
    }
    tempCaptureRequest.pMetadata         = m_pMetadata;
    tempCaptureRequest.pInputBuffer      = NULL;
    tempCaptureRequest.pOutputBuffers    = NULL;
    tempCaptureRequest.numOutputBuffers  = 1;

    result = pCHIAppCallbacks->chi_override_process_request(&m_camera3Device,
                                                            reinterpret_cast<camera3_capture_request_t*>(&tempCaptureRequest),
                                                            NULL);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// HAL3Module::SetTorchModeInternal
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID HAL3Module::SetTorchModeInternal(
    UINT32          logicalCameraId,
    UINT32          fwNotificationId,
    TorchModeStatus torchStatus,
    BOOL            inReleaseMode)
{
    CAMX_ASSERT(logicalCameraId < m_numLogicalCameras);

    // Don't change status if status didn't change
    if (torchStatus != m_torchStatus[logicalCameraId])
    {
        switch (torchStatus)
        {
            // Turn the torch off or move to not available
            case TorchModeStatusNotAvailable:
            case TorchModeStatusAvailableOff:
                // Nothing to do if changing from off or not available
                if (TorchModeStatusAvailableOn == m_torchStatus[logicalCameraId])
                {
                    /// @todo (CAMX-265) Turn off
                    SubmitRequestForTorch(logicalCameraId, torchStatus);
                    /// @todo (CAMX-265) De-init
                    DeInitConfigurationForTorch(logicalCameraId);
                }
                break;
            // Turn the torch on
            case TorchModeStatusAvailableOn:
                // Should never change from not available to on (should be caught by upper layers)
                CAMX_ASSERT(TorchModeStatusNotAvailable != m_torchStatus[logicalCameraId]);
                // Nothing to do if changing from on
                if (TorchModeStatusAvailableOff == m_torchStatus[logicalCameraId])
                {
                    /// @todo (CAMX-265) Init
                    // Setup configure streams needed for Torch mode.
                    Camera3StreamConfig streamConfigs;
                    InitConfigurationForTorch(logicalCameraId, &streamConfigs);
                    /// @todo (CAMX-265) Turn on
                    SubmitRequestForTorch(logicalCameraId, torchStatus);
                }
                break;
            default:
                CAMX_ASSERT_ALWAYS_MESSAGE("Invalid torch status: %x", static_cast<UINT32>(torchStatus));
                break;
        }
        m_torchStatus[logicalCameraId] = torchStatus;

        /// @todo (CAMX-265) If no flash, skip callback.  Also, don't pass the CB pointer into the HAL3Module,
        ///                  instead call back into the camxhal3 static functions and insert the frameworkId
        ///                  there (and get rid of frameworkId in the torch calls...)
        CAMX_LOG_INFO(CamxLogGroupHAL,
                      "Torch notify state is (inReleaseMode = %d, dropCallbacks = %d)",
                      inReleaseMode,
                      m_dropCallbacks);

        if ((NULL != m_pModuleCbs) &&
            ((FALSE == inReleaseMode) || (FALSE == m_dropCallbacks)))
        {
            CHAR cameraIdString[8];
            OsUtils::SNPrintF(cameraIdString, sizeof(cameraIdString), "%d", fwNotificationId);
            INT status = torchStatus;
            m_pModuleCbs->torch_mode_status_change(m_pModuleCbs, cameraIdString, status);
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// HAL3Module::Dump
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID HAL3Module::Dump(
    INT fd
    ) const
{
    CAMX_UNREFERENCED_PARAM(fd);
    // nothing worth dumping in here
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// HAL3Module::HAL3Module
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
HAL3Module::HAL3Module()
{
    CamxResult result = CamxResultSuccess;
    CSLCameraPlatform CSLPlatform = {};

    CAMX_LOG_CONFIG(CamxLogGroupHAL, "***************************************************");
    CAMX_LOG_CONFIG(CamxLogGroupHAL, "SHA1:     %s", CAMX_SHA1);
    CAMX_LOG_CONFIG(CamxLogGroupHAL, "COMMITID: %s", CAMX_COMMITID);
    CAMX_LOG_CONFIG(CamxLogGroupHAL, "BUILD TS: %s", CAMX_BUILD_TS);
    CAMX_LOG_CONFIG(CamxLogGroupHAL, "***************************************************");

    m_hChiOverrideModuleHandle = NULL;
    m_numLogicalCameras        = 0;
    m_pStaticSettings          = HwEnvironment::GetInstance()->GetStaticSettings();
    result                     = CSLQueryCameraPlatform(&CSLPlatform);

    CAMX_ASSERT(CamxResultSuccess == result);

    CamX::Utils::Memset(&m_ChiAppCallbacks, 0, sizeof(m_ChiAppCallbacks));
    CamX::Utils::Memset(m_pStaticMetadata, 0, sizeof(m_pStaticMetadata));

    for (UINT32 sensor = 0; sensor < MaxNumImageSensors; sensor++)
    {
        m_torchStatus[sensor] = TorchModeStatusAvailableOff;
    }
    m_pMetadata = NULL;

    // Set Camera Launch status to False at the time of constructor
    DisplayConfigInterface::SetCameraStatus(FALSE);

    static const UINT NumCHIOverrideModules = 2;

    UINT16 fileCount        = 0;
    const  CHAR*  pD        = NULL;
    INT    fileIndexBitra  = FILENAME_MAX;
    INT    fileIndex        = 0;

    CHAR moduleFileName[NumCHIOverrideModules * FILENAME_MAX];

    switch (CSLPlatform.socId)
    {
        case CSLCameraTitanSocSM6350:
        case CSLCameraTitanSocSM7225:
#if defined(_LP64)
            fileCount = OsUtils::GetFilesFromPath("/vendor/lib64/camera/oem/bitra",
                                                  FILENAME_MAX,
                                                  &moduleFileName[0],
                                                  "*",
                                                  "chi",
                                                  "*",
                                                  "*",
                                                  &SharedLibraryExtension[0]);
            if (0 == fileCount)
            {
                fileCount = OsUtils::GetFilesFromPath("/vendor/lib64/camera/qti/bitra",
                                                      FILENAME_MAX,
                                                      &moduleFileName[0],
                                                      "*",
                                                      "chi",
                                                      "*",
                                                      "*",
                                                      &SharedLibraryExtension[0]);
            }
#else // using LP32
            fileCount = OsUtils::GetFilesFromPath("/vendor/lib/camera/oem/bitra",
                                                  FILENAME_MAX,
                                                  &moduleFileName[0],
                                                  "*",
                                                  "chi",
                                                  "*",
                                                  "*",
                                                  &SharedLibraryExtension[0]);
            if (0 == fileCount)
            {
                fileCount = OsUtils::GetFilesFromPath("/vendor/lib/camera/qti/bitra",
                                                      FILENAME_MAX,
                                                      &moduleFileName[0],
                                                      "*",
                                                      "chi",
                                                      "*",
                                                      "*",
                                                      &SharedLibraryExtension[0]);
            }
#endif // _LP64
            if (0 == fileCount)
            {
                fileCount = OsUtils::GetFilesFromPath(CHIOverrideModulePath,
                                                      FILENAME_MAX,
                                                      &moduleFileName[0],
                                                      "*",
                                                      "chi",
                                                      "*",
                                                      "*",
                                                      &SharedLibraryExtension[0]);
            }
            break;
        default:
            fileCount = OsUtils::GetFilesFromPath(CHIOverrideModulePath,
                                                  FILENAME_MAX,
                                                  &moduleFileName[0],
                                                  "*",
                                                  "chi",
                                                  "*",
                                                  "*",
                                                  &SharedLibraryExtension[0]);
            break;
    }

    if (0 == fileCount)
    {
        CAMX_LOG_ERROR(CamxLogGroupHAL, "FATAL: No CHI Module library found in %s - Cannot proceed", CHIOverrideModulePath);
    }
    else
    {
        pD = OsUtils::StrStr(&moduleFileName[0], "bitra");

        // pD is NULL if Bitra is not present in first file index
        if (pD != NULL)
        {
            fileIndexBitra = 0;
            fileIndex      = FILENAME_MAX;
        }

        if (NumCHIOverrideModules >= fileCount)
        {
            if (CSLPlatform.socId == CSLCameraTitanSocSM6350 || CSLPlatform.socId == CSLCameraTitanSocSM7225)
            {
                CAMX_LOG_INFO(CamxLogGroupHAL, "opening CHI Module - %s", &moduleFileName[fileIndexBitra]);
                m_hChiOverrideModuleHandle = OsUtils::LibMap(&moduleFileName[fileIndexBitra]);
            }
            else
            {
                CAMX_LOG_INFO(CamxLogGroupHAL, "opening CHI Module - %s", &moduleFileName[fileIndex]);
                m_hChiOverrideModuleHandle = OsUtils::LibMap(&moduleFileName[fileIndex]);
            }

            if (NULL != m_hChiOverrideModuleHandle)
            {
                CHIHALOverrideEntry funcCHIHALOverrideEntry =
                    reinterpret_cast<CHIHALOverrideEntry>(
                        CamX::OsUtils::LibGetAddr(m_hChiOverrideModuleHandle, "chi_hal_override_entry"));

                if (NULL != funcCHIHALOverrideEntry)
                {
                    funcCHIHALOverrideEntry(&m_ChiAppCallbacks);

                    CAMX_ASSERT(NULL != m_ChiAppCallbacks.chi_get_num_cameras);
                    CAMX_ASSERT(NULL != m_ChiAppCallbacks.chi_get_camera_info);
                    CAMX_ASSERT(NULL != m_ChiAppCallbacks.chi_get_info);
                    CAMX_ASSERT(NULL != m_ChiAppCallbacks.chi_finalize_override_session);
                    CAMX_ASSERT(NULL != m_ChiAppCallbacks.chi_initialize_override_session);
                    CAMX_ASSERT(NULL != m_ChiAppCallbacks.chi_override_process_request);
                    CAMX_ASSERT(NULL != m_ChiAppCallbacks.chi_override_flush);
                    CAMX_ASSERT(NULL != m_ChiAppCallbacks.chi_override_dump);
                    CAMX_ASSERT(NULL != m_ChiAppCallbacks.chi_teardown_override_session);
                    CAMX_ASSERT(NULL != m_ChiAppCallbacks.chi_extend_open);
                    CAMX_ASSERT(NULL != m_ChiAppCallbacks.chi_extend_close);
                    CAMX_ASSERT(NULL != m_ChiAppCallbacks.chi_remap_camera_id);
                    CAMX_ASSERT(NULL != m_ChiAppCallbacks.chi_modify_settings);
                    CAMX_ASSERT(NULL != m_ChiAppCallbacks.chi_get_default_request_settings);

                    if ((NULL != m_ChiAppCallbacks.chi_get_num_cameras)             &&
                        (NULL != m_ChiAppCallbacks.chi_get_camera_info)             &&
                        (NULL != m_ChiAppCallbacks.chi_get_info)                    &&
                        (NULL != m_ChiAppCallbacks.chi_finalize_override_session)   &&
                        (NULL != m_ChiAppCallbacks.chi_initialize_override_session) &&
                        (NULL != m_ChiAppCallbacks.chi_override_process_request)    &&
                        (NULL != m_ChiAppCallbacks.chi_override_flush)              &&
                        (NULL != m_ChiAppCallbacks.chi_override_dump)               &&
                        (NULL != m_ChiAppCallbacks.chi_teardown_override_session)   &&
                        (NULL != m_ChiAppCallbacks.chi_extend_open)                 &&
                        (NULL != m_ChiAppCallbacks.chi_extend_close)                &&
                        (NULL != m_ChiAppCallbacks.chi_remap_camera_id)             &&
                        (NULL != m_ChiAppCallbacks.chi_modify_settings)             &&
                        (NULL != m_ChiAppCallbacks.chi_get_default_request_settings))
                    {
                        CAMX_LOG_VERBOSE(CamxLogGroupHAL, "CHI Module library function pointers exchanged");
                    }
                    else
                    {
                        CAMX_LOG_ERROR(CamxLogGroupHAL, "CHI Module library function pointers exchanged FAILED");
                    }
                }
            }
            else
            {
                CAMX_LOG_ERROR(CamxLogGroupHAL, "Couldn't open CHI Module lib. All usecases will go thru HAL implementation");
            }
        }
        else
        {
            if (fileCount > NumCHIOverrideModules)
            {
                CAMX_LOG_ERROR(CamxLogGroupHAL, "Cannot have more than %d CHI override module present", NumCHIOverrideModules);
            }
        }
    }

    if (NULL != m_ChiAppCallbacks.chi_get_num_cameras)
    {
        m_ChiAppCallbacks.chi_get_num_cameras(&m_numFwCameras, &m_numLogicalCameras);
    }
    else
    {
        CAMX_ASSERT_ALWAYS_MESSAGE("Override module is mandatory.  Returning 0 cameras, and app will not behave properly");
        m_numFwCameras = 0;
    }

    m_pThermalManager = ThermalManager::Create();
    if (NULL == m_pThermalManager)
    {
        CAMX_LOG_WARN(CamxLogGroupHAL, "Failed to create ThermalManager");
        // Not a fatal error. Camera can continue to operate without this
    }

    // There are arrays capped with a max number of sensors.  If there are more than MaxNumImageSensors logical
    // cameras, this assert will fire.
    CAMX_ASSERT(m_numLogicalCameras < MaxNumImageSensors);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// HAL3Module::~HAL3Module
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
HAL3Module::~HAL3Module()
{
    if (NULL != m_pThermalManager)
    {
        CAMX_DELETE m_pThermalManager;
        m_pThermalManager = NULL;
    }

    if (NULL != m_hChiOverrideModuleHandle)
    {
        OsUtils::LibUnmap(m_hChiOverrideModuleHandle);
        m_hChiOverrideModuleHandle = NULL;
    }

    for (UINT32 i = 0; i < MaxNumImageSensors; i++)
    {
        if (NULL != m_pStaticMetadata[i])
        {
            HAL3MetadataUtil::FreeMetadata(m_pStaticMetadata[i]);
            m_pStaticMetadata[i] = NULL;
        }
    }
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// HAL3Module::ProcessCameraOpen
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult HAL3Module::ProcessCameraOpen(
    UINT32  logicalCameraId,
    VOID*   pPriv)
{
    CamxResult result   = CamxResultSuccess;

    if (logicalCameraId >= m_numLogicalCameras)
    {
        CAMX_LOG_ERROR(CamxLogGroupHAL, "Invalid logical camera id: %d", logicalCameraId);
        result = CamxResultEInvalidArg;
    }
    else if (TRUE == m_perCameraInfo[logicalCameraId].isCameraOpened)
    {
        CAMX_LOG_ERROR(CamxLogGroupHAL, "Camera id already in use: %d", logicalCameraId);
        result = CamxResultEBusy;
    }
    else if ((m_numCamerasOpened + 1) > MaxConcurrentDevices)
    {
        CAMX_LOG_ERROR(CamxLogGroupHAL, "Too many concurrent devices to open camera id: %d", logicalCameraId);
        result = CamxResultETooManyUsers;
    }
    else
    {
        result = m_ChiAppCallbacks.chi_extend_open(logicalCameraId, pPriv);

        if (CamxResultSuccess == result)
        {
            m_perCameraInfo[logicalCameraId].isCameraOpened = TRUE;
            m_numCamerasOpened++;
            CAMX_LOG_CONFIG(CamxLogGroupHAL, "number of Camera Opened %d", m_numCamerasOpened);
        }
        else
        {
            CAMX_LOG_ERROR(CamxLogGroupHAL, "Camera Open failed with error status = %s",
                Utils::CamxResultToString(result));
        }
    }

    if (1 == m_numCamerasOpened)
    {
        // Set Camera Launch status to True at the time of camera open
        DisplayConfigInterface::SetCameraStatus(TRUE);
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// HAL3Module::ProcessCameraClose
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult HAL3Module::ProcessCameraClose(
    UINT32 logicalCameraId,
    VOID*  pPriv)
{
    CamxResult result = CamxResultSuccess;

    m_ChiAppCallbacks.chi_extend_close(logicalCameraId, pPriv);

    if (m_numLogicalCameras <= logicalCameraId)
    {
        CAMX_LOG_ERROR(CamxLogGroupHAL, "Invalid camera id: %d", logicalCameraId);
        result = CamxResultEInvalidArg;
    }
    else if (FALSE == m_perCameraInfo[logicalCameraId].isCameraOpened)
    {
        CAMX_LOG_ERROR(CamxLogGroupHAL, "Camera id is closed already: %d", logicalCameraId);
        result = CamxResultEBusy;
    }
    else
    {
        m_perCameraInfo[logicalCameraId].isCameraOpened = FALSE;
        m_numCamerasOpened--;
    }

    if (0 == m_numCamerasOpened)
    {
        // Set Camera Launch status to True at the time of camera open
        DisplayConfigInterface::SetCameraStatus(FALSE);
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// HAL3Module::GetNumCameras
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UINT32 HAL3Module::GetNumCameras() const
{
    return m_numFwCameras;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// HAL3Module::GetCameraInfo
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult HAL3Module::GetCameraInfo(
    UINT32      logicalCameraId,
    CameraInfo* pCameraInfo
    ) const
{
    CamxResult            result        = CamxResultSuccess;
    ChiCameraInfo         chiCameraInfo = { 0 };
    const StaticSettings* pSettings     = HwEnvironment::GetInstance()->GetStaticSettings();

    chiCameraInfo.pLegacy = pCameraInfo;

    if (NULL != m_ChiAppCallbacks.chi_get_camera_info)
    {
        result = m_ChiAppCallbacks.chi_get_camera_info(logicalCameraId, reinterpret_cast<struct camera_info*>(pCameraInfo));

        if ((CamxResultSuccess == result) && (NULL != pCameraInfo->pStaticCameraInfo) &&
            (TRUE == pSettings->MetadataVisibility))
        {
            if (NULL == m_pStaticMetadata[logicalCameraId])
            {
                SIZE_T                staticEntryCapacity;
                SIZE_T                staticDataSize;
                HAL3MetadataUtil::CalculateSizeStaticMeta(&staticEntryCapacity,
                    &staticDataSize, TagSectionVisibleToFramework);

                HAL3Module::GetInstance()->m_pStaticMetadata[logicalCameraId] = HAL3MetadataUtil::CreateMetadata(
                    staticEntryCapacity,
                    staticDataSize);

                result = HAL3MetadataUtil::CopyMetadata(m_pStaticMetadata[logicalCameraId],
                    // NOWHINE CP036a: Need cast
                    const_cast<Metadata*>(pCameraInfo->pStaticCameraInfo), TagSectionVisibleToFramework);
            }
            if (CamxResultSuccess == result && NULL != m_pStaticMetadata[logicalCameraId])
            {
                // NOWHINE CP036a: Need cast
                pCameraInfo->pStaticCameraInfo = const_cast<Metadata*>(m_pStaticMetadata[logicalCameraId]);
            }
        }
    }
    else
    {
        result = CamxResultEFailed;
        CAMX_ASSERT_ALWAYS_MESSAGE("CHI override module is not present!");
    }

    return result;
}

CAMX_NAMESPACE_END
