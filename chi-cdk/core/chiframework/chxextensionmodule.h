////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2017-2020 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  chxextensionmodule.h
/// @brief CHX Extension Module
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef CHXEXTENSIONMODULE_H
#define CHXEXTENSIONMODULE_H

#include <assert.h>
#include <set>

#include "camxdefs.h"
#include "chi.h"
#include "chioverride.h"
#include "chxdefs.h"
#include "camxcdktypes.h"
#include "chxutils.h"
#include <log/log.h>
#include <stdio.h>
#include "chxperf.h"
#include "chxblmclient.h"
#include "chiofflineloggerinterface.h"

#undef LOG_TAG
#define LOG_TAG "CHIUSECASE"

// NOWHINE ENTIRE FILE - Temporarily bypassing for existing CHI files

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Forward Declarations
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class Mutex;
class Session;
class UsecaseFactory;
class UsecaseSelector;
class Usecase;
class PerfLockManager;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Constant Declarations
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static const UINT   MaxSessions                  = 32;           ///< Max sessions
static const UINT   MaxSensorModes               = 50;           ///< Max number of sensor modes
static const UINT   MaxHFRConfigs                = 64;           ///< Max number of HFR configurations
static const UINT   MaxCameras                   = 8;            ///< Max number of cameras supported
static const UINT32 INVALID_INDEX                = 0xFFFFFFFF;   ///< Invalid Index
static const UINT32 InvalidPhysicalCameraId      = 0XFFFFFFFF;   ///< Invalid Physical Camera ID
static const UINT32 InvalidSessionId             = 0xFFFFFFFF;   ///< Invalid session id
// max resolution cap for snapshot YUV
static const UINT   SnapshotYUVOutputCap         = 3840 * 2160;


/// @brief Pipeline create data
struct PipelineCreateData
{
    const CHAR*                   pPipelineName;                     ///< Pipeline name
    CHIPIPELINECREATEDESCRIPTOR*  pPipelineCreateDescriptor;         ///< Pipeline create descriptor
    UINT32                        numOutputs;                        ///< Number of output buffers of this pipeline
    CHIPORTBUFFERDESCRIPTOR*      pOutputDescriptors;                ///< Output buffer descriptors
    UINT32                        numInputs;                         ///< Number of inputs
    CHIPIPELINEINPUTOPTIONS*      pInputOptions;                     ///< Input buffer requirements for this pipeline
};

/// @brief Create session data
struct SessionCreateData
{
    UINT32                          numPipelines;                   ///< Number of pipelines in this session
    CHIPIPELINEINFO*                pPipelineInfo;                  ///< Pipeline info
    CHICALLBACKS*                   pCallbacks;                     ///< Callbacks
    VOID*                           pPrivateCallbackData;           ///< Private callbacks
    CHISESSIONFLAGS                 flags;                          ///< Flags
};

/// @brief Submit request data
struct SubmitRequestData
{
    UINT64                hSession;         ///< Session handle
    UINT64                hPipeline;        ///< Pipeline handle
    UINT32                numPipelines;     ///< Number of pipelines
    CHIPIPELINEREQUEST*   pChiRequest;      ///< Capture request descriptor
};

/// @brief Realtime On Chip Processor
typedef enum
{
    RealtimeEngineType_IFE = 0,   ///< IFE Based Realtime processing engine
    RealtimeEngineType_BPS,       ///< BPS Based Realtime processing engine
    RealtimeEngineType_MAX,       ///< MAX Realtime processing engine type
} RealtimeEngineType;


/// @brief PhysicalCameraConfiguration
struct PhysicalCameraConfiguration
{
    UINT32             sensorId;                ///< Sensor Id, this is slot id of sensor.
    FLOAT              transitionZoomRatioMin;  ///< Min Zoom ratio at which the camera will be active
    FLOAT              transitionZoomRatioMax;  ///< Max Zoom ratio at which the camera will be active
    BOOL               enableSmoothTransition;  ///< Enable Smooth Transition for the camera during SAT
    BOOL               alwaysOn;                ///< Indicate if this camera always on
    RealtimeEngineType realtimeEngine;          ///< Indicate the type of real time processing engine
};

/// @bried Camera info
struct DeviceInfo
{
    UINT32                        cameraId;         ///< Physical device cameraID
    const CHICAMERAINFO*          m_pDeviceCaps;    ///< device capability.
    const CHISENSORMODEINFO*      pSensorModeInfo;  ///< sensor mode table array.
    PhysicalCameraConfiguration*  pDeviceConfig;    ///< Physical device configuration
};

/// @brief Logical Camera type
enum LogicalCameraType
{
    LogicalCameraType_Default  =0,     ///< Single physical camera
    LogicalCameraType_RTB,             ///< RTB mode
    LogicalCameraType_SAT,             ///< SAT mode
    LogicalCameraType_BAYERMONO,       ///< Dual camera with Bayer and MONO sensor
    LogicalCameraType_VR,              ///< VR camera
    LogicalCameraType_DualApp,         ///< Logical camera type for application dual camera solution
    LogicalCameraType_MAX,             ///< Max of logical camera types
};

/// @brief Logical Camera info
struct LogicalCameraInfo
{
    UINT32                   cameraId;                        ///< Logical cameraID
    camera_info              m_cameraInfo;                    ///< Application data
    CHICAMERAINFO            m_cameraCaps;                    ///< camera sub device capability
    CHISENSORMODEINFO*       pSensorModeInfo;                 ///< sensor mode table array.
    UINT32                   numPhysicalCameras;              ///< Number of physical device attached to this logical
                                                              /// camera
    DeviceInfo**             ppDeviceInfo;                    ///< Array of physical device info related to this logical camera
    const camera3_device_t*  m_pCamera3Device;                ///< Camera3 device pointer from application.
    LogicalCameraType        logicalCameraType;               ///< Type of this logical camera
    UINT32                   primaryCameraId;                 ///< Primary Camera from the App's perspective from the list of
                                                              ///  conected physical cameras in the logical camera
    BOOL                     publicVisiblity;                 ///< Is this visible
    UINT32                   numAvailableStreamMap;           ///< Number of availbale stream configs
    StreamMap*               pAvailableStreamMap;             ///< Stream Configuration available for this camera
    UINT32                   maxResourceCost;                 ///< Maximum resource cost for this camera
};

/// @brief Vendor Tags
struct VendorTagInfo
{
    const CHAR*  pSectionName;                ///< Vendor Tag Section Name
    const CHAR*  pTagName;                    ///< Vendor Tag Name
    UINT32       tagId;                       ///< Vendor Tag Id used to query
};

/// @brief HFR configuration
struct HFRConfigurationParams
{
    INT32 width;                ///< Width
    INT32 height;               ///< Height
    INT32 minFPS;               ///< minimum preview FPS
    INT32 maxFPS;               ///< maximum video FPS
    INT32 batchSizeMax;         ///< maximum batch size
};

/// @brief custom Video Mitigation Params
struct VideoMitigationsParams
{
    INT32 width;                ///< Width
    INT32 height;               ///< Height
    INT32 maxPreviewFPS;        ///< Max Preview FPS
    INT32 videoFPS;             ///< video FPS
    BOOL isLiveshotSupported;  ///< liveshot supported or not
    BOOL isEISSupported;       ///< EIS supported or not
};

/// @brief Logical camera configuration
struct LogicalCameraConfiguration
{
    UINT16                      logicalCameraId;                       ///< Logical camera Id exposed to APP
    UINT32                      logicalCameraType;                     ///< Logical camera type
    BOOL                        publicVisibility;                      ///< Indicate if this camera id is exposed to APP
    UINT16                      numOfDevices;                          ///< Number of physical device
    PhysicalCameraConfiguration deviceInfo[MaxDevicePerLogicalCamera]; ///< Physical device information array
    UINT32                      primarySensorID;                       ///< Primary camera from App's perspective.
                                                                       /// The primary camera's zoom values maps to the user zoom
};

/// @brief Advance feature types
enum AdvanceFeatureType
{
    AdvanceFeatureNone     = 0x0,                   ///< mask for none features
    AdvanceFeatureZSL      = 0x1,                   ///< mask for feature ZSL
    AdvanceFeatureMFNR     = 0x2,                   ///< mask for feature MFNR
    AdvanceFeatureHDR      = 0x4,                   ///< mask for feature HDR(AE_Bracket)
    AdvanceFeatureSWMF     = 0x8,                   ///< mask for feature SWMF
    AdvanceFeatureMFSR     = 0x10,                  ///< mask for feature MFSR
    AdvanceFeatureQCFA     = 0x20,                  ///< mask for feature QuadCFA
    AdvanceFeature2Wrapper = 0x40,                  ///< mask for feature2 wrapper
    AdvanceFeatureCountMax = AdvanceFeature2Wrapper ///< Max of advance feature mask
};

/// @brief 3A Debug-Data & Tuning-metadata file dump types, from not dumping anything to dump all
enum EnableDumpDebugDataType
{
    DumpDebugDataNone,
    DumpDebugDataPerSnapshot,
    DumpDebugDataPerVideoFrame,
    DumpDebugDataAllFrames,
};

/// @brief Recovery thread data
struct RecoveryThreadPrivateData
{
    UINT32 logicalCameraId;   ///< Logical camera Id
    VOID*  pData;             ///< Pointer to the data that needs to be stored
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief The ExtensionModule class provides entry/landing implementation of CHX
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class CDK_VISIBILITY_PUBLIC ExtensionModule
{
public:

    /// Return the singleton instance of ExtensionModule
    CDK_VISIBILITY_PUBLIC static ExtensionModule* GetInstance();

    /// Get info from CHI about a given logical camera
    CHX_INLINE const LogicalCameraInfo* GetCameraInfo(
        uint32_t logicalCameraId) const
    {
        return &m_logicalCameraInfo[logicalCameraId];
    }

    /// Get pltformID
    CHX_INLINE INT32 GetPlatformID()
    {
        return m_platformID;
    }

    /// Get library handle
    CHX_INLINE OSLIBRARYHANDLE GetLibrary()
    {
        return m_libraryHandle;
    }

    /// Generic, extensible interface for retrieving info from CHI
    CDKResult GetInfo(
       CDKGetInfoCmd       infoCmd,
       void*               pInputParams,
       void*               pOutputParams);

    /// Get the index of logical camera information by logical camera id
    UINT32 GetCameraIdIndex(
        UINT32 logicalCameraId) const;

    /// Get the primary camera index in device info array
    UINT32 GetPrimaryCameraIndex(
        const LogicalCameraInfo *pLogicalCameraInfo);

    /// Get physical camera information
    const LogicalCameraInfo* GetPhysicalCameraInfo(
        UINT32 physicalCameraId
        )const;

    /// Get Camera Active Array Size
    CDKResult GetActiveArray(
        UINT32   cameraId,
        CHIRECT* pChiRect);

    /// Build logical camera information
    CDKResult BuildLogicalCameraInfo(
        UINT32* sensorIdMap,
        BOOL    isMultiCameraEnabled,
        BOOL    isForExposed);

    UINT32 GetVendorTagId(
        VendorTag tag
        ) const
    {
        return m_pvtVendorTags[static_cast<UINT>(tag)].tagId;
    }

    /// Get sensor mode info
    CDKResult GetPhysicalCameraSensorModes(
        UINT32              physicalCameraId,       ///< Physical camera id
        UINT32*             pNumSensorModes,        ///< Number of sensor modes
        CHISENSORMODEINFO** ppAllSensorModes);      ///< Sensor mode list

    /// Create a pipeline
    CHIPIPELINEDESCRIPTOR CreatePipelineDescriptor(
        PipelineCreateData* pPipelineCreateData);   ///< Pipeline create data

    /// Destroy a pipeline
    VOID DestroyPipelineDescriptor(
        CHIPIPELINEDESCRIPTOR pipelineHandle);       ///< Pipeline handle

    /// Create a session
    CHIHANDLE CreateSession(
        SessionCreateData* pSessionCreateData);     ///< Session create data

    /// Destroy a session
    VOID DestroySession(
        CHIHANDLE sessionHandle,
        BOOL isForced);                             ///< Pipeline handle

    /// Flush a session
    CDKResult Flush(
        CHIHANDLE sessionHandle);                   ///< Pipeline handle

    /// Flush session with sessionFlushInfo
    CDKResult Flush(
        CHISESSIONFLUSHINFO hSessionFlushinfo);     ///< SessionFlushInfo

    /// Submit request
    CDKResult SubmitRequest(
        CHIPIPELINEREQUEST* pSubmitRequestData);     ///< Submit request data

    /// Activate pipeline in a session
    CDKResult ActivatePipeline(
        CHIHANDLE             sessionHandle,        ///< Session handle
        CHIPIPELINEDESCRIPTOR pipelineHandle);      ///< Pipeline handle

    /// Deactivate pipeline in a session
    CDKResult DeactivatePipeline(
        CHIHANDLE                 sessionHandle,        ///< Session handle
        CHIPIPELINEDESCRIPTOR     pipelineHandle,       ///< Pipeline handle
        CHIDEACTIVATEPIPELINEMODE mode);                ///< Deactivate pipeline mode

    /// Create a CHI fence
    CDKResult CreateChiFence(
        CHIFENCECREATEPARAMS* pInfo,
        CHIFENCEHANDLE*       phChiFence);

    /// Release a Chi fence
    CDKResult ReleaseChiFence(
        CHIFENCEHANDLE hChiFence);

    /// Wait on a CHI fence asynchronously
    CDKResult WaitChiFenceAsync(
        PFNCHIFENCECALLBACK pCallbackFn,
        CHIFENCEHANDLE      hChiFence,
        VOID*               pData);

    /// Signal a CHI fence
    CDKResult SignalChiFence(
        CHIFENCEHANDLE hChiFence,
        CDKResult      statusResult);

    /// Obtain a CHI fence status
    CDKResult GetChiFenceStatus(
        CHIFENCEHANDLE hChiFence,
        CDKResult*     pFenceResult);

    /// Query metadata information from the pipeline
    CDKResult QueryPipelineMetadataInfo(
        CHIHANDLE                   sessionHandle,          ///< Session handle
        CHIPIPELINEDESCRIPTOR       pipelineHandle,         ///< Pipeline handle
        CHIPIPELINEMETADATAINFO*    pPipelineMetadataInfo); ///< metadata information

    /// Enumerate sensor modes given the physical camera ID
    CDKResult EnumerateSensorModes(
        UINT32             physCameraId,
        UINT32             numSensorModes,
        CHISENSORMODEINFO* pSensorModeInfo);         ///< Enumerate sensor modes

    /// Interface function invoked by the CHI driver to give an opportunity to the override module to take control of a
    /// usecase on configure streams

    /// @brief Called by the driver to allow for override to detect special camera IDs for additional processing
    UINT32 RemapCameraId(
        UINT32              frameworkCameraId,
        CameraIdRemapMode   mode);

    /// @brief Called by the driver to allow for additional override processing during open()
    CDKResult ExtendOpen(
        uint32_t    logicalCameraId,        ///< Logical camera ID.
        VOID*       pPriv);                 ///< private data for the client to make use of during HAL open

                                            /// @brief Called by the driver to allow for additional override processing during open()
    VOID ExtendClose(
        uint32_t    logicalCameraId,        ///< Logical camera ID.
        VOID*       pPriv);                 ///< private data for the client to make use of during HAL open

    VOID GetNumCameras(
        UINT32* pNumFwCameras,              ///< return the number of cameras to expose to the framework
        UINT32* pNumLogicalCameras);        ///< return the number of logical cameras created by the override

    CDKResult GetCameraInfo(
        uint32_t     logicalCameraId,       ///< Camera Id
        camera_info* cameraInfo);           ///< Camera Info

    CDKResult InitializeOverrideSession(
        uint32_t                        logicalCameraId,    ///< Camera Id
        const camera3_device_t*         camera3_device,     ///< Camera device
        const chi_hal_ops_t*            chiHalOps,          ///< Callbacks into the HAL driver
        camera3_stream_configuration_t* pStreamConfig,      ///< Stream config
        int*                            pIsOverride,        ///< TRUE to take control of the usecase
        VOID**                          pPriv);             ///< Private data

    /// Interface function invoked by the CHI driver to allow the override module to finalize session creation
    CDKResult FinalizeOverrideSession(
        const camera3_device_t* camera3_device, ///< Camera device
        VOID*                   pPriv);         ///< Private data

    /// Interface function invoked by the CHI driver to destroy/teardown a session
    VOID TeardownOverrideSession(
        const camera3_device_t* camera3_device, ///< Camera device
        UINT64                  session,        ///< Session Handle
        VOID*                   pPriv);         ///< Private data

    /// Interface function invoked by the CHI driver to pass along the process capture request to the override module
    CDKResult OverrideProcessRequest(
        const camera3_device_t*     camera3_device,     ///< Camera device
        camera3_capture_request_t*  pCaptureRequest,    ///< Capture request
        VOID*                       pPriv);             ///< Private data

    VOID HandleProcessRequestErrorAllPCRs(
        camera3_capture_request_t* pRequest,    ///< Framework request
        UINT32 logicalCameraId);                ///< Logical camera id

    /// For a given Logical Id, Calculate Cost Based On worstcase scenario
    VOID SetMaxLogicalCameraResourceCost(
        UINT32  logicalId);  ///< logical camera id

    /// Calculate  Sensor Resolution for a given physical id
    UINT32 GetSelectedResolutionForActiveSensorMode(
        UINT32                          physCameraId,    ///< physical Camera Id
        camera3_stream_configuration_t* pStreamConfig);  ///< Stream config

    ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// SignalRecoveryCondition
    ///
    /// @brief  Wake up recovery thread
    ///
    /// @param  cameraId        CameraId with which the usecase is associated with
    ///
    /// @return  None
    ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID SignalRecoveryCondition(
        UINT32 cameraId);

    ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// RecoveryThread
    ///
    /// @brief  Create recovery thread
    ///
    /// @param  pThreadData  Handle to context
    ///
    /// @return  None
    ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static VOID* RecoveryThread(
        VOID* pThreadData);

    ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// RequestThreadProcessing
    ///
    /// @brief  Handle recovery request
    ///
    /// @param  logicalCameraId logical camera id
    ///
    /// @return None
    ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID RequestThreadProcessing(
        UINT32 logicalCameraId);

    ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// OfflineLoggerThread
    ///
    /// @brief  Create OfflineLogger ASCII thread
    ///
    /// @param  pThread Data        Handle to context
    ///
    /// @return  None
    ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static VOID* OfflineLoggerASCIIThread(
        VOID* pThreadData);

    ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// OfflineLoggerBinaryThread
    ///
    /// @brief  Create OfflineLogger Binary logger thread
    ///
    /// @param  pThread Data        Handle to context
    ///
    /// @return  None
    ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static VOID* OfflineLoggerBinaryThread(
        VOID* pThreadData);

    ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// RequestOfflineLogThreadProcessing
    ///
    /// @brief  Where we do Offlinelogger flush log to android system
    ///
    /// @return  None
    ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID RequestOfflineLogThreadProcessing();

    ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// RequestBinaryLogThreadProcessing
    ///
    /// @brief  Where we do Offlinelogger Binary logger flush log to android system
    ///
    /// @return  None
    ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID RequestBinaryLogThreadProcessing();

    ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// SignalOfflineLoggerThread
    ///
    /// @brief  Calling this function will signal OfflineLogger thread to trigger flush
    ///
    /// @return  None
    ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID SignalOfflineLoggerThread(OfflineLoggerType);

    ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// CheckAndSetRecovery
    ///
    /// @brief  Determine whether recovery can be set in progress
    ///
    /// @param  logicalCameraId logical camera id
    ///
    /// @return BOOL
    ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    BOOL CheckAndSetRecovery(
        UINT32 logicalCameraId);

    ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// TriggerRecovery
    ///
    /// @brief Trigger recovery
    ///
    /// @param  logicalCameraId  The logical camera associated with the usecase to teardown
    ///
    /// @return  None
    ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID TriggerRecovery(
        UINT32 logicalCameraId);

    ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// TeardownOverrideUsecase
    ///
    /// @brief Interface function invoked by the CHI driver to destroy/teardown a Usecase
    ///
    /// @param  camera3_device The logical camera associated with the usecase to teardown
    /// @param  isForced       If true, then initiate flush while the Usecase is destroyed
    ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID TeardownOverrideUsecase(
        const camera3_device_t* camera3_device,
        BOOL                    isForced);

    ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetHWSyncMode
    ///
    /// @brief Interface function invoked usecase to get the HW sync mode
    ///
    /// @param  logicalCameraType The type of this logical camera
    /// @param  pDeviceCaps       camera info
    ///
    /// @return SensorSyncMode
    ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    SensorSyncMode GetHWSyncMode(
        LogicalCameraType    logicalCameraType,
        const CHICAMERAINFO* pDeviceCaps);

    /// Interface function invoked by the CHI driver to flush
    CDKResult OverrideFlush(
        const camera3_device_t*     camera3_device);     ///< Camera device

    /// Interface function invoked by the CHI driver to dump
    CDKResult OverrideDump(
        const camera3_device_t*     camera3_device,
        int                         fd);     ///< Camera device

    /// @brief Allows implementation-specific settings to be toggled in the override at runtime
    VOID ModifySettings(
        VOID*       pPriv);                 ///< private data for the client to make use of at runtime in the override

    /// @brief Add any vendor tag specific request template settings
    VOID DefaultRequestSettings(
        uint32_t                  cameraId,
        int                       requestTemplate,
        const camera_metadata_t** settings);

    /// Sets the HAL callbacks
    VOID SetHALOps(
        uint32_t             logicalCameraId,           ///< Logical camera id
        const chi_hal_ops_t* pHalOps);                  ///< Callback functions

    /// Sets the CHI context ops from the CHI driver
    VOID SetCHIContextOps();

    CHX_INLINE const chi_hal_ops_t* GetHalOps(
        uint32_t             logicalCameraId) const     ///< Logical camera id
    {
        return &m_HALOps[logicalCameraId];
    }

    /// Setting to force-disable ZSL...otherwise on by default
    CHX_INLINE BOOL DisableZSL() const
    {
        return *m_pDisableZSL;
    }

    /// Setting to select In-sensor HDR 3 exposure usecase for: None / Preview / Seamless snapshot
    CHX_INLINE UINT32 SelectInSensorHDR3ExpUsecase() const
    {
        return *m_pSelectInSensorHDR3ExpUsecase;
    }

    /// Setting to override the use case by user
    CHX_INLINE UINT OverrideUseCase() const
    {
        return *m_pForceUsecaseId;
    }

    /// Setting to force-use GPU Rotation Node
    CHX_INLINE BOOL UseGPURotationUsecase() const
    {
        return *m_pGPUNodeRotateUsecase;
    }

    /// Setting to force-use GPU Downscale Node
    CHX_INLINE BOOL UseGPUDownscaleUsecase() const
    {
        return *m_pGPUNodeDownscaleUsecase;
    }

    /// Setting to Enable MFNR Usecase
    CHX_INLINE BOOL EnableMFNRUsecase() const
    {
        return *m_pEnableMFNRUsecase;
    }

    /// Setting to Enable MFNR Anchor Selection Algorithm
    CHX_INLINE UINT32 EnableMFNRAnchorSelectionAlgorithm() const
    {
        return *m_pAnchorSelectionAlgoForMFNR;
    }

    /// Setting to Enable MFNR Anchor Selection Algorithm type
    CHX_INLINE UINT32 EnableMFNRAnchorSelectionAlgorithmType() const
    {
        return *m_pAnchorAlgoSelectionType;
    }

    /// Setting to Enable MFSR Usecase
    CHX_INLINE BOOL EnableMFSRUsecase() const
    {
        if (NULL != m_pEnableMFSRUsecase)
        {
            return *m_pEnableMFSRUsecase;
        }
        return FALSE;
    }

    /// Setting to Enable HFR Usecase
    CHX_INLINE BOOL EnableHFRNo3AUsecas() const
    {
        return *m_pHFRNo3AUsecase;
    }

    /// Setting to check if unified buffer manager is enabled
    CHX_INLINE BOOL EnableUnifiedBufferManager() const
    {
        if (NULL != m_pEnableUnifiedBufferManager)
        {
            return (*m_pEnableUnifiedBufferManager == 0) ? FALSE : TRUE;
        }
        return TRUE;
    }

    /// Setting to check if CHI late binding is enabled
    CHX_INLINE BOOL EnableCHILateBinding() const
    {
        if (NULL != m_pEnableCHILateBinding)
        {
            return (*m_pEnableCHILateBinding == 0) ? FALSE : TRUE;
        }
        return FALSE;
    }

    CHX_INLINE UINT32 GetForceSensorMode() const
    {
        if (NULL != m_pForceSensorMode)
        {
            return *m_pForceSensorMode;
        }
        return 0xffffffff;
    }

    CHX_INLINE UINT32 EnableFDStreamSupport() const
    {
        return *m_pFDStreamSupport;
    }

    CHX_INLINE BOOL EnableRawHDRSnapshot() const
    {
        return *m_pEnableRAWHDR;
    }

    CHX_INLINE BOOL EnableMultiCameraJPEG() const
    {
        if (NULL != m_pEnableMultiCameraJPEG)
        {
            return (*m_pEnableMultiCameraJPEG == 0) ? FALSE : TRUE;
        }
        return TRUE;
    }

    CHX_INLINE BOOL EnableFeature2Dump() const
    {
        return static_cast<BOOL>(*m_pEnableFeature2Dump);
    }

    CHX_INLINE UINT32 ForceHWMFFixedNumOfFrames() const
    {
        return *m_pForceHWMFFixedNumOfFrames;
    }

    CHX_INLINE UINT32 ForceSWMFFixedNumOfFrames() const
    {
        return *m_pForceSWMFFixedNumOfFrames;
    }

    CHX_INLINE BOOL IsSystemLogEnabled() const
    {
        return static_cast<BOOL>(*m_pEnableSystemLogging);
    }

    CHX_INLINE BOOL GetDCVRMode() const
    {
         return *m_pEnableMultiVRCamera;
    }

    CHX_INLINE UINT32 GetStatsSkipPattern() const
    {
        if (NULL != m_pStatsSkipPattern)
        {
            return *m_pStatsSkipPattern;
        }
        return 0;
    }

    CHX_INLINE UINT32 EnableDumpDebugData() const
    {
        return *m_pEnableDumpDebugData;
    }

    CHX_INLINE UINT32 Enable3ADebugData() const
    {
        return *m_pEnable3ADebugData;
    }

    CHX_INLINE UINT32 EnableConcise3ADebugData() const
    {
        return *m_pEnableConcise3ADebugData;
    }

    CHX_INLINE UINT32 EnableTuningMetadata() const
    {
        return *m_pEnableTuningMetadata;
    }

    CHX_INLINE UINT32 DebugDataSizeAEC() const
    {
        return *m_pDebugDataSizeAEC;
    }

    CHX_INLINE UINT32 DebugDataSizeAWB() const
    {
        return *m_pDebugDataSizeAWB;
    }

    CHX_INLINE UINT32 DebugDataSizeAF() const
    {
        return *m_pDebugDataSizeAF;
    }

    CHX_INLINE UINT32 ConciseDebugDataSizeAEC() const
    {
        return *m_pConciseDebugDataSizeAEC;
    }

    CHX_INLINE UINT32 ConciseDebugDataSizeAWB() const
    {
        return *m_pConciseDebugDataSizeAWB;
    }

    CHX_INLINE UINT32 ConciseDebugDataSizeAF() const
    {
        return *m_pConciseDebugDataSizeAF;
    }

    CHX_INLINE UINT32 TuningDumpDataSizeIFE() const
    {
        return *m_pTuningDumpDataSizeIFE;
    }

    CHX_INLINE UINT32 TuningDumpDataSizeIPE() const
    {
        return *m_pTuningDumpDataSizeIPE;
    }

    CHX_INLINE UINT32 TuningDumpDataSizeBPS() const
    {
        return *m_pTuningDumpDataSizeBPS;
    }

    /// Setting to Enable EIS V2 Usecase
    CHX_INLINE BOOL EnableEISV2Usecase() const
    {
        return *m_pEISV2Enable;
    }

    /// Setting to Enable EIS V3 Usecase
    CHX_INLINE BOOL EnableEISV3Usecase() const
    {
        return *m_pEISV3Enable;
    }

    CHX_INLINE BOOL UseFeatureForQuadCFA() const
    {
        return *m_pUseFeatureForQCFA;
    }

    CHX_INLINE BOOL ExposeFullsizeForQuadCFA() const
    {
        return *m_pExposeFullsizeForQCFA;
    }

    CHX_INLINE UINT32 GetAdvanceFeatureMask() const
    {
        return *m_pAdvanceFeataureMask;
    }

    /// Setting to enable BLM Client
    CHX_INLINE BOOL EnableBLMClient() const
    {
        return *m_pEnableBLMClient;
    }

    /// Setting to Disable ASD Processing
    CHX_INLINE UINT32 DisableASDProcessing() const
    {
        return *m_pDisableASDProcessing;
    }

    LogicalCameraType GetCameraType(UINT32 cameraId) const;

    CHX_INLINE UINT32 GetDCFrameSyncMode() const
    {
        return *m_pEnableMultiCameraFrameSync;
    }

    CHX_INLINE UINT32 GetOutputFormat() const
    {
        return *m_pOutputFormat;
    }

    CHX_INLINE UINT32 IsCameraClose() const
    {
        BOOL isClosed = FALSE;

        if (NULL != m_pOverrideCameraClose)
        {
            isClosed = *m_pOverrideCameraClose;
        }

        return isClosed;
    }

    CHX_INLINE UINT32 IsCameraOpen() const
    {
        return *m_pOverrideCameraOpen;
    }

    // Get number of PCRs before stream on
    CHX_INLINE UINT32 GetNumPCRsBeforeStreamOn() const
    {
        UINT32 numPCRSBeforeStreamOn = *m_pNumPCRsBeforeStreamOn;
        // Disable EarlyPCR if Batchmode is enabled
        if (1 < m_usecaseNumBatchedFrames)
        {
            numPCRSBeforeStreamOn = 0;
        }
        return numPCRSBeforeStreamOn;
    }

    /// Setting to override the FOVC Usecase settings
    CHX_INLINE UINT EnableFOVCUseCase() const
    {
        if (NULL != m_pEnableFOVC)
        {
            return *m_pEnableFOVC;
        }
        return FALSE;
    }

    /// Setting to Enable Usecase Handling of Partial Data
    CHX_INLINE PartialMetaSupport EnableCHIPartialData() const
    {
        return static_cast<PartialMetaSupport>(*m_pCHIPartialDataSupport);
    }

    /// Setting to Enable Recovery for Usecase Handling of Partial Data
    CHX_INLINE BOOL EnableCHIPartialDataRecovery() const
    {
        return static_cast<BOOL>(*m_pCHIEnablePartialDataRecovery);
    }

    /// Setting to Enable Offline Noise Reprocessing pipeline
    CHX_INLINE BOOL EnableOfflineNoiseReprocessing() const
    {
        return static_cast<BOOL>(*m_pEnableOfflineNoiseReprocessing);
    }

    /// Setting to Enable CHITargetBufferManager Chi fence
    CHX_INLINE BOOL EnableTBMChiFence() const
    {
        if (NULL != m_pEnableTBMChiFence)
        {
            return static_cast<BOOL>(*m_pEnableTBMChiFence);
        }
        return FALSE;
    }

    /// Setting to Enable MFSR CHITargetBufferManager Chi fence
    CHX_INLINE BOOL EnableMFSRChiFence() const
    {
        if (NULL != m_pEnableMFSRChiFence)
        {
            return static_cast<BOOL>(*m_pEnableMFSRChiFence);
        }
        return FALSE;
    }

    /// Setting to get max hal requests
    CHX_INLINE UINT GetMaxHalRequests() const
    {
        if (NULL != m_pMaxHALRequests)
        {
            return static_cast<UINT>(*m_pMaxHALRequests);
        }
        return 0;
    }

    /// Setting to Enable Screen Grab
    CHX_INLINE BOOL EnableScreenGrab() const
    {
        // for bitra SOC, return value from override settings
        // for other SOCs Screen grab is disabled for now
        if ((NULL != m_pEnableScreenGrab) && (CHISocIdSM6350 == m_platformID))
        {
            return static_cast<BOOL>(*m_pEnableScreenGrab);
        }
        return FALSE;
    }

    /// Setting to get HW Sync scheme
    CHX_INLINE UINT32 GetMultiCameraHWSyncMask() const
    {
        if (NULL != m_pMultiCameraHWSyncMask)
        {
            return static_cast<UINT32>(*m_pMultiCameraHWSyncMask);
        }
        return 0;
    }

    CHX_INLINE FLOAT AECGainThresholdForQCFA() const
    {
        return m_AECGainThresholdForQCFA;
    }

    VOID GetVendorTagOps(
        CHITAGSOPS* pVendorTagOps);

    // Gets the metadata ops table
    VOID GetMetadataOps(
        CHIMETADATAOPS* pMetadataOps);

    // get available request keys for the framework
    CDKResult GetAvailableRequestKeys(
        UINT32   logicalCameraId,
        UINT32*  pTagList,
        UINT32   maxTagListCount,
        UINT32*  pTagCount);

    VOID SearchNumBatchedFrames(
        uint32_t                        cameraId,
        camera3_stream_configuration_t* pStreamConfigs,
        UINT*                           pBatchSize,
        BOOL*                           pHALOutputBufferCombined,
        UINT*                           pFPSValue,
        UINT                            maxSessionFps);

    struct CameraDeviceInfo
    {
        const camera3_device_t *m_pCamera3Device; ///< Camera3 device
    };


    Usecase* GetSelectedUsecase(const camera3_device*);

    CHX_INLINE UINT GetUsecaseMaxFPS()
    {
        return m_usecaseMaxFPS;
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetPreviewFPS
    ///
    /// @brief  Get the preview stream FPS
    ///
    /// @param  None
    ///
    /// @return FPS value for preview stream
    ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CHX_INLINE UINT GetPreviewFPS()
    {
        return m_previewFPS;
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetVideoFPS
    ///
    /// @brief   Get the video stream FPS
    ///
    /// @param   None
    ///
    /// @return  FPS value for video stream
    ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CHX_INLINE UINT GetVideoFPS()
    {
        return m_videoFPS;
    }

    CHX_INLINE UINT GetVideoHDRMode()
    {
        return m_VideoHDRMode;
    }

    CHX_INLINE camera_metadata_t* GetLastRequestSettings(UINT logicalcameraId)
    {
        return m_pLastKnownRequestSettings[logicalcameraId];
    }

    CHX_INLINE UINT GetNumBatchedFrames()
    {
        if (0 != m_usecaseNumBatchedFrames)
        {
            return m_usecaseNumBatchedFrames;
        }
        return 1;
    }

    CHX_INLINE BOOL GetHALOutputBufferCombined()
    {
        return m_HALOutputBufferCombined;
    }

    CHX_INLINE UINT32 GetOpMode(UINT32 cameraId)
    {
        return m_operationMode[cameraId];
    }

    VOID ReturnFrameworkResult(
        const camera3_capture_result_t* pResult, UINT32 cameraID);

    VOID ReturnFrameworkMessage(
        const camera3_notify_msg_t *msg, UINT32 cameraID);

    VOID DumpDebugData(UINT32 cameraID);

    /// OfflineLogger flush logic, can be customized
    BOOL OfflineLoggerFlushTriggerLogic();

    UINT32 GetCameraIdfromDevice(const camera3_device_t * pCamera3Device);

    CHX_INLINE UINT IsTorchWidgetUsecase()
    {
        return m_torchWidgetUsecase;
    }

    CHX_INLINE UINT32 GetNumMetadataResults()
    {
        return m_numMetadataResults;
    }

    CHX_INLINE OSLIBRARYHANDLE GetPerfLibHandle()
    {
        return m_perfLibHandle;
    }

    /// Set the resource cost of for a physical camera
    CHX_INLINE VOID SetResourceCost(
        UINT  cameraId,
        INT32 numIFEsUsed)
    {
        UINT logicalCameraId = m_cameraMap[cameraId];
        if (logicalCameraId < m_numLogicalCameras)
        {
            m_pResourcesUsedLock->Lock();
            m_IFEResourceCost[logicalCameraId] = (numIFEsUsed * m_singleISPResourceCost);
            m_pResourcesUsedLock->Unlock();
        }
        else
        {
            CHX_LOG_WARN("[CHI_RC_ERROR] Cannot set resource cost for camera Id %u logicalCameraId %u",
                cameraId, logicalCameraId);
        }
    }

    /// Reset the resource cost of for a physical camera
    CHX_INLINE VOID ResetResourceCost(
        UINT  cameraId)
    {
        UINT logicalCameraId = m_cameraMap[cameraId];
        if (logicalCameraId < m_numLogicalCameras)
        {
            m_pResourcesUsedLock->Lock();
            m_IFEResourceCost[logicalCameraId] = 0;
            m_pResourcesUsedLock->Unlock();
        }
        else
        {
            CHX_LOG_WARN("[CHI_RC_ERROR] Cannot set resource cost for camera Id %u logicalCameraId %u",
                cameraId, logicalCameraId);
        }
    }

    // Mapping the config settings pointer to each variable
    VOID MappingConfigSettings(
        UINT32  numTokens,
        VOID*   pInputTokens);

    CHX_INLINE BOOL IsSWMFEnabled() const
    {
        return (*m_pAdvanceFeataureMask & AdvanceFeatureSWMF) ? TRUE : FALSE;
    }

    CHX_INLINE CDKResult GetAvailableStreamMap(
        UINT32 cameraId, StreamMap** pStreamMap, UINT32* numStreamMap) const
    {
        CDKResult result = CDKResultSuccess;

        if (cameraId < m_numLogicalCameras)
        {
            *numStreamMap  = m_logicalCameraInfo[cameraId].numAvailableStreamMap;
            *pStreamMap    = m_logicalCameraInfo[cameraId].pAvailableStreamMap;
        }
        else
        {
            CHX_LOG_ERROR("Invalid CameraId = %d", cameraId);
            numStreamMap = 0;
        }

        return result;
    }

    // Gets the total resource cost of all the active cameras
    CHX_INLINE INT32 GetActiveResourceCost() const
    {
        INT resourceCost = 0;

        for (UINT cameraId = 0; cameraId < m_numLogicalCameras; ++cameraId)
        {
            resourceCost += m_IFEResourceCost[cameraId];
        }

        return resourceCost;
    }

    CDKResult GetMatchingStreamMapData(
        camera3_stream_configuration_t* pStreamConfig,
        camera3_stream_t*               pStream,
        const camera_metadata_t*        pSessionParam,
        StreamMap*                      pStreamMapData);

    CHX_INLINE UINT GetNumberOfLogicalConfig() const
    {
        return m_numOfLogicalCameraConfiguration;
    }

    CHX_INLINE const LogicalCameraInfo* GetAllLogicalCameraInfo() const
    {
        return &m_logicalCameraInfo[0];
    }

    CHX_INLINE VOID SetScreenGrabLiveShotScene(BOOL screenGrabScene)
    {
        m_isScreenGrabLiveShotScene = screenGrabScene;
    }

    CHX_INLINE BOOL GetScreenGrabLiveShotScene()
    {
        return m_isScreenGrabLiveShotScene;
    }

private:

    /// Sort the Logical Camera configuration by their logical IDs
    CDKResult SortCameras();

    /// Enumerate the cameras
    CDKResult EnumerateCameras();

    /// Retrieves devices static settings
    CDKResult GetDevicesSettings();

    /// Retrieves devices static settings
    CDKResult UpdateStaticSettings(UINT32 cameraId);

    /// Swap srcCamera and dstCamera.
    VOID SwapCameras(UINT32 srcCamera, UINT32 dstCamera);

    /// Swap Rear Main at 0th cameraId and Front Main at cameraId 1.
    VOID SwapFrontAndRearMain();

    /// Fill logical camera static capabilities
    CDKResult FillLogicalCameraCaps();

    /// Constructor
    ExtensionModule();
    /// Destructor
    ~ExtensionModule();

    /// Do not support the copy constructor or assignment operator
    ExtensionModule(const ExtensionModule& rExtensionModule) = delete;
    ExtensionModule& operator= (const ExtensionModule& rExtensionModule) = delete;
    VOID FreeLastKnownRequestSetting(UINT32 logicalCameraId);

    CDKResult InitializeAvailableStreamMap(UINT32 cameraId);

    CHIHANDLE               m_hCHIContext;                          ///< CHI context handle
    ChiFenceOps             m_chiFenceOps;                          ///< CHI Fence Operations
    chi_hal_ops_t           m_HALOps[MaxNumImageSensors];           ///< HAL ops
    UsecaseSelector*        m_pUsecaseSelector;                     ///< Usecase selector
    UsecaseFactory*         m_pUsecaseFactory;                      ///< Usecase factory
    Usecase*                m_pSelectedUsecase[MaxNumImageSensors]; ///< Selected usecase
    INT32                   m_platformID;                           ///< platform ID
    UINT32                  m_numPhysicalCameras;                   ///< Number of physical camera sensors in the system
    UINT32                  m_numExposedLogicalCameras;             ///< Number of logical camera exposed to application
    UINT32                  m_numLogicalCameras;                    ///< Number of logical camera in configuration table
    LogicalCameraInfo       m_logicalCameraInfo[MaxNumImageSensors];///< Logical camera information
    UINT32                  m_cameraMap[MaxNumImageSensors];        ///< Camera ID map
    UINT32                  m_cameraReverseMap[MaxNumImageSensors];
    FLOAT                   m_AECGainThresholdForQCFA;              ///< AECGainThresholdForQCFA settings
    UINT32*                 m_pConfigSettings;                      ///< Configuration settings
    UINT32*                 m_pDisableZSL;
    UINT32*                 m_pForceUsecaseId;
    UINT32*                 m_pEnableFOVC;
    UINT32*                 m_pGPUNodeRotateUsecase;            ///< Select GPU Node Rotate usecase
    UINT32*                 m_pGPUNodeDownscaleUsecase;         ///< Select GPU Node Downscale usecase
    UINT32*                 m_pEnableMFNRUsecase;               ///< Select MFNR usecase
    UINT32*                 m_pAnchorSelectionAlgoForMFNR;      ///< Select MFNR Anchor Selection Algorithm
    UINT32*                 m_pAnchorAlgoSelectionType;         ///< Select MFNR Anchor Selection Algorithm Type
    UINT32*                 m_pEnableMFSRUsecase;               ///< Select MFSR usecase
    UINT32*                 m_pHFRNo3AUsecase;                  ///< Select HFR without 3A usecase
    UINT32*                 m_pForceSensorMode;                 ///< Select a specific sensor mode
    UINT32*                 m_pEISV2Enable;                     ///< Enable EIS V2
    UINT32*                 m_pEISV3Enable;                     ///< Enable EIS V3
    UINT32*                 m_pDisableASDProcessing;            ///< Disable ASD processing
    UINT32*                 m_pEnableMultiVRCamera;             ///< Enbale VR DC
    UINT32*                 m_pOverrideCameraClose;             ///< Camera close indicator
    UINT32*                 m_pOverrideCameraOpen;              ///< Camera open indicator
    UINT32*                 m_pNumPCRsBeforeStreamOn;           ///< Number of PCRs before stream on
    UINT32*                 m_pStatsSkipPattern;                ///< Stats skip pattern
    UINT32*                 m_pEnableMultiCameraFrameSync;      ///< DC Frame sync enabled or not
    UINT32*                 m_pEnableDumpDebugData;             ///< Dump debug-data into a file, enable or not
    UINT32*                 m_pEnable3ADebugData;               ///< General switch to enable 3A debug data
    UINT32*                 m_pEnableConcise3ADebugData;        ///< General switch to enable 3A debug data
    UINT32*                 m_pEnableTuningMetadata;            ///< General switch to enable ISP Tuning metadata-data
    UINT32*                 m_pDebugDataSizeAEC;                ///< Debug-data size reserved for AEC
    UINT32*                 m_pDebugDataSizeAWB;                ///< Debug-data size reserved for AWB
    UINT32*                 m_pDebugDataSizeAF;                 ///< Debug-data size reserved for AF
    UINT32*                 m_pConciseDebugDataSizeAEC;         ///< Debug-data size reserved for AF
    UINT32*                 m_pConciseDebugDataSizeAWB;         ///< Debug-data size reserved for AF
    UINT32*                 m_pConciseDebugDataSizeAF;          ///< Debug-data size reserved for AF
    UINT32*                 m_pTuningDumpDataSizeIFE;           ///< Debug-data size reserved for IFE
    UINT32*                 m_pTuningDumpDataSizeIPE;           ///< Debug-data size reserved for IPE
    UINT32*                 m_pTuningDumpDataSizeBPS;           ///< Debug-data size reserved for BPS
    UINT32*                 m_pOutputFormat;                    ///< Output format for IMPL_Defined
    UINT32*                 m_pCHIPartialDataSupport;           ///< CHI Partial Data Handling
    UINT32*                 m_pCHIEnablePartialDataRecovery;    ///< CHI Partial Data Recovery
    UINT32*                 m_pFDStreamSupport;                 ///< Support FD Stream in Real Time
    UINT32*                 m_pSelectInSensorHDR3ExpUsecase;    ///< Select in-sensor HDR 3 exposure usecase
    UINT32*                 m_pEnableUnifiedBufferManager;      ///< Enable Unified Buffer Manager
    UINT32*                 m_pEnableCHILateBinding;            ///< Enable CHI image buffer late binding
    UINT32*                 m_pEnableOfflineNoiseReprocessing;  ///< Enable Offline Noise Reprocessing
    UINT32*                 m_pEnableFeature2Dump;              ///< Enable feature2 image/metadata dump for simulator
    UINT32*                 m_pForceHWMFFixedNumOfFrames;       ///< Force HWMF fixed num of frames
    UINT32*                 m_pForceSWMFFixedNumOfFrames;       ///< Force SWMF fixed num of frames
    UINT32*                 m_pEnableSystemLogging;             ///< Enable system log
    UINT32*                 m_pEnableTBMChiFence;               ///< Enable CHITargetBufferManager chi fence
    UINT32*                 m_pEnableMFSRChiFence;              ///< Enable MFSR CHITargetBufferManager chi fence
    UINT32*                 m_pEnableScreenGrab;                ///< Enable Screen Grab
    UINT32*                 m_pEnableRAWHDR;                    ///< Enable HDR with RAW frame processing
    UINT32*                 m_pEnableMultiCameraJPEG;           ///< Enable per camera Jpeg Capture in multicamera
    static const UINT       m_numVendorTags           = static_cast<UINT>(VendorTag::NumVendorTags); ///< Num vendor tags
    UINT32*                 m_pMaxHALRequests;                  ///< Max hal requests that can be in pipeline
    static const UINT       DefaultSettingsNumEntries = 32;     ///< Num of entries
    static const UINT       DefaultSettingsDataSize   = 1024;    ///< Data size bytes
    camera_metadata_t*      m_pDefaultSettings[MaxNumImageSensors];   ///< Default settings
    VendorTagInfo           m_pvtVendorTags[m_numVendorTags];   ///< List of private vendor tags
    UINT*                   m_pUseFeatureForQCFA;               ///< Use FeatureQuadCFA or UsecaseQuadCFA
    UINT*                   m_pDefaultMaxFPS;                   ///< Default MaxFPS
    UINT*                   m_pAdvanceFeataureMask;             ///< Advance Features Mask
    UINT*                   m_pBPSRealtimeSensorId;             ///< BPS Realtime Sensor Id
    UINT                    m_usecaseMaxFPS;                    ///< Max FPS required for high speed mode
    UINT                    m_previewFPS;                       ///< FPS of the preview stream set by App
    UINT                    m_videoFPS;                         ///< FPS of the video stream set by App
    UINT32*                 m_pOverrideBurstShot;               ///< Burst shot support read from override setting
    UINT                    m_VideoHDRMode;                     ///< video HDR mode
    UINT                    m_usecaseNumBatchedFrames;          ///< Number of framework frames batched together if
                                                                ///  batching is enabled
    BOOL                    m_HALOutputBufferCombined;          ///< If the output buffers for batching mode is combined
                                                                ///  1. one request for one video frame. The framework send
                                                                ///     in a group requests for a group video frames, e.g,
                                                                ///     8 requests for 480 fps when running in 60 fps.
                                                                ///     in this case, m_HALOutputBufferCombined = FALSE
                                                                ///  2. one request for several video frames, such as 16
                                                                ///     in 960 fps mode. The framework send in one request
                                                                ///     has a output buffer which could hold 16 frames, in
                                                                ///     this case, m_HALOutputBufferCombined = TRUE
                                                                ///
    BOOL                    m_torchWidgetUsecase;               ///< Indicate torch widget usecase.
    UINT32*                 m_pEnableBLMClient;                 ///< Enable Bandwidth Limit Management

    PerfLockManager*        m_pPerfLockManager[MaxNumImageSensors];                 ///< PerfLock Manager
    PerfLockType            m_CurrentpowerHint;                 ///< Current power Hint
    PerfLockType            m_previousPowerHint;                ///< Previous power Hint
    CHXBLMClient*           m_pBLMClient;                       ///< BLM Client
    UINT                    m_numMetadataResults;               ///< number of metadata results expected from the driver

    UINT32                  m_originalFrameWorkNumber[MaxNumImageSensors];   ///< Original framework number
    Mutex*                  m_pPCRLock[MaxNumImageSensors];                  ///< Lock for process capture request
    Mutex*                  m_pRecoveryLock[MaxNumImageSensors];       ///< Lock for process capture request
    Mutex*                  m_pDestroyLock[MaxNumImageSensors];        ///< Lock for destroying usecase
    Condition*              m_pRecoveryCondition[MaxNumImageSensors];  ///< Condition to wait for recovery done.
    BOOL                    m_TeardownInProgress[MaxNumImageSensors];  ///< Flag to indicate teardown is in progress
    BOOL                    m_RecoveryInProgress[MaxNumImageSensors];  ///< Flag to indicate recovery is in progress
    UINT32                  m_operationMode[MaxNumImageSensors];       ///< op_mode sent from Frameworks

    camera_metadata_t*      m_pLastKnownRequestSettings[MaxNumImageSensors]; ///< Save last known metadata to send aftet recovery
    UINT32                  m_firstFrameAfterRecovery[MaxNumImageSensors];   ///< First frame after recovery to send settings
    UINT32                  m_longExposureFrame;                             ///< Long exposure snapshot frameNumber
    volatile UINT32         m_aFlushInProgress[MaxNumImageSensors];          ///< Is flush in progress
    volatile UINT32         m_aLongExposureInProgress;                       ///< Is long exposure in progress

    camera3_stream_configuration_t* m_pStreamConfig[MaxNumImageSensors];        ///< Stream configuration array to use in recovery
    UINT32                          m_SelectedUsecaseId[MaxNumImageSensors];    ///< Selected usecase id to use in recoverys
    CameraDeviceInfo                m_pCameraDeviceInfo[MaxNumImageSensors];    ///< Device info for all logical camera Id

    BOOL                            m_firstResult;                      ///< First result after configure streams
    OSLIBRARYHANDLE                 m_perfLibHandle;                    ///< PerfLock Library handle
    OFFLINELOGGERHANDLE             m_offlineLoggerHandle;              ///< Offlinelog Libaray handle
    CHIOFFLINELOGOPS                m_offlineLoggerOps;                 ///< Offlinelog Libaray operation
    BOOL                            m_isUsecaseInBadState[MaxNumImageSensors];  ///< Flag to indicate if recovery is required
    LogicalCameraConfiguration*     m_pLogicalCameraConfigurationInfo;  ///< Array of Logical Cameras
    UINT                            m_numOfLogicalCameraConfiguration;  ///< Number of Logical Cameras
    Mutex*                          m_pTriggerRecoveryLock[MaxNumImageSensors];      ///< Lock to trigger recovery
    Condition*                      m_pTriggerRecoveryCondition[MaxNumImageSensors]; ///< Condition to trigger recovery
    BOOL                            m_IsRecoverySignaled[MaxNumImageSensors];  ///< Is recovery in progress
    PerThreadData                   m_pRecoveryThread[MaxNumImageSensors];     ///< Recovery thread
    RecoveryThreadPrivateData       m_recoveryThreadPrivateData[MaxNumImageSensors]; ///< Recovery thread private data
    PerThreadData                   m_pOfflineLoggerThread[OFFLINELOG_MAX_TYPE];   ///< Offlinelog thread create data
    BOOL                            m_terminateRecoveryThread[MaxNumImageSensors]; ///< Do we want to terminate the recovery thread
    BOOL                            m_terminateOfflineLogThread;        ///< Do we want to terminate the offlinelog thread

    BOOL                            m_bTeardownRequired[MaxNumImageSensors]; ///< Keep track of usecase to tear down
    UINT32                          m_hasFlushOccurred[MaxNumImageSensors];  ///< Keep track of whether flush has happened
    BOOL                            m_bAsciiLogEnable;                       ///< Offlinelogger ASCii feature enable/disable
    BOOL                            m_bBinaryLogEnable;                      ///< Offlinelogger Binary feature enable/disable
    Mutex*                          m_pFlushNeededMutex[OFFLINELOG_MAX_TYPE];  ///< If flush needed mutex
    Condition*                      m_pFlushNeededCond[OFFLINELOG_MAX_TYPE]; ///< OfflineLogger condition variable array
    static const UINT32             DefaultFrameRateforHighSpeedSession = 120; ///< Default frame rate for high speed
    UINT32                          m_singleISPResourceCost;                 ///< Single ISP resource cost
    UINT32                          m_totalResourceBudget;                   ///< Total resource cost
    UINT32                          m_IFEResourceCost[MaxNumImageSensors];   ///< Current IFE resource for Logical multi camera
    Mutex*                          m_pResourcesUsedLock;                    ///< Mutex for access to m_resourcesUsed
    UINT32*                         m_pMultiCameraHWSyncMask;                ///< Indicates camera hw sync mask
    std::set<UINT>                  m_logicalCameraRCVBypassSet;             ///< Set of Logical cameraIds that triggered the
                                                                             ///   bypass for resource cost validation
    OSLIBRARYHANDLE                 m_libraryHandle;                         ///< camera Library handle
    UINT32                          m_consecutiveRecoveryCount[MaxNumImageSensors]; ///< counter for consecutive recovery
    UINT32                          m_lastFrameBeforeRecovery[MaxNumImageSensors];  ///< last frame number before recovery
    static const UINT               MaxNumberOfConsecutiveRecoveryAllowed = 3;      ///< Max number of consecutive recovery
    static const UINT               MaxNumFramesForConsecutiveRecovery    = 50;     ///< Threshold for number of frames for
                                                                                    ///  consecutive recovery
    UINT32*                         m_pExposeFullsizeForQCFA;                       ///< If Expose full size for quadcfa sensor
    BOOL                            m_isScreenGrabLiveShotScene;                    ///< If screen grab scenerio
};

#endif // CHXEXTENSIONMODULE_H
