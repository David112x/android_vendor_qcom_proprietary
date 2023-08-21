////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2017-2020 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  chxextensionmodule.cpp
/// @brief Extension Module implementation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include <assert.h>
#if defined (_WIN32)    // This file for Win32 build only
#include <malloc.h>
#else
#include <stdlib.h>
#include <log/log.h>
#endif // WIN32

#include <algorithm>
#include <vector>
#include <string>

#include <utils/Errors.h>

#include "chi.h"
#include "chioverride.h"
#include "camxcdktypes.h"
#include "chxusecaseutils.h"

#include "chxextensionmodule.h"
#include "chxsensorselectmode.h"
#include "chxsession.h"
#include "chxusecase.h"
#include "chxmulticamcontroller.h"
#include "camxchiofflinelogger.h"

#include "postprocserviceintf.h"

// NOWHINE ENTIRE FILE - Temporarily bypassing for existing CHI files

extern CHICONTEXTOPS       g_chiContextOps;
extern CHIBUFFERMANAGEROPS g_chiBufferManagerOps;
extern BOOL                g_enableSystemLog;

CDK_VISIBILITY_PUBLIC UINT32 g_enableChxLogs     = 3;
CDK_VISIBILITY_PUBLIC BOOL   g_logRequestMapping = TRUE;
CDK_VISIBILITY_PUBLIC BOOL   g_enableSystemLog   = TRUE;

static const UINT          SingleISPResourceCost = 50;

/// @brief Primarily an example for how to use the VOID* in ExtendOpen and ModifySettings.  Must match data structure
///        being passed into the function
struct ChiOverrideToken
{
    UINT32    id;
    UINT32    size;
};

/// @brief Primarily an example for how to use the VOID* in ExtendOpen and ModifySettings  Must match data structure
///        being passed into the function
struct ChiOverrideExtendSettings
{
    UINT32              numTokens;
    ChiOverrideToken*   pTokens;
};

/// @brief Primarily an example for how to use the VOID* in ExtendClose and ModifySettings  Must match data structure
///        being passed into the function
struct ChiOverrideExtendClose
{
    UINT32              numTokens;
    ChiOverrideToken*   pTokens;
};

/// Constants
static const UINT MaxPhyIdsTagSize   = 10; ///< Maximum Tag Size of physical cameraId HAL tag

/// @brief Logical camera configuration which will be configured by customer themselves according to their requirements.
static LogicalCameraConfiguration logicalCameraConfiguration[] =
{
    /*cameraId cameraType              exposeFlag phyDevCnt  sensorId, transition low, high, smoothZoom, alwaysOn  realtimeEngine             primarySensorID*/
    {0,        LogicalCameraType_Default, TRUE,      1,    {{0,                    0.0, 0.0,   FALSE,    TRUE,     RealtimeEngineType_IFE}},  0   }, ///< Wide camera
    {1,        LogicalCameraType_Default, TRUE,      1,    {{2,                    0.0, 0.0,   FALSE,    TRUE,     RealtimeEngineType_IFE}},  2   }, ///< Front camera
    {2,        LogicalCameraType_Default, TRUE,      1,    {{1,                    0.0, 0.0,   FALSE,    TRUE,     RealtimeEngineType_IFE}},  1   }, ///< Tele camera
    {3,        LogicalCameraType_SAT,     TRUE,      2,    {{0,                    1.0, 2.0,   TRUE,     FALSE,    RealtimeEngineType_IFE},
                                                            {1,                    2.0, 8.0,   TRUE,     FALSE,    RealtimeEngineType_IFE}},  0   }, ///< SAT
    {4,        LogicalCameraType_RTB,     TRUE,      2,    {{0,                    1.0, 2.0,   FALSE,    TRUE,     RealtimeEngineType_IFE},
                                                            {1,                    2.0, 8.0,   FALSE,    TRUE,     RealtimeEngineType_IFE}},  1   }, ///< RTB
    {5,        LogicalCameraType_Default, TRUE,      1,    {{3,                    0.0, 0.0,   FALSE,    TRUE,     RealtimeEngineType_IFE}},  3   }, ///< Extra Camera

    {6,        LogicalCameraType_SAT,     TRUE,      3,    {{2,                    1.0, 1.5,   FALSE,    FALSE,    RealtimeEngineType_IFE},
                                                            {0,                    1.5, 2.0,   TRUE,     FALSE,    RealtimeEngineType_IFE},
                                                            {1,                    2.0, 8.0,   TRUE,     FALSE,    RealtimeEngineType_IFE}},  2   },   ///< U+W+T FOV transition
    {7,        LogicalCameraType_DualApp, FALSE,     2,    {{0,                    1.0, 2.0,   TRUE,     FALSE,    RealtimeEngineType_IFE},
                                                            {1,                    2.0, 8.0,   TRUE,     FALSE,    RealtimeEngineType_IFE}},  0   }, ///< Dual application
    {8,        LogicalCameraType_Default, TRUE,      1,    {{4,                    0.0, 0.0,   FALSE,    TRUE,     RealtimeEngineType_IFE}},  4   }, /// Extra camera2 / Wide camera
    {99,       LogicalCameraType_Default, FALSE,     1,    {{3,                    0.0, 0.0,   FALSE,    TRUE,     RealtimeEngineType_IFE}},  3   }, ///< Secure camera

    /*-------------------------------------------------------------------------------------------------*/
    /*                                   Add cameras for triple camera module                          */
    /*-------------------------------------------------------------------------------------------------*/
    {9,        LogicalCameraType_Default, TRUE,      1,    {{5,                    0.0, 0.0,   FALSE,    TRUE,     RealtimeEngineType_IFE}},   5   }, ///< Tele camera
    {10,       LogicalCameraType_Default, TRUE,      1,    {{6,                    0.0, 0.0,   FALSE,    TRUE,     RealtimeEngineType_IFE}},   6   }, ///< Ultra wide  camera
    {11,       LogicalCameraType_SAT,     TRUE,      3,    {{6,                    1.0, 1.5,   FALSE,    FALSE,    RealtimeEngineType_IFE},
                                                            {4,                    1.5, 2.0,   TRUE,     FALSE,    RealtimeEngineType_IFE},
                                                            {5,                    2.0, 8.0,   TRUE,     FALSE,    RealtimeEngineType_IFE}},   6   },   ///< U+W+T FOV transition
    {12,       LogicalCameraType_RTB,     TRUE,      2,    {{4,                    1.0, 2.0,   FALSE,    TRUE,     RealtimeEngineType_IFE},
                                                            {5,                    2.0, 8.0,   FALSE,    TRUE,     RealtimeEngineType_IFE}},   5   },   ///< U+W+T RTB transition
};

/// @brief Default Stream Mapping structure expected as a part of configure stream.
/*
    This table used to differentiate similar property stream sets configured as a part of configure_stream()
    Application can override this and send stream property a part of session parameter.

    @Width: and @height: is configured stream resolution info.
    @Format: Configured stream format info.
    @SequenceIndex: Is order/sequence in which stream is configured from application.
                    Needed incase application want to configure diferent streams of same size and format. 0 otherwise
    @Stream Intent: is used as value to understand if stream is Snapshot based or preview based.
    @Camera Index : This is camera position index Ex: W + T, W is index 0 and TELE is 1. Needed only if isPhysicalStream is set
    @isPhysicalStream: Set if this stream is needed from physical camera
    @isThumbnailPostView: Postview for snapshot

    <Width, Height, Format, SequenceIndex> is KEY to match vendor tag against configure_stream().
    <Other parameters> are values.
*/
static StreamMap streamMap[] =
{
    {0, 0, HAL_PIXEL_FORMAT_YCbCr_420_888, 0, ANDROID_CONTROL_CAPTURE_INTENT_PREVIEW,        0, 0, 0}, //PreviewYuv
    {0, 0, HAL_PIXEL_FORMAT_YCbCr_420_888, 0, ANDROID_CONTROL_CAPTURE_INTENT_STILL_CAPTURE,  0, 1, 0}, //Snapshot YUV
    {0, 0, HAL_PIXEL_FORMAT_YCbCr_420_888, 1, ANDROID_CONTROL_CAPTURE_INTENT_STILL_CAPTURE,  1, 1, 0}, //Snapshot YUV
    {0, 0, HAL_PIXEL_FORMAT_YCbCr_420_888, 2, ANDROID_CONTROL_CAPTURE_INTENT_STILL_CAPTURE,  2, 1, 0}, //Snppshot YUV
    {0, 0, HAL_PIXEL_FORMAT_RAW10,         0, ANDROID_CONTROL_CAPTURE_INTENT_STILL_CAPTURE,  0, 0, 0}, //Snppshot RAW callback
    {0, 0, HAL_PIXEL_FORMAT_RAW10,         1, ANDROID_CONTROL_CAPTURE_INTENT_STILL_CAPTURE,  1, 0, 0}, //Snppshot RAW callback
    {0, 0, HAL_PIXEL_FORMAT_RAW10,         2, ANDROID_CONTROL_CAPTURE_INTENT_STILL_CAPTURE,  2, 0, 0}, //Snppshot RAW callback
    {0, 0, HAL_PIXEL_FORMAT_RAW16,         0, ANDROID_CONTROL_CAPTURE_INTENT_STILL_CAPTURE,  0, 0, 0}, //Snppshot RAW callback
    {0, 0, HAL_PIXEL_FORMAT_RAW16,         1, ANDROID_CONTROL_CAPTURE_INTENT_STILL_CAPTURE,  1, 0, 0}, //Snppshot RAW callback
    {0, 0, HAL_PIXEL_FORMAT_RAW16,         2, ANDROID_CONTROL_CAPTURE_INTENT_STILL_CAPTURE,  2, 0, 0}, //Snppshot RAW callback
    {0, 0, HAL_PIXEL_FORMAT_YCbCr_420_888, 0, ANDROID_CONTROL_CAPTURE_INTENT_STILL_CAPTURE,  0, 0, 1}, //PostView
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// SignalOfflineLoggerThreadWrapper
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static VOID SignalOfflineLoggerThreadWrapper(OfflineLoggerType logger_type)
{
    // Allow caller to signal corresponding Offline logger thread to trigger flush
    (void)ExtensionModule::GetInstance()->SignalOfflineLoggerThread(logger_type);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ExtensionModule::GetInstance
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
ExtensionModule* ExtensionModule::GetInstance()
{
    static ExtensionModule s_extensionModuleSingleton;

    return &s_extensionModuleSingleton;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ExtensionModule::ExtensionModule
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
ExtensionModule::ExtensionModule()
    : m_hCHIContext(NULL)
    , m_numPhysicalCameras(0)
    , m_numLogicalCameras(0)
    , m_previousPowerHint(PERF_LOCK_COUNT)
    , m_numMetadataResults(0)
    , m_terminateOfflineLogThread(FALSE)
{
    // initialize member variables and pointers
    m_platformID                      = ChxUtils::ReadSocID();
    m_pDisableZSL                     = NULL;
    m_pForceUsecaseId                 = NULL;
    m_pEnableFOVC                     = NULL;
    m_pGPUNodeRotateUsecase           = NULL;
    m_pGPUNodeDownscaleUsecase        = NULL;
    m_pEnableBLMClient                = NULL;
    m_pEnableMFNRUsecase              = NULL;
    m_pAnchorSelectionAlgoForMFNR     = NULL;
    m_pAnchorAlgoSelectionType        = NULL;
    m_pEnableMFSRUsecase              = NULL;
    m_pHFRNo3AUsecase                 = NULL;
    m_pForceSensorMode                = NULL;
    m_pEISV2Enable                    = NULL;
    m_pEISV3Enable                    = NULL;
    m_pDisableASDProcessing           = NULL;
    m_pEnableMultiVRCamera            = NULL;
    m_pOverrideCameraClose            = NULL;
    m_pOverrideCameraOpen             = NULL;
    m_pNumPCRsBeforeStreamOn          = NULL;
    m_pStatsSkipPattern               = NULL;
    m_pEnableMultiCameraFrameSync     = NULL;
    m_pEnableDumpDebugData            = NULL;
    m_pEnable3ADebugData              = NULL;
    m_pEnableConcise3ADebugData       = NULL;
    m_pEnableTuningMetadata           = NULL;
    m_pDebugDataSizeAEC               = NULL;
    m_pDebugDataSizeAWB               = NULL;
    m_pDebugDataSizeAF                = NULL;
    m_pConciseDebugDataSizeAEC        = NULL;
    m_pConciseDebugDataSizeAWB        = NULL;
    m_pConciseDebugDataSizeAF         = NULL;
    m_pTuningDumpDataSizeIFE          = NULL;
    m_pTuningDumpDataSizeIPE          = NULL;
    m_pTuningDumpDataSizeBPS          = NULL;
    m_pOutputFormat                   = NULL;
    m_pCHIPartialDataSupport          = NULL;
    m_pCHIEnablePartialDataRecovery   = NULL;
    m_pFDStreamSupport                = NULL;
    m_pSelectInSensorHDR3ExpUsecase   = NULL;
    m_pEnableUnifiedBufferManager     = NULL;
    m_pEnableCHILateBinding           = NULL;
    m_pEnableOfflineNoiseReprocessing = NULL;
    m_pEnableFeature2Dump             = NULL;
    m_pEnableRAWHDR                   = NULL;
    m_pForceHWMFFixedNumOfFrames      = NULL;
    m_pEnableSystemLogging            = NULL;
    m_pEnableTBMChiFence              = NULL;
    m_pMaxHALRequests                 = NULL;
    m_pEnableMFSRChiFence             = NULL;
    m_pEnableScreenGrab               = NULL;
    m_pUseFeatureForQCFA              = NULL;
    m_pDefaultMaxFPS                  = NULL;
    m_pAdvanceFeataureMask            = NULL;
    m_pBPSRealtimeSensorId            = NULL;
    m_pForceSWMFFixedNumOfFrames      = NULL;
    m_usecaseMaxFPS                   = 0;
    m_VideoHDRMode                    = 0;
    m_usecaseNumBatchedFrames         = 0;
    m_HALOutputBufferCombined         = FALSE;
    m_torchWidgetUsecase              = 0;
    m_CurrentpowerHint                = PERF_LOCK_COUNT;
    m_firstResult                     = 0;
    m_bAsciiLogEnable                 = 0;
    m_bBinaryLogEnable                = 0;
    m_numExposedLogicalCameras        = 0;
    m_offlineLoggerOps                = {};
    m_previewFPS                      = 0;
    m_videoFPS                        = 0;
    m_AECGainThresholdForQCFA         = 0.0;
    m_pEnableMultiCameraJPEG          = NULL;
    m_pMultiCameraHWSyncMask          = NULL;
    m_pBLMClient                      = NULL;
    m_pOverrideBurstShot              = NULL;
    m_pExposeFullsizeForQCFA          = NULL;
    m_isScreenGrabLiveShotScene       = FALSE;
    m_libraryHandle                   = NULL;

    memset(&m_HALOps, 0, sizeof(m_HALOps));
    memset(&m_chiFenceOps, 0, sizeof(m_chiFenceOps));
    memset(m_logicalCameraInfo, 0, sizeof(m_logicalCameraInfo));
    memset(m_IFEResourceCost, 0, sizeof(m_IFEResourceCost));
    m_pUsecaseSelector = UsecaseSelector::Create(this);
    m_pUsecaseFactory  = UsecaseFactory::Create(this);

    // To make vendor tags avaliable in ConsolidateCameraInfo
    MultiCamControllerManager::GetInstance();

    m_longExposureFrame      = static_cast<UINT32>(InvalidFrameNumber);
    m_singleISPResourceCost  = SingleISPResourceCost;       // Single ISP resource cost
    m_totalResourceBudget    = m_singleISPResourceCost * 2; // Assume two IFEs as default.
    m_pResourcesUsedLock     = Mutex::Create();

#if defined(_LP64)
    const CHAR* pChiDriver        = "/vendor/lib64/hw/camera.qcom.so";
    const CHAR* pChiOfflineLogger = "/vendor/lib64/libofflinelog.so";
#else // _LP64
    const CHAR* pChiDriver        = "/vendor/lib/hw/camera.qcom.so";
    const CHAR* pChiOfflineLogger = "/vendor/lib/libofflinelog.so";
#endif // _LP64
    const CHAR* pPerfModule       = "libqti-perfd-client.so";

    m_perfLibHandle = ChxUtils::LibMap(pPerfModule);

    if (NULL == m_perfLibHandle)
    {
        CHX_LOG_ERROR("Failed to load perf lib");
    }

    ChxUtils::Memset(&m_pPerfLockManager[0], 0, sizeof(m_pPerfLockManager));

    m_pConfigSettings           = static_cast<UINT32*>(CHX_CALLOC(sizeof(UINT32) * MaxConfigSettings));

    // Initialize offlinelogger CondVars and Mutex for each type
    for (UINT type = 0; type < OFFLINELOG_MAX_TYPE; type++)
    {
        m_pFlushNeededCond[type]  = Condition::Create();
        m_pFlushNeededMutex[type] = Mutex::Create();
    }

    // Create OfflineLogger thread and wait on being signaled by flush trigger logic
    m_offlineLoggerHandle = ChxUtils::LibMap(pChiOfflineLogger);

    if (NULL != m_offlineLoggerHandle)
    {
        PCHIOFFLINELOGENTRY funcPloggerEntry = reinterpret_cast<PCHIOFFLINELOGENTRY>(ChxUtils::LibGetAddr(m_offlineLoggerHandle, "ChiOfflineLogEntry"));

        if (NULL != funcPloggerEntry)
        {
            funcPloggerEntry(&m_offlineLoggerOps);
            ChiLog::SetOfflineLoggerOps(&m_offlineLoggerOps);

            // Create offlinelogger ASCII logger thread
            UINT logger_type = static_cast<UINT>(OfflineLoggerType::ASCII);
            m_pOfflineLoggerThread[logger_type].pPrivateData = this;

            ChxUtils::ThreadCreate(ExtensionModule::OfflineLoggerASCIIThread,
                &m_pOfflineLoggerThread[logger_type],
                &m_pOfflineLoggerThread[logger_type].hThreadHandle);

            // Create offlinelogger Binary logger thread
            logger_type = static_cast<UINT>(OfflineLoggerType::BINARY);
            m_pOfflineLoggerThread[logger_type].pPrivateData = this;

            ChxUtils::ThreadCreate(ExtensionModule::OfflineLoggerBinaryThread,
                &m_pOfflineLoggerThread[logger_type],
                &m_pOfflineLoggerThread[logger_type].hThreadHandle);

            // Pass in OfflineLogger thread signal function pointer into Camx OfflineLogger
            if (NULL != m_offlineLoggerOps.pLinkFlushTrigger)
            {
                for (UINT logger_type = 0; logger_type < OFFLINELOG_MAX_TYPE; logger_type++)
                {
                    m_offlineLoggerOps.pLinkFlushTrigger(static_cast<OfflineLoggerType>(logger_type),
                        &SignalOfflineLoggerThreadWrapper);
                }
            }
        }
    }

    m_numOfLogicalCameraConfiguration = CHX_ARRAY_SIZE(logicalCameraConfiguration);

    if (0 != m_numOfLogicalCameraConfiguration)
    {
        m_pLogicalCameraConfigurationInfo = static_cast<LogicalCameraConfiguration*>
            (CHX_CALLOC(sizeof(LogicalCameraConfiguration)* m_numOfLogicalCameraConfiguration));
        if (NULL != m_pLogicalCameraConfigurationInfo)
        {
            ChxUtils::Memcpy(m_pLogicalCameraConfigurationInfo, logicalCameraConfiguration,
                sizeof(LogicalCameraConfiguration)*m_numOfLogicalCameraConfiguration);
        }
        else
        {
            CHX_LOG_ERROR("Allocation failed for m_pLogicalCameraConfigurationInfo: Fatal");
        }
    }

    OSLIBRARYHANDLE handle  = ChxUtils::LibMap(pChiDriver);

    if (NULL != handle)
    {
        m_libraryHandle = handle;
        CHX_LOG("CHI Opened driver library");

        PCHIENTRY funcPChiEntry = reinterpret_cast<PCHIENTRY>(ChxUtils::LibGetAddr(handle, "ChiEntry"));

        if (NULL != funcPChiEntry)
        {
            CHX_LOG("CHI obtained ChiEntry point function");

            funcPChiEntry(&g_chiContextOps);

            m_hCHIContext = g_chiContextOps.pOpenContext();

            CHX_LOG("CHI context functions - CreateSession  %p", g_chiContextOps.pCreateSession);

            if (NULL != m_hCHIContext)
            {
                ChiVendorTagsOps vendorTagOps = { 0 };

                g_chiContextOps.pTagOps(&vendorTagOps);
                g_chiContextOps.pGetFenceOps(&m_chiFenceOps);

                // Pass in CHI get overrideSetting function pointer into Camx OfflineLogger
                if (NULL != m_offlineLoggerOps.pGetSetting)
                {
                    UINT logger_type = static_cast<UINT>(OfflineLoggerType::ASCII);
                    m_offlineLoggerOps.pGetSetting(static_cast<OfflineLoggerType>(logger_type), (g_chiContextOps.pGetSettings));
                    logger_type      = static_cast<UINT>(OfflineLoggerType::BINARY);
                    m_offlineLoggerOps.pGetSetting(static_cast<OfflineLoggerType>(logger_type), (g_chiContextOps.pGetSettings));
                }

                // Since this is a one time operation do it at boot up time
                if (NULL != vendorTagOps.pQueryVendorTagLocation)
                {
                    m_pvtVendorTags[static_cast<UINT>(VendorTag::SensorBpsModeIndex)].pSectionName    = "com.qti.sensorbps";
                    m_pvtVendorTags[static_cast<UINT>(VendorTag::SensorBpsModeIndex)].pTagName        = "mode_index";
                    m_pvtVendorTags[static_cast<UINT>(VendorTag::SensorBpsGain)].pSectionName         = "com.qti.sensorbps";
                    m_pvtVendorTags[static_cast<UINT>(VendorTag::SensorBpsGain)].pTagName             = "gain";
                    m_pvtVendorTags[static_cast<UINT>(VendorTag::DebugDataTag)].pSectionName
                        = "org.quic.camera.debugdata";
                    m_pvtVendorTags[static_cast<UINT>(VendorTag::DebugDataTag)].pTagName              = "DebugDataAll";
                    m_pvtVendorTags[static_cast<UINT>(VendorTag::SensorModeIndex)].pSectionName
                        = "org.codeaurora.qcamera3.sensor_meta_data";
                    m_pvtVendorTags[static_cast<UINT>(VendorTag::SensorModeIndex)].pTagName           = "sensor_mode_index";
                    m_pvtVendorTags[static_cast<UINT>(VendorTag::CropRegions)].pSectionName           = "com.qti.cropregions";
                    m_pvtVendorTags[static_cast<UINT>(VendorTag::CropRegions)].pTagName               = "crop_regions";
                    m_pvtVendorTags[static_cast<UINT>(VendorTag::TuningMode)].pSectionName
                        = "org.quic.camera2.tuning.mode";
                    m_pvtVendorTags[static_cast<UINT>(VendorTag::TuningMode)].pTagName                = "TuningMode";
                    m_pvtVendorTags[static_cast<UINT>(VendorTag::RefCropSize)].pSectionName
                        = "org.quic.camera2.ref.cropsize";
                    m_pvtVendorTags[static_cast<UINT>(VendorTag::RefCropSize)].pTagName               = "RefCropSize";
                    m_pvtVendorTags[static_cast<UINT>(VendorTag::MultiCamera)].pSectionName
                        = "com.qti.chi.multicamerainfo";
                    m_pvtVendorTags[static_cast<UINT>(VendorTag::MultiCamera)].pTagName               = "MultiCameraIds";
                    m_pvtVendorTags[static_cast<UINT>(VendorTag::IsFlashRequiredTag)].pSectionName    = "com.qti.stats_control";
                    m_pvtVendorTags[static_cast<UINT>(VendorTag::IsFlashRequiredTag)].pTagName        = "is_flash_snapshot";
                    m_pvtVendorTags[static_cast<UINT>(VendorTag::Feature1Mode)].pSectionName
                        = "org.quic.camera2.tuning.feature";
                    m_pvtVendorTags[static_cast<UINT>(VendorTag::Feature1Mode)].pTagName              = "Feature1Mode";
                    m_pvtVendorTags[static_cast<UINT>(VendorTag::Feature2Mode)].pSectionName
                        = "org.quic.camera2.tuning.feature";
                    m_pvtVendorTags[static_cast<UINT>(VendorTag::Feature2Mode)].pTagName              = "Feature2Mode";
                    m_pvtVendorTags[static_cast<UINT>(VendorTag::VideoHDR10Mode)].pSectionName
                        = "org.quic.camera2.streamconfigs";
                    m_pvtVendorTags[static_cast<UINT>(VendorTag::VideoHDR10Mode)].pTagName            = "HDRVideoMode";
                    m_pvtVendorTags[static_cast<UINT>(VendorTag::StatsSkipMode)].pSectionName         = "com.qti.chi.statsSkip";
                    m_pvtVendorTags[static_cast<UINT>(VendorTag::StatsSkipMode)].pTagName             = "skipFrame";
                    m_pvtVendorTags[static_cast<UINT>(VendorTag::BurstFps)].pSectionName
                        = "org.quic.camera.BurstFPS";
                    m_pvtVendorTags[static_cast<UINT>(VendorTag::BurstFps)].pTagName                  = "burstfps";
                    m_pvtVendorTags[static_cast<UINT>(VendorTag::CustomNoiseReduction)].pSectionName
                        = "org.quic.camera.CustomNoiseReduction";
                    m_pvtVendorTags[static_cast<UINT>(VendorTag::CustomNoiseReduction)].pTagName      = "CustomNoiseReduction";
                    m_pvtVendorTags[static_cast<UINT>(VendorTag::FastShutterMode)].pSectionName
                        = "org.quic.camera.SensorModeFS";
                    m_pvtVendorTags[static_cast<UINT>(VendorTag::FastShutterMode)].pTagName           = "SensorModeFS";
                    m_pvtVendorTags[static_cast<UINT>(VendorTag::IFEMaxWidth)].pSectionName
                        = "org.quic.camera.HWResourceInfo";
                    m_pvtVendorTags[static_cast<UINT>(VendorTag::IFEMaxWidth)].pTagName               = "IFEMaxLineWidth";
                    m_pvtVendorTags[static_cast<UINT>(VendorTag::IHDRControl)].pSectionName           = "com.qti.insensor_control";
                    m_pvtVendorTags[static_cast<UINT>(VendorTag::IHDRControl)].pTagName               = "seamless_insensor_state";
                    m_pvtVendorTags[static_cast<UINT>(VendorTag::HistNodeLTCRatioIndex)].pSectionName
                        = "org.quic.camera2.statsconfigs";
                    m_pvtVendorTags[static_cast<UINT>(VendorTag::HistNodeLTCRatioIndex)].pTagName     = "HistNodeLTCRatioIndex";
                    m_pvtVendorTags[static_cast<UINT>(VendorTag::ZSLTimestampRange)].pSectionName
                        = "com.qti.chi.ZSLSettings";
                    m_pvtVendorTags[static_cast<UINT>(VendorTag::ZSLTimestampRange)].pTagName         = "ZSLTimeRange";
                    m_pvtVendorTags[static_cast<UINT>(VendorTag::IPEOverrideScale)].pSectionName
                        = "org.quic.camera.overrideIPEScaleProfile";
                    m_pvtVendorTags[static_cast<UINT>(VendorTag::IPEOverrideScale)].pTagName
                        = "OverrideIPEScaleProfile";
                    m_pvtVendorTags[static_cast<UINT>(VendorTag::LivePreview)].pSectionName
                        = "com.qti.chi.livePreview";
                    m_pvtVendorTags[static_cast<UINT>(VendorTag::LivePreview)].pTagName
                        = "enable";
                    m_pvtVendorTags[static_cast<UINT>(VendorTag::DebugDumpConfig)].pSectionName
                        = "org.quic.camera.debugDumpConfig";
                    m_pvtVendorTags[static_cast<UINT>(VendorTag::DebugDumpConfig)].pTagName           = "DebugDumpConfig";
                    m_pvtVendorTags[static_cast<UINT>(VendorTag::GPUOverrideRotation)].pSectionName
                        = "org.quic.camera.overrideGPURotationUsecase";
                    m_pvtVendorTags[static_cast<UINT>(VendorTag::GPUOverrideRotation)].pTagName
                        = "OverrideGPURotationUsecase";
                    m_pvtVendorTags[static_cast<UINT>(VendorTag::T2TConfigRegROI)].pSectionName
                        = "org.quic.camera2.objectTrackingConfig";
                    m_pvtVendorTags[static_cast<UINT>(VendorTag::T2TConfigRegROI)].pTagName
                        = "RegisterROI";
                    m_pvtVendorTags[static_cast<UINT>(VendorTag::T2TResultROI)].pSectionName
                        = "org.quic.camera2.objectTrackingResults";
                    m_pvtVendorTags[static_cast<UINT>(VendorTag::T2TResultROI)].pTagName
                        = "ResultROI";

                    for (UINT32 id = 0; id < MaxNumImageSensors; id++)
                    {
                        m_pStreamConfig[id]    = NULL;
                        m_pDefaultSettings[id] = NULL;
                    }
                    // Go thru each tag and get the tag id from the driver
                    for (UINT index = 0; index < m_numVendorTags; index++)
                    {
                        CDKResult result = vendorTagOps.pQueryVendorTagLocation(m_pvtVendorTags[index].pSectionName,
                                                                                m_pvtVendorTags[index].pTagName,
                                                                                &m_pvtVendorTags[index].tagId);
                        if (CDKResultSuccess == result)
                        {
                            CHX_LOG("Vendor Tag %d: Section %s, TagName %s, TagId: 0x%x",
                                index,
                                m_pvtVendorTags[index].pSectionName,
                                m_pvtVendorTags[index].pTagName,
                                m_pvtVendorTags[index].tagId);
                        }
                        else
                        {
                            CHX_LOG("Failed to find TagId: %d, Section %s, TagName %s",
                                index,
                                m_pvtVendorTags[index].pSectionName,
                                m_pvtVendorTags[index].pTagName);
                        }
                    }
                }

                g_chiContextOps.pGetBufferManagerOps(&g_chiBufferManagerOps);
            }
        }
    }

    // Postproc service needs to be bring up along with Camera server
    RegisterIPostProcService();

}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ExtensionModule::~ExtensionModule
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
ExtensionModule::~ExtensionModule()
{
    m_pUsecaseSelector->Destroy();
    m_pUsecaseSelector = NULL;

    m_terminateOfflineLogThread = TRUE;

    for (UINT i = 0; i < MaxNumImageSensors; i++)
    {
        if (NULL != m_pPerfLockManager[i])
        {
            m_pPerfLockManager[i]->Destroy();
            m_pPerfLockManager[i] = NULL;
        }
    }

    if (NULL != m_pBLMClient)
    {
        m_pBLMClient->Destroy();
    }
    m_chiFenceOps = {NULL};

    g_chiContextOps.pCloseContext(m_hCHIContext);
    m_hCHIContext = NULL;

    for (UINT i = 0; i < m_numLogicalCameras; i++)
    {
        // if physical camera numbers is bigger than 1, it should be multi camera.
        // and the static meta is allocated in chiextension module. Free it when
        // exit
        if (1 < m_logicalCameraInfo[i].numPhysicalCameras)
        {
            if (NULL != m_logicalCameraInfo[i].m_cameraInfo.static_camera_characteristics)
            {
                free_camera_metadata(
                    const_cast<camera_metadata_t*>(m_logicalCameraInfo[i].m_cameraInfo.static_camera_characteristics));
                m_logicalCameraInfo[i].m_cameraInfo.static_camera_characteristics = NULL;
            }
        }

        // if it is not physical camera, the sensor mode is from physical camera which is not allocated.
        if (1 == m_logicalCameraInfo[i].numPhysicalCameras)
        {
            if (NULL != m_logicalCameraInfo[i].pSensorModeInfo)
            {
                CHX_FREE(m_logicalCameraInfo[i].pSensorModeInfo);
                m_logicalCameraInfo[i].pSensorModeInfo = NULL;
            }
        }

        if (NULL != m_logicalCameraInfo[i].ppDeviceInfo)
        {
            // if logical camera is invalid, the resource for logical camera is not allocated.
            for (UINT j = 0; j < m_logicalCameraInfo[i].numPhysicalCameras; j++)
            {
                if (NULL != m_logicalCameraInfo[i].ppDeviceInfo[j])
                {
                    CHX_FREE(m_logicalCameraInfo[i].ppDeviceInfo[j]);
                    m_logicalCameraInfo[i].ppDeviceInfo[j] = NULL;
                }
            }

            CHX_FREE(m_logicalCameraInfo[i].ppDeviceInfo);
            m_logicalCameraInfo[i].ppDeviceInfo = NULL;
        }

        if (NULL != m_logicalCameraInfo[i].m_cameraInfo.conflicting_devices)
        {
            for (UINT devIndex = 0; devIndex < m_logicalCameraInfo[i].m_cameraInfo.conflicting_devices_length; ++devIndex)
            {
                if (NULL != m_logicalCameraInfo[i].m_cameraInfo.conflicting_devices[devIndex])
                {
                    CHX_FREE(m_logicalCameraInfo[i].m_cameraInfo.conflicting_devices[devIndex]);
                    m_logicalCameraInfo[i].m_cameraInfo.conflicting_devices[devIndex] = NULL;
                }
            }

            CHX_FREE(m_logicalCameraInfo[i].m_cameraInfo.conflicting_devices);
            m_logicalCameraInfo[i].m_cameraInfo.conflicting_devices = NULL;
        }

        if (NULL != m_logicalCameraInfo[i].pAvailableStreamMap)
        {
            CHX_FREE(m_logicalCameraInfo[i].pAvailableStreamMap);
            m_logicalCameraInfo[i].pAvailableStreamMap = NULL;
        }
    }

    if (NULL != m_pConfigSettings)
    {
        CHX_FREE(m_pConfigSettings);
        m_pConfigSettings = NULL;
    }

    for (UINT32 id = 0; id < MaxNumImageSensors; id++)
    {
        if (NULL != m_pStreamConfig[id])
        {
            if (NULL != m_pStreamConfig[id]->streams)
            {
                CHX_FREE(m_pStreamConfig[id]->streams);
                m_pStreamConfig[id]->streams = NULL;
            }
            if (NULL != m_pStreamConfig[id]->session_parameters)
            {
                free_camera_metadata(const_cast<camera_metadata_t*>(m_pStreamConfig[id]->session_parameters));
            }
            CHX_FREE(m_pStreamConfig[id]);
            m_pStreamConfig[id] = NULL;
        }

#if defined (_LINUX)
        if (m_pDefaultSettings[id] != NULL)
        {
            free_camera_metadata(m_pDefaultSettings[id]);
            m_pDefaultSettings[id] = NULL;
        }
#endif
    }

    // Destroy offlinelogger mutex and conditional vars
    for (UINT type = 0; type < OFFLINELOG_MAX_TYPE; type++)
    {
        if (NULL != m_pFlushNeededCond[type])
        {
            m_pFlushNeededCond[type]->Destroy();
            m_pFlushNeededCond[type] = NULL;
        }
        if (NULL != m_pFlushNeededMutex[type])
        {
            m_pFlushNeededMutex[type]->Destroy();
            m_pFlushNeededMutex[type] = NULL;
        }
    }

    if (NULL != m_pLogicalCameraConfigurationInfo)
    {
        CHX_FREE(m_pLogicalCameraConfigurationInfo);
        m_pLogicalCameraConfigurationInfo = NULL;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ExtensionModule::SetHALOps
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID ExtensionModule::SetHALOps(
    uint32_t             logicalCameraId,
    const chi_hal_ops_t* pHalOps)
{
    if (NULL != pHalOps)
    {
        m_HALOps[logicalCameraId].process_capture_result = pHalOps->process_capture_result;
        m_HALOps[logicalCameraId].notify_result = pHalOps->notify_result;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ExtensionModule::SetCHIContextOps
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID ExtensionModule::SetCHIContextOps()
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ExtensionModule::RemapCameraId
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UINT32 ExtensionModule::RemapCameraId(
    UINT32              frameworkCameraId,
    CameraIdRemapMode   mode)
{
    (VOID)mode;

    UINT32 cameraId = GetCameraIdIndex(frameworkCameraId);

    if (INVALID_INDEX != cameraId)
    {
        switch (mode)
        {
            case IdRemapCamera:
                frameworkCameraId = cameraId;
                break;

            case IdReverseMapCamera:
                frameworkCameraId = m_cameraReverseMap[cameraId];
                break;

            case IdRemapTorch:
                UINT32 pPrimaryCameraIndex = GetPrimaryCameraIndex(&m_logicalCameraInfo[cameraId]);
                frameworkCameraId          = m_logicalCameraInfo[cameraId].ppDeviceInfo[pPrimaryCameraIndex]->cameraId;
                break;
        }
    }

    return frameworkCameraId;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ExtensionModule::ExtendOpen
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ExtensionModule::ExtendOpen(
    uint32_t  logicalCameraId,
    VOID*     pPriv)
{
    // The ExtensionModule has been initialized, and if there is any work needed to be done during HAL open, this is where
    // to add that code

    // sample code...this uses the input to create a data structure used to hold configuration settings
    ChiOverrideExtendSettings* pExtend = static_cast<ChiOverrideExtendSettings*>(pPriv);
    CDKResult  result              = CDKResultSuccess;

    if ((NULL == m_pConfigSettings) || (pExtend->numTokens > MaxConfigSettings))
    {
        CHX_LOG_ERROR("ExtendOpen failed! m_pConfigSettings=%p, numTokens=%d MaxConfigSettings=%d",
                      m_pConfigSettings, pExtend->numTokens, MaxConfigSettings);
        result = CDKResultEInvalidArg;
    }

    if (CDKResultSuccess == result)
    {
        m_pResourcesUsedLock->Lock();
        if (!m_logicalCameraRCVBypassSet.empty())
        {
            CHX_LOG_INFO("[CHI_RC_DEBUG] Bypass resourceCost for logical cameraId %u", logicalCameraId);
        }
        else
        {
            UINT32 activeResourceCost = GetActiveResourceCost();

            if ((m_logicalCameraInfo[logicalCameraId].maxResourceCost + activeResourceCost) > m_totalResourceBudget)
            {
                CHX_LOG_ERROR("ExtendOpen failed! for cameraId %d HW resource insufficient! maxresourceCost=%d"
                    "activeResourseCost =%d, m_totalResourceBudget = %d", logicalCameraId,
                    m_logicalCameraInfo[logicalCameraId].maxResourceCost,
                    activeResourceCost, m_totalResourceBudget);
                result = CamxResultETooManyUsers; // over capacity
            }
        }
        m_pResourcesUsedLock->Unlock();
    }

    if (CDKResultSuccess == result)
    {
        if (NULL == m_pPerfLockManager[logicalCameraId])
        {
            m_pPerfLockManager[logicalCameraId] = PerfLockManager::Create();
            if (NULL == m_pPerfLockManager[logicalCameraId])
            {
                CHX_LOG_ERROR("Failed to create perflock manager %d", logicalCameraId);
            }
        }

        if (NULL != m_pPerfLockManager[logicalCameraId])
        {
            m_pPerfLockManager[logicalCameraId]->AcquirePerfLock(PERF_LOCK_OPEN_CAMERA, 1000);
        }

        MappingConfigSettings(pExtend->numTokens, static_cast<VOID*>(pExtend->pTokens));

        // Update camera open and close status in static settings
        for (UINT8 index = 0; index < m_logicalCameraInfo[logicalCameraId].numPhysicalCameras; index++)
        {
            UINT32 cameraId = m_logicalCameraInfo[logicalCameraId].ppDeviceInfo[index]->cameraId;
            *m_pOverrideCameraOpen     |=  (1 << cameraId);
            *m_pOverrideCameraClose    &= ~(1 << cameraId);
        }

        if(CDKResultSuccess == result)
        {
            if ((EnableBLMClient()) && (NULL == m_pBLMClient))
            {
                m_pBLMClient = CHXBLMClient::Create();
                if (m_pBLMClient != NULL)
                {
                    CHX_LOG_INFO("BLM Client Created");
                }
                else
                {
                    CHX_LOG_ERROR("BLM Client creation failed");
                }
            }
        }

        m_pResourcesUsedLock->Lock();
        m_IFEResourceCost[logicalCameraId] = m_logicalCameraInfo[logicalCameraId].maxResourceCost;
        m_pResourcesUsedLock->Unlock();

        CHX_LOG_INFO("Open Logical cameraId: %d  numPhysicalCameras: %d"
                     "CameraOpen Mask = 0x%x CameraClose Mask 0x%x maxresourceCost %d",
                    logicalCameraId, m_logicalCameraInfo[logicalCameraId].numPhysicalCameras,
                    *m_pOverrideCameraOpen, *m_pOverrideCameraClose,
                    m_logicalCameraInfo[logicalCameraId].maxResourceCost);

    }
    return result;

}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ExtensionModule::ExtendClose
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID ExtensionModule::ExtendClose(
    uint32_t  logicalCameraId,
    VOID*     pPriv)
{
    // The ExtensionModule has been initialized, and if there is any work needed to be done during HAL close, this is where
    // to add that code

    if (NULL != m_pPerfLockManager[logicalCameraId])
    {
        m_pPerfLockManager[logicalCameraId]->AcquirePerfLock(PERF_LOCK_CLOSE_CAMERA, 1000);
    }

    // sample code...this uses the input to create a data structure used to hold configuration settings
    ChiOverrideExtendClose* pExtend = static_cast<ChiOverrideExtendClose*>(pPriv);

    if ((NULL == m_pConfigSettings) || (pExtend->numTokens > MaxConfigSettings))
    {
        CHX_LOG_ERROR("ExtendClose failed! m_pConfigSettings=%p, numTokens=%d MaxConfigSettings=%d",
                      m_pConfigSettings, pExtend->numTokens, MaxConfigSettings);
        return;
    }

    MappingConfigSettings(pExtend->numTokens, static_cast<VOID*>(pExtend->pTokens));

    for (UINT8 index = 0; index < m_logicalCameraInfo[logicalCameraId].numPhysicalCameras; index++)
    {
        UINT32 cameraId = m_logicalCameraInfo[logicalCameraId].ppDeviceInfo[index]->cameraId;
        *m_pOverrideCameraOpen    &= ~(1 << cameraId);
        *m_pOverrideCameraClose   |=  (1 << cameraId);
    }

    m_pResourcesUsedLock->Lock();
    m_logicalCameraRCVBypassSet.erase(logicalCameraId);
    if (m_logicalCameraRCVBypassSet.empty())
    {
        CHX_LOG_INFO("[CHI_RC_DEBUG] Reset resource cost validation bypass %d", logicalCameraId);
    }
    m_pResourcesUsedLock->Unlock();

    ResetResourceCost(m_logicalCameraInfo[logicalCameraId].cameraId);

    CHX_LOG_INFO("Close Logical cameraId: %d  numPhysicalCameras: %d CameraOpen Mask = 0x%x  CameraClose Mask 0x%x ",
        logicalCameraId, m_logicalCameraInfo[logicalCameraId].numPhysicalCameras,
        *m_pOverrideCameraOpen, *m_pOverrideCameraClose);

    FreeLastKnownRequestSetting(logicalCameraId);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ExtensionModule::SearchNumBatchedFrames
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID ExtensionModule::SearchNumBatchedFrames(
    uint32_t                        cameraId,
    camera3_stream_configuration_t* pStreamConfigs,
    UINT*                           pBatchSize,
    BOOL*                           pHALOutputBufferCombined,
    UINT*                           pFPSValue,
    UINT                            maxSessionFps)
{
    INT32 width  = 0;
    INT32 height = 0;

    // We will take the following steps -
    //  1) We will search SupportedHFRVideoSizes, for the matching Video/Preview stream.
    //     Note: For the use case of multiple output streams, application must select one unique size from this metadata
    //           to use (e.g., preview and recording streams must have the same size). Otherwise, the high speed capture
    //           session creation will fail
    //  2) If a single entry is found in SupportedHFRVideoSizes, we choose the batchsize from that entry

    for (UINT streamIndex = 0; streamIndex < pStreamConfigs->num_streams; streamIndex++)
    {
        if (CAMERA3_STREAM_OUTPUT == pStreamConfigs->streams[streamIndex]->stream_type)
        {
            width  = pStreamConfigs->streams[streamIndex]->width;
            height = pStreamConfigs->streams[streamIndex]->height;
            break;
        }
    }

    SIZE_T numDefaultHFRVideoSizes    = 0;
    /// @todo Implement metadata merging
    camera_metadata_entry_t entry   = { 0 };

    entry.tag = ANDROID_CONTROL_AVAILABLE_HIGH_SPEED_VIDEO_CONFIGURATIONS;

#if defined (_LINUX)
    find_camera_metadata_entry((camera_metadata_t*)(
        m_logicalCameraInfo[cameraId].m_cameraInfo.static_camera_characteristics), entry.tag, &entry);
#endif // _LINUX

    numDefaultHFRVideoSizes = entry.count;

    const HFRConfigurationParams* pHFRParams[MaxHFRConfigs] = { NULL };
    HFRConfigurationParams*       pDefaultHFRVideoSizes     = reinterpret_cast<HFRConfigurationParams*>(entry.data.i32);
    UINT                          numHFREntries             = 0;

    if ((0 != width) && (0 != height))
    {
        for (UINT i = 0; i < numDefaultHFRVideoSizes; i++)
        {
            if ((pDefaultHFRVideoSizes[i].width == width) && (pDefaultHFRVideoSizes[i].height == height))
            {
                // Check if maxSessionFps is non-zero, otherwise default it to HFR size max fps
                if ((maxSessionFps != 0) && (maxSessionFps != (UINT)pDefaultHFRVideoSizes[i].maxFPS))
                {
                    // Go to next entry
                    continue;
                }
                // Out of the pair of entries in the table, we would like to store the second entry
                pHFRParams[numHFREntries++] = &pDefaultHFRVideoSizes[i + 1];
                // Make sure that we don't hit the other entry in the pair, again
                i++;
            }
        }

        if (numHFREntries > 0)
        {
            if (pHFRParams[0]->batchSizeMax == 1)
            {
                *pBatchSize = pHFRParams[0]->maxFPS / 60; // recalculate the batchzie for superbuffer mode
                *pHALOutputBufferCombined = TRUE;
            }
            else
            {
                *pBatchSize = pHFRParams[0]->batchSizeMax;
                *pHALOutputBufferCombined = FALSE;
            }
            *pFPSValue  = pHFRParams[0]->maxFPS;
            CHX_LOG("HFR entry batch size HALOutputBufferCombined fps min max (%d %d %d %d)",
                    pHFRParams[0]->batchSizeMax,
                    *pHALOutputBufferCombined,
                    pHFRParams[0]->minFPS,
                    pHFRParams[0]->maxFPS);
        }
        else
        {
            CHX_LOG_ERROR("Failed to find supported HFR entry");
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ExtensionModule::GetInfo
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ExtensionModule::GetInfo(
   CDKGetInfoCmd       infoCmd,
   void*               pInputParams,
   void*               pOutputParams)
{
    CDKResult result = CDKResultEInvalidArg;

    if ((CDKGetInfoMax > infoCmd) &&
        (NULL != pInputParams)     &&
        (NULL != pOutputParams))
    {
        switch (infoCmd)
        {
            case CDKGetInfoNumPhysicalCameras:
            {
                UINT32 cameraId    = (static_cast<CDKInfoCameraId*>(pInputParams))->cameraId;
                UINT32 cameraIndex = GetCameraIdIndex(cameraId);

                if (INVALID_INDEX != cameraIndex)
                {
                    (static_cast<CDKInfoNumCameras*>(pOutputParams))->numCameras =
                        m_logicalCameraInfo[cameraId].numPhysicalCameras;
                    result = CDKResultSuccess;
                }
                else
                {
                    CHX_LOG_ERROR("Invalid logical cameraId=%u", cameraId);
                }
            }
            break;

            case CDKGetInfoPhysicalCameraIds:
            {
                CDKInfoPhysicalCameraIds* pPhysIds = static_cast<CDKInfoPhysicalCameraIds*>(pOutputParams);
                if (NULL != pPhysIds->physicalCameraIds)
                {
                    UINT32 cameraId    = (static_cast<CDKInfoCameraId*>(pInputParams))->cameraId;
                    UINT32 cameraIndex = GetCameraIdIndex(cameraId);

                    if (INVALID_INDEX != cameraIndex)
                    {

                        for (UINT32 camIdx = 0; camIdx < m_logicalCameraInfo[cameraId].numPhysicalCameras; camIdx++)
                        {
                            pPhysIds->physicalCameraIds[camIdx] = m_logicalCameraInfo[cameraId].ppDeviceInfo[camIdx]->cameraId;
                        }
                        pPhysIds->numCameras = m_logicalCameraInfo[cameraId].numPhysicalCameras;
                        result               = CDKResultSuccess;
                    }
                    else
                    {
                        CHX_LOG_ERROR("Invalid logical cameraId=%u", cameraId);
                    }
                }
                else
                {
                    CHX_LOG_ERROR("pPhysIds->physicalCameraIds is NULL");
                }
            }
            break;

            default:
                break;
        }
    }
    else
    {
        CHX_LOG_ERROR("Invalid Params: infoCmd=%u inputParams=%p outputParams=%p", infoCmd, pInputParams, pOutputParams);
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ExtensionModule::GetCameraIdIndex
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UINT32 ExtensionModule::GetCameraIdIndex(
    UINT32 logicalCameraId) const
{
    UINT32 index = INVALID_INDEX;

    for (UINT32 i = 0; i < m_numLogicalCameras; i++)
    {
        if (m_cameraMap[i] == logicalCameraId)
        {
            index = i;
            break;
        }
    }

    CHX_LOG_INFO("AppId => LogicalId:%d  =>  %d",logicalCameraId, index);
    return index;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ExtensionModule::GetPrimaryCameraIndex
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UINT32 ExtensionModule::GetPrimaryCameraIndex(const LogicalCameraInfo * pLogicalCameraInfo)
{
    UINT32 primaryCameraId     = pLogicalCameraInfo->primaryCameraId;
    UINT32 primaryCameraIndex  = INVALID_INDEX;

    for (UINT32 i = 0; i < pLogicalCameraInfo->numPhysicalCameras; i++)
    {
        if (primaryCameraId == pLogicalCameraInfo->ppDeviceInfo[i]->cameraId)
        {
            primaryCameraIndex = i;
            break;
        }
    }

    return primaryCameraIndex;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ExtensionModule::GetNumCameras
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID ExtensionModule::GetNumCameras(
    UINT32* pNumFwCameras,
    UINT32* pNumLogicalCameras)
{

    if (0 == m_numLogicalCameras)
    {
        SortCameras();
        EnumerateCameras();
    }

    FillLogicalCameraCaps();

    *pNumLogicalCameras = m_numExposedLogicalCameras;
    *pNumFwCameras      = m_numExposedLogicalCameras;
    CHX_LOG("exposedNumOfCamera:%d, all:%d", m_numExposedLogicalCameras, m_numLogicalCameras);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ExtensionModule::GetPhysicalCameraInfo
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
const LogicalCameraInfo* ExtensionModule::GetPhysicalCameraInfo(
    UINT32 physicalCameraId
    )const
{
    const LogicalCameraInfo *pLogicalCameraInfo = NULL;

    for (UINT32 i = 0 ; i < m_numOfLogicalCameraConfiguration; i++)
    {
        if ((LogicalCameraType_Default != m_logicalCameraInfo[i].logicalCameraType))
        {
            continue;
        }

        if ((NULL != m_logicalCameraInfo[i].ppDeviceInfo) &&
            (m_logicalCameraInfo[i].ppDeviceInfo[0]->cameraId == physicalCameraId))
        {
            pLogicalCameraInfo = &m_logicalCameraInfo[i];
            break;
        }
    }

    return pLogicalCameraInfo;
}

CDKResult ExtensionModule::GetActiveArray(
    UINT32   cameraId,
    CHIRECT* pChiRect)
{
    CDKResult                result             = CDKResultSuccess;
    const LogicalCameraInfo* pLogicalCameraInfo = NULL;

    pLogicalCameraInfo = GetPhysicalCameraInfo(cameraId);

    if ((NULL != pLogicalCameraInfo) &&
        (NULL != pChiRect))
    {
        *pChiRect = pLogicalCameraInfo->m_cameraCaps.sensorCaps.activeArray;
    }
    else
    {
        CHX_LOG_ERROR("invalid Camera Id = %d pChiRect = %p", cameraId, pChiRect);
        result = CDKResultEInvalidArg;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ExtensionModule::FillLogicalCameraCaps
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ExtensionModule::FillLogicalCameraCaps()
{
    CDKResult result = CDKResultSuccess;

    for (UINT camIndex = 0; camIndex < m_numLogicalCameras; ++camIndex)
    {
        LogicalCameraInfo* pLogicalCam       = &(m_logicalCameraInfo[camIndex]);
        LogicalCameraType  logicalCameraType = GetCameraType(m_logicalCameraInfo[camIndex].cameraId);

        if ((1 < pLogicalCam->numPhysicalCameras) && (LogicalCameraType_Default != logicalCameraType))
        {
            UINT           physicalIds[MaxNumImageSensors];
            UINT           remappedPhysicalIds[MaxNumImageSensors];
            std::string    physicalIdString[MaxNumImageSensors];
            camera_info_t* pPhysicalCamInfo[MaxNumImageSensors];
            std::string    physicalIdAllString;

            camera_info_t* pLogicalCamInfo   = &pLogicalCam->m_cameraInfo;
            UINT           remappedLogicalId = m_cameraMap[pLogicalCam->cameraId];
            std::string    logicalIdString   = std::to_string(remappedLogicalId);

            pLogicalCamInfo->resource_cost       = 100;
            pLogicalCamInfo->conflicting_devices = static_cast<CHAR**>(CHX_CALLOC(
                pLogicalCam->numPhysicalCameras * sizeof(CHAR*)));

            if (NULL != pLogicalCamInfo->conflicting_devices)
            {
                for (UINT phyIndex = 0; phyIndex < pLogicalCam->numPhysicalCameras; ++phyIndex)
                {
                    physicalIds[phyIndex]         = pLogicalCam->ppDeviceInfo[phyIndex]->cameraId;
                    remappedPhysicalIds[phyIndex] = m_cameraReverseMap[physicalIds[phyIndex]];
                    physicalIdString[phyIndex]    = std::to_string(remappedPhysicalIds[phyIndex]);
                    physicalIdAllString           += " "  + physicalIdString[phyIndex];
                    pPhysicalCamInfo[phyIndex]    = &(m_logicalCameraInfo[physicalIds[phyIndex]].m_cameraInfo);

                    pPhysicalCamInfo[phyIndex]->conflicting_devices = static_cast<CHAR**>(CHX_CALLOC(sizeof(CHAR*)));
                    pLogicalCamInfo->conflicting_devices[phyIndex]  = static_cast<CHAR*>(
                        CHX_CALLOC(physicalIdString[phyIndex].size() + 1));

                    if ((NULL == pPhysicalCamInfo[phyIndex]->conflicting_devices) ||
                        (NULL == pLogicalCamInfo->conflicting_devices[phyIndex]))
                    {
                        CHX_LOG_ERROR("Logical camera %d Allocation of conflicting device failed", remappedLogicalId);
                        result =  CDKResultENoMemory;
                        break;
                    }
                    pPhysicalCamInfo[phyIndex]->conflicting_devices[0] = static_cast<CHAR*>(
                        CHX_CALLOC(logicalIdString.size() + 1));

                    if (NULL == pPhysicalCamInfo[phyIndex]->conflicting_devices[0])
                    {
                        CHX_LOG_ERROR("Logical camera %d Allocation of conflicting device for physical camera failed",
                            remappedLogicalId);
                        result =  CDKResultENoMemory;
                        break;
                    }
                    memcpy(pLogicalCamInfo->conflicting_devices[phyIndex], physicalIdString[phyIndex].c_str(),
                        physicalIdString[phyIndex].size());
                    memcpy(pPhysicalCamInfo[phyIndex]->conflicting_devices[0], logicalIdString.c_str(),
                        logicalIdString.size());
                    pPhysicalCamInfo[phyIndex]->conflicting_devices_length = 1;

                    CHX_LOG("Logical camera %d physicalId %d remapped physicalId %d res %d",
                        remappedLogicalId, physicalIds[phyIndex], remappedPhysicalIds[phyIndex], result);
                }

                CHX_LOG_CONFIG("Logical camera %d numPhysicalCameras %d, physicalIds %s res %d",
                    remappedLogicalId, pLogicalCam->numPhysicalCameras, physicalIdAllString.c_str(),
                    result);

                if (CDKResultSuccess == result)
                {
                    pLogicalCamInfo->conflicting_devices_length = pLogicalCam->numPhysicalCameras;

                    CHIHANDLE    hStaticMetaDataHandle = const_cast<camera_metadata_t*>(
                        pLogicalCamInfo->static_camera_characteristics);
                    camera_metadata_t* pCameraCharacteristics = const_cast<camera_metadata_t*>(
                        pLogicalCamInfo->static_camera_characteristics);

                    CHITAGSOPS   tagOps = { 0 };
                    g_chiContextOps.pTagOps(&tagOps);

                    CHAR logicalMultiCamPhysicalIds[MaxPhyIdsTagSize];
                    UINT logicalMultiCamPhysicalIdSize = 0;

                    for (UINT phyIndex = 0; phyIndex < pLogicalCam->numPhysicalCameras; ++phyIndex)
                    {
                        if (MaxPhyIdsTagSize > logicalMultiCamPhysicalIdSize + physicalIdString[phyIndex].size())
                        {
                            memcpy(logicalMultiCamPhysicalIds + logicalMultiCamPhysicalIdSize,
                                physicalIdString[phyIndex].c_str(),
                                physicalIdString[phyIndex].size());
                            logicalMultiCamPhysicalIdSize += physicalIdString[phyIndex].size();
                            *logicalMultiCamPhysicalIds = '\0';
                            logicalMultiCamPhysicalIdSize++;
                        }
                        else
                        {
                            result = CDKResultENeedMore;
                            break;
                        }
                    }

                    if (CDKResultSuccess == result)
                    {
                        tagOps.pSetMetaData(hStaticMetaDataHandle, ANDROID_LOGICAL_MULTI_CAMERA_PHYSICAL_IDS,
                            logicalMultiCamPhysicalIds, logicalMultiCamPhysicalIdSize);

                        BYTE sensorSyncType = 0;
                        tagOps.pSetMetaData(hStaticMetaDataHandle, ANDROID_LOGICAL_MULTI_CAMERA_SENSOR_SYNC_TYPE,
                            &sensorSyncType, sizeof(BYTE));

                        camera_metadata_entry_t entry = { 0 };

                        INT status = find_camera_metadata_entry(pCameraCharacteristics, ANDROID_REQUEST_AVAILABLE_CAPABILITIES,
                            &entry);
                        if (android::OK == status)
                        {
                            UINT8* pCapRequestKeys = entry.data.u8;

                            std::vector<UINT8> newCapRequestKeys(pCapRequestKeys, pCapRequestKeys + entry.count);

                            newCapRequestKeys.push_back(ANDROID_REQUEST_AVAILABLE_CAPABILITIES_LOGICAL_MULTI_CAMERA);

                            update_camera_metadata_entry(pCameraCharacteristics, entry.index,
                                 (const VOID *)newCapRequestKeys.data(), newCapRequestKeys.size(), NULL);
                        }

                        status = find_camera_metadata_entry(pCameraCharacteristics, ANDROID_INFO_SUPPORTED_HARDWARE_LEVEL, &entry);

                        if (android::OK == status)
                        {
                            INT hardwareLevel = ANDROID_INFO_SUPPORTED_HARDWARE_LEVEL_FULL;
                            status = update_camera_metadata_entry(pCameraCharacteristics, entry.index, &hardwareLevel,
                                entry.count, &entry);
                        }
                    }

                    CHX_LOG_INFO("Logical camera %d numPhysicalCameras %d, updated all logical camera characteristics result %d",
                        remappedLogicalId, pLogicalCam->numPhysicalCameras, result);
                }
            }
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ExtensionModule::swapCameras
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID ExtensionModule::SwapCameras(
    UINT32 srcCamera,
    UINT32 dstCamera)
{
    CHISENSORMODEINFO*              pSensorModeInfo;
    UINT32                          numPhysicalCameras;
    const camera3_device_t*         pCamera3Device;
    LogicalCameraType               logicalCameraType;
    UINT32                          primaryCameraId;
    BOOL                            publicVisiblity;
    camera_info                     cameraInfo;
    CHICAMERAINFO                   cameraCaps;
    DeviceInfo**                    ppDeviceInfo;
    UINT32                          cameraMap;
    UINT32                          cameraReverseMap;
    LogicalCameraConfiguration      pLogicalCameraConfigurationInfo;
    UINT32                          cameraID;
    PhysicalCameraConfiguration*    pDeviceConfig;
    CHICAMERAINFO*                  pDeviceCaps;
    VOID*                           pLegacy;

    if ((INVALID_INDEX != srcCamera) && (INVALID_INDEX != dstCamera)
        && (MaxNumImageSensors > srcCamera) && (MaxNumImageSensors > dstCamera))
    {
        pSensorModeInfo                                     = m_logicalCameraInfo[srcCamera].pSensorModeInfo;
        m_logicalCameraInfo[srcCamera].pSensorModeInfo      = m_logicalCameraInfo[dstCamera].pSensorModeInfo;
        m_logicalCameraInfo[dstCamera].pSensorModeInfo      = pSensorModeInfo;

        numPhysicalCameras                                  = m_logicalCameraInfo[srcCamera].numPhysicalCameras;
        m_logicalCameraInfo[srcCamera].numPhysicalCameras   = m_logicalCameraInfo[dstCamera].numPhysicalCameras;
        m_logicalCameraInfo[dstCamera].numPhysicalCameras   = numPhysicalCameras;

        pCamera3Device                                      = m_logicalCameraInfo[srcCamera].m_pCamera3Device;
        m_logicalCameraInfo[srcCamera].m_pCamera3Device     = m_logicalCameraInfo[dstCamera].m_pCamera3Device;
        m_logicalCameraInfo[dstCamera].m_pCamera3Device     = pCamera3Device;

        logicalCameraType                                   = m_logicalCameraInfo[srcCamera].logicalCameraType;
        m_logicalCameraInfo[srcCamera].logicalCameraType    = m_logicalCameraInfo[dstCamera].logicalCameraType;
        m_logicalCameraInfo[dstCamera].logicalCameraType    = logicalCameraType;

        primaryCameraId                                     = m_logicalCameraInfo[srcCamera].primaryCameraId;
        m_logicalCameraInfo[srcCamera].primaryCameraId      = m_logicalCameraInfo[dstCamera].primaryCameraId;
        m_logicalCameraInfo[dstCamera].primaryCameraId      = primaryCameraId;

        publicVisiblity                                     = m_logicalCameraInfo[srcCamera].publicVisiblity;
        m_logicalCameraInfo[srcCamera].publicVisiblity      = m_logicalCameraInfo[dstCamera].publicVisiblity;
        m_logicalCameraInfo[dstCamera].publicVisiblity      = publicVisiblity;

        cameraInfo                                          = m_logicalCameraInfo[srcCamera].m_cameraInfo;
        m_logicalCameraInfo[srcCamera].m_cameraInfo         = m_logicalCameraInfo[dstCamera].m_cameraInfo;
        m_logicalCameraInfo[dstCamera].m_cameraInfo         = cameraInfo;

        cameraCaps                                          = m_logicalCameraInfo[srcCamera].m_cameraCaps;
        m_logicalCameraInfo[srcCamera].m_cameraCaps         = m_logicalCameraInfo[dstCamera].m_cameraCaps;
        m_logicalCameraInfo[dstCamera].m_cameraCaps         = cameraCaps;

        cameraID                                                 = m_logicalCameraInfo[srcCamera].ppDeviceInfo[0]->cameraId;
        m_logicalCameraInfo[srcCamera].ppDeviceInfo[0]->cameraId = m_logicalCameraInfo[dstCamera].ppDeviceInfo[0]->cameraId;
        m_logicalCameraInfo[dstCamera].ppDeviceInfo[0]->cameraId = cameraID;

        pSensorModeInfo                                                 =
            const_cast<CHISENSORMODEINFO*>(m_logicalCameraInfo[srcCamera].ppDeviceInfo[0]->pSensorModeInfo);
        m_logicalCameraInfo[srcCamera].ppDeviceInfo[0]->pSensorModeInfo = m_logicalCameraInfo[dstCamera].ppDeviceInfo[0]->pSensorModeInfo;
        m_logicalCameraInfo[dstCamera].ppDeviceInfo[0]->pSensorModeInfo = pSensorModeInfo;

        CHICAMERAINFO*   srcpDeviceCaps                                 =
            const_cast<CHICAMERAINFO*>(m_logicalCameraInfo[srcCamera].ppDeviceInfo[0]->m_pDeviceCaps);
        CHICAMERAINFO*   dstpDeviceCaps                                 =
            const_cast<CHICAMERAINFO*>(m_logicalCameraInfo[dstCamera].ppDeviceInfo[0]->m_pDeviceCaps);
        pLegacy                                                         =
            m_logicalCameraInfo[srcCamera].ppDeviceInfo[0]->m_pDeviceCaps->pLegacy;
        srcpDeviceCaps->pLegacy                                         =
            m_logicalCameraInfo[dstCamera].ppDeviceInfo[0]->m_pDeviceCaps->pLegacy;
        dstpDeviceCaps->pLegacy                                         = pLegacy;

        pDeviceConfig                                                 = m_logicalCameraInfo[srcCamera].ppDeviceInfo[0]->pDeviceConfig;
        m_logicalCameraInfo[srcCamera].ppDeviceInfo[0]->pDeviceConfig = m_logicalCameraInfo[dstCamera].ppDeviceInfo[0]->pDeviceConfig;
        m_logicalCameraInfo[dstCamera].ppDeviceInfo[0]->pDeviceConfig = pDeviceConfig;

        cameraReverseMap                                    = m_cameraReverseMap[m_logicalCameraInfo[srcCamera].ppDeviceInfo[0]->cameraId];
        m_cameraReverseMap[m_logicalCameraInfo[srcCamera].ppDeviceInfo[0]->cameraId] =
            m_cameraReverseMap[m_logicalCameraInfo[dstCamera].ppDeviceInfo[0]->cameraId];
        m_cameraReverseMap[m_logicalCameraInfo[dstCamera].ppDeviceInfo[0]->cameraId] = cameraReverseMap;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ExtensionModule::SwapFrontAndRearMain
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID ExtensionModule::SwapFrontAndRearMain()
{
    UINT32 rearCameraIndex  = INVALID_INDEX;
    UINT32 frontCameraIndex = INVALID_INDEX;

    // If zeroth logical camera is not REAR then make REAR as 0 logical camera.
    if ((1 < m_numLogicalCameras) && (REAR != m_logicalCameraInfo[0].m_cameraCaps.sensorCaps.positionType))
    {
        for (UINT32 camID = 1; camID < m_numLogicalCameras; ++camID)
        {
            if (REAR == m_logicalCameraInfo[camID].m_cameraCaps.sensorCaps.positionType)
            {
                rearCameraIndex = camID;
                break;
            }
        }
        SwapCameras(0, rearCameraIndex);
    }

    // If 1st logical camera is not FRONT then make FRONT as 1st logical camera.
    if ((2 < m_numLogicalCameras) && (FRONT != m_logicalCameraInfo[1].m_cameraCaps.sensorCaps.positionType))
    {
        for (UINT32 camID = 2; camID < m_numLogicalCameras; ++camID)
        {
            if (FRONT == m_logicalCameraInfo[camID].m_cameraCaps.sensorCaps.positionType)
            {
                frontCameraIndex = camID;
                break;
            }
        }
        SwapCameras(1, frontCameraIndex);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ExtensionModule::BuildLogicalCameraInfo
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ExtensionModule::BuildLogicalCameraInfo(
        UINT32* sensorIdMap,
        BOOL    isMultiCameraEnabled,
        BOOL    isForExposed)
{
    CDKResult          result              = CDKResultSuccess;
    CHITAGSOPS         tagOps              = { 0 };
    UINT32             tag;
    LogicalCameraType  logicalCameraType   = LogicalCameraType_Default;
    UINT32             numOfPhysicalCamera = g_chiContextOps.pGetNumCameras(m_hCHIContext);

    LogicalCameraConfiguration *pLogicalCameraConf = NULL;
    LogicalCameraInfo          *pLogicalCameraInfo = NULL;
    UINT32                      sensorId           = INVALID_INDEX;
    UINT32                      physicalCameraId   = INVALID_INDEX;
    BOOL                        isValid            = TRUE;

    CHIHANDLE  staticMetaDataHandle = NULL;
    CDKResult  tagOpResult          = CDKResultEFailed;

    g_chiContextOps.pTagOps(&tagOps);
    ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// 1) Loop physical camera in configuration table
    ///       Get sensor mode and fill device information for every physical camera.
    /// 2) Loop non-physical camera in configuration table
    ///       Loop physical cameras for this logical camera
    ///           Fill device info for non-physical camera
    ///       Composite sensor mode for non-physical camera
    ///       Consolidate static metadata for non-physical camera
    ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    // Go through logical camera configuration to create logic camera for physical camera. And get sensor mode
    // information and camera capability
    for (UINT32 i = 0 ; i < m_numOfLogicalCameraConfiguration; i++)
    {
        pLogicalCameraConf = &m_pLogicalCameraConfigurationInfo[i];

        if (isForExposed != pLogicalCameraConf->publicVisibility)
        {
            continue;
        }

        if (LogicalCameraType_Default != pLogicalCameraConf->logicalCameraType)
        {
            continue;
        }

        pLogicalCameraInfo                     = &m_logicalCameraInfo[m_numLogicalCameras];
        // use logical camera create sequence id as camera id
        pLogicalCameraInfo->cameraId           = m_numLogicalCameras;

        pLogicalCameraInfo->logicalCameraType  = static_cast<LogicalCameraType>(
            pLogicalCameraConf->logicalCameraType);
        pLogicalCameraInfo->numPhysicalCameras = pLogicalCameraConf->numOfDevices;
        CHICAMERAINFO* pChiCameraInfo          = &(pLogicalCameraInfo->m_cameraCaps);
        pChiCameraInfo->pLegacy                = &(pLogicalCameraInfo->m_cameraInfo);

        sensorId                               = pLogicalCameraConf->deviceInfo[0].sensorId;
        isValid                                = TRUE;

        CHX_LOG("index:%d,logical cameraid:%d,physicalCameraId:%d,type:%d,publicvisiblity:%d",
            i, pLogicalCameraConf->logicalCameraId, pLogicalCameraConf->deviceInfo[0].sensorId,
            pLogicalCameraConf->logicalCameraType, pLogicalCameraConf->publicVisibility);

        if (MaxNumImageSensors <= sensorId)
        {
            CHX_LOG_ERROR("Invalid sensor id! please check if m_pLogicalCameraConfigurationInfo[%d] is valid!", i);
            isValid = FALSE;
            return CDKResultEOutOfBounds;
        }

        physicalCameraId = sensorIdMap[sensorId];
        if (physicalCameraId == INVALID_INDEX)
        {
            CHX_LOG_WARN("Please check if sensor id:%d is probed successfully!", sensorId);
            continue;
        }

        result = g_chiContextOps.pGetCameraInfo(m_hCHIContext, physicalCameraId,
            pChiCameraInfo);

        if (CDKResultSuccess == result)
        {
            camera_metadata_ro_entry_t entry   = { 0 };
            const camera_metadata_t* pMetadata =
                (static_cast<camera_info_t*>(pChiCameraInfo->pLegacy))->static_camera_characteristics;

            if (0 == find_camera_metadata_ro_entry(pMetadata, ANDROID_REQUEST_PARTIAL_RESULT_COUNT, &entry))
            {
                UINT32 value = static_cast<UINT32>(*(entry.data.i32));
                m_numMetadataResults = (m_numMetadataResults > value) ? m_numMetadataResults : value;
            }

            pLogicalCameraInfo->primaryCameraId = sensorIdMap[pLogicalCameraConf->primarySensorID];
            if (pLogicalCameraInfo->primaryCameraId == INVALID_INDEX)
            {
                CHX_LOG_WARN("Invalid Primary Cam Id. Check if sensor id:%d is configured right!", sensorId);
                isValid = FALSE;
                continue;
            }

            pLogicalCameraInfo->pSensorModeInfo =
                static_cast<CHISENSORMODEINFO*>(CHX_CALLOC(sizeof(CHISENSORMODEINFO) * pChiCameraInfo->numSensorModes));

            if (NULL != pLogicalCameraInfo->pSensorModeInfo)
            {
                result = EnumerateSensorModes(physicalCameraId,
                                                pChiCameraInfo->numSensorModes,
                                                pLogicalCameraInfo->pSensorModeInfo);

                if (CDKResultSuccess == result)
                {
                    pLogicalCameraInfo->ppDeviceInfo =
                        static_cast<DeviceInfo**>(CHX_CALLOC(sizeof(DeviceInfo *) * pLogicalCameraConf->numOfDevices));

                    if (NULL != pLogicalCameraInfo->ppDeviceInfo)
                    {
                        pLogicalCameraInfo->ppDeviceInfo[0] =
                                        static_cast<DeviceInfo*>(CHX_CALLOC(sizeof(DeviceInfo)));

                        if (NULL != pLogicalCameraInfo->ppDeviceInfo[0])
                        {
                            pLogicalCameraInfo->ppDeviceInfo[0]->cameraId        = physicalCameraId;
                            pLogicalCameraInfo->ppDeviceInfo[0]->pSensorModeInfo = pLogicalCameraInfo->pSensorModeInfo;
                            pLogicalCameraInfo->ppDeviceInfo[0]->m_pDeviceCaps   = &(pLogicalCameraInfo->m_cameraCaps);
                            pLogicalCameraInfo->ppDeviceInfo[0]->pDeviceConfig   = &(pLogicalCameraConf->deviceInfo[0]);
                        }
                        else
                        {
                            CHX_LOG_ERROR("Out of memory:pLogicalCameraInfo->ppDeviceInfo[0] allocate failed!");
                            isValid = FALSE;
                            result = CDKResultENoMemory;
                        }
                    }
                    else
                    {
                        CHX_LOG_ERROR("Out of memory:pLogicalCameraInfo->ppDeviceInfo allocate failed!");
                        if (NULL != pLogicalCameraInfo->pSensorModeInfo)
                        {
                            CHX_FREE(pLogicalCameraInfo->pSensorModeInfo);
                            pLogicalCameraInfo->pSensorModeInfo = NULL;
                        }
                        isValid = FALSE;
                        result = CDKResultENoMemory;
                    }
                }
                else
                {
                    CHX_LOG_ERROR("EnumerateSensorModes failed:physical cameraID:%d,numofModes:%d",
                        physicalCameraId,
                        pChiCameraInfo->numSensorModes);
                    if (NULL != pLogicalCameraInfo->pSensorModeInfo)
                    {
                        CHX_FREE(pLogicalCameraInfo->pSensorModeInfo);
                        pLogicalCameraInfo->pSensorModeInfo = NULL;
                    }
                    isValid = FALSE;
                }
            }
            else
            {
                isValid = FALSE;
                result  = CDKResultENoMemory;
                CHX_LOG_ERROR("Out of memory:pLogicalCameraInfo->pSensorModeInfo allocate failed!");
            }
        }
        else
        {
            isValid = FALSE;
            CHX_LOG_ERROR("GetCameraInfo failed! Please check if the sensor(slot id:%d) probe successfully!",
                physicalCameraId);
        }

        if (FALSE == isValid)
        {
            // go on going through other physical device.
            result = CDKResultSuccess;
        }
        else
        {
            if (CDKResultSuccess == tagOps.pQueryVendorTagLocation("org.codeaurora.qcamera3.logicalCameraType",
                "logical_camera_type", &tag))
            {
                staticMetaDataHandle = const_cast<camera_metadata_t*>(
                    pLogicalCameraInfo->m_cameraInfo.static_camera_characteristics);
                tagOps.pSetMetaData(staticMetaDataHandle, tag, &pLogicalCameraInfo->logicalCameraType, sizeof(SBYTE));
            }

            tagOpResult = tagOps.pQueryVendorTagLocation("org.codeaurora.qcamera3.logicalCameraType",
                                                         "physicalCameraIds",
                                                         &tag);

            if (CDKResultSuccess == tagOpResult)
            {
                staticMetaDataHandle = const_cast<camera_metadata_t*>(
                    pLogicalCameraInfo->m_cameraInfo.static_camera_characteristics);
                tagOps.pSetMetaData(staticMetaDataHandle, tag,
                    &pLogicalCameraInfo->ppDeviceInfo[0]->cameraId,  sizeof(INT));
            }

            // Expose only back and front camera, if multicamera is not enabled
            if ((TRUE == pLogicalCameraConf->publicVisibility) &&
               ((TRUE == isMultiCameraEnabled) || ((FALSE == isMultiCameraEnabled) && (2 > m_numExposedLogicalCameras))))
            {
                m_cameraMap[m_numLogicalCameras]     = m_numLogicalCameras;
                m_numExposedLogicalCameras ++;
            }
            else
            {
                m_cameraMap[m_numLogicalCameras]     = pLogicalCameraConf->logicalCameraId;
            }
            m_cameraReverseMap[physicalCameraId] = pLogicalCameraInfo->cameraId;
            m_numLogicalCameras++ ;
            m_numPhysicalCameras++;
            pLogicalCameraInfo->publicVisiblity = pLogicalCameraConf->publicVisibility;
        }
    }

    // Swap cameras Rear Main as CameraId 0 and Front Main as CameraId 1.
    SwapFrontAndRearMain();

    // Go through logical camera configuration to create non-physical camera logical camera.
    for(UINT32 i = 0 ; i < m_numOfLogicalCameraConfiguration; i++)
    {
        if (FALSE == isMultiCameraEnabled)
        {
            CHX_LOG_INFO("Multi Camera is disabled!");
            break;
        }

        pLogicalCameraConf = &m_pLogicalCameraConfigurationInfo[i];

        if (isForExposed != pLogicalCameraConf->publicVisibility)
        {
            continue;
        }

        if (LogicalCameraType_Default == pLogicalCameraConf->logicalCameraType)
        {
            continue;
        }

#if ((CAMX_ANDROID_API) && (CAMX_ANDROID_API < 28)) // Android-P or better
        if (LogicalCameraType_DualApp == pLogicalCameraConf->logicalCameraType)
        {
            continue;
        }
#endif // ((CAMX_ANDROID_API) && (CAMX_ANDROID_API >= 28))

        CHX_LOG("index:%d,logical cameraid:%d,sensorId:%d,type:%d, publicVisibility:%d",
            i, pLogicalCameraConf->logicalCameraId, pLogicalCameraConf->deviceInfo[0].sensorId,
            pLogicalCameraConf->logicalCameraType, pLogicalCameraConf->publicVisibility);

        pLogicalCameraInfo                     = &m_logicalCameraInfo[m_numLogicalCameras];
        pLogicalCameraInfo->cameraId           = m_numLogicalCameras;
        pLogicalCameraInfo->logicalCameraType  = LogicalCameraType(pLogicalCameraConf->logicalCameraType);
        pLogicalCameraInfo->numPhysicalCameras = pLogicalCameraConf->numOfDevices;

        pLogicalCameraInfo->primaryCameraId    = sensorIdMap[pLogicalCameraConf->primarySensorID];
        if (pLogicalCameraInfo->primaryCameraId == INVALID_INDEX)
        {
            CHX_LOG_ERROR("Invalid Primary Cam Id. Check if logical id:%d is configured right!",
                          pLogicalCameraConf->logicalCameraId);
            isValid = FALSE;
            continue;
        }

        isValid                                = TRUE;

        // number of phsyical device in this logical camera should be less than number of physical device on this device
        if (pLogicalCameraConf->numOfDevices > numOfPhysicalCamera)
        {
            CHX_LOG_ERROR("Please check m_pLogicalCameraConfigurationInfo[%d] definition, nonOfPhysicalCamera:%d!", i,
                numOfPhysicalCamera);
            isValid = FALSE;
            continue;
        }

        pLogicalCameraInfo->ppDeviceInfo       =
            static_cast<DeviceInfo**>(CHX_CALLOC(sizeof(DeviceInfo *) * pLogicalCameraConf->numOfDevices));

        // fill device information for logical camera from physical camera
        if (NULL != pLogicalCameraInfo->ppDeviceInfo)
        {
            // Fill device information of logical camera
            for(UINT32 deviceIndex = 0 ; deviceIndex < pLogicalCameraConf->numOfDevices; deviceIndex ++)
            {
                sensorId = pLogicalCameraConf->deviceInfo[deviceIndex].sensorId;
                if (MaxNumImageSensors <= sensorId)
                {
                    CHX_LOG_ERROR("Invalid sensor id! please check if m_pLogicalCameraConfigurationInfo[%d] is valid!", i);
                    isValid = FALSE;
                    break;
                }

                physicalCameraId = sensorIdMap[sensorId];

                if (physicalCameraId == INVALID_INDEX)
                {
                    CHX_LOG_ERROR("Please check if sensor id:%d is probed successfully!", sensorId);
                    isValid = FALSE;
                    break;
                }

                const LogicalCameraInfo* pPhysicalCameraInfo = GetPhysicalCameraInfo(physicalCameraId);
                if (NULL != pPhysicalCameraInfo)
                {
                    pLogicalCameraInfo->ppDeviceInfo[deviceIndex] =
                                        static_cast<DeviceInfo*>(CHX_CALLOC(sizeof(DeviceInfo)));

                    if (NULL != pLogicalCameraInfo->ppDeviceInfo[deviceIndex])
                    {
                        *pLogicalCameraInfo->ppDeviceInfo[deviceIndex] = *pPhysicalCameraInfo->ppDeviceInfo[0];
                    }
                    else
                    {
                        CHX_LOG_ERROR("Out of memory:pLogicalCameraInfo->ppDeviceInfo[0] allocate failed!");
                        isValid = FALSE;
                        result = CDKResultENoMemory;
                    }

                    pLogicalCameraInfo->ppDeviceInfo[deviceIndex]->pDeviceConfig = &(pLogicalCameraConf->deviceInfo[deviceIndex]);
                }
                else
                {
                    CHX_LOG_ERROR("Please check m_pLogicalCameraConfigurationInfo[%d] definition, camera Id:%d is not exited",
                        i, physicalCameraId);
                    isValid = FALSE;
                    break;
                }
            }

            // Fill Logical camera camera information and logical camera capability.
            if (TRUE == isValid)
            {
                // if it is not default logical camera, it needs to call multi camera controller to create
                // consolidate camera info and consolidate capability for logical camera
                if (LogicalCameraType_Default != pLogicalCameraInfo->logicalCameraType)
                {
                    // Find primary camera index by matching logical camera configuration table to physical device id
                    UINT32 primaryCameraIndex = GetPrimaryCameraIndex(pLogicalCameraInfo);

                    camera_info_t* pCameraInfo = static_cast<camera_info_t*>
                        (pLogicalCameraInfo->ppDeviceInfo[primaryCameraIndex]->m_pDeviceCaps->pLegacy);

                    ChxUtils::Memcpy(&(pLogicalCameraInfo->m_cameraInfo), pCameraInfo, sizeof(camera_info_t));

                    pLogicalCameraInfo->m_cameraInfo.static_camera_characteristics =
                        reinterpret_cast<const camera_metadata_t*>(ChxUtils::AndroidMetadata::AllocateCopyMetaData(
                           pCameraInfo->static_camera_characteristics));

                    pLogicalCameraInfo->m_cameraCaps.pLegacy = &(pLogicalCameraInfo->m_cameraInfo);

                    MultiCamController::ConsolidateCameraInfo(pLogicalCameraInfo);

                    if (CDKResultSuccess == tagOps.pQueryVendorTagLocation("org.codeaurora.qcamera3.logicalCameraType",
                        "logical_camera_type", &tag))
                    {
                        staticMetaDataHandle = const_cast<camera_metadata_t*>(
                            pLogicalCameraInfo->m_cameraInfo.static_camera_characteristics);

                        tagOps.pSetMetaData(staticMetaDataHandle, tag, &pLogicalCameraInfo->logicalCameraType, sizeof(SBYTE));
                    }

                    {
                        INT32 cameraId[MaxDevicePerLogicalCamera];

                        for (UINT32 deviceId = 0; deviceId < pLogicalCameraInfo->numPhysicalCameras; deviceId++)
                        {
                            cameraId[deviceId] = pLogicalCameraInfo->ppDeviceInfo[deviceId]->cameraId;
                        }

                        if (CDKResultSuccess == tagOps.pQueryVendorTagLocation(
                            "org.codeaurora.qcamera3.logicalCameraType", "physicalCameraIds", &tag))
                        {
                            staticMetaDataHandle = const_cast<camera_metadata_t*>(
                                pLogicalCameraInfo->m_cameraInfo.static_camera_characteristics);
                            tagOps.pSetMetaData(staticMetaDataHandle, tag,
                                &cameraId[0], sizeof(INT) * pLogicalCameraInfo->numPhysicalCameras);
                        }
                    }

                    UINT32 burstShotFPSTag;
                    BOOL   isBurstShotSupported = TRUE;

                    if ((CHISocIdSM6350 == m_platformID) || (CHISocIdSM7225 == m_platformID))
                    {
                        // HEICSupport
                        UINT32 heicSupportTag;
                        if (CamxResultSuccess == tagOps.pQueryVendorTagLocation("org.quic.camera.HEICSupport",
                                                                                "HEICEnabled",
                                                                                &heicSupportTag))
                        {
                            BOOL heicSupportCapability = FALSE;
                            staticMetaDataHandle = const_cast<camera_metadata_t*>(
                                pLogicalCameraInfo->m_cameraInfo.static_camera_characteristics);
                            tagOps.pSetMetaData(staticMetaDataHandle, heicSupportTag,
                                                        &heicSupportCapability, 1);
                        }
                    }

                    // disable RTB Burstshot
                    if (LogicalCameraType_RTB == pLogicalCameraInfo->logicalCameraType)
                    {
                        isBurstShotSupported = FALSE;
                        if (CDKResultSuccess == tagOps.pQueryVendorTagLocation("org.quic.camera.BurstFPS",
                                                                                "isBurstShotSupported", &burstShotFPSTag))
                        {
                             CHIHANDLE staticMetaDataHandle = const_cast<camera_metadata_t*>(
                                                            pLogicalCameraInfo->m_cameraInfo.static_camera_characteristics);
                             tagOps.pSetMetaData(staticMetaDataHandle, burstShotFPSTag,
                                                 &isBurstShotSupported, sizeof(SBYTE));
                        }
                    }

                     // for bitra Soc,enable isLiveshotSizeSameAsVideoSize if enableScreenGrab overridesetting is TRUE
                    if(CHISocIdSM6350 == m_platformID)
                    {
                        UINT32 isLiveshotSizeSameAsVideoSizeVendorTag = 0;
                        BOOL   isLiveshotSizeSameAsVideoSize = FALSE;
                        if (LogicalCameraType_SAT == pLogicalCameraInfo->logicalCameraType)
                        {
                            if ((NULL != m_pEnableScreenGrab) && (1 == *m_pEnableScreenGrab))
                            {
                                isLiveshotSizeSameAsVideoSize = TRUE;
                            }

                            if (CDKResultSuccess == tagOps.pQueryVendorTagLocation("org.quic.camera.LiveshotSize",
                                                                                   "isLiveshotSizeSameAsVideoSize",
                                                                                   &isLiveshotSizeSameAsVideoSizeVendorTag))
                            {
                                tagOps.pSetMetaData(staticMetaDataHandle,
                                                    isLiveshotSizeSameAsVideoSizeVendorTag,
                                                    &isLiveshotSizeSameAsVideoSize,
                                                    1);
                                CHX_LOG_INFO("force isLiveshotSizeSameAsVideoSize to %d for SAT CamType",
                                    isLiveshotSizeSameAsVideoSize);
                            }
                        }
                    }

                    // here use deviceinfo[primaryCameraIndex] sensor mode info to fill logical camera sensor mode info.
                    // todo: to check if it is necessary to merge sensor mode information from multi sensor.
                    pLogicalCameraInfo->pSensorModeInfo =
                        const_cast<CHISENSORMODEINFO*>(pLogicalCameraInfo->ppDeviceInfo[primaryCameraIndex]->pSensorModeInfo);

                    InitializeAvailableStreamMap(m_numLogicalCameras);

                }
            }
        }
        else
        {   isValid            = FALSE;
            CHX_LOG_ERROR("Allocate failed:pLogicalCameraInfo->ppDeviceInfo!");
        }

        if (TRUE == isValid)
        {
            if (TRUE == pLogicalCameraConf->publicVisibility)
            {
                m_cameraMap[m_numLogicalCameras]     = m_numLogicalCameras;
                m_numExposedLogicalCameras ++;
            }
            else
            {
                m_cameraMap[m_numLogicalCameras]     = pLogicalCameraConf->logicalCameraId;
            }
            m_cameraReverseMap[m_numLogicalCameras] = pLogicalCameraInfo->cameraId;
            m_numLogicalCameras++ ;
            pLogicalCameraInfo->publicVisiblity = pLogicalCameraConf->publicVisibility;
        }

    }
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ExtensionModule::EnumerateCameras
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ExtensionModule::EnumerateCameras()
{
    CDKResult    result              = CDKResultEFailed;
    CHITAGSOPS   tagOps              = { 0 };
    UINT32       tag;

    UINT32       numOfPhysicalCamera = g_chiContextOps.pGetNumCameras(m_hCHIContext);

    UINT32       sensorIdMap[MaxNumImageSensors];
    UINT32       sensorId           = INVALID_INDEX;

    g_chiContextOps.pTagOps(&tagOps);

    ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// 1) Get map between sensor ID and camera ID
    ///       sensor ID is slot id which is from sensor driver
    ///       camera ID is used in camx which is used to access tuning data/sensor physical device.
    /// 2) Build logical camera information for the camera which will be exposed to application
    /// 3) Build logical camera information for the camera which will be NOT exposed to application
    ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    for (UINT32 i = 0 ; i < MaxNumImageSensors ; i++)
    {
        sensorIdMap[i] = INVALID_INDEX;
        m_cameraMap[i] = INVALID_INDEX;
    }

    CHICAMERAINFO cameraInfo;
    camera_info_t cam_info;
    cameraInfo.pLegacy = &cam_info;
    BOOL bMultiCameraEnabled = FALSE;
    // Get the sensor id and camera id map information of physical camera
    for (UINT32 i = 0 ; i < numOfPhysicalCamera ; i++)
    {
        result = g_chiContextOps.pGetCameraInfo(m_hCHIContext, i,
            &cameraInfo);

        if (CDKResultSuccess == result)
        {
            UINT32 sensorId = cameraInfo.sensorCaps.sensorId;
            sensorIdMap[sensorId] = i;

            CHX_LOG_INFO("cameraID map information: sensorid:%d,cameraid:%d", sensorId,i);

            if ((REAR_AUX == cameraInfo.sensorCaps.positionType) ||
                (FRONT_AUX == cameraInfo.sensorCaps.positionType))
            {
                bMultiCameraEnabled = TRUE;
                CHX_LOG_INFO("Multi camera is enabled!");
            }
        }
    }

    // Update BPS based camera based on static device configuration
    GetDevicesSettings();

    if (NULL != m_pBPSRealtimeSensorId)
    {
        CHX_LOG_INFO("BPS Realtime Sensor ID = %d", *m_pBPSRealtimeSensorId);

        LogicalCameraConfiguration *pLogicalCameraConf;
        for (UINT32 i = 0 ; i < m_numOfLogicalCameraConfiguration; i++)
        {
            pLogicalCameraConf = &m_pLogicalCameraConfigurationInfo[i];

            for (UINT32 deviceIndex = 0; deviceIndex < pLogicalCameraConf->numOfDevices; deviceIndex++)
            {
                if (*m_pBPSRealtimeSensorId == pLogicalCameraConf->deviceInfo[deviceIndex].sensorId)
                {
                    pLogicalCameraConf->deviceInfo[deviceIndex].realtimeEngine = RealtimeEngineType_BPS;
                    CHX_LOG_INFO("Mapping physical camera with device idx %d "
                        "from logical camera %d to use BPS Realtime engine",
                        deviceIndex,
                        pLogicalCameraConf->logicalCameraId);
                }
            }
        }
    }

    // build logical camera information for the logical camera which will be exposed to application
    BuildLogicalCameraInfo(sensorIdMap, bMultiCameraEnabled, TRUE);

    // build logical camera information for the logical camera which will be not exposed to application
    BuildLogicalCameraInfo(sensorIdMap, bMultiCameraEnabled, FALSE);

    CHX_LOG_INFO("Camera Count: m_numExposedLogicalCameras = %d, m_numLogicalCameras = %d Physical camera = %d",
        m_numExposedLogicalCameras, m_numLogicalCameras, m_numPhysicalCameras);

    for (UINT i = 0; i < m_numLogicalCameras; i++)
    {
        SetMaxLogicalCameraResourceCost(i);

        CHX_LOG_INFO("Internal camera ID = %d Num Devices = %d, logical camera(APP) ID = %d,isExposed:%d cost %d",
            i,
            m_logicalCameraInfo[i].numPhysicalCameras,
            m_cameraMap[i],
            m_logicalCameraInfo[i].publicVisiblity,
            m_logicalCameraInfo[i].maxResourceCost);

        for (UINT j = 0; j < m_logicalCameraInfo[i].numPhysicalCameras; j++)
        {
            CHX_LOG_INFO("CameraID Published ID = %d DeviceID = %d", i, m_logicalCameraInfo[i].ppDeviceInfo[j]->cameraId);
        }

        UpdateStaticSettings(m_logicalCameraInfo[i].cameraId);
    }

    for (UINT i = 0; i < m_numPhysicalCameras; i++)
    {
        CHIHANDLE    staticMetaDataHandle = const_cast<camera_metadata_t*>(
            m_logicalCameraInfo[i].m_cameraInfo.static_camera_characteristics);

        CHITAGSOPS   tagOps        = { 0 };
        UINT32 tag;
        g_chiContextOps.pTagOps(&tagOps);

        CHX_LOG_INFO("position:%d,index:%d,", m_logicalCameraInfo[i].m_cameraCaps.sensorCaps.positionType, i);

        if (CDKResultSuccess == tagOps.pQueryVendorTagLocation("org.quic.camera.HWResourceInfo",
                "numIFEsforGivenTarget", &tag))
        {
            if (i == 0)
            {
                UINT32 numIFEsforGivenTarget = 0;
                tagOps.pGetMetaData(staticMetaDataHandle, tag, &numIFEsforGivenTarget,
                                    sizeof(numIFEsforGivenTarget));
                m_totalResourceBudget = numIFEsforGivenTarget * m_singleISPResourceCost;
                CHX_LOG_INFO("Number of IFE for a given target=%d", numIFEsforGivenTarget);
            }
        }
        else
        {
            m_totalResourceBudget = m_singleISPResourceCost * 2;
        }
    }

    CHX_LOG_INFO(" m_totalResourceBudget=%d,  m_singleISPResourceCost=%d",
            m_totalResourceBudget, m_singleISPResourceCost);

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ExtensionModule::GetCameraInfo
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ExtensionModule::GetCameraInfo(
    uint32_t     logicalCameraId,
    camera_info* cameraInfo)
{
    CDKResult     result      = CDKResultEFailed;

    if (logicalCameraId < m_numLogicalCameras)
    {
        // no need for a deep copy...assuming 1 physical camera per logical camera (for now)
        ChxUtils::Memcpy(cameraInfo,
            &m_logicalCameraInfo[logicalCameraId].m_cameraInfo, sizeof(camera_info));

        result = CDKResultSuccess;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ExtensionModule::ModifySettings
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID ExtensionModule::ModifySettings(
    VOID*     pPriv)
{
    // pPriv is an implementation-specific data type.  Implementations can cast this to the format they expect to consume, and
    // store/modify any data which is available in this class

    // sample code...this uses the input to create a data structure used to hold configuration settings
    ChiModifySettings* pMod = static_cast<ChiModifySettings*>(pPriv);
    CHX_ASSERT(pMod->token.size <= sizeof(UINT32));

    if (pMod->token.id == static_cast<UINT32>(ChxSettingsToken::OverrideCameraClose))
    {
        // Config settings for camera close indication based on ExtendClose
        *static_cast<UINT32*>(pMod->pData) = ExtensionModule::GetInstance()->IsCameraClose();
        m_pConfigSettings[pMod->token.id] = *static_cast<UINT32*>(pMod->pData);
    }
    else if (pMod->token.id == static_cast<UINT32>(ChxSettingsToken::OverrideCameraOpen))
    {
        // Config settings for camera open indication based on ExtendOpen
        *static_cast<UINT32*>(pMod->pData) = ExtensionModule::GetInstance()->IsCameraOpen();
        m_pConfigSettings[pMod->token.id] = *static_cast<UINT32*>(pMod->pData);
    }
    else if (pMod->token.id == static_cast<UINT32>(ChxSettingsToken::AECGainThresholdForQCFA))
    {
        // Config settings for AECGainThresholdForQCFA
        m_AECGainThresholdForQCFA = *static_cast<FLOAT*>(pMod->pData);
    }
    else if (pMod->token.id == static_cast<UINT32>(ChxSettingsToken::OverrideLogLevels))
    {
        if (NULL != pMod->pData)
        {
            g_enableChxLogs = *static_cast<UINT32*>(pMod->pData);
        }
    }
    else if (pMod->token.id == static_cast<UINT32>(ChxSettingsToken::EnableRequestMapping))
    {
        // This setting's pData is NOT A POINTER. It is a BOOL saved in the pointer
        g_logRequestMapping = (TRUE == reinterpret_cast<UINT64>(pMod->pData));
    }
    else if (pMod->token.id == static_cast<UINT32>(ChxSettingsToken::EnableSystemLogging))
    {
        // This setting's pData is NOT A POINTER. It is a BOOL saved in the pointer
        g_enableSystemLog = (TRUE == reinterpret_cast<UINT64>(pMod->pData));
    }
    else if (pMod->token.id == 13)
    {
        m_pNumPCRsBeforeStreamOn = static_cast<UINT32*>(pMod->pData);
    }
    else if (pMod->token.id == static_cast<UINT32>(ChxSettingsToken::EnableAsciilog))
    {
        UINT32 asciiLogEnable    = *static_cast<UINT32*>(pMod->pData);
        m_bAsciiLogEnable        = asciiLogEnable != 0;
        ChiLog::g_asciiLogEnable = m_bAsciiLogEnable;

        // Disable offlinelogger by checking offlinelogger staticSetting token, 0=Disable/1=Enable
        if (FALSE == m_bAsciiLogEnable)
        {
            if (NULL != m_offlineLoggerHandle)
            {
                if (NULL != m_offlineLoggerOps.pDisableOfflineLog)
                {
                    // This is to disable offlinelogger
                    m_offlineLoggerOps.pDisableOfflineLog(OfflineLoggerType::ASCII);
                }
            }
        }
    }
    else if (pMod->token.id == static_cast<UINT32>(ChxSettingsToken::EnableBinarylog))
    {
        UINT32 binarylogEnable = *static_cast<UINT32*>(pMod->pData);
        m_bBinaryLogEnable = binarylogEnable != 0;

        // Disable offlinelogger by checking offlinelogger staticSetting token, 0=Disable/1=Enable
        if (FALSE == m_bBinaryLogEnable)
        {
            if (NULL != m_offlineLoggerHandle)
            {
                if (NULL != m_offlineLoggerOps.pDisableOfflineLog)
                {
                    // This is to disable offlinelogger
                    m_offlineLoggerOps.pDisableOfflineLog(OfflineLoggerType::BINARY);
                }
            }
        }
    }
    else if (pMod->token.id == static_cast<UINT32>(ChxSettingsToken::Enable3ADebugData))
    {
        // This setting's pData is NOT A POINTER. It is a BOOL saved in the pointer
        m_pConfigSettings[pMod->token.id] = (TRUE == reinterpret_cast<UINT64>(pMod->pData));
    }
    else if (pMod->token.id == static_cast<UINT32>(ChxSettingsToken::EnableConcise3ADebugData))
    {
        // This setting's pData is NOT A POINTER. It is a BOOL saved in the pointer
        m_pConfigSettings[pMod->token.id] = (TRUE == reinterpret_cast<UINT64>(pMod->pData));
    }
    else if (pMod->token.id == static_cast<UINT32>(ChxSettingsToken::EnableTuningMetadata))
    {
        // This setting's pData is NOT A POINTER. It is a BOOL saved in the pointer
        m_pConfigSettings[pMod->token.id] = (TRUE == reinterpret_cast<UINT64>(pMod->pData));
    }
    else if (pMod->token.id == static_cast<UINT32>(ChxSettingsToken::ExposeFullSizeForQCFA))
    {
        // This setting's pData is NOT A POINTER. It is a BOOL saved in the pointer
        m_pConfigSettings[pMod->token.id] = (TRUE == reinterpret_cast<UINT64>(pMod->pData));
    }
    else if (pMod->token.id == static_cast<UINT32>(ChxSettingsToken::EnableScreenGrab))
    {
        // This setting's pData is NOT A POINTER. It is a BOOL saved in the pointer
        m_pConfigSettings[pMod->token.id] = (TRUE == reinterpret_cast<UINT64>(pMod->pData));
    }
    else
    {
        if (NULL != pMod->pData)
        {
            m_pConfigSettings[pMod->token.id] = *static_cast<UINT32*>(pMod->pData);
        }
        else
        {
            m_pConfigSettings[pMod->token.id] = 0;
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ExtensionModule::DefaultRequestSettings
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID ExtensionModule::DefaultRequestSettings(
    uint32_t                  cameraId,
    int                       requestTemplate,
    const camera_metadata_t** settings)
{
    (VOID)cameraId;
    (VOID)requestTemplate;
    (VOID)settings;
#if defined (_LINUX)
    if (NULL == m_pDefaultSettings[cameraId])
    {
        m_pDefaultSettings[cameraId] = allocate_camera_metadata(DefaultSettingsNumEntries, DefaultSettingsDataSize);
    }

    if (m_pDefaultSettings[cameraId] != NULL)
    {
        CHITAGSOPS              tagOps  = { 0 };
        UINT32                  tag     = 0;
        camera_metadata_entry_t entry   = {0};

        g_chiContextOps.pTagOps(&tagOps);

        if (0 != m_logicalCameraInfo[cameraId].numAvailableStreamMap)
        {
            if (CDKResultSuccess == tagOps.pQueryVendorTagLocation("org.codeaurora.qcamera3.sessionParameters",
                 "availableStreamMap", &tag))
            {
                CHIHANDLE  metadataHandle =const_cast<camera_metadata_t*>(m_pDefaultSettings[cameraId]);

                tagOps.pSetMetaData(metadataHandle, tag, m_logicalCameraInfo
                    [cameraId].pAvailableStreamMap,
                    (sizeof(StreamMap) * m_logicalCameraInfo[cameraId].numAvailableStreamMap));
            }
        }

        // Fill in any default settings (vendor tags) that needs to be added
        *settings = (const camera_metadata_t*)(m_pDefaultSettings[cameraId]);
    }
#endif
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ExtensionModule::InitializeOverrideSession
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ExtensionModule::InitializeOverrideSession(
    uint32_t                        logicalCameraId,
    const camera3_device_t*         pCamera3Device,
    const chi_hal_ops_t*            chiHalOps,
    camera3_stream_configuration_t* pStreamConfig,
    int*                            pIsOverrideEnabled,
    VOID**                          pPrivate)
{
    CDKResult          result             = CDKResultSuccess;
    UINT32             modeCount          = 0;
    ChiSensorModeInfo* pAllModes          = NULL;
    UINT32             fps                = *m_pDefaultMaxFPS;
    BOOL               isVideoMode        = FALSE;
    uint32_t           operation_mode;
    static BOOL        fovcModeCheck      = EnableFOVCUseCase();
    UsecaseId          selectedUsecaseId  = UsecaseId::NoMatch;
    UINT               minSessionFps      = 0;
    UINT               maxSessionFps      = 0;
    CDKResult          tagOpResult        = CDKResultEFailed;
    ChiBLMParams       blmParams;

    *pPrivate             = NULL;
    *pIsOverrideEnabled   = FALSE;
    m_aFlushInProgress[logicalCameraId] = FALSE;
    m_firstResult                       = FALSE;
    m_hasFlushOccurred[logicalCameraId] = FALSE;
    blmParams.height                    = 0;
    blmParams.width                     = 0;

    if (NULL == m_hCHIContext)
    {
        m_hCHIContext = g_chiContextOps.pOpenContext();
    }

    ChiVendorTagsOps vendorTagOps = { 0 };
    g_chiContextOps.pTagOps(&vendorTagOps);
    operation_mode                = pStreamConfig->operation_mode >> 16;
    operation_mode                = operation_mode & 0x000F;
    pStreamConfig->operation_mode = pStreamConfig->operation_mode & 0xFFFF;

    UINT numOutputStreams = 0;
    for (UINT32 stream = 0; stream < pStreamConfig->num_streams; stream++)
    {
        if (0 != (pStreamConfig->streams[stream]->usage & GrallocUsageHwVideoEncoder))
        {
            isVideoMode = TRUE;

            if((pStreamConfig->streams[stream]->height * pStreamConfig->streams[stream]->width) >
             (blmParams.height * blmParams.width))
            {
                blmParams.height = pStreamConfig->streams[stream]->height;
                blmParams.width  = pStreamConfig->streams[stream]->width;
            }
        }

        if (CAMERA3_STREAM_OUTPUT == pStreamConfig->streams[stream]->stream_type)
        {
            numOutputStreams++;
        }

        //If video stream not present in that case store Preview/Snapshot Stream info
        if((pStreamConfig->streams[stream]->height > blmParams.height) &&
            (pStreamConfig->streams[stream]->width > blmParams.width)  &&
            (isVideoMode == FALSE))
        {
            blmParams.height = pStreamConfig->streams[stream]->height;
            blmParams.width  = pStreamConfig->streams[stream]->width;
        }
    }

    if (numOutputStreams > MaxExternalBuffers)
    {
        CHX_LOG_ERROR("numOutputStreams(%u) greater than MaxExternalBuffers(%u)", numOutputStreams, MaxExternalBuffers);
        result = CDKResultENotImplemented;
    }

    if ((isVideoMode == TRUE) && (operation_mode != 0))
    {
        UINT32             numSensorModes  = m_logicalCameraInfo[logicalCameraId].m_cameraCaps.numSensorModes;
        CHISENSORMODEINFO* pAllSensorModes = m_logicalCameraInfo[logicalCameraId].pSensorModeInfo;

        if ((operation_mode - 1) >= numSensorModes)
        {
            result = CDKResultEOverflow;
            CHX_LOG_ERROR("operation_mode: %d, numSensorModes: %d", operation_mode, numSensorModes);
        }
        else
        {
            fps = pAllSensorModes[operation_mode - 1].frameRate;
        }
    }

    if (CDKResultSuccess == result)
    {
#if defined(CAMX_ANDROID_API) && (CAMX_ANDROID_API >= 28) //Android-P or better
        camera_metadata_t *metadata = const_cast<camera_metadata_t*>(pStreamConfig->session_parameters);

        camera_metadata_entry_t entry = { 0 };

        // The client may choose to send NULL sesssion parameter, which is fine. For example, torch mode
        // will have NULL session param.
        if (metadata != NULL)
        {
            entry.tag = ANDROID_CONTROL_AE_TARGET_FPS_RANGE;

            int ret = find_camera_metadata_entry(metadata, entry.tag, &entry);

            if(ret == 0) {
                minSessionFps = entry.data.i32[0];
                maxSessionFps = entry.data.i32[1];
                m_usecaseMaxFPS = maxSessionFps;
            }
        }

        CHITAGSOPS   tagOps       = { 0 };
        UINT32       tagLocation  = 0;

        g_chiContextOps.pTagOps(&tagOps);

        tagOpResult = tagOps.pQueryVendorTagLocation(
            "org.codeaurora.qcamera3.sessionParameters",
            "availableStreamMap",
            &tagLocation);

        if (CDKResultSuccess == tagOpResult)
        {
            camera_metadata_entry_t entry = { 0 };

            if (metadata != NULL)
            {
                int ret = find_camera_metadata_entry(metadata, tagLocation, &entry);
            }
        }

        tagOpResult = tagOps.pQueryVendorTagLocation(
            "org.codeaurora.qcamera3.sessionParameters",
            "overrideResourceCostValidation",
            &tagLocation);

        if ((NULL != metadata) && (CDKResultSuccess == tagOpResult))
        {
            camera_metadata_entry_t resourcecostEntry = { 0 };

            if (0 == find_camera_metadata_entry(metadata, tagLocation, &resourcecostEntry))
            {
                BOOL bypassRCV = static_cast<BOOL>(resourcecostEntry.data.u8[0]);

                if (TRUE == bypassRCV)
                {
                    m_pResourcesUsedLock->Lock();
                    m_logicalCameraRCVBypassSet.insert(logicalCameraId);
                    m_pResourcesUsedLock->Unlock();
                }
            }
        }

#endif

        CHIHANDLE    staticMetaDataHandle = const_cast<camera_metadata_t*>(
                                            m_logicalCameraInfo[logicalCameraId].m_cameraInfo.static_camera_characteristics);
        UINT32       metaTagPreviewFPS    = 0;
        UINT32       metaTagVideoFPS      = 0;

        m_previewFPS           = 0;
        m_videoFPS             = 0;
        GetInstance()->GetVendorTagOps(&vendorTagOps);

        result = vendorTagOps.pQueryVendorTagLocation("org.quic.camera2.streamBasedFPS.info", "PreviewFPS",
                                                      &metaTagPreviewFPS);
        if (CDKResultSuccess == result)
        {
            vendorTagOps.pGetMetaData(staticMetaDataHandle, metaTagPreviewFPS, &m_previewFPS,
                                      sizeof(m_previewFPS));
        }

        result = vendorTagOps.pQueryVendorTagLocation("org.quic.camera2.streamBasedFPS.info", "VideoFPS", &metaTagVideoFPS);
        if (CDKResultSuccess == result)
        {
            vendorTagOps.pGetMetaData(staticMetaDataHandle, metaTagVideoFPS, &m_videoFPS,
                                      sizeof(m_videoFPS));
        }

        if ((StreamConfigModeConstrainedHighSpeed == pStreamConfig->operation_mode) ||
            (StreamConfigModeSuperSlowMotionFRC == pStreamConfig->operation_mode))
        {
            if ((StreamConfigModeConstrainedHighSpeed == pStreamConfig->operation_mode) &&
                (30 >= maxSessionFps))
            {
                minSessionFps   = DefaultFrameRateforHighSpeedSession;
                maxSessionFps   = DefaultFrameRateforHighSpeedSession;
                m_usecaseMaxFPS = maxSessionFps;

                CHX_LOG_INFO("minSessionFps = %d maxSessionFps = %d", minSessionFps, maxSessionFps);
            }

            SearchNumBatchedFrames(logicalCameraId, pStreamConfig,
                                   &m_usecaseNumBatchedFrames, &m_HALOutputBufferCombined,
                                   &m_usecaseMaxFPS, maxSessionFps);
            if (480 > m_usecaseMaxFPS)
            {
                m_CurrentpowerHint = PERF_LOCK_POWER_HINT_VIDEO_ENCODE_HFR;
            }
            else
            {
                // For 480FPS or higher, require more aggresive power hint
                m_CurrentpowerHint = PERF_LOCK_POWER_HINT_VIDEO_ENCODE_HFR_480FPS;
            }
        }
        else
        {
            // Not a HFR usecase, batch frames value need to be set to 1.
            m_usecaseNumBatchedFrames = 1;
            m_HALOutputBufferCombined = FALSE;
            if (maxSessionFps == 0)
            {
                m_usecaseMaxFPS = fps;
            }
            if (TRUE == isVideoMode)
            {
                if (30 >= m_usecaseMaxFPS)
                {
                    m_CurrentpowerHint = PERF_LOCK_POWER_HINT_VIDEO_ENCODE;
                }
                else
                {
                    m_CurrentpowerHint = PERF_LOCK_POWER_HINT_VIDEO_ENCODE_60FPS;
                }
            }
            else
            {
                m_CurrentpowerHint = PERF_LOCK_POWER_HINT_PREVIEW;
            }
        }

        if ((NULL != m_pPerfLockManager[logicalCameraId]) && (m_CurrentpowerHint != m_previousPowerHint))
        {
            m_pPerfLockManager[logicalCameraId]->ReleasePerfLock(m_previousPowerHint);
        }

        // Example [B == batch]: (240 FPS / 4 FPB = 60 BPS) / 30 FPS (Stats frequency goal) = 2 BPF i.e. skip every other stats
        *m_pStatsSkipPattern = m_usecaseMaxFPS / m_usecaseNumBatchedFrames / 30;
        if (*m_pStatsSkipPattern < 1)
        {
            *m_pStatsSkipPattern = 1;
        }

        m_VideoHDRMode = (StreamConfigModeVideoHdr == pStreamConfig->operation_mode);

        m_torchWidgetUsecase = (StreamConfigModeQTITorchWidget == pStreamConfig->operation_mode);

        // this check is introduced to avoid set *m_pEnableFOVC == 1 if fovcEnable is disabled in
        // overridesettings & fovc bit is set in operation mode.
        // as well as to avoid set,when we switch Usecases.
        if (TRUE == fovcModeCheck)
        {
            *m_pEnableFOVC = ((pStreamConfig->operation_mode & StreamConfigModeQTIFOVC) == StreamConfigModeQTIFOVC) ? 1 : 0;
        }

        SetHALOps(logicalCameraId, chiHalOps);

        m_logicalCameraInfo[logicalCameraId].m_pCamera3Device = pCamera3Device;

        selectedUsecaseId = m_pUsecaseSelector->GetMatchingUsecase(&m_logicalCameraInfo[logicalCameraId],
                                                                   pStreamConfig);

        CHX_LOG_CONFIG("Session_parameters FPS range %d:%d, previewFPS %d, videoFPS %d "
                       "BatchSize: %u HALOutputBufferCombined %d FPS: %u SkipPattern: %u, "
                       "cameraId = %d selected use case = %d",
                       minSessionFps,
                       maxSessionFps,
                       m_previewFPS,
                       m_videoFPS,
                       m_usecaseNumBatchedFrames,
                       m_HALOutputBufferCombined,
                       m_usecaseMaxFPS,
                       *m_pStatsSkipPattern,
                       logicalCameraId,
                       selectedUsecaseId);

        // FastShutter mode supported only in ZSL usecase.
        if ((pStreamConfig->operation_mode == StreamConfigModeFastShutter) &&
            (UsecaseId::PreviewZSL         != selectedUsecaseId))
        {
            pStreamConfig->operation_mode = StreamConfigModeNormal;
        }
        m_operationMode[logicalCameraId] = pStreamConfig->operation_mode;
    }

    if (m_pBLMClient != NULL)
    {
        blmParams.numcamera         = m_logicalCameraInfo[logicalCameraId].numPhysicalCameras;
        blmParams.logicalCameraType = m_logicalCameraInfo[logicalCameraId].logicalCameraType;
        blmParams.FPS               = m_usecaseMaxFPS;
        blmParams.selectedusecaseId = selectedUsecaseId;
        blmParams.socId             = GetPlatformID();
        blmParams.isVideoMode       = isVideoMode;

        m_pBLMClient->SetUsecaseBwLevel(blmParams);
    }

    if (UsecaseId::NoMatch != selectedUsecaseId)
    {
        m_pSelectedUsecase[logicalCameraId] =
            m_pUsecaseFactory->CreateUsecaseObject(&m_logicalCameraInfo[logicalCameraId],
                                                   selectedUsecaseId, pStreamConfig);

        if (NULL != m_pSelectedUsecase[logicalCameraId])
        {
            m_pStreamConfig[logicalCameraId] = static_cast<camera3_stream_configuration_t*>(
                CHX_CALLOC(sizeof(camera3_stream_configuration_t)));
            m_pStreamConfig[logicalCameraId]->streams = static_cast<camera3_stream_t**>(
                CHX_CALLOC(sizeof(camera3_stream_t*) * pStreamConfig->num_streams));
            m_pStreamConfig[logicalCameraId]->num_streams = pStreamConfig->num_streams;

            for (UINT32 i = 0; i< m_pStreamConfig[logicalCameraId]->num_streams; i++)
            {
                m_pStreamConfig[logicalCameraId]->streams[i] = pStreamConfig->streams[i];
            }

            m_pStreamConfig[logicalCameraId]->operation_mode = pStreamConfig->operation_mode;

            if (NULL != pStreamConfig->session_parameters)
            {
                m_pStreamConfig[logicalCameraId]->session_parameters =
                    (const camera_metadata_t *)allocate_copy_camera_metadata_checked(
                    pStreamConfig->session_parameters,
                    get_camera_metadata_size(pStreamConfig->session_parameters));
            }
            // use camera device / used for recovery only for regular session
            m_SelectedUsecaseId[logicalCameraId] = (UINT32)selectedUsecaseId;
            CHX_LOG_CONFIG("Logical cam Id = %d usecase addr = %p", logicalCameraId, m_pSelectedUsecase[
                logicalCameraId]);

            m_pCameraDeviceInfo[logicalCameraId].m_pCamera3Device = pCamera3Device;

            *pIsOverrideEnabled = TRUE;

            m_TeardownInProgress[logicalCameraId]      = FALSE;
            m_RecoveryInProgress[logicalCameraId]      = FALSE;
            m_terminateRecoveryThread[logicalCameraId] = FALSE;

            m_pPCRLock[logicalCameraId]                  = Mutex::Create();
            m_pDestroyLock[logicalCameraId]              = Mutex::Create();
            m_pRecoveryLock[logicalCameraId]             = Mutex::Create();
            m_pTriggerRecoveryLock[logicalCameraId]      = Mutex::Create();
            m_pTriggerRecoveryCondition[logicalCameraId] = Condition::Create();
            m_pRecoveryCondition[logicalCameraId]        = Condition::Create();
            m_recoveryThreadPrivateData[logicalCameraId] = { logicalCameraId, this };

            // Create recovery thread and wait on being signaled
            m_pRecoveryThread[logicalCameraId].pPrivateData = &m_recoveryThreadPrivateData[logicalCameraId];

            result = ChxUtils::ThreadCreate(ExtensionModule::RecoveryThread,
                &m_pRecoveryThread[logicalCameraId],
                &m_pRecoveryThread[logicalCameraId].hThreadHandle);
            if (CDKResultSuccess != result)
            {
                CHX_LOG_ERROR("Failed to create recovery thread for logical camera %d result %d", logicalCameraId, result);
            }
        }
        else
        {
            CHX_LOG_ERROR("For cameraId = %d CreateUsecaseObject failed", logicalCameraId);
            m_logicalCameraInfo[logicalCameraId].m_pCamera3Device = NULL;
        }
    }

    if ((CDKResultSuccess != result) || (UsecaseId::Torch == selectedUsecaseId))
    {
        // reset resource count in failure case or Torch case
        ResetResourceCost(m_logicalCameraInfo[logicalCameraId].cameraId);
    }

    CHX_LOG_INFO(" logicalCameraId = %d, m_totalResourceBudget = %d, activeResourseCost = %d, m_IFEResourceCost = %d",
            logicalCameraId, m_totalResourceBudget, GetActiveResourceCost(), m_IFEResourceCost[logicalCameraId]);

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ExtensionModule::FinalizeOverrideSession
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ExtensionModule::FinalizeOverrideSession(
     const camera3_device_t* camera3_device,
     VOID*                   pPriv)
{
    (VOID)camera3_device;
    (VOID)pPriv;

    return CDKResultSuccess;
}


////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////
// ExtensionModule::GetSelectedUsecase
////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////
Usecase* ExtensionModule::GetSelectedUsecase(
    const camera3_device_t *pCamera3Device)
{
    Usecase* pUsecase = NULL;

    //TODO: enable when FastAec is enabled
    //look for matching entry in the fast aec list first

    //for (UINT32 id = 0; id < MaxNumImageSensors; id++)
    //{
    //    if ((NULL != m_pFastAecSession[id]) &&
    //        (m_pFastAecSession[id]->GetCamera3Device() == pCamera3Device))
    //    {
    //        pUsecase = m_pFastAecSession[id]->GetUsecase();
    //        break;
    //    }
    //}

    if (NULL == pUsecase)
    {
        for (UINT32 id = 0; id < MaxNumImageSensors; id++)
        {
            if (m_pCameraDeviceInfo[id].m_pCamera3Device && m_pCameraDeviceInfo[id].m_pCamera3Device
                == pCamera3Device)
            {
                pUsecase = m_pSelectedUsecase[id];
                break;
            }
        }
    }

    return pUsecase;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ExtensionModule::TeardownOverrideSession
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID ExtensionModule::TeardownOverrideSession(
    const camera3_device_t* camera3_device,
    UINT64                  session,
    VOID*                   pPriv)
{
    (VOID)session;
    (VOID)pPriv;

    UINT32 logicalCameraId = GetCameraIdfromDevice(camera3_device);

    CHX_LOG_INFO("Tearing down logical CameraId: %d session", logicalCameraId);

    if (logicalCameraId >= MaxNumImageSensors)
    {
        CHX_LOG_ERROR("logicalCameraId: %d excess MaxnumImageSensors", logicalCameraId);
        return;
    }

    m_pRecoveryLock[logicalCameraId]->Lock();
    if (TRUE == m_RecoveryInProgress[logicalCameraId])
    {
        CHX_LOG_INFO("Wait for recovery to finish, before starting teardown for cameraId: %d", logicalCameraId);
        m_pRecoveryCondition[logicalCameraId]->Wait(m_pRecoveryLock[logicalCameraId]->GetNativeHandle());
    }
    m_TeardownInProgress[logicalCameraId] = TRUE;
    m_pRecoveryLock[logicalCameraId]->Unlock();
    TeardownOverrideUsecase(camera3_device, FALSE);
    m_logicalCameraInfo[logicalCameraId].m_pCamera3Device = NULL;
    m_pCameraDeviceInfo[logicalCameraId].m_pCamera3Device = NULL;

    CHX_LOG("Free up stream config memory");
    // free up m_pStreamConfig
    if (NULL != m_pStreamConfig[logicalCameraId])
    {
        if (NULL != m_pStreamConfig[logicalCameraId]->streams)
        {
            CHX_FREE(m_pStreamConfig[logicalCameraId]->streams);
            m_pStreamConfig[logicalCameraId]->streams = NULL;
        }
        if (NULL != m_pStreamConfig[logicalCameraId]->session_parameters)
        {
            free_camera_metadata(const_cast<camera_metadata_t*>(m_pStreamConfig[logicalCameraId]->session_parameters));
        }
        CHX_FREE(m_pStreamConfig[logicalCameraId]);
        m_pStreamConfig[logicalCameraId] = NULL;
    }

    if (NULL != m_pPerfLockManager[logicalCameraId])
    {
        m_pPerfLockManager[logicalCameraId]->ReleasePerfLock(m_CurrentpowerHint);
    }

    m_pRecoveryLock[logicalCameraId]->Lock();
    m_TeardownInProgress[logicalCameraId] = FALSE;
    m_pRecoveryLock[logicalCameraId]->Unlock();

    if (NULL != m_pTriggerRecoveryLock[logicalCameraId])
    {
        m_pTriggerRecoveryLock[logicalCameraId]->Lock();
        m_terminateRecoveryThread[logicalCameraId] = TRUE;
        m_pTriggerRecoveryCondition[logicalCameraId]->Signal();
        m_pTriggerRecoveryLock[logicalCameraId]->Unlock();
    }

    ChxUtils::ThreadTerminate(m_pRecoveryThread[logicalCameraId].hThreadHandle);

    if (NULL != m_pPCRLock[logicalCameraId])
    {
        m_pPCRLock[logicalCameraId]->Destroy();
        m_pPCRLock[logicalCameraId] = NULL;
    }

    if (NULL != m_pRecoveryLock[logicalCameraId])
    {
        m_pRecoveryLock[logicalCameraId]->Destroy();
        m_pRecoveryLock[logicalCameraId] = NULL;
    }

    if (NULL != m_pRecoveryCondition[logicalCameraId])
    {
        m_pRecoveryCondition[logicalCameraId]->Destroy();
        m_pRecoveryCondition[logicalCameraId] = NULL;
    }

    if (NULL != m_pTriggerRecoveryLock[logicalCameraId])
    {
        m_pTriggerRecoveryLock[logicalCameraId]->Destroy();
        m_pTriggerRecoveryLock[logicalCameraId] = NULL;
    }

    if (NULL != m_pTriggerRecoveryCondition[logicalCameraId])
    {
        m_pTriggerRecoveryCondition[logicalCameraId]->Destroy();
        m_pTriggerRecoveryCondition[logicalCameraId] = NULL;
    }

    if (NULL != m_pDestroyLock[logicalCameraId])
    {
        m_pDestroyLock[logicalCameraId]->Destroy();
        m_pDestroyLock[logicalCameraId] = NULL;
    }

    if (m_pBLMClient != NULL)
    {
        m_pBLMClient->CancelUsecaseHint();
    }
    ResetResourceCost(m_logicalCameraInfo[logicalCameraId].cameraId);

    CHX_LOG_INFO("Tearing down of logical CameraId: %d session is Done!!", logicalCameraId);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ExtensionModule::TeardownOverrideUsecase
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID ExtensionModule::TeardownOverrideUsecase(
    const camera3_device_t* camera3_device,
    BOOL                    isForced)
{
    CHX_LOG_INFO("[E]");

    // Overriding the camera close indication in ExtendClose
    UINT32   logicalCameraId = GetCameraIdfromDevice(camera3_device);
    Usecase* pUsecase        = NULL;

    if (logicalCameraId < MaxNumImageSensors)
    {
        m_pDestroyLock[logicalCameraId]->Lock();
        pUsecase = m_pSelectedUsecase[logicalCameraId];
        m_pSelectedUsecase[logicalCameraId] = NULL;
        m_pDestroyLock[logicalCameraId]->Unlock();
    }

    if (NULL != pUsecase)
    {
        pUsecase->DestroyObject(isForced);
    }

    CHX_LOG_INFO("[X]");
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ExtensionModule::OverrideProcessRequest
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ExtensionModule::OverrideProcessRequest(
    const camera3_device_t*     camera3_device,
    camera3_capture_request_t*  pCaptureRequest,
    VOID*                       pPriv)
{
    CDKResult result = CDKResultSuccess;

    for (UINT32 i = 0; i < pCaptureRequest->num_output_buffers; i++)
    {
        if (NULL != pCaptureRequest->output_buffers)
        {
            ChxUtils::WaitOnAcquireFence(&pCaptureRequest->output_buffers[i]);

            INT*   pAcquireFence = (INT*)&pCaptureRequest->output_buffers[i].acquire_fence;

            *pAcquireFence = -1;
        }
    }

    UINT32 logicalCameraId = GetCameraIdfromDevice(camera3_device);
    if (CDKInvalidId != logicalCameraId)
    {
        if (NULL != pCaptureRequest->settings)
        {
            FreeLastKnownRequestSetting(logicalCameraId);
            m_pLastKnownRequestSettings[logicalCameraId] = allocate_copy_camera_metadata_checked(pCaptureRequest->settings,
                get_camera_metadata_size(pCaptureRequest->settings));
        }

        // Set valid metadata after flush if settings aren't available
        if ((TRUE == m_hasFlushOccurred[logicalCameraId]) &&
            (NULL == pCaptureRequest->settings))
        {
            CHX_LOG_INFO("Setting Request to m_pLastKnownRequestSettings after flush for frame_number:%d",
                pCaptureRequest->frame_number);
            pCaptureRequest->settings = m_pLastKnownRequestSettings[logicalCameraId];
            m_hasFlushOccurred[logicalCameraId] = FALSE;
        }

        if (TRUE == static_cast<BOOL>(ChxUtils::AtomicLoadU32(&m_aFlushInProgress[logicalCameraId])))
        {
            CHX_LOG_INFO("flush enabled, frame %d", pCaptureRequest->frame_number);
            HandleProcessRequestErrorAllPCRs(pCaptureRequest, logicalCameraId);
            return CDKResultSuccess;
        }

        if (ChxUtils::AndroidMetadata::IsLongExposureCapture(const_cast<camera_metadata_t*>(pCaptureRequest->settings)))
        {
            ChxUtils::AtomicStoreU32(&m_aLongExposureInProgress, TRUE);
            m_longExposureFrame = pCaptureRequest->frame_number;
            CHX_LOG_INFO("Long exposure enabled in frame %d", pCaptureRequest->frame_number);
        }

        m_pRecoveryLock[logicalCameraId]->Lock();
        if (TRUE == m_RecoveryInProgress[logicalCameraId])
        {
            CHX_LOG_INFO("Wait for recovery to finish, before proceeding with new request for cameraId: %d", logicalCameraId);
            m_pRecoveryCondition[logicalCameraId]->Wait(m_pRecoveryLock[logicalCameraId]->GetNativeHandle());
        }
        m_pRecoveryLock[logicalCameraId]->Unlock();

        // Save the original metadata
        const camera_metadata_t* pOriginalMetadata = pCaptureRequest->settings;
        (VOID)pPriv;

        m_pPCRLock[logicalCameraId]->Lock();
        if (NULL != m_pSelectedUsecase[logicalCameraId])
        {
            m_originalFrameWorkNumber[logicalCameraId] = pCaptureRequest->frame_number;

            // Recovery happened if framework didn't send any metadata, send valid metadata
            if (m_firstFrameAfterRecovery[logicalCameraId] == pCaptureRequest->frame_number &&
                NULL == pCaptureRequest->settings)
            {
                CHX_LOG_INFO("Setting Request for first frame after case =%d", m_firstFrameAfterRecovery[logicalCameraId]);
                pCaptureRequest->settings = m_pLastKnownRequestSettings[logicalCameraId];
                m_firstFrameAfterRecovery[logicalCameraId] = 0;
            }

            if (pCaptureRequest->output_buffers != NULL)
            {
                for (UINT i = 0; i < pCaptureRequest->num_output_buffers; i++)
                {
                    if ((NULL != m_pPerfLockManager[logicalCameraId]) &&
                        (pCaptureRequest->output_buffers[i].stream->format == ChiStreamFormatBlob) &&
                        ((pCaptureRequest->output_buffers[i].stream->data_space ==
                            static_cast<android_dataspace_t>(DataspaceV0JFIF)) ||
                        (pCaptureRequest->output_buffers[i].stream->data_space ==
                            static_cast<android_dataspace_t>(DataspaceJFIF))))
                    {
                        m_pPerfLockManager[logicalCameraId]->AcquirePerfLock(PERF_LOCK_SNAPSHOT_CAPTURE, 2000);
                        break;
                    }

                    if ((NULL != m_pPerfLockManager[logicalCameraId]) &&
                        TRUE == UsecaseSelector::IsHEIFStream(pCaptureRequest->output_buffers[i].stream))
                    {
                        m_pPerfLockManager[logicalCameraId]->AcquirePerfLock(PERF_LOCK_SNAPSHOT_CAPTURE, 2000);
                        break;
                    }
                }
            }

            result = m_pSelectedUsecase[logicalCameraId]->ProcessCaptureRequest(pCaptureRequest);
        }

        if (pCaptureRequest->settings != NULL)
        {
            // Restore the original metadata pointer that came from the framework
            pCaptureRequest->settings = pOriginalMetadata;
        }

        // Need to return success on PCR to allow FW to continue sending requests
        if (result == CDKResultEBusy)
        {
            result = CDKResultSuccess;
        }

        if (result == CamxResultECancelledRequest)
        {
            // Ignore the Failure if flush or recovery returned CamcelRequest
            CHX_LOG("Flush/Recovery is in progress %d and so ignore failure", pCaptureRequest->frame_number);
            result = CDKResultSuccess;
        }

        m_pPCRLock[logicalCameraId]->Unlock();
    }
    else
    {
        CHX_LOG_ERROR("Invalid logical camera id device:%p!!", camera3_device);
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ExtensionModule::SignalRecoveryCondition
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID ExtensionModule::SignalRecoveryCondition(
    UINT32 cameraId)
{
    if (TRUE == CheckAndSetRecovery(cameraId))
    {
        CHX_LOG_CONFIG("Signaling trigger recovery for cameraId=%d", cameraId);

        m_pTriggerRecoveryLock[cameraId]->Lock();
        m_bTeardownRequired[cameraId]  = TRUE;
        m_IsRecoverySignaled[cameraId] = TRUE;
        m_pTriggerRecoveryCondition[cameraId]->Signal();
        m_pTriggerRecoveryLock[cameraId]->Unlock();
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ExtensionModule::RecoveryThread
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID* ExtensionModule::RecoveryThread(
    VOID* pThreadData)
{
    PerThreadData*      pPerThreadData                    = reinterpret_cast<PerThreadData*>(pThreadData);
    RecoveryThreadPrivateData* pRecoveryThreadPrivateData =
        reinterpret_cast<RecoveryThreadPrivateData*>(pPerThreadData->pPrivateData);
    ExtensionModule*    pExtensionModule           = reinterpret_cast<ExtensionModule*>(pRecoveryThreadPrivateData->pData);

    UINT32 logicalCameraId = pRecoveryThreadPrivateData->logicalCameraId;

    pExtensionModule->RequestThreadProcessing(logicalCameraId);

    return NULL;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ExtensionModule::OfflineLoggerASCIIThread
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID* ExtensionModule::OfflineLoggerASCIIThread(
    VOID* pThreadData)
{
    PerThreadData*      pPerThreadData   = reinterpret_cast<PerThreadData*>(pThreadData);
    ExtensionModule*    pExtensionModule = reinterpret_cast<ExtensionModule*>(pPerThreadData->pPrivateData);

    pExtensionModule->RequestOfflineLogThreadProcessing();

    return NULL;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ExtensionModule::OfflineLoggerBinaryThread
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID* ExtensionModule::OfflineLoggerBinaryThread(
    VOID* pThreadData)
{
    PerThreadData*      pPerThreadData   = reinterpret_cast<PerThreadData*>(pThreadData);
    ExtensionModule*    pExtensionModule = reinterpret_cast<ExtensionModule*>(pPerThreadData->pPrivateData);

    pExtensionModule->RequestBinaryLogThreadProcessing();

    return NULL;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ExtensionModule::SignalOfflineLoggerThread
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID ExtensionModule::SignalOfflineLoggerThread(OfflineLoggerType logger_type)
{
    // Allow caller to signal Offline logger thread to trigger flush
    m_pFlushNeededCond[static_cast<UINT>(logger_type)]->Signal();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ExtensionModule::RequestOfflineLogThreadProcessing
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID ExtensionModule::RequestOfflineLogThreadProcessing()
{
    CDKResult result  = CDKResultSuccess;
    UINT ascii_logger = static_cast<UINT>(OfflineLoggerType::ASCII);
    while (TRUE)
    {
        if (FALSE == m_terminateOfflineLogThread)
        {
            m_pFlushNeededMutex[ascii_logger]->Lock();
            // Wait for flush trigger logic to be satisfied before executing
            m_pFlushNeededCond[ascii_logger]->Wait(m_pFlushNeededMutex[ascii_logger]->GetNativeHandle());
            m_pFlushNeededMutex[ascii_logger]->Unlock();
        }
        else
        {
            // Terminate thread
            CHX_LOG_INFO("ASCII OfflineLogger Thread is now terminated!");
            break;
        }

        if (NULL != m_offlineLoggerHandle)
        {
            if (NULL != m_offlineLoggerOps.pFlushLog)
            {
                if (TRUE == m_bAsciiLogEnable)
                {
                    // This is offlinelogger thread to continuously flush log into file system.
                    m_offlineLoggerOps.pFlushLog(OfflineLoggerType::ASCII, FALSE);
                }
            }
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ExtensionModule::RequestBinaryLogThreadProcessing
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID ExtensionModule::RequestBinaryLogThreadProcessing()
{
    CDKResult result   = CDKResultSuccess;
    UINT binary_logger = static_cast<UINT>(OfflineLoggerType::BINARY);
    while (TRUE)
    {
        if (FALSE == m_terminateOfflineLogThread)
        {
            m_pFlushNeededMutex[binary_logger]->Lock();
            // Wait for flush trigger logic to be satisfied before executing
            m_pFlushNeededCond[binary_logger]->Wait(
                      m_pFlushNeededMutex[binary_logger]->GetNativeHandle());
            m_pFlushNeededMutex[binary_logger]->Unlock();
        }
        else
        {
            // Terminate thread
            CHX_LOG_INFO("BinaryLogger Thread is now terminated!");
            break;
        }

        if (NULL != m_offlineLoggerHandle)
        {
            if (NULL != m_offlineLoggerOps.pFlushLog)
            {
                if (TRUE == m_bBinaryLogEnable)
                {
                    // This is offlinelogger thread to continuously flush log into file system.
                    m_offlineLoggerOps.pFlushLog(OfflineLoggerType::BINARY, FALSE);
                }
            }
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ExtensionModule::RequestThreadProcessing
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID ExtensionModule::RequestThreadProcessing(
    UINT32 logicalCameraId)
{
    BOOL     isTeardownInProgress               = FALSE;
    Usecase* pUsecase                           = NULL;
    m_consecutiveRecoveryCount[logicalCameraId] = 0;
    m_lastFrameBeforeRecovery[logicalCameraId]  = 0;

    CHX_LOG_CONFIG("Entering Recovery thread of logical cameraId: %d", logicalCameraId);
    while (TRUE)
    {
        m_pTriggerRecoveryLock[logicalCameraId]->Lock();
        if ((FALSE == m_IsRecoverySignaled[logicalCameraId]) && (FALSE == m_terminateRecoveryThread[logicalCameraId]))
        {
            m_pTriggerRecoveryCondition[logicalCameraId]->Wait(m_pTriggerRecoveryLock[logicalCameraId]->GetNativeHandle());
        }
        m_pTriggerRecoveryLock[logicalCameraId]->Unlock();

        // Stop triggering recovery if there are too many recovery within short period of
        // time. If that happens, it indicates that camera driver is in bad state

        // compare the previous and the current frame number and increment the counter that
        // track consecutive recovery
        if ((m_originalFrameWorkNumber[logicalCameraId] - m_lastFrameBeforeRecovery[logicalCameraId])
            < MaxNumFramesForConsecutiveRecovery)
        {
            m_consecutiveRecoveryCount[logicalCameraId]++;
        }
        else
        {
            if (0 != m_consecutiveRecoveryCount[logicalCameraId])
            {
                m_consecutiveRecoveryCount[logicalCameraId] = 0;
            }
            // increase the recovery count to account for the 1st recovery
            else
            {
                m_consecutiveRecoveryCount[logicalCameraId]++;
            }
        }

        // Save the last frame number before triggering recovery
        m_lastFrameBeforeRecovery[logicalCameraId] = m_originalFrameWorkNumber[logicalCameraId];

        // if counter reaches a threshold stop recovering, call sigabort instead
        if (m_consecutiveRecoveryCount[logicalCameraId] > MaxNumberOfConsecutiveRecoveryAllowed)
        {
            CHX_LOG_ERROR("FATAL: Consecutive %d recovery detected for logical cameraId: %d,"
                " stop trying recovery, raising sigabort",
                m_consecutiveRecoveryCount[logicalCameraId], logicalCameraId);
            ChxUtils::RaiseSignalAbort();
        }

        if (TRUE == m_terminateRecoveryThread[logicalCameraId])
        {
            CHX_LOG_CONFIG("Terminating recovery thread for logical cameraId: %d", logicalCameraId);
            break;
        }

        // Find usecase to teardown if it was signaled in signalrecoverycondition
        if (TRUE == m_bTeardownRequired[logicalCameraId])
        {
            CHX_LOG_INFO("Found usecase to teardown, cameraid: %d", logicalCameraId);
            pUsecase = m_pSelectedUsecase[logicalCameraId];

            if (NULL != pUsecase)
            {
                CHX_LOG_INFO("Preparing for recovery, cameraId: %d", logicalCameraId);
                pUsecase->PrepareForRecovery();
                CHX_LOG_CONFIG("Triggering recovery, cameraId: %d", logicalCameraId);
                TriggerRecovery(logicalCameraId);
            }
            else
            {
                CHX_LOG_ERROR("Could not trigger recovery, Null usecase");
            }
            m_IsRecoverySignaled[logicalCameraId] = FALSE;
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ExtensionModule::CheckAndSetRecovery
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL ExtensionModule::CheckAndSetRecovery(
    UINT32 logicalCameraId)
{
    BOOL setRecovery     = TRUE;
    BOOL flushInProgress = FALSE;

    // Set Recovery to true ONLY when both either teardown or HAL-flush are not in progress.
    // HAl-flush/teardown are from App/frameworks. Hence given higher priority over internal recovery.
    m_pRecoveryLock[logicalCameraId]->Lock();

    if (TRUE == static_cast<BOOL>(ChxUtils::AtomicLoadU32(&m_aFlushInProgress[logicalCameraId])))
    {
        flushInProgress = TRUE;
    }

    if ((TRUE == m_TeardownInProgress[logicalCameraId]) || (TRUE == flushInProgress))
    {
        CHX_LOG_INFO("Teardown already in progress, no need to recover");
        setRecovery = FALSE;
    }
    else
    {
        CHX_LOG_INFO("Teardown not in progress, start recovery");
        m_RecoveryInProgress[logicalCameraId] = TRUE;
    }
    m_pRecoveryLock[logicalCameraId]->Unlock();

    return setRecovery;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ExtensionModule::TriggerRecovery
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID ExtensionModule::TriggerRecovery(
    UINT32 logicalCameraId)
{
    UINT32      recoveryStartTime   = 0;
    UINT32      recoveryEndTime     = 0;
    CHIDateTime dateTime;

    CdkUtils::GetDateTime(&dateTime);
    recoveryStartTime = ((1000 * dateTime.seconds) + ((dateTime.microseconds) / 1000L));
    CHX_LOG_INFO("CHI override is recovering from an error, lets create case again for cameraId: %d", logicalCameraId);

    TeardownOverrideUsecase(m_logicalCameraInfo[logicalCameraId].m_pCamera3Device, TRUE);

    m_pSelectedUsecase[logicalCameraId]     = m_pUsecaseFactory->CreateUsecaseObject(&m_logicalCameraInfo[logicalCameraId],
                                                                static_cast<UsecaseId>(m_SelectedUsecaseId[logicalCameraId]),
                                                                m_pStreamConfig[logicalCameraId]);

    m_firstFrameAfterRecovery[logicalCameraId] = m_originalFrameWorkNumber[logicalCameraId] + 1;
    m_bTeardownRequired[logicalCameraId]       = FALSE;

    CdkUtils::GetDateTime(&dateTime);
    recoveryEndTime = ((1000 * dateTime.seconds) + ((dateTime.microseconds) / 1000L));
    CHX_LOG_INFO("CHI override has successfully recovered in %ums for CameraId %d use case is created for next request = %d",
                 (recoveryEndTime - recoveryStartTime),
                 logicalCameraId,
                 m_firstFrameAfterRecovery[logicalCameraId]);

    m_pRecoveryLock[logicalCameraId]->Lock();
    m_pRecoveryCondition[logicalCameraId]->Broadcast();
    m_RecoveryInProgress[logicalCameraId] = FALSE;
    m_pRecoveryLock[logicalCameraId]->Unlock();

    m_isUsecaseInBadState[logicalCameraId] = FALSE;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ExtensionModule::FreeLastKnownRequestSetting
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID ExtensionModule::FreeLastKnownRequestSetting(UINT32 logicalCameraId)
{
    if (logicalCameraId != CDKInvalidId && NULL != m_pLastKnownRequestSettings[logicalCameraId])
    {
        CHX_LOG_INFO("Freeing last known request setting");
        free_camera_metadata(m_pLastKnownRequestSettings[logicalCameraId]);
        m_pLastKnownRequestSettings[logicalCameraId] = NULL;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ExtensionModule::OverrideFlush
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ExtensionModule::OverrideFlush(
    const camera3_device_t* camera3_device)
{
    CDKResult result = CDKResultSuccess;

    UINT32 logicalCameraId = GetCameraIdfromDevice(camera3_device);

    if (CDKInvalidId != logicalCameraId)
    {
        m_pRecoveryLock[logicalCameraId]->Lock();
        if (TRUE == m_RecoveryInProgress[logicalCameraId])
        {
            CHX_LOG_INFO("Wait for recovery to finish, before starting flush");
            m_pRecoveryCondition[logicalCameraId]->Wait(m_pRecoveryLock[logicalCameraId]->GetNativeHandle());
        }
        ChxUtils::AtomicStoreU32(&m_aFlushInProgress[logicalCameraId], TRUE);
        m_hasFlushOccurred[logicalCameraId] = TRUE;
        m_pRecoveryLock[logicalCameraId]->Unlock();

        if (NULL != m_pPerfLockManager[logicalCameraId])
        {
            m_pPerfLockManager[logicalCameraId]->AcquirePerfLock(PERF_LOCK_CLOSE_CAMERA, 1000);
        }
        // If recovery fails, this may goes NULL
        if (NULL != m_pSelectedUsecase[logicalCameraId])
        {
            result = m_pSelectedUsecase[logicalCameraId]->Flush();
        }

        ChxUtils::AtomicStoreU32(&m_aFlushInProgress[logicalCameraId], FALSE);
    }
    else
    {
        CHX_LOG_WARN("Invalid logical camera id device:%p!!", camera3_device);
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ExtensionModule::OverrideDump
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ExtensionModule::OverrideDump(
    const camera3_device_t*     camera3_device,
    int                         fd)
{
    CDKResult   result          = CDKResultSuccess;

    UINT32 logicalCameraId = GetCameraIdfromDevice(camera3_device);

    if (logicalCameraId != CDKInvalidId)
    {
        if (NULL != m_pSelectedUsecase[logicalCameraId])
        {
            result = m_pSelectedUsecase[logicalCameraId]->Dump(fd);
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ExtensionModule::HandleProcessRequestErrorAllPCRs
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID ExtensionModule::HandleProcessRequestErrorAllPCRs(
    camera3_capture_request_t* pRequest,
    UINT32 logicalCameraId)
{
    ChiMessageDescriptor messageDescriptor;

    messageDescriptor.messageType                            = ChiMessageTypeError;
    messageDescriptor.message.errorMessage.frameworkFrameNum = pRequest->frame_number;
    messageDescriptor.message.errorMessage.errorMessageCode  = MessageCodeRequest;
    messageDescriptor.message.errorMessage.pErrorStream      = NULL;

    ReturnFrameworkMessage((camera3_notify_msg_t*)&messageDescriptor, logicalCameraId);

    camera3_capture_result_t result = {0};
    result.frame_number             = pRequest->frame_number;
    result.result                   = NULL;
    result.num_output_buffers       = pRequest->num_output_buffers;
    result.output_buffers           = reinterpret_cast<const camera3_stream_buffer_t *>(pRequest->output_buffers);
    result.partial_result           = 0;

    for (UINT i = 0; i < pRequest->num_output_buffers; i++)
    {
        camera3_stream_buffer_t* pStreamBuffer =
            const_cast<camera3_stream_buffer_t*>(&pRequest->output_buffers[i]);
        pStreamBuffer->release_fence = -1;
        pStreamBuffer->status = 1;
    }

    if(NULL != pRequest->input_buffer)
    {
        result.input_buffer = reinterpret_cast<const camera3_stream_buffer_t *>(pRequest->input_buffer);
        camera3_stream_buffer_t* pStreamBuffer =
            const_cast<camera3_stream_buffer_t*>(pRequest->input_buffer);
        pStreamBuffer->release_fence = -1;
        pStreamBuffer->status = 1;
    }
    else
    {
        result.input_buffer = NULL;
    }

    CHX_LOG_ERROR("Sending Request error for frame %d ", pRequest->frame_number);
    ReturnFrameworkResult(reinterpret_cast<const camera3_capture_result_t*>(&result), logicalCameraId);
}



////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ExtensionModule::CreatePipelineDescriptor
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CHIPIPELINEDESCRIPTOR ExtensionModule::CreatePipelineDescriptor(
    PipelineCreateData* pPipelineCreateData) ///< Pipeline create descriptor
{
    return (g_chiContextOps.pCreatePipelineDescriptor(m_hCHIContext,
                                                      pPipelineCreateData->pPipelineName,
                                                      pPipelineCreateData->pPipelineCreateDescriptor,
                                                      pPipelineCreateData->numOutputs,
                                                      pPipelineCreateData->pOutputDescriptors,
                                                      pPipelineCreateData->numInputs,
                                                      pPipelineCreateData->pInputOptions));
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ExtensionModule::DestroyPipelineDescriptor
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID ExtensionModule::DestroyPipelineDescriptor(
    CHIPIPELINEDESCRIPTOR pipelineHandle)
{
    return g_chiContextOps.pDestroyPipelineDescriptor(m_hCHIContext, pipelineHandle);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ExtensionModule::CreateSession
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CHIHANDLE ExtensionModule::CreateSession(
    SessionCreateData* pSessionCreateData) ///< Session create descriptor
{
    return g_chiContextOps.pCreateSession(m_hCHIContext,
                                          pSessionCreateData->numPipelines,
                                          pSessionCreateData->pPipelineInfo,
                                          pSessionCreateData->pCallbacks,
                                          pSessionCreateData->pPrivateCallbackData,
                                          pSessionCreateData->flags);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ExtensionModule::DestroySession
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID ExtensionModule::DestroySession(
    CHIHANDLE sessionHandle,
    BOOL isForced)
{
    return g_chiContextOps.pDestroySession(m_hCHIContext, sessionHandle, isForced);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ExtensionModule::Flush
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ExtensionModule::Flush(
    CHISESSIONFLUSHINFO hSessionFlushInfo)
{
    CHX_LOG("[E] Flushing Session Handle: %p with SessionFlushInfo", hSessionFlushInfo.pSessionHandle);
    return g_chiContextOps.pFlushSession(m_hCHIContext, hSessionFlushInfo);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ExtensionModule::Flush
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ExtensionModule::Flush(
    CHIHANDLE sessionHandle)
{
    CHX_LOG("Flushing Session Handle: %p", sessionHandle);
    CHISESSIONFLUSHINFO hSessionFlushInfo = { 0 };
    hSessionFlushInfo.pSessionHandle = sessionHandle;
    return g_chiContextOps.pFlushSession(m_hCHIContext, hSessionFlushInfo);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ExtensionModule::SubmitRequest
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ExtensionModule::SubmitRequest(
    CHIPIPELINEREQUEST* pSubmitRequest)
{
    CDKResult result = CDKResultSuccess;

    result = g_chiContextOps.pSubmitPipelineRequest(m_hCHIContext, pSubmitRequest);

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ExtensionModule::ActivatePipeline
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ExtensionModule::ActivatePipeline(
    CHIHANDLE             sessionHandle,
    CHIPIPELINEDESCRIPTOR pipelineHandle)
{
    return g_chiContextOps.pActivatePipeline(m_hCHIContext, sessionHandle, pipelineHandle, NULL);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ExtensionModule::DeactivatePipeline
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ExtensionModule::DeactivatePipeline(
    CHIHANDLE                 sessionHandle,
    CHIPIPELINEDESCRIPTOR     pipelineHandle,
    CHIDEACTIVATEPIPELINEMODE modeBitmask)
{
    return g_chiContextOps.pDeactivatePipeline(m_hCHIContext, sessionHandle, pipelineHandle, modeBitmask);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ExtensionModule::CreateChiFence
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ExtensionModule::CreateChiFence(
    CHIFENCECREATEPARAMS*  pInfo,
    CHIFENCEHANDLE*        phChiFence)
{
    CDKResult result = CDKResultEFailed;

    if (NULL != m_chiFenceOps.pCreateFence)
    {
        result = m_chiFenceOps.pCreateFence(m_hCHIContext, pInfo, phChiFence);
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ExtensionModule::ReleaseChiFence
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ExtensionModule::ReleaseChiFence(
    CHIFENCEHANDLE  hChiFence)
{
    CDKResult result = CDKResultEFailed;

    if (NULL != m_chiFenceOps.pReleaseFence)
    {
        result = m_chiFenceOps.pReleaseFence(m_hCHIContext, hChiFence);
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ExtensionModule::WaitChiFenceAsync
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ExtensionModule::WaitChiFenceAsync(
    PFNCHIFENCECALLBACK  pCallbackFn,
    CHIFENCEHANDLE       hChiFence,
    VOID*                pData)
{
    CDKResult result = CDKResultEFailed;

    if (NULL != m_chiFenceOps.pWaitFenceAsync)
    {
        result = m_chiFenceOps.pWaitFenceAsync(m_hCHIContext, pCallbackFn, hChiFence, pData);
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ExtensionModule::SignalChiFence
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ExtensionModule::SignalChiFence(
    CHIFENCEHANDLE  hChiFence,
    CDKResult       statusResult)
{
    CDKResult result = CDKResultEFailed;

    if (NULL != m_chiFenceOps.pSignalFence)
    {
        result = m_chiFenceOps.pSignalFence(m_hCHIContext, hChiFence, statusResult);
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ExtensionModule::GetChiFenceStatus
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ExtensionModule::GetChiFenceStatus(
    CHIFENCEHANDLE  hChiFence,
    CDKResult*      pFenceResult)
{
    CDKResult result = CDKResultEFailed;

    if (NULL != m_chiFenceOps.pGetFenceStatus)
    {
        result = m_chiFenceOps.pGetFenceStatus(m_hCHIContext, hChiFence, pFenceResult);
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ExtensionModule::QueryPipelineMetadataInfo
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ExtensionModule::QueryPipelineMetadataInfo(
    CHIHANDLE                   sessionHandle,
    CHIPIPELINEDESCRIPTOR       pipelineHandle,
    CHIPIPELINEMETADATAINFO*    pPipelineMetadataInfo)
{
    return g_chiContextOps.pQueryPipelineMetadataInfo(m_hCHIContext, sessionHandle, pipelineHandle, pPipelineMetadataInfo);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ExtensionModule::EnumerateSensorModes
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ExtensionModule::EnumerateSensorModes(
    UINT32             physCameraId,
    UINT32             numSensorModes,
    CHISENSORMODEINFO* pSensorModeInfo)
{
    return g_chiContextOps.pEnumerateSensorModes(m_hCHIContext, physCameraId, numSensorModes, pSensorModeInfo);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ExtensionModule::GetVendorTagOps
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID ExtensionModule::GetVendorTagOps(
    CHITAGSOPS* pVendorTagOps)
{
    g_chiContextOps.pTagOps(pVendorTagOps);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ExtensionModule::GetMetadataOps
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID ExtensionModule::GetMetadataOps(
    CHIMETADATAOPS* pMetadataOps)
{
    g_chiContextOps.pMetadataOps(pMetadataOps);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ExtensionModule::GetAvailableRequestKeys
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ExtensionModule::GetAvailableRequestKeys(
    UINT32   logicalCameraId,
    UINT32*  pTagList,
    UINT32   maxTagListCount,
    UINT32*  pTagCount)
{
    CDKResult result = CDKResultSuccess;

    camera_metadata_entry_t entry = { 0 };

    entry.tag = ANDROID_REQUEST_AVAILABLE_REQUEST_KEYS;

    find_camera_metadata_entry((camera_metadata_t*)(
        m_logicalCameraInfo[logicalCameraId].m_cameraInfo.static_camera_characteristics),
        entry.tag,
        &entry);

    if (maxTagListCount > entry.count)
    {
        ChxUtils::Memcpy(pTagList, entry.data.i32, entry.count * sizeof(UINT32));
        *pTagCount = entry.count;
    }
    else
    {
        result = CDKResultENeedMore;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ExtensionModule::GetPhysicalCameraSensorModes
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ExtensionModule::GetPhysicalCameraSensorModes(
    UINT32              physicalCameraId,
    UINT32*             pNumSensorModes,
    CHISENSORMODEINFO** ppAllSensorModes)
{
    CDKResult               result              = CDKResultEInvalidArg;
    const LogicalCameraInfo *pLogicalCameraInfo = NULL;

    if (physicalCameraId < m_numPhysicalCameras)
    {
        pLogicalCameraInfo = GetPhysicalCameraInfo(physicalCameraId);

        if ((NULL != pLogicalCameraInfo) && (NULL != pNumSensorModes) && (NULL != ppAllSensorModes))
        {
            *pNumSensorModes  = pLogicalCameraInfo->m_cameraCaps.numSensorModes;
            *ppAllSensorModes = pLogicalCameraInfo->pSensorModeInfo;
            result            = CDKResultSuccess;
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ExtensionModule::ReturnFrameworkResult
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID ExtensionModule::ReturnFrameworkResult(
    const camera3_capture_result_t* pResult,
    UINT32 cameraID)
{
    if ((NULL != m_pPerfLockManager[cameraID]) && (FALSE == m_firstResult))
    {
        m_pPerfLockManager[cameraID]->AcquirePerfLock(m_CurrentpowerHint);
        m_previousPowerHint = m_CurrentpowerHint;
        m_firstResult = TRUE;
    }

    if (pResult->frame_number == m_longExposureFrame)
    {
        if (pResult->num_output_buffers != 0)
        {
            CHX_LOG_INFO("Returning long exposure snapshot");
            ChxUtils::AtomicStoreU32(&m_aLongExposureInProgress, FALSE);
            m_longExposureFrame = static_cast<UINT32>(InvalidFrameNumber);
        }
    }

    m_HALOps[cameraID].process_capture_result(m_logicalCameraInfo[cameraID].m_pCamera3Device, pResult);

    if (pResult->output_buffers != NULL)
    {
        for (UINT i = 0; i < pResult->num_output_buffers; i++)
        {
            if ((NULL != m_pPerfLockManager[cameraID]) &&
                (pResult->output_buffers[i].stream->format == ChiStreamFormatBlob) &&
                ((pResult->output_buffers[i].stream->data_space == static_cast<android_dataspace_t>(DataspaceV0JFIF)) ||
                (pResult->output_buffers[i].stream->data_space == static_cast<android_dataspace_t>(DataspaceJFIF))))
            {
                 m_pPerfLockManager[cameraID]->ReleasePerfLock(PERF_LOCK_SNAPSHOT_CAPTURE);
                 break;
            }
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ExtensionModule::ReturnFrameworkMessage
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID ExtensionModule::ReturnFrameworkMessage(
    const camera3_notify_msg_t* pMessage,
    UINT32 cameraID)
{
    m_HALOps[cameraID].notify_result(m_logicalCameraInfo[cameraID].m_pCamera3Device, pMessage);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ExtensionModule::DumpDebugData
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID ExtensionModule::DumpDebugData(UINT32 cameraID)
{
    CHX_LOG_INFO("m_pchiStreamConfig->num_streams =  %d", m_pStreamConfig[cameraID]->num_streams);
    for (UINT32 stream = 0; stream < m_pStreamConfig[cameraID]->num_streams; stream++)
    {
        if (NULL != m_pStreamConfig[cameraID]->streams[stream])
        {
            CHX_LOG_INFO("  stream[%d] = %p - info:", stream,
                m_pStreamConfig[cameraID]->streams[stream]);
            CHX_LOG_INFO("            format       : %d",
                m_pStreamConfig[cameraID]->streams[stream]->format);
            CHX_LOG_INFO("            width        : %d",
                m_pStreamConfig[cameraID]->streams[stream]->width);
            CHX_LOG_INFO("            height       : %d",
                m_pStreamConfig[cameraID]->streams[stream]->height);
            CHX_LOG_INFO("            stream_type  : %08x",
                m_pStreamConfig[cameraID]->streams[stream]->stream_type);
            CHX_LOG_ERROR("            usage        : %08x",
                m_pStreamConfig[cameraID]->streams[stream]->usage);
            CHX_LOG_INFO("            max_buffers  : %d",
                m_pStreamConfig[cameraID]->streams[stream]->max_buffers);
            CHX_LOG_INFO("            rotation     : %08x",
                m_pStreamConfig[cameraID]->streams[stream]->rotation);
            CHX_LOG_INFO("            data_space   : %08x",
                m_pStreamConfig[cameraID]->streams[stream]->data_space);
        }
        else
        {
            CHX_LOG_ERROR("Invalid streamconfig info");
            break;
        }
    }
    CHX_LOG_INFO("  operation_mode: %d", m_pStreamConfig[cameraID]->operation_mode);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ExtensionModule::GetSelectedResolutionForActiveSensorMode
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UINT32 ExtensionModule::GetSelectedResolutionForActiveSensorMode(
    UINT32                          physCameraId,
    camera3_stream_configuration_t* pStreamConfig)
{
    CHISENSORMODEINFO* sensorMode   = UsecaseSelector::GetSensorModeInfo(physCameraId, pStreamConfig, 1);
    UINT32 selectedSensorResolution = sensorMode->frameDimension.width;

    return selectedSensorResolution;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ExtensionModule::GetCameraIdfromDevice
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UINT32 ExtensionModule::GetCameraIdfromDevice(const camera3_device_t *pCamera3Device)
{
    UINT32 id = CDKInvalidId;

    for (UINT32 i = 0; i < MaxNumImageSensors; i++)
    {
        if ((NULL != m_logicalCameraInfo[i].m_pCamera3Device) &&
            (m_logicalCameraInfo[i].m_pCamera3Device == pCamera3Device))
        {
            id = i;
            break;
        }
    }
    return id;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ExtensionModule::GetCameraType
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
LogicalCameraType ExtensionModule::GetCameraType(UINT32 cameraId) const
{
    CHIHANDLE         staticMetaDataHandle = const_cast<camera_metadata_t*>(
            m_logicalCameraInfo[cameraId].m_cameraInfo.static_camera_characteristics);
    CHITAGSOPS        tagOps               = { 0 };
    LogicalCameraType logicalCameraType    = LogicalCameraType_Default;
    UINT32            tag;

    g_chiContextOps.pTagOps(&tagOps);

    if (CDKResultSuccess == tagOps.pQueryVendorTagLocation("org.codeaurora.qcamera3.logicalCameraType",
            "logical_camera_type", &tag))
    {
        tagOps.pGetMetaData(staticMetaDataHandle, tag, &logicalCameraType, sizeof(SBYTE));
    }

    return logicalCameraType;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ExtensionModule::SortCameras
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ExtensionModule::SortCameras()
{
    CDKResult                   result = CDKResultSuccess;
    INT                         j      = 0;
    LogicalCameraConfiguration  temp;

    for (INT i = 1; i < static_cast<INT>(m_numOfLogicalCameraConfiguration); i++)
    {
        temp = m_pLogicalCameraConfigurationInfo[i];

        for (j = i - 1; (j >= 0) && (m_pLogicalCameraConfigurationInfo[j].logicalCameraId > temp.logicalCameraId); j--)
        {
            m_pLogicalCameraConfigurationInfo[j + 1] = m_pLogicalCameraConfigurationInfo[j];
        }

        m_pLogicalCameraConfigurationInfo[j + 1] = temp;
    }
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ExtensionModule::GetDevicesSettings
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ExtensionModule::GetDevicesSettings()
{
    CDKResult    result            = CDKResultSuccess;
    const UINT32 numExtendSettings = static_cast<UINT32>(ChxSettingsToken::CHX_SETTINGS_TOKEN_COUNT);

    // Pointers are allocated by CamX to provide static settings to CHI
    CHIEXTENDSETTINGS* extTokens;
    CHIMODIFYSETTINGS* extSettings;

    // Get the settings and update internal structures.
    g_chiContextOps.pGetSettings(&extTokens, &extSettings);
    MappingConfigSettings(extTokens->numTokens, extTokens->pTokens);
    for (UINT i = 0; i < numExtendSettings; i++)
    {
        ModifySettings(&extSettings[i]);
    }

    // Free structures allocated by CamX for static settings exchange.
    // TODO: This is potentially dangerous as if MEMSPY is enabled, the
    //       CAMX and CHI Free calls differ.
    CHX_FREE(extTokens->pTokens);
    CHX_FREE(extTokens);
    CHX_FREE(extSettings);

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ExtensionModule::InitializeAvailableStreamConfig
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ExtensionModule::InitializeAvailableStreamMap(UINT32 cameraId)
{
    CDKResult result = CDKResultSuccess;

    if (NULL == m_logicalCameraInfo[cameraId].pAvailableStreamMap)
    {
        UINT32 numStreamMap = sizeof(streamMap)/sizeof(StreamMap);

        if (0 != numStreamMap)
        {
             m_logicalCameraInfo[cameraId].pAvailableStreamMap = static_cast<StreamMap*>
                (CHX_CALLOC(numStreamMap * sizeof(StreamMap)));

             if (NULL != m_logicalCameraInfo[cameraId].pAvailableStreamMap)
             {
                 for (UINT32 i = 0; i < numStreamMap; i++)
                 {
                     StreamMap* pStreamMap = &m_logicalCameraInfo[cameraId].pAvailableStreamMap[i];
                     *pStreamMap = streamMap[i];
                     m_logicalCameraInfo[cameraId].numAvailableStreamMap++;
                 }
             }
             else
             {
                 result = CDKResultEFailed;
             }
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ExtensionModule::UpdateStaticSettings
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ExtensionModule::UpdateStaticSettings(UINT32 logicalCameraId)
{
    CDKResult result      = CDKResultSuccess;
    CDKResult tagOpResult = CDKResultEFailed;

    CHITAGSOPS   tagOps       = { 0 };
    UINT32       tagLocation  = 0;

    g_chiContextOps.pTagOps(&tagOps);


    if (CDKResultSuccess == result)
    {
        camera_metadata_entry_t entry      = {0};
        camera_metadata_t* static_metadata = const_cast<camera_metadata_t *>(
            m_logicalCameraInfo[logicalCameraId].m_cameraInfo.static_camera_characteristics);

        if (CDKResultSuccess == tagOps.pQueryVendorTagLocation("org.codeaurora.qcamera3.sessionParameters",
            "availableStreamMap", &tagLocation))
        {
            tagOpResult = tagOps.pSetMetaData(
                static_metadata,
                tagLocation,
                m_logicalCameraInfo[logicalCameraId].pAvailableStreamMap,
                (sizeof(StreamMap) * m_logicalCameraInfo[logicalCameraId].numAvailableStreamMap));

            if (CDKResultSuccess != tagOpResult)
            {
                CHX_LOG_ERROR("Could not add availableStreamMap  = %d  %p cameraId = %d tagLocation = %d",
                    m_logicalCameraInfo[logicalCameraId].numAvailableStreamMap,
                    m_logicalCameraInfo[logicalCameraId].pAvailableStreamMap,
                    m_numLogicalCameras, tagLocation);
            }
        }

        const CHAR* pTagNames[] = { "overrideResourceCostValidation", "availableStreamMap" };

        for (UINT sessionTagIndex = 0; sessionTagIndex < CHX_ARRAY_SIZE(pTagNames); ++sessionTagIndex)
        {
            if (0 == find_camera_metadata_entry(static_metadata, ANDROID_REQUEST_AVAILABLE_SESSION_KEYS, &entry))
            {
                BOOL sessionKeyUpdated = FALSE;
                std::vector<INT32> availableSessionKeys(entry.data.i32, entry.data.i32 + entry.count);

                tagOpResult = tagOps.pQueryVendorTagLocation(
                    "org.codeaurora.qcamera3.sessionParameters",
                    pTagNames[sessionTagIndex],
                    &tagLocation);

                if (CDKResultSuccess == tagOpResult)
                {
                    if (std::find(availableSessionKeys.begin(), availableSessionKeys.end(), tagLocation)
                        == availableSessionKeys.end())
                    {
                        sessionKeyUpdated = TRUE;
                        availableSessionKeys.push_back(static_cast<INT32>(tagLocation));
                    }
                }


                if ((0 != availableSessionKeys.size()) && (NULL != availableSessionKeys.data()))
                {
                    if (0 != update_camera_metadata_entry(static_metadata, entry.index,
                        (const void*)availableSessionKeys.data(), availableSessionKeys.size(),
                        NULL))
                    {
                        CHX_LOG_ERROR("Failed to set session key");
                    }
                }

                if ((0 == find_camera_metadata_entry(static_metadata, ANDROID_REQUEST_AVAILABLE_REQUEST_KEYS, &entry)) &&
                    (TRUE == sessionKeyUpdated))
                {
                    std::vector<INT32> availableRequestKeys(entry.data.i32, entry.data.i32 + entry.count);

                    tagOpResult = tagOps.pQueryVendorTagLocation(
                        "org.codeaurora.qcamera3.sessionParameters",
                        pTagNames[sessionTagIndex],
                        &tagLocation);

                    if (CDKResultSuccess == tagOpResult)
                    {
                        if (std::find(availableRequestKeys.begin(), availableRequestKeys.end(), tagLocation)
                            == availableRequestKeys.end())
                        {
                            availableRequestKeys.push_back(static_cast<INT32>(tagLocation));
                        }
                    }

                    if ((0 != availableRequestKeys.size()) && (NULL != availableRequestKeys.data()))
                    {
                        if (0 != update_camera_metadata_entry(static_metadata, entry.index,
                            (const void*)availableRequestKeys.data(), availableRequestKeys.size(),
                            NULL))
                        {
                            CHX_LOG_ERROR("Failed to set session key");
                        }
                    }
                }
                else
                {
                    CHX_LOG_ERROR("Failed to find ANDROID_REQUEST_AVAILABLE_REQUEST_KEYS entry");
                }
            }
            else
            {
                CHX_LOG_ERROR("Failed to find ANDROID_REQUEST_AVAILABLE_SESSION_KEYS entry");
            }
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ExtensionModule::GetMatchingStreamMapData
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ExtensionModule::GetMatchingStreamMapData(
    camera3_stream_configuration_t* pStreamConfig,
    camera3_stream_t*               pStream,
    const camera_metadata_t*        pSessionParam,
    StreamMap*                      pStreamMapData)
{
    CDKResult result               = CDKResultSuccess;
    INT32     streamSequenceIndex  = 0;
    CDKResult tagOpResult          = CDKResultEFailed;

    if ((NULL == pStream) || (NULL == pSessionParam) || (NULL == pStreamMapData) ||
        (NULL == pStreamConfig))
    {
        CHX_LOG_ERROR("Invalid Arguments");
        result = CDKResultEFailed;
    }

    if (CDKResultSuccess == result)
    {
        CHITAGSOPS   tagOps       = { 0 };
        UINT32       tagLocation  = 0;

        g_chiContextOps.pTagOps(&tagOps);

        for (UINT stream = 0; stream < pStreamConfig->num_streams; stream++)
        {
            camera3_stream_t* p_stream = pStreamConfig->streams[stream];

            if (pStream == p_stream)
            {
                break;
            }
            if( ((HAL_PIXEL_FORMAT_RAW10 == pStream->format) ||
                (HAL_PIXEL_FORMAT_RAW16 == pStream->format)) &&
                (pStream->format == p_stream->format) )
            {
                streamSequenceIndex++;
            }
            else if ((pStream->format == p_stream->format) &&
                ((pStream->width * pStream->height) == (p_stream->width * p_stream->height)))
            {
                streamSequenceIndex++;
            }
        }

        tagOpResult = tagOps.pQueryVendorTagLocation(
            "org.codeaurora.qcamera3.sessionParameters",
            "availableStreamMap",
            &tagLocation);

        if (CDKResultSuccess == tagOpResult)
        {
            camera_metadata_entry_t entry = { 0 };
            camera_metadata_t *metadata   = const_cast<camera_metadata_t*>(pSessionParam);

            if (0 == find_camera_metadata_entry(metadata, tagLocation, &entry))
            {
                UINT count = 0;
                UINT found = FALSE;
                for (UINT32 i = 0; i < entry.count;)
                {
                    pStreamMapData->width                 = entry.data.i32[i];
                    pStreamMapData->height                = entry.data.i32[i + 1];
                    pStreamMapData->format                = entry.data.i32[i + 2];
                    pStreamMapData->sequenceIndex         = entry.data.i32[i + 3];
                    pStreamMapData->streamIntent          = entry.data.i32[i + 4];
                    pStreamMapData->cameraIndex           = entry.data.i32[i + 5];
                    pStreamMapData->isPhysicalStream      = entry.data.i32[i + 6];
                    pStreamMapData->isThumbnailPostView   = entry.data.i32[i + 7];

                    if (((pStreamMapData->width * pStreamMapData->height) == 0) &&
                        (pStream->format == pStreamMapData->format))
                    {
                        CHX_LOG_WARN("Stream Mapping data not available. Using Default");

                        if( (HAL_PIXEL_FORMAT_RAW10 == pStream->format) ||
                            (HAL_PIXEL_FORMAT_RAW16 == pStream->format))
                        {
                            if ((ANDROID_CONTROL_CAPTURE_INTENT_STILL_CAPTURE == pStreamMapData->streamIntent) &&
                                (streamSequenceIndex == pStreamMapData->sequenceIndex))
                            {
                                found = TRUE;
                                break;
                            }
                        }
                        else if ((pStream->width * pStream->height) < SnapshotYUVOutputCap)
                        {
                            if (pStreamMapData->streamIntent == ANDROID_CONTROL_CAPTURE_INTENT_PREVIEW)
                            {
                                found = TRUE;
                                break;
                            }
                        }
                        else
                        {
                            if ((pStreamMapData->streamIntent == ANDROID_CONTROL_CAPTURE_INTENT_STILL_CAPTURE) &&
                                (streamSequenceIndex == pStreamMapData->sequenceIndex))
                            {
                                found = TRUE;
                                break;
                            }
                        }

                        if (TRUE == found)
                        {
                            break;
                        }
                    }
                    else if (((pStream->width * pStream->height) == (pStreamMapData->width * pStreamMapData->height)) &&
                             (pStream->format == pStreamMapData->format) &&
                             (streamSequenceIndex == pStreamMapData->sequenceIndex))
                    {
                        found = TRUE;
                        break;
                    }
                    count++;
                    i = i + (sizeof(StreamMap)/sizeof(INT32));
                }

                if (FALSE == found)
                {
                    ChxUtils::Memset(pStreamMapData, 0, sizeof(StreamMap));
                }
                else
                {
                    CHX_LOG_INFO("Matched Stream : %p sequence: %d Res:%dX%d Format: %d sequence: %d Intent = %d"
                        "physicalStream: %d, thumbnailPostview: %d",
                        pStream, streamSequenceIndex,
                        pStreamMapData->width,pStreamMapData->height,
                        pStreamMapData->format, pStreamMapData->sequenceIndex,
                        pStreamMapData->streamIntent,
                        pStreamMapData->isPhysicalStream,
                        pStreamMapData->isThumbnailPostView);
                }
            }
            else
            {
                CHX_LOG_ERROR("Could not find Session Tag for streamMapping");
                result = CDKResultEFailed;
            }
        }
        else
        {
            CHX_LOG_ERROR("Query Failed for streamMapping");
            result = CDKResultEFailed;
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ExtensionModule::MappingConfigSettings
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID ExtensionModule::MappingConfigSettings(
    UINT32  numTokens,
    VOID*   pInputTokens)
{
    ChiOverrideToken*   pTokens = static_cast<ChiOverrideToken*>(pInputTokens);

    for (UINT i = 0; i < numTokens; i++)
    {
        switch (pTokens[i].id)
        {
            case static_cast<UINT32>(ChxSettingsToken::OverrideForceUsecaseId):
                m_pForceUsecaseId                 = &m_pConfigSettings[i];
                break;
            case static_cast<UINT32>(ChxSettingsToken::OverrideDisableZSL):
                m_pDisableZSL                     = &m_pConfigSettings[i];
                break;
            case static_cast<UINT32>(ChxSettingsToken::OverrideGPURotationUsecase):
                m_pGPUNodeRotateUsecase           = &m_pConfigSettings[i];
                break;
            case static_cast<UINT32>(ChxSettingsToken::OverrideEnableMFNR):
                m_pEnableMFNRUsecase              = &m_pConfigSettings[i];
                break;
            case static_cast<UINT32>(ChxSettingsToken::AnchorSelectionAlgoForMFNR):
                m_pAnchorSelectionAlgoForMFNR     = &m_pConfigSettings[i];
                break;
            case static_cast<UINT32>(ChxSettingsToken::OverrideHFRNo3AUseCase):
                m_pHFRNo3AUsecase                 = &m_pConfigSettings[i];
                break;
            case static_cast<UINT32>(ChxSettingsToken::OverrideForceSensorMode):
                m_pForceSensorMode                = &m_pConfigSettings[i];
                break;
            case static_cast<UINT32>(ChxSettingsToken::DefaultMaxFPS):
                m_pDefaultMaxFPS                  = &m_pConfigSettings[i];
                break;
            case static_cast<UINT32>(ChxSettingsToken::FovcEnable):
                m_pEnableFOVC                     = &m_pConfigSettings[i];
                break;
            case static_cast<UINT32>(ChxSettingsToken::OverrideCameraClose):
                m_pOverrideCameraClose            = &m_pConfigSettings[i];
                break;
            case static_cast<UINT32>(ChxSettingsToken::OverrideCameraOpen):
                m_pOverrideCameraOpen             = &m_pConfigSettings[i];
                break;
            case static_cast<UINT32>(ChxSettingsToken::EISV2Enable):
                m_pEISV2Enable                    = &m_pConfigSettings[i];
                break;
            case static_cast<UINT32>(ChxSettingsToken::EISV3Enable):
                m_pEISV3Enable                    = &m_pConfigSettings[i];
                break;
            case static_cast<UINT32>(ChxSettingsToken::NumPCRsBeforeStreamOn):
                m_pNumPCRsBeforeStreamOn          = &m_pConfigSettings[i];
                break;
            case static_cast<UINT32>(ChxSettingsToken::StatsProcessingSkipFactor):
                m_pStatsSkipPattern               = &m_pConfigSettings[i];
                break;
            case static_cast<UINT32>(ChxSettingsToken::DumpDebugDataEveryProcessResult):
                m_pEnableDumpDebugData            = &m_pConfigSettings[i];
                break;
            case static_cast<UINT32>(ChxSettingsToken::Enable3ADebugData):
                m_pEnable3ADebugData              = &m_pConfigSettings[i];
                break;
            case static_cast<UINT32>(ChxSettingsToken::EnableConcise3ADebugData) :
                m_pEnableConcise3ADebugData       = &m_pConfigSettings[i];
                break;
            case static_cast<UINT32>(ChxSettingsToken::EnableTuningMetadata):
                m_pEnableTuningMetadata           = &m_pConfigSettings[i];
                break;
            case static_cast<UINT32>(ChxSettingsToken::DebugDataSizeAEC):
                m_pDebugDataSizeAEC               = &m_pConfigSettings[i];
                break;
            case static_cast<UINT32>(ChxSettingsToken::DebugDataSizeAWB):
                m_pDebugDataSizeAWB               = &m_pConfigSettings[i];
                break;
            case static_cast<UINT32>(ChxSettingsToken::DebugDataSizeAF):
                m_pDebugDataSizeAF                = &m_pConfigSettings[i];
                break;
            case static_cast<UINT32>(ChxSettingsToken::ConciseDebugDataSizeAEC) :
                m_pConciseDebugDataSizeAEC        = &m_pConfigSettings[i];
                break;
            case static_cast<UINT32>(ChxSettingsToken::ConciseDebugDataSizeAWB) :
                m_pConciseDebugDataSizeAWB        = &m_pConfigSettings[i];
                break;
            case static_cast<UINT32>(ChxSettingsToken::ConciseDebugDataSizeAF) :
                m_pConciseDebugDataSizeAF         = &m_pConfigSettings[i];
                break;
            case static_cast<UINT32>(ChxSettingsToken::TuningDumpDataSizeIFE):
                m_pTuningDumpDataSizeIFE          = &m_pConfigSettings[i];
                break;
            case static_cast<UINT32>(ChxSettingsToken::TuningDumpDataSizeIPE):
                m_pTuningDumpDataSizeIPE          = &m_pConfigSettings[i];
                break;
            case static_cast<UINT32>(ChxSettingsToken::TuningDumpDataSizeBPS):
                m_pTuningDumpDataSizeBPS          = &m_pConfigSettings[i];
                break;
            case static_cast<UINT32>(ChxSettingsToken::MultiCameraVREnable):
                m_pEnableMultiVRCamera            = &m_pConfigSettings[i];
                break;
            case static_cast<UINT32>(ChxSettingsToken::OverrideGPUDownscaleUsecase):
                m_pGPUNodeDownscaleUsecase        = &m_pConfigSettings[i];
                break;
            case static_cast<UINT32>(ChxSettingsToken::AdvanceFeatureMask):
                m_pAdvanceFeataureMask            = &m_pConfigSettings[i];
                break;
            case static_cast<UINT32>(ChxSettingsToken::DisableASDStatsProcessing):
                m_pDisableASDProcessing           = &m_pConfigSettings[i];
                break;
            case static_cast<UINT32>(ChxSettingsToken::MultiCameraFrameSync):
                m_pEnableMultiCameraFrameSync     = &m_pConfigSettings[i];
                break;
            case static_cast<UINT32>(ChxSettingsToken::OutputFormat):
                m_pOutputFormat                   = &m_pConfigSettings[i];
                break;
            case static_cast<UINT32>(ChxSettingsToken::EnableCHIPartialData):
                m_pCHIPartialDataSupport          = &m_pConfigSettings[i];
                break;
            case static_cast<UINT32>(ChxSettingsToken::EnableFDStreamInRealTime):
                m_pFDStreamSupport                = &m_pConfigSettings[i];
                break;
            case static_cast<UINT32>(ChxSettingsToken::SelectInSensorHDR3ExpUsecase):
                m_pSelectInSensorHDR3ExpUsecase   = &m_pConfigSettings[i];
                break;
            case static_cast<UINT32>(ChxSettingsToken::EnableUnifiedBufferManager):
                m_pEnableUnifiedBufferManager     = &m_pConfigSettings[i];
                break;
            case static_cast<UINT32>(ChxSettingsToken::EnableCHIImageBufferLateBinding):
                m_pEnableCHILateBinding           = &m_pConfigSettings[i];
                break;
            case static_cast<UINT32>(ChxSettingsToken::EnableCHIPartialDataRecovery):
                m_pCHIEnablePartialDataRecovery   = &m_pConfigSettings[i];
                break;
            case static_cast<UINT32>(ChxSettingsToken::UseFeatureForQCFA):
                m_pUseFeatureForQCFA              = &m_pConfigSettings[i];
                break;
            case static_cast<UINT32>(ChxSettingsToken::EnableOfflineNoiseReprocess):
                m_pEnableOfflineNoiseReprocessing = &m_pConfigSettings[i];
                break;
            case static_cast<UINT32>(ChxSettingsToken::EnableFeature2Dump):
                m_pEnableFeature2Dump             = &m_pConfigSettings[i];
                break;
            case static_cast<UINT32>(ChxSettingsToken::ForceHWMFFixedNumOfFrames):
                m_pForceHWMFFixedNumOfFrames      = &m_pConfigSettings[i];
                break;
            case static_cast<UINT32>(ChxSettingsToken::ForceSWMFFixedNumOfFrames):
                m_pForceSWMFFixedNumOfFrames      = &m_pConfigSettings[i];
                break;
            case static_cast<UINT32>(ChxSettingsToken::EnableSystemLogging) :
                m_pEnableSystemLogging              = &m_pConfigSettings[i];
                break;
            case static_cast<UINT32>(ChxSettingsToken::EnableTBMChiFence):
                m_pEnableTBMChiFence              = &m_pConfigSettings[i];
                break;
            case static_cast<UINT32>(ChxSettingsToken::EnableRawHDR):
                m_pEnableRAWHDR                   = &m_pConfigSettings[i];
                break;
            case static_cast<UINT32>(ChxSettingsToken::OverrideForceBurstShot) :
                m_pOverrideBurstShot               = &m_pConfigSettings[i];
                break;
            case static_cast<UINT32>(ChxSettingsToken::BPSRealtimeSensorId):
                m_pBPSRealtimeSensorId            = &m_pConfigSettings[i];
                break;
            case static_cast<UINT32>(ChxSettingsToken::EnableMFSRChiFence) :
                m_pEnableMFSRChiFence             = &m_pConfigSettings[i];
                break;
            case static_cast<UINT32>(ChxSettingsToken::MultiCameraJPEG) :
                m_pEnableMultiCameraJPEG          = &m_pConfigSettings[i];
                break;
            case static_cast<UINT32>(ChxSettingsToken::MaxHALRequests) :
                m_pMaxHALRequests                 = &m_pConfigSettings[i];
                break;
            case static_cast<UINT32>(ChxSettingsToken::MultiCameraHWSyncMask):
                m_pMultiCameraHWSyncMask          = &m_pConfigSettings[i];
                break;
            case static_cast<UINT32>(ChxSettingsToken::AnchorAlgoSelectionType) :
                m_pAnchorAlgoSelectionType = &m_pConfigSettings[i];
                break;
            case static_cast<UINT32>(ChxSettingsToken::EnableBLMClient) :
                m_pEnableBLMClient = &m_pConfigSettings[i];
                break;
            case static_cast<UINT32>(ChxSettingsToken::ExposeFullSizeForQCFA) :
                m_pExposeFullsizeForQCFA          = &m_pConfigSettings[i];
                break;
            case static_cast<UINT32>(ChxSettingsToken::EnableScreenGrab) :
                m_pEnableScreenGrab               = &m_pConfigSettings[i];
                break;
            default:
                break;
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ExtensionModule::SetMaxLogicalCameraResourceCost
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID ExtensionModule::SetMaxLogicalCameraResourceCost(
    UINT32  logicalCameraId)
{
    if (0 == m_logicalCameraInfo[logicalCameraId].numPhysicalCameras)
    {
        m_logicalCameraInfo[logicalCameraId].maxResourceCost = 0; // torch
    }
    else if (1 < m_logicalCameraInfo[logicalCameraId].numPhysicalCameras)
    {
        m_logicalCameraInfo[logicalCameraId].maxResourceCost = m_totalResourceBudget;
    }
    else if (RealtimeEngineType_BPS == m_logicalCameraInfo[logicalCameraId].ppDeviceInfo[0]->pDeviceConfig->realtimeEngine)
    {
        m_logicalCameraInfo[logicalCameraId].maxResourceCost = 0;
    }
    else
    {
        CHIHANDLE staticMetaDataHandle = const_cast<camera_metadata_t*>(
                  m_logicalCameraInfo[logicalCameraId].m_cameraInfo.static_camera_characteristics);

        CHITAGSOPS        tagOps               = { 0 };
        UINT32            numIFEsRequiredTagId = 0;
        UINT32            maxNumOfIFEsTagId    = 0;
        INT32             numberOfIFEsRequired = 2;
        INT32             maxNumOfIFEs         = 2;

        g_chiContextOps.pTagOps(&tagOps);

        if (NULL != tagOps.pQueryVendorTagLocation)
        {
            tagOps.pQueryVendorTagLocation("org.quic.camera.HWResourceInfo",
                    "maxIFEsRequired", &numIFEsRequiredTagId);
            tagOps.pQueryVendorTagLocation("org.quic.camera.HWResourceInfo",
                    "numIFEsforGivenTarget", &maxNumOfIFEsTagId);
            if ((0 < numIFEsRequiredTagId) && (0 < maxNumOfIFEsTagId))
            {
                tagOps.pGetMetaData(staticMetaDataHandle, numIFEsRequiredTagId, &numberOfIFEsRequired,
                    sizeof(numberOfIFEsRequired));
                tagOps.pGetMetaData(staticMetaDataHandle, maxNumOfIFEsTagId, &maxNumOfIFEs,
                    sizeof(maxNumOfIFEs));

                if ((0 == maxNumOfIFEs) || (numberOfIFEsRequired > maxNumOfIFEs))
                {
                    m_logicalCameraInfo[logicalCameraId].maxResourceCost = m_totalResourceBudget;
                    CHX_LOG_INFO("[CAMX_RC_ERROR] Invalid IFE Resources used %d max %d", numberOfIFEsRequired,
                        maxNumOfIFEs);
                }
                else
                {
                    m_logicalCameraInfo[logicalCameraId].maxResourceCost = numberOfIFEsRequired * m_singleISPResourceCost;
                    CHX_LOG_INFO("[CAMX_RC_DEBUG] IFE Resources used %d max %d", numberOfIFEsRequired, maxNumOfIFEs);
                }
            }
            else
            {
                CHX_LOG_ERROR("[CAMX_RC_ERROR] Cannot read vendor tag required %x max %x", numIFEsRequiredTagId,
                    maxNumOfIFEsTagId);
            }
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ExtensionModule::GetHWSyncMode
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
SensorSyncMode ExtensionModule::GetHWSyncMode(
    LogicalCameraType    logicalCameraType,
    const CHICAMERAINFO* pDeviceCaps)
{
    // Here is the sensor hardware sync configuration for MTP multi camera solution. Customer need to
    // configure sensor hardware sync mode as their hardware design.

    BOOL           enableHWSync = FALSE;
    SensorSyncMode syncMode     = NoSync;
    UINT32         syncMask     = GetMultiCameraHWSyncMask();

    switch (logicalCameraType)
    {
        case LogicalCameraType_BAYERMONO:
        case LogicalCameraType_RTB:
            if (syncMask & MultiCameraHWSyncRTB)
            {
                enableHWSync = TRUE;
            }
            break;
        case LogicalCameraType_SAT:
            if (syncMask & MultiCameraHWSyncSAT)
            {
                enableHWSync = TRUE;
            }
            break;
        case LogicalCameraType_VR:
            if (syncMask & MultiCameraHWSyncVR)
            {
                enableHWSync = TRUE;
            }
            break;
        case LogicalCameraType_DualApp:
        case LogicalCameraType_Default:
            enableHWSync = FALSE;
            break;
        case LogicalCameraType_MAX:
            CHX_LOG_ERROR("invalid logical camera type");
            break;
    };

    if (TRUE == enableHWSync)
    {
        if (REAR == pDeviceCaps->sensorCaps.positionType)
        {
            syncMode = MasterMode;
        }
        else if (REAR_AUX == pDeviceCaps->sensorCaps.positionType)
        {
            syncMode = SlaveMode;
        }
        else
        {
            syncMode = NoSync;
        }
    }

    CHX_LOG_INFO("logical camera type %d sensor id %d enable HW sync %d sync mode %d.",
        logicalCameraType, pDeviceCaps->sensorCaps.sensorId, enableHWSync, syncMode);

    return syncMode;
}
