////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2016-2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  camxhal3module.h
/// @brief Declarations for HAL3Module class. The purpose of the HAL3Device class is to abstract camera_module_t methods. For
///        further information on the corresponding methods in the HAL3 API, please refer to hardware/hardware.h and
///        hardware/camera3.h.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#ifndef CAMXHAL3MODULE_H
#define CAMXHAL3MODULE_H

#include "camxdefs.h"
#include "camxchitypes.h"
#include "camxhwenvironment.h"
#include "camxsettingsmanager.h"
#include "camxthermalmanager.h"
#include "camxvendortags.h"

#include "chi.h"
#include "chioverride.h"

CAMX_NAMESPACE_BEGIN

class MetadataPool;
class UseCaseManager;
class ThermalManager;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Type Definitions
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief Structure to encapsulate basic properties of an image sensor.
struct ImageSensorDescriptor
{
    UINT32 CSLDeviceIndex;  ///< The unique CSL device index as returned by CSLEnumerateDevices
};

/// @brief Misc information about each camera
struct HAL3PerCameraInfo
{
    MetadataPool* pStaticMetadataPool;      ///< Static Metadatapool
    BOOL          isCameraOpened;           ///< Is camera opened or not indicator
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief The HAL3Module class provides the implementation used by HAL3 for the methods in camera_module_t.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class HAL3Module
{
public:
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // Public Methods
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetInstance
    ///
    /// @brief  This function returns the singleton instance of the HAL3Module.
    ///
    /// @return A pointer to the singleton instance of the HAL3Module
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static HAL3Module* GetInstance();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetNumCameras
    ///
    /// @brief  Gets the number of cameras/sensors exposed to the framework
    ///
    /// @return Number of cameras/sensors exposed to the framework
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    UINT32 GetNumCameras() const;

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetCameraInfo
    ///
    /// @brief  This function returns the static camera information for the given camera device. This method is an abstraction
    ///         of the camera_module_t::get_camera_info() API.
    ///
    /// @param  logicalCameraId     The index of the camera to return information about
    /// @param  pCameraInfo         The structure to write the static camera information to for a given camera device
    ///
    /// @return CamxResultSuccess if successful
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult GetCameraInfo(
        UINT32      logicalCameraId,
        CameraInfo* pCameraInfo
        ) const;

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetInfo
    ///
    /// @brief  Generic query interface for getting info from the CHI.
    ///
    /// @param  infoCmd         Type of info required
    /// @param  pInputParams    Caller-owned buffer for additional input params necessary for the query
    /// @param  pOutputParams   Caller-owned buffer for query result
    ///
    /// @return CamxResultSuccess if successful
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CAMX_INLINE CamxResult GetInfo(
       CDKGetInfoCmd infoCmd,
       VOID*         pInputParams,
       VOID*         pOutputParams)
    {
        return m_ChiAppCallbacks.chi_get_info(infoCmd, pInputParams, pOutputParams);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ProcessCameraOpen
    ///
    /// @brief  Called when a camera is opened to do any housekeeping
    ///
    /// @param  frameworkCameraId   Framework camera id being opened
    /// @param  pPriv               Private data (implementation dependent)
    ///
    /// @return CamxResultSuccess if successful, Failure otherwise
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult ProcessCameraOpen(
        UINT32  frameworkCameraId,
        VOID*   pPriv);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ProcessCameraClose
    ///
    /// @brief  Called when a camera is closed to do any housekeeping
    ///
    /// @param  logicalCameraId    Logical camera id that is closed
    /// @param  pPriv              Private data (implementation dependent)
    ///
    /// @return CamxResultSuccess if successful, failure otherwise
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult ProcessCameraClose(
        UINT32 logicalCameraId,
        VOID*  pPriv);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// SetCbs
    ///
    /// @brief  This function is used by the framework to provide the HAL with callback function pointers. The callbacks are
    ///         used to inform the framework of device and torch status changes. This method is an abstraction of the
    ///         camera_module_t::set_callbacks() API.
    ///
    /// @param  pModuleCbs A table of callback function pointers
    ///
    /// @return CamxResultSuccess if successful
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult SetCbs(
        const camera_module_callbacks_t* pModuleCbs);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// SetDropCallbacks
    ///
    /// @brief  Used to tell the module to drop the callbacks under certain error conditions.
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID SetDropCallbacks();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// SetTorchMode
    ///
    /// @brief  Turn on or off the torch mode of the flash unit associated with a given camera ID.
    ///
    /// @param  logicalCameraId     The index of the camera for which to enable the torch
    /// @param  fwNotificationId    ID to return to the FW for notification
    /// @param  enableTorch         Whether to attempt to turn the torch on or off
    ///
    /// @return CamxResultSuccess if able to reserve and set the torch mode
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult SetTorchMode(
        UINT32  logicalCameraId,
        UINT32  fwNotificationId,
        BOOL    enableTorch);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ReserveTorchForCamera
    ///
    /// @brief  Turn on or off the torch mode of the flash unit associated with a given camera ID.
    ///
    /// @param  logicalCameraId     The index of the camera for which to reserve the torch
    /// @param  fwNotificationId    ID to return to the FW for notification
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID ReserveTorchForCamera(
        UINT32  logicalCameraId,
        UINT32  fwNotificationId);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ReleaseTorchForCamera
    ///
    /// @brief  Turn on or off the torch mode of the flash unit associated with a given camera ID.
    ///
    /// @param  logicalCameraId     The index of the camera for which to release the torch
    /// @param  fwNotificationId    ID to return to the FW for notification
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID ReleaseTorchForCamera(
        UINT32  logicalCameraId,
        UINT32  fwNotificationId);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// Dump
    ///
    /// @brief  This method is used by the application framework to print debugging state for the camera module. It will be
    ///         called when using the dumpsys tool or capturing a bugreport.
    ///
    /// @param  fd The file descriptor which can be used to write debugging text using dprintf() or write().
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID Dump(
        INT fd)
        const;

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetCHIAppCallbacks
    ///
    /// @brief  Get the pointer to the CHI App callback functions structure
    ///
    /// @return Pointer to the CHIAppCallbacks structure
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CAMX_INLINE chi_hal_callback_ops_t* GetCHIAppCallbacks()
    {
        return &m_ChiAppCallbacks;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// IsCHIOverrideModulePresent
    ///
    /// @brief  Function to check for the presence of a CHI override module
    ///
    /// @return TRUE if present, FALSE otherwise
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CAMX_INLINE BOOL IsCHIOverrideModulePresent() const
    {
        return ((NULL == m_hChiOverrideModuleHandle) ? FALSE : TRUE);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetThermalManager
    ///
    /// @brief  Get the thermal mitigation manager
    ///
    /// @return Pointer to the manager
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CAMX_INLINE ThermalManager* GetThermalManager() const
    {
        return m_pThermalManager;
    }

private:

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// InitializeStaticMetadata
    ///
    /// @brief  Initialize static metadata for the given camera ID. This will be called only once at the HAL3 module creation.
    ///
    /// @param  cameraId     The index of the camera for which the static data need to be populated.
    /// @param  pCameraInfo  The structure to write the static camera information to for a given camera device
    ///
    /// @return CamxResultSuccess if successful
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult InitializeStaticMetadata(
        UINT32      cameraId,
        CameraInfo* pCameraInfo
        ) const;

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// DeInitConfigurationForTorch
    ///
    /// @brief  Delete stream configuration used for Torch.
    ///
    /// @param  cameraId        The index of the camera for which the torch request received.
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID DeInitConfigurationForTorch(
        UINT32 cameraId);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// InitConfigurationForTorch
    ///
    /// @brief  Setup the stream configuration needed for Torch.
    ///
    /// @param  cameraId        The index of the camera for which the torch request received.
    /// @param  pStreamConfigs  Structure to write the stream config needed for Torch.
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID InitConfigurationForTorch(
        UINT32               cameraId,
        Camera3StreamConfig* pStreamConfigs);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// SubmitRequestForTorch
    ///
    /// @brief  Prepare and submit capture request for torch.
    ///
    /// @param  cameraId        The index of the camera for which the torch request received.
    /// @param  torchStatus     The new status of the torch.to prepare request based on On/Off status.
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID SubmitRequestForTorch(
    UINT32          cameraId,
    TorchModeStatus torchStatus);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// SetTorchModeInternal
    ///
    /// @brief  Turn on or off the torch mode of the flash unit associated with a given camera ID.
    ///
    /// @param  cameraId            The index of the camera for which to enable the torch
    /// @param  fwNotificationId    ID to return to the FW for notification
    /// @param  torchStatus         The new status of the torch.
    /// @param  inReleaseMode       In release torch flow or not (post failure or actual close)
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID SetTorchModeInternal(
        UINT32          cameraId,
        UINT32          fwNotificationId,
        TorchModeStatus torchStatus,
        BOOL            inReleaseMode);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// HAL3Module
    ///
    /// @brief  Default constructor for the HAL3Module class
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    HAL3Module();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ~HAL3Module
    ///
    /// @brief  Default destructor for the HAL3Module class
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    ~HAL3Module();

    // Do not implement the copy constructor or assignment operator
    HAL3Module(const HAL3Module& rHAL3Module) = delete;
    HAL3Module& operator= (const HAL3Module& rHAL3Module) = delete;

    /// This is an array of available image sensor devices
    ImageSensorDescriptor   m_imageSensors[MaxNumImageSensors];

    /// This is an array of the torch status for all sensor devices
    TorchModeStatus m_torchStatus[MaxNumImageSensors];

    /// This contains the framework callback functions to notify the client when the device or torch status changes
    const camera_module_callbacks_t* m_pModuleCbs;
    chi_hal_callback_ops_t           m_ChiAppCallbacks;          ///< CHI HAL override entry

    OSLIBRARYHANDLE       m_hChiOverrideModuleHandle;            ///< CHI override module handle
    UINT32                m_numCamerasOpened;                    ///< Number of concurrently opened cameras
    HAL3PerCameraInfo     m_perCameraInfo[MaxNumImageSensors];   ///< Information per camera
    const StaticSettings* m_pStaticSettings;                     ///< Static settings
    UINT32                m_numFwCameras;                        ///< Number of cameras exposed to the framework
    UINT32                m_numLogicalCameras;                   ///< Number of logical cameras created by the override layer
    Camera3Stream         m_torchStream;                         ///< Dummy Stream needed for torch.
    camera3_device_t      m_camera3Device;                       ///< Camera3 device needed for torch
    Metadata*             m_pMetadata;                           ///< Metadata needed for torch
    BOOL                  m_dropCallbacks;                       ///< Drop callbacks as in error state
    Metadata*             m_pStaticMetadata[MaxNumImageSensors]; ///< Framework static metadata
    ThermalManager*       m_pThermalManager;                     ///< Manager for Thermal Mitigation events
};

CAMX_NAMESPACE_END

#endif // CAMXHAL3MODULE_H
