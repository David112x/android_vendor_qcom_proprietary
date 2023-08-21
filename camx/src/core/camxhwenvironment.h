////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2017-2020 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  camxhwenvironment.h
/// @brief HwEnvironment class declaration, for storing device static information
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef CAMXHWENVIRONMENT_H
#define CAMXHWENVIRONMENT_H

#include "chinode.h"
#include "camxstaticcaps.h"
#include "camxcslsensordefs.h"
#include "chiaecinterface.h"
#include "chiafdinterface.h"
#include "chiafinterface.h"
#include "chiawbinterface.h"
#include "chiasdinterface.h"
#include "chiisphvxdefs.h"
#include "chistatsalgo.h"
#include "g_camxsettings.h"

CAMX_NAMESPACE_BEGIN

struct HwContextCreateData;
struct VendorTagInfo;
struct CAMXCustomizeOEMInterface;

class  HwEnvironment;
class  HwFactory;
class  ImageSensorModuleData;
class  ImageSensorModuleDataManager;
class  SettingsManager;
class  TuningDataManager;
struct ComponentVendorTagsInfo;

static const INT32 CustomHFR60Fps = 60;
static const INT32 CustomHFR30Fps = 30;


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ExternalComponentStatsAlgo
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
enum class ExternalComponentStatsAlgo
{
    ALGOAEC  = 0,       ///< AEC
    ALGOAF,             ///< AF
    ALGOAWB,            ///< AWB
    ALGOAFD,            ///< AFD
    ALGOASD,            ///< ASD
    ALGOPD,             ///< PD
    ALGOHIST = 7,       ///< Histogram Algo
    ALGOTRACK,          ///< Track
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ExternalComponentNodeAlgo
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
enum class ExternalComponentNodeAlgo
{
    COMPONENTNODE      = 0,   ///< Node
    COMPONENTALGORITHM = 1,   ///< Algorithm
    COMPONENTHVX       = 2,   ///< Hvx
};

/// @brief table entry for external components.
struct ExternalComponentInfo
{
    CHAR*                          pComponentName;     ///< External Component name

    union
    {
        CHINODEINTERFACE           nodeInterface;      ///< node Interface for custom node to call into Chi.
        CHIALGORITHMINTERFACE      algoInterface;      ///< algo Interface for custom node to call into Chi.
    };

    union
    {
        CHINODECALLBACKS                nodeCallbacks;          ///< Node Callback Interface for Chi to call into custom node.
        CHIHISTALGORITHMCALLBACKS       histAlgoCallbacks;      ///< Algo Callback Interface for Chi to call into custom node.
        CHIAECALGORITHMCALLBACKS        AECAlgoCallbacks;       ///< Algo Callback Interface for Chi to call into custom node.
        CHIAFALGORITHMCALLBACKS         AFAlgoCallbacks;        ///< Algo Callback Interface for Chi to call into custom node.
        CHIAWBALGORITHMCALLBACKS        AWBAlgoCallbacks;       ///< Algo Callback Interface for Chi to call into custom node.
        CHIAFDALGORITHMCALLBACKS        AFDAlgoCallbacks;       ///< Algo Callback Interface for Chi to call into custom node.
        CHIASDALGORITHMCALLBACKS        ASDAlgoCallbacks;       ///< Algo Callback Interface for Chi to call into custom node.
        CHIPDLIBRARYCALLBACKS           PDLibCallbacks;         ///< Algo Callback Interface for Chi to call into custom node.
        CHIISPHVXALGORITHMCALLBACKS     HVXAlgoCallbacks;       ///< Algo Callback Interface for Chi to call into custom node.
        CHITRACKERALGORITHMCALLBACKS    trackerAlgoCallbacks;   ///< Algo Callback Interface for chi to call into custom code.
    };

    ExternalComponentNodeAlgo      nodeAlgoType;       ///< 0 for node and 1 for algo
    BOOL                           inUse;              ///< 1 for inUse and 0 for free
    ExternalComponentStatsAlgo     statsAlgo;          ///< stats algo type
};

/// @brief table entry for supported devices.
struct HwDeviceTypeInfo
{
    CSLVersion  hwVersion;                          ///< HW version as reported by CSL.
    CSLVersion  driverVersion;                      ///< KMD driver version as reported by CSL.
    INT32       deviceIndex[CamxMaxDeviceIndex];    ///< Array of device Indices as reported by CSL.
    UINT        deviceIndexCount;                   ///< Number of valid entries in deviceIndex.
};

/// @brief Table entry for detected camera sensors.
struct HwSensorInfo
{
    ImageSensorModuleData*  pData;              ///< Sensor module data object.
    INT32                   deviceIndex;        ///< The device Index of the detected sensor module.
    SensorModuleStaticCaps  moduleCaps;         ///< Capabilities of the sensor module.
    CSLSensorCapability     CSLCapability;      ///< Capabilities from CSL for the sensor module.
};

/// @brief Encapsulates static camera information to be used by HAL.
struct HwCameraInfo
{
    ImageSensorFacing               imageSensorFacing;                      ///< The direction that the camera faces.
    ImageOrientation                imageOrientation;                       ///< The orientation of the camera image.
    UINT32                          mountAngle;                             ///< Camera Mount Angle
    INT                             resourceCost;                           ///< The total resource "cost" of using this camera.
    UINT32                          conflictingDevices[MaxNumImageSensors]; ///< An array of camera device IDs that cannot be
                                                                            ///  simultaneously opened while this camera device
                                                                            ///  is in use.
    SIZE_T                          conflictingDevicesLength;               ///< The number of entries in conflictingDevices.
    const PlatformStaticCaps*       pPlatformCaps;                          ///< Static capabilities of the chipset.
    const SensorModuleStaticCaps*   pSensorCaps;                            ///< Static capabilities of the image sensor module.
    const HwEnvironmentStaticCaps*  pHwEnvironmentCaps;                     ///< Static capabilities of overall hardware
                                                                            ///  environment. i.e. conbination of capabilities
                                                                            ///  of sensor, chipset, and other hardwares
};

/// @brief Encapsulates HW workarounds
struct HWBugWorkaround
{
    UINT32      workaroundId;       ///< Workaround Id: unique per HW platform
    const CHAR* pDesc;              ///< Workaround description
    BOOL        enabled;            ///< TRUE if the workaround is enabled; FALSE otherwise.
    BOOL        presilCSIMEnabled;  ///< Enabled on CSIM?
    BOOL        presilRUMIEnabled;  ///< Enabled on RUMI?
};

/// @brief Encapsulates an array of workarounds
struct HWBugWorkarounds
{
    UINT32                  numWorkarounds; ///< Number of workarounds
    const HWBugWorkaround*  pWorkarounds;   ///< Pointer to workaround array
};

/// @brief Static entry points for HW specific HwContext
struct HwContextStaticEntry
{
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// Create
    ///
    /// @brief  Create an instance of the device HwContext
    ///
    /// @param  pCreateData  Hardware context create data.
    ///
    /// @return CamxResultSuccess if successful
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult (*Create)(
        HwContextCreateData* pCreateData);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetStaticMetadataKeysInfo
    ///
    /// @brief  Retrieve static available metadata keys array info by metadata tag from the HwContext.
    ///
    /// @param  pKeysInfo  Static available metadata keys info pointer to be filled in.
    /// @param  tag        Static metadata type to be retrieved.
    ///
    /// @return CamxResultSuccess if successful
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult (*GetStaticMetadataKeysInfo)(
        StaticMetadataKeysInfo* pKeysInfo,
        CameraMetadataTag       tag);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetStaticCaps
    ///
    /// @brief  Retrieve static capabilities for the platform from the HwContext. This does not include capabilities of the
    ///         camera sensor.
    ///
    /// @param  pCaps   Static capabilities to be populated.
    ///
    /// @return CamxResultSuccess if successful
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult (*GetStaticCaps)(
        PlatformStaticCaps* pCaps);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// CreateHwFactory
    ///
    /// @brief  Create the HW Factory object
    ///
    /// @return The HW factory
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    HwFactory* (*CreateHwFactory)();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// QueryVendorTagsInfo
    ///
    /// @brief  Retrieve supported vendor tags
    ///
    /// @return CamxResultSuccess if successful
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult (*QueryVendorTagsInfo)(
        VendorTagInfo* pVendorTagInfo);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetHWBugWorkarounds
    ///
    /// @brief  Retrieve static list of the known HW bug workarounds and their properties for the platform from the HwContext.
    ///
    /// @param  pWorkarounds   Static workarounds to be populated.
    ///
    /// @return CamxResultSuccess if successful
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult (*GetHWBugWorkarounds)(
        HWBugWorkarounds* pWorkarounds);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// QueryExternalComponentVendorTagsInfo
    ///
    /// @brief  Retrieve supported external component vendor tags
    ///
    /// @return CamxResultSuccess if successful
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult (*QueryExternalComponentVendorTagsInfo)(
        ComponentVendorTagsInfo* pComponentVendorTagsInfo);
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief Class that implements the Hw environment storing static information
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class HwEnvironment
{
public:
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetInstance
    ///
    /// @brief  This function returns the singleton instance of the HwEnvironment.
    ///
    /// @return A pointer to the singleton instance of the HwEnvironment
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static HwEnvironment* GetInstance();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetNCSObject
    ///
    /// @brief  This function returns the singleton instance of the NCS Object.
    ///
    /// @return A pointer to the singleton instance NCS Object
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID* GetNCSObject();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// SetNCSObject
    ///
    /// @brief  This function sets the singleton instance of the NCS Object.
    ///
    /// @param  pNCSObject Pointer to the NCS Object
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID SetNCSObject(
        VOID* pNCSObject);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// InitializeSensorHwDeviceCache
    ///
    /// @brief  Initialize cache for sensor devices
    ///
    /// @param  cameraID              Physical Camera ID
    /// @param  pHwContext            HW context
    /// @param  hCSLSession           CSL session handle
    /// @param  hCSLSessionIdx        Session idx for multiple realtime sessions
    /// @param  pSensorDeviceHandles  Sensor device handles
    /// @param  pSensorDeviceCache    Sensor device cache
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID InitializeSensorHwDeviceCache(
        UINT32                 cameraID,
        HwContext*             pHwContext,
        CSLHandle              hCSLSession,
        UINT32                 hCSLSessionIdx,
        VOID*                  pSensorDeviceHandles,
        VOID*                  pSensorDeviceCache);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetNumCameras
    ///
    /// @brief  Get the number of cameras available. This method must be called after Probe.
    ///
    /// @return The number of cameras available.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    UINT32 GetNumCameras()
    {
        return m_numberSensors;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetCameraInfo
    ///
    /// @brief  Get the static information for a camera device.
    ///
    /// @param  cameraID    The cameraID of the camera to retrieve information for.
    /// @param  pCameraInfo The camera information.
    ///
    /// @return CamxResultSuccess if successful.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult GetCameraInfo(
        UINT32          cameraID,
        HwCameraInfo*   pCameraInfo
        ) const;

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetSettingsManager
    ///
    /// @brief  Returns a pointer to the hardware independent settings manager.
    ///
    /// @return A pointer to the hardware independent settings manager.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    const SettingsManager* GetSettingsManager() const;

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetStaticSettings
    ///
    /// @brief  Returns a pointer to the static settings from the hardware independent settings manager.
    ///
    /// @return A pointer to the static settings from the hardware independent settings manager.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    const StaticSettings* GetStaticSettings() const;

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetHwFactory
    ///
    /// @brief  Get the HW specific factory
    ///
    /// @return Pointer to the HwFactory
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CAMX_INLINE const HwFactory* GetHwFactory() const
    {
        return m_pHwFactory;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetHwStaticEntry
    ///
    /// @brief  Returns a pointer to the hardware static entry methods.
    ///
    /// @return A pointer to the hardware static entry methods.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CAMX_INLINE const HwContextStaticEntry* GetHwStaticEntry() const
    {
        return &m_staticEntryMethods;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetTuningDataManager
    ///
    /// @brief  Returns a pointer to the tuning manager for a given camera.
    ///
    /// @param  cameraID    The cameraID of the camera to retrieve information for.
    ///
    /// @return A pointer to the tuning data manager.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CAMX_INLINE TuningDataManager* GetTuningDataManager(
        UINT32 cameraID) const
    {
        CAMX_ASSERT(cameraID < m_numberSensors);
        return m_pTuningManager[cameraID];
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetPlatformStaticCaps
    ///
    /// @brief  Retrieve the already initialized platform static caps for the chipset
    ///
    /// @return A pointer to the already initialized platform static caps.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CAMX_INLINE const PlatformStaticCaps* GetPlatformStaticCaps() const
    {
        return &m_platformCaps[0];
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetSensorInfoTable
    ///
    /// @brief  Retrieve the already initialized sensor information table
    ///
    /// @return A pointer to the already initialized sensor information table.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CAMX_INLINE const HwSensorInfo* GetSensorInfoTable() const
    {
        return &m_sensorInfoTable[0];
    }
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetExternalComponent
    ///
    /// @brief  Returns a pointer to the external component.
    ///
    /// @return pointer to the external component.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CAMX_INLINE ExternalComponentInfo* GetExternalComponent() const
    {
        return (ExternalComponentInfo*)&m_externalComponent[0];
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetNumExternalComponent
    ///
    /// @brief  Returns the number of external components.
    ///
    /// @return number of external components.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CAMX_INLINE UINT GetNumExternalComponent() const
    {
        return m_numExternalComponent;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetStaticMetadataKeysInfo
    ///
    /// @brief  Retrieve static available metadata keys array info by metadata tag from the HwContext.
    ///
    /// @param  pKeysInfo  Static available metadata keys info pointer to be filled in.
    /// @param  tag        Static metadata type to be retrieved.
    ///
    /// @return CamxResultSuccess if successful
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult GetStaticMetadataKeysInfo(
        StaticMetadataKeysInfo* pKeysInfo,
        CameraMetadataTag       tag
        ) const;


    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetDeviceVersion
    ///
    /// @brief  Query version of a particular device from CSL.
    ///
    /// @param  deviceType  The device to query the version of.
    /// @param  pVersion    Pointer to version structure to be populated with device version number. If a device is not
    ///                     supported the version will be reported as 0.0.0
    ///
    /// @return CamxResultSuccess if successful.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult GetDeviceVersion(
        CSLDeviceType   deviceType,
        CSLVersion*     pVersion
        ) const;

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetDriverVersion
    ///
    /// @brief  Query version of a particular device driver from CSL.
    ///
    /// @param  deviceType  The device to query the version of.
    /// @param  pVersion    Pointer to version structure to be populated with driver version number. If a device is not
    ///                     supported the version will be reported as 0.0.0
    ///
    /// @return CamxResultSuccess if successful.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult GetDriverVersion(
        CSLDeviceType   deviceType,
        CSLVersion*     pVersion
        ) const;

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetDeviceIndices
    ///
    /// @brief  Query the set of device indices available for a given device type.
    ///
    /// @param  deviceType                      The device type to query.
    /// @param  pDeviceIndices                  Client allocated array to write the set of device indices to.
    /// @param  deviceIndicesLength             The number of entries allocated in pDeviceIndices.
    /// @param  pDeviceIndicesLengthRequired    The number of indices available and the required entries in pDeviceIndices to
    ///                                         hold all available entries.
    ///
    /// @return CamxResultSuccess if successful.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult GetDeviceIndices(
        CSLDeviceType   deviceType,
        INT32*          pDeviceIndices,
        UINT            deviceIndicesLength,
        UINT*           pDeviceIndicesLengthRequired
        ) const;

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetImageSensorModuleData
    ///
    /// @brief  Get the image sensor module data instance for this camera session.
    ///
    /// @param  cameraID    The cameraID of the camera to retrieve information for.
    ///
    /// @return A const pointer to the ImageSensorModuleData class. NULL if not valid.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    const ImageSensorModuleData* GetImageSensorModuleData(
        UINT32 cameraID
        ) const;

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// IsHWBugWorkaroundEnabled
    ///
    /// @brief  Check if a HW bug workaround is enabled
    ///
    /// @param  workaroundId    The workaround ID
    ///
    /// @return TRUE if enabled; FALSE otherwise.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    BOOL IsHWBugWorkaroundEnabled(
        const UINT32 workaroundId
        ) const;

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// SearchExternalComponent
    ///
    /// @brief  Retrieve External Component info.
    ///
    /// @param  pNodeComponentInfo          Name of External Components to retrieve information for.
    /// @param  nodeExternalComponentCount  count of external components
    ///
    /// @return CamxResultSuccess if successful
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult SearchExternalComponent(
        ExternalComponentInfo* pNodeComponentInfo,
        UINT                   nodeExternalComponentCount
    ) const;

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ReadEEPROMData
    ///
    /// @brief  Reads the data from EEPROM and updates to sensor static capabilities.
    ///
    /// @param  cameraID    The cameraID of the camera to retrieve information for.
    /// @param  hCSL        Handle to the CSL session.
    ///
    /// @return success of failure.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult ReadEEPROMData(
        UINT32      cameraID,
        CSLHandle   hCSL);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ClosestMaxFps
    ///
    /// @brief  Returns the closest supported  max fps.
    ///
    /// @param  fps             Fps calculated based on node's clockrate.
    /// @param  sensorIndex     Corresponding sensor index.
    ///
    /// @return Closest fps value from the supported fps list.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    INT ClosestMaxFps(
        INT     fps,
        INT     sensorIndex);


    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// DumpEEPROMData
    ///
    /// @brief  Dump the EEPROM data from static capabilities.
    ///
    /// @param  cameraId    The cameraID of the camera to retrieve information for.
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID DumpEEPROMData(
        UINT32      cameraId);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetSocId
    ///
    /// @brief  Get the CSL Camera Family Soc Id
    ///
    /// @return CSL Camera Family Soc Id
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CAMX_INLINE CSLCameraFamilySoc GetSocId() const
    {
        return m_socId;
    }

    INT32 m_eebinDeviceIndex;   ///< EEBIN device Index

private:
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// InitCapsStatus
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    enum InitCapsStatus
    {
        InitCapsInvalid      = 0,   ///< Invalid status.
        InitCapsInitialize   = 1,   ///< Initialize status.
        InitCapsRunning      = 2,   ///< Running init capability.
        InitCapsDone         = 3,   ///< Init capability done.
    };

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// Initialize
    ///
    /// @brief  Global initialize of the camera HW. This method will probe the HW and discover the installed camera sensors.
    ///         This method should only be called once during the startup of the camera process.
    ///
    /// @return CamxResultSuccess if successful.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult Initialize();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// InitCaps
    ///
    /// @brief  This function sets up the caps structures within the HwEnvironment.  Must be called after creation is complete
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID InitCaps();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ~HwEnvironment
    ///
    /// @brief  Destructor
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    ~HwEnvironment();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// HwEnvironment
    ///
    /// @brief  Constructor
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    HwEnvironment();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// QueryHwContextStaticEntryMethods
    ///
    /// @brief  Retrieve the static entry methods for the platform specific HwContext.
    ///
    /// @return CamxResultSuccess if successful.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult QueryHwContextStaticEntryMethods();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ProbeImageSensorModules
    ///
    /// @brief  Probe the image sensor modules available and record detected image sensor modules and their capabilities.
    ///
    ///         This method will update s_numberSensors as well. In case of any errors for any sensor s_numberSensors will not
    ///         be updated and sensor not recorded as detected.
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID ProbeImageSensorModules();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// EnumerateDevices
    ///
    /// @brief  Enumerate the devices available and store to the device table.
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID EnumerateDevices();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetEEbinDeviceIndex
    ///
    /// @brief  Enumerate the devices available and store to the device table.
    ///
    /// @param  deviceIndex                     Device Index of EEPROM device.
    /// @param  deviceType                      EEPROM device type.
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID GetEEbinDeviceIndex(
        INT32           deviceIndex,
        CSLDeviceType   deviceType);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// EnumerateSensorDevices
    ///
    /// @brief  Enumerate the sensor devices available and store to the device table.
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID EnumerateSensorDevices();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// InitializeSensorSubModules
    ///
    /// @brief  Create and initialize sensor sub modules Actuator, EEPROM etc.
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID InitializeSensorSubModules();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// InitializeSensorStaticCaps
    ///
    /// @brief  Initialize sensor static capabilities.
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID InitializeSensorStaticCaps();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// InitializeHwEnvironmentStaticCaps
    ///
    /// @brief  Initialize hw environment static capabilities base on capabilities of sensor, chipset platform, and all other
    ///         hardware.
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID InitializeHwEnvironmentStaticCaps();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetSensorVersion
    ///
    /// @brief  Query version of the sensor.
    ///
    /// @param  cameraId  The camera id to query the version of sensor.
    ///
    /// @return sensor version.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    UINT32 GetSensorVersion(
        UINT32         cameraId
        ) const;

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// InitializeScalerStaticCaps
    ///
    /// @brief  Initialize HW environment scaler static capabilities base on capabilities of sensor, chipset platform, and all
    ///         other hardware.
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID InitializeScalerStaticCaps();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// IsSupportedVideoResolution
    ///
    /// @brief  is it a supported video resolution or not
    ///
    /// @param  pPlatformCaps  platform capabilities for the sensor
    /// @param  width          width
    /// @param  height         height
    ///
    /// @return TRUE or FALSE
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static CAMX_INLINE BOOL IsSupportedVideoResolution(
           const PlatformStaticCaps* pPlatformCaps,
           INT32                     width,
           INT32                     height)
    {
        BOOL isVideoResolution = FALSE;

        for (UINT index = 0; index < pPlatformCaps->numVideoResolutions; index++)
        {
            if (pPlatformCaps->videoResolutions[index].width  == width &&
                pPlatformCaps->videoResolutions[index].height == height)
            {
                isVideoResolution = TRUE;
                break;
            }
        }

        return isVideoResolution;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// InitializeJPEGMaxSizeStaticCaps
    ///
    /// @brief  Initialize HW environment JPEG  max size static capabilities base on capabilities of sensor, chipset platform,
    ///         and all other hardware.
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID InitializeJPEGMaxSizeStaticCaps();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// InitializeAETargetFPSRangesStaticCaps
    ///
    /// @brief  Initialize HW environment AE target FPS ranges static capabilities base on capabilities of sensor, chipset
    ///         platform, and all other hardware.
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID InitializeAETargetFPSRangesStaticCaps();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// InitializeNonCameraSensorCaps
    ///
    /// @brief  Initialize Non-Camera Sensor realted capabilities by probing the sensor
    ///         platform interfaces, and all other hardware.
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID InitializeNonCameraSensorCaps();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// CreateTuningDataManager
    ///
    /// @brief  Create Tuning Manager for the probed sensor
    ///
    /// @param  pData              Pointer to the image sensor module data
    /// @param  sensorIndex        Corresponding sensor index
    ///
    /// @return CamxResultSuccess if successful
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult CreateTuningDataManager(
        ImageSensorModuleData* pData,
        UINT                   sensorIndex);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// SetAvailableStreamConfig
    ///
    /// @brief  Set an available scaler stream configuration
    ///
    /// @param  pStreamConfig       The pointer of stream congifuration to be set.
    /// @param  format              Stream format.
    /// @param  width               Stream width.
    /// @param  height              Stream height.
    /// @param  type                Stream type.
    /// @param  pNumStreamConfigs   Number available stream configurations
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static CAMX_INLINE VOID SetAvailableStreamConfig(
        ScalerStreamConfig* pStreamConfig,
        INT32                      format,
        UINT32                     width,
        UINT32                     height,
        INT32                      type,
        UINT*                      pNumStreamConfigs)
    {
        pStreamConfig->format = format;
        pStreamConfig->width  = width;
        pStreamConfig->height = height;
        pStreamConfig->type   = type;

        if (NULL != pNumStreamConfigs)
        {
            (*pNumStreamConfigs)++;
        }
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// SetAvailableCustomHFRConfig
    ///
    /// @brief  Set an available scaler stream configuration
    ///
    /// @param  pHfrConfig          The pointer of custom HFR congifuration to be set.
    /// @param  width               Stream width.
    /// @param  height              Stream height.
    /// @param  fps                 Max FPS supported (must be 60 or 90)
    /// @param  pNumCustomHFRSizes  Number available HFR configurations
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static CAMX_INLINE VOID SetAvailableCustomHFRConfig(
        HFRCustomParams* pHfrConfig,
        UINT32                     width,
        UINT32                     height,
        INT32                      fps,
        UINT*                      pNumCustomHFRSizes)
    {
        pHfrConfig->width  = width;
        pHfrConfig->height = height;
        pHfrConfig->maxFPS = fps;

        if (NULL != pNumCustomHFRSizes)
        {
            (*pNumCustomHFRSizes)++;
        }
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// SetAvailableCustomHFRPreviewVideoConfig
    ///
    /// @brief  Set an available scaler stream configuration
    ///
    /// @param  pHfrConfig              The pointer of custom HFR congifuration to be set.
    /// @param  width                   Stream width.
    /// @param  height                  Stream height.
    /// @param  previewFPS              Preview FPS supported
    /// @param  videoFPS                Video FPS supported
    /// @param  pNumPreviewVideoParams  Number of supported preview-video fps combinations
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static CAMX_INLINE VOID SetAvailableCustomHFRPreviewVideoConfig(
        HFRCustomPreviewVideoParams* pHfrConfig,
        UINT32                       width,
        UINT32                       height,
        INT32                        previewFPS,
        INT32                        videoFPS,
        UINT*                        pNumPreviewVideoParams)
    {
        pHfrConfig->width      = width;
        pHfrConfig->height     = height;
        pHfrConfig->previewFPS = previewFPS;
        pHfrConfig->videoFPS   = videoFPS;

        if (NULL != pNumPreviewVideoParams)
        {
            (*pNumPreviewVideoParams)++;
        }
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// SetVideoEISLiveshotParams
    ///
    /// @brief  Set fps EIS Liveshot per video resolution
    ///
    /// @param  pVideoMitigations                The pointer of custom HFR congifuration to be set.
    /// @param  width                            width.
    /// @param  height                           height.
    /// @param  maxPreviewFPS                    Max Preview FPS supported
    /// @param  videoFPS                         Video FPS supported
    /// @param  liveShotSupported                Liveshot supported or not
    /// @param  EISSupported                     EIS supported or not
    /// @param  pNumSupportedVideoResolutions    Number of video mitigations
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static CAMX_INLINE VOID SetVideoEISLiveshotParams(
        VideoMitigationsParams*      pVideoMitigations,
        UINT32                       width,
        UINT32                       height,
        INT32                        maxPreviewFPS,
        INT32                        videoFPS,
        INT32                        liveShotSupported,
        INT32                        EISSupported,
        UINT*                        pNumSupportedVideoResolutions)
    {
        pVideoMitigations->width                 = width;
        pVideoMitigations->height                = height;
        pVideoMitigations->maxPreviewFPS         = maxPreviewFPS;
        pVideoMitigations->videoFPS              = videoFPS;
        pVideoMitigations->isLiveshotSupported   = liveShotSupported;
        pVideoMitigations->isEISSupported        = EISSupported;

        if (NULL != pNumSupportedVideoResolutions)
        {
            (*pNumSupportedVideoResolutions)++;
        }
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// SetAvailableMinFrameDuration
    ///
    /// @brief  Set an available scaler minimum frame duration
    ///
    /// @param  pFrameDuration              The pointer of stream frame duration to be set.
    /// @param  format                      Stream format.
    /// @param  width                       Stream width.
    /// @param  height                      Stream height.
    /// @param  minFrameDurationNanoSeconds Duration in nano second
    /// @param  pNumMinFrameDurations       Number stream minimum frame durations
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static CAMX_INLINE VOID SetAvailableMinFrameDuration(
        ScalerFrameDurationINT64*  pFrameDuration,
        INT64                      format,
        INT64                      width,
        INT64                      height,
        INT64                      minFrameDurationNanoSeconds,
        UINT*                      pNumMinFrameDurations)
    {
        pFrameDuration->format                      = format;
        pFrameDuration->width                       = width;
        pFrameDuration->height                      = height;
        pFrameDuration->minFrameDurationNanoSeconds = minFrameDurationNanoSeconds;
        if (NULL != pNumMinFrameDurations)
        {
            (*pNumMinFrameDurations)++;
        }
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// SetAvailableMinStallDuration
    ///
    /// @brief  Set an available scaler minimum stall duration
    ///
    /// @param  pStallDuration              The pointer of stream stall duration to be set.
    /// @param  format                      Stream format.
    /// @param  width                       Stream width.
    /// @param  height                      Stream height.
    /// @param  stallDurationNanoSeconds    Stall duration in nano second
    /// @param  pNumMinStallDurations       Number stream minimum stall durations
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static CAMX_INLINE VOID SetAvailableMinStallDuration(
        ScalerStallDurationINT64*  pStallDuration,
        INT64                      format,
        INT64                      width,
        INT64                      height,
        INT64                      stallDurationNanoSeconds,
        UINT*                      pNumMinStallDurations)
    {
        pStallDuration->format                      = format;
        pStallDuration->width                       = width;
        pStallDuration->height                      = height;
        pStallDuration->stallDurationNanoSeconds    = stallDurationNanoSeconds;
        if (NULL != pNumMinStallDurations)
        {
            (*pNumMinStallDurations)++;
        }
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// CalculateRawSize
    ///
    /// @brief  Calculate RAW size.
    ///
    /// @param  bitWidth    Stream bpp.
    /// @param  width       Stream width.
    /// @param  height      Stream height.
    ///
    /// @return The adjusted size.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    UINT32 CalculateRawSize(
        UINT32 bitWidth,
        UINT32 width,
        UINT32 height);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetHWResourceInfo
    ///
    /// @brief  Gets the worstcase hardware resouce details
    ///
    /// @param  pSensorCaps       Sensor capability
    /// @param  sensorIndex       Sensor index.
    /// @param  pMaxIFEsRequired  Maximum number of IFEs required
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID GetHWResourceInfo(
        const SensorModuleStaticCaps*   pSensorCaps,
        const INT                       sensorIndex,
        INT32*                          pMaxIFEsRequired);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // Private Member Data
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    HwFactory*                       m_pHwFactory;                                     ///< The HW Factory
    SettingsManager*                 m_pSettingsManager;                               ///< Static hardware-independent settings
                                                                                       ///  manager
    ImageSensorModuleDataManager*    m_pImageSensorModuleDataManager;                  ///< Static image sensor data module
                                                                                       ///  manager.
    HwContextStaticEntry             m_staticEntryMethods;                             ///< Static entry methods for platform
                                                                                       ///  specific HwContext.
    ExternalComponentInfo            m_externalComponent[MaxExternalComponents];       ///< List of external components.
    UINT                             m_numExternalComponent;                           ///< number of External components
    HwDeviceTypeInfo                 m_cslDeviceTable[CSLDeviceTypeMaxDevice];         ///< Table of devices supported for
                                                                                       ///  this HW.
    HwSensorInfo                     m_sensorInfoTable[MaxNumImageSensors];            ///< Table of detected image sensors.
    UINT                             m_numberSensors;                                  ///< The number of sensors in
                                                                                       ///  s_sensorInfoTable
    InitCapsStatus                   m_initCapsStatus;                                 ///< Workaround (CAMX-2684)
    PlatformStaticCaps               m_platformCaps[MaxNumImageSensors];               ///< Static platform capabilities.
    HwEnvironmentStaticCaps          m_caps[MaxNumImageSensors];                       ///< Static environment capabilities.
    TuningDataManager*               m_pTuningManager[MaxNumImageSensors];             ///< Tuning manager per detected, will
                                                                                       ///  be available to each node through
                                                                                       ///  HwContext
    HWBugWorkarounds                 m_workarounds;                                    ///< HW workarounds
    VOID*                            m_pNCSObject;                                     ///< NCS Service object
    Mutex*                           m_pHWEnvLock;                                     ///< Used to serialize hwenv ops

    CAMXCustomizeOEMInterface*       m_pOEMInterface;                                  ///< OEM interface

    CSLCameraFamilySoc               m_socId;                                          ///< CSL Camera Family Soc Id

    // Do not implement the copy constructor or assignment operator
    HwEnvironment(const HwEnvironment&)             = delete;                          ///< Disallow the copy constructor.
    HwEnvironment& operator=(const HwEnvironment&)  = delete;                          ///< Disallow assignment operator.
};

CAMX_NAMESPACE_END

#endif // CAMXHWENVIRONMENT_H
