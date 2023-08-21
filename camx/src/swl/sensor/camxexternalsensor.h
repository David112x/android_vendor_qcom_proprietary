////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2018 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  camxexternalsensor.h
/// @brief Sensor Module class declarations
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef CAMXEXTERNALSENSOR_H
#define CAMXEXTERNALSENSOR_H

#include "camxmem.h"
#include "camxcsiphysubmodule.h"

CAMX_NAMESPACE_BEGIN


/// @brief Enum describing sensor configuration status
enum class SensorState
{
    SensorUninitialized,    ///< Sensor Uninitialized
    SensorAcquired,         ///< Sensor Acquired
    SensorStreamedOn,       ///< Sensor Streamed On
    SensorStreamedOff,      ///< Sensor Streamed Off
    SensorReleased,         ///< Sensor Released
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief Class that implements the external sensor class
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class ExternalSensor
{
public:
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// Create
    ///
    /// @brief  This method creates an instance of the ExternalSensor module
    ///
    /// @return Pointer to the concrete ExternalSensor object
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static ExternalSensor* Create();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// Destroy
    ///
    /// @brief  This method destroys the derived instance of the interface
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID Destroy();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// Initialize
    ///
    /// @brief  method to Initialize External Sensor Object.
    ///
    /// @param  cameraId              Camera ID
    /// @param  hCSLSession           CSL session handle
    /// @param  pHwContext            HW context
    /// @param  currentSensorMode     Resolution Index
    ///
    /// @return CamxResultSuccess if successful
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult Initialize(
        UINT32     cameraId,
        CSLHandle  hCSLSession,
        HwContext* pHwContext,
        UINT32     currentSensorMode);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// Uninitialize
    ///
    /// @brief  method to Release Sensor and PHY Devices.
    ///
    /// @return CamxResultSuccess if successful
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult Uninitialize();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// StreamOn
    ///
    /// @brief  Method to stream on
    ///
    /// @return CamxResultSuccess if successful
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult StreamOn();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// StreamOff
    ///
    /// @brief  Method to stream off
    ///
    /// @return CamxResultSuccess if successful
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult StreamOff();

private:
    ExternalSensor(const ExternalSensor&) = delete;             ///< Disallow the copy constructor.
    ExternalSensor& operator=(const ExternalSensor&) = delete;  ///< Disallow assignment operator.

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ExternalSensor
    ///
    /// @brief  Constructor
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    ExternalSensor();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ~ExternalSensor
    ///
    /// @brief  Destructor
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    ~ExternalSensor();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetSensorModuleIndexes
    ///
    /// @brief  Get sensor module and submodule device indexes
    ///
    /// @param  pCSIPHYCapability pointer to capability of CSIPHY module
    ///
    /// @return CamxResultSuccess
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult GetSensorModuleIndexes(
        CSLCSIPHYCapability*     pCSIPHYCapability);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// AddDeviceIndex
    ///
    /// @brief  Add the given device index to the list of indices for this node
    ///
    /// @param  deviceIndex Device index to add
    ///
    /// @return CamxResult
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult AddDeviceIndex(
        INT32 deviceIndex);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// AddOutputDeviceIndices
    ///
    /// @brief  Update output port device index list by adding the device index list passed
    ///
    /// @param  portId           Output port id
    /// @param  pDeviceIndices   Pointer to array of device indices that would be added to the access list for the output port
    /// @param  deviceIndexCount The number of valid entries in the device index array
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID AddOutputDeviceIndices(
        UINT portId,
        const INT32 * pDeviceIndices,
        UINT deviceIndexCount);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// OutputPortIndex
    ///
    /// @brief  Return output port index for a given port id
    ///
    /// @param  portId port id
    ///
    /// @return port index
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    UINT OutputPortIndex(
        UINT portId);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// InitializeCmdBufferManagerList
    ///
    /// @brief  Helper method to allow HW subnodes to initialize command manager store based on their requirements.
    ///
    /// @param  maxNumManagers  Max number of managers that may be requested throughout the nodes' lifetime
    ///
    /// @return CamxResult
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult InitializeCmdBufferManagerList(
        UINT maxNumManagers);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// LoadSensorInitConfigCmd
    ///
    /// @brief   Load sensor init config cmd
    ///
    /// @return  CamxResultSuccess
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult LoadSensorInitConfigCmd();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// CreateAndSubmitCommand
    ///
    /// @brief  Helper method to create and submit sensor configurations packet/commands
    ///
    /// @param  cmdSize                  command size of the setting
    /// @param  cmdType                  command type of the setting
    /// @param  opCode                   opCode for the setting
    /// @param  currentResolutionIndex   opCode for the setting
    /// @param  regSettingIdx            support slave address for each reg setting
    ///
    /// @return CamxResultSuccess, if SUCCESS
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult CreateAndSubmitCommand(
        UINT cmdSize,
        I2CRegSettingType cmdType,
        CSLPacketOpcodesSensor opCode,
        UINT32 currentResolutionIndex,
        UINT regSettingIdx);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// CreateCmdBufferManager
    ///
    /// @brief  A helper function to allow sub-classes to create and add command buffer managers that parent node will manage.
    ///
    /// @param  pParams             Parameters the manager was created with.
    /// @param  ppCmdBufferManager  Pointer to the manager
    ///
    /// @return CamxResult
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult CreateCmdBufferManager(
        const ResourceParams * pParams,
        CmdBufferManager ** ppCmdBufferManager);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetCmdBuffer
    ///
    /// @brief  Helper method to acquire a free command buffer from the given manager.
    ///
    /// @param  pCmdBufferManager   Command buffer manager from which a buffer is acquired
    ///
    /// @return Pointer to a CmdBuffer object
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CmdBuffer * GetCmdBuffer(
        CmdBufferManager * pCmdBufferManager);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetPacket
    ///
    /// @brief  Helper method to acquire a free packet from the given manager.
    ///
    /// @param  pCmdBufferManager   Command buffer manager from which a buffer is acquired
    ///
    /// @return Pointer to a Packet object
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    Packet * GetPacket(
        CmdBufferManager * pCmdBufferManager);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// LoadSensorConfigCmds
    ///
    /// @brief  Helper method to load sensor configurations packet/commands
    ///
    /// @return CamxResultSuccess
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult LoadSensorConfigCmds();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// CreateSensorSubmodules
    ///
    /// @brief  Creates sensor submodules and physical resources
    ///
    /// @return CamxResultSuccess
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult CreateSensorSubmodules();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// CreateSubModules
    ///
    /// @brief  Creates sub modules
    ///
    /// @return CamxResultSuccess is creation of submodules successfull
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult CreateSubModules();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetInitialCameraInfo
    ///
    /// @brief  Get the initial camera infomation.
    ///
    /// @param  pCameraInfo point to Camera ID, type and role.
    ///
    /// @return CamxResult
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult GetInitialCameraInfo(
        StatsCameraInfo * pCameraInfo);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// AddCSLDeviceHandle
    ///
    /// @brief  Add the given CSL device handle to the list for this node for linking.
    ///
    /// @param  hCslDeiveHandle to add
    ///
    /// @return CamxResult
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult AddCSLDeviceHandle(
        CSLDeviceHandle hCslDeiveHandle);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// PrepareStreamOff
    ///
    /// @brief  Method that is called by topology before streamOff is sent to HW. Nodes may use
    ///         this hook to do preparation with respect to the bitmask passed in.
    ///
    /// @return CamxResultSuccess if successful
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult PrepareStreamOff();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// CreatePerFrameUpdateResources
    ///
    /// @brief  Create packet and command buffer manager for per frame updates
    ///
    /// @return CamxResultSuccess, if SUCCESS
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult CreatePerFrameUpdateResources();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// UpdateStartupExposureSettings
    ///
    /// @brief  Update the initial aec settings for startup
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID UpdateStartupExposureSettings();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// PrepareSensorUpdate
    ///
    /// @brief  Prepare sensor update parameters from AEC published information
    ///
    /// @param  pSensorParam              Parameters to be updated in sensor
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID PrepareSensorUpdate(
        SensorParam * pSensorParam);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetSensorDataObject
    ///
    /// @brief  Retrieve image sensor data object
    ///
    /// @return pointer to image sensor object
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CAMX_INLINE ImageSensorData* GetSensorDataObject()
    {
        return m_pSensorModuleData->GetSensorDataObject();
    }

    SensorSubDevicesCache*      m_pSensorSubDevicesCache;               ///< pointer to singleton instance of sensor cache
    CSLDeviceHandle             m_hSensorDevice;                        ///< Sensor image device handle
    UINT32                      m_currentResolutionIndex;               ///< Current sensor resolution index
    CmdBufferManager**          m_ppCmdBufferManagers;                  ///< Array of cmd managers,allocated by subclassed HW
    UINT32                      m_cameraId;                             ///< CameraID corresponding to this sensor node
    HwContext*                  m_pHwContext;                           ///< Pointer to HW context
    CSIPHYSubmodule*            m_pCSIPHY;                              ///< Pointer to CSIPHY
    ImageSensorModuleData*      m_pSensorModuleData;                    ///< Pointer to ImageSensorModuleData for this camera
    UINT                        m_deviceIndexCount;                     ///< The number of devices acquired by this node
    INT32                       m_deviceIndices[CamxMaxDeviceIndex];    ///< Array of device indices
    AllOutputPortsData          m_outputPortsData;                      ///< Node output port data - Do not remove from private
    UINT                        m_maxNumCmdBufferManagers;              ///< Max require number of command buffer managers
    UINT                        m_numCmdBufferManagers;                 ///< Number of cmd managers in m_pCmdManagers
    Packet*                     m_pConfigPacket;                        ///< Config packet
    CmdBufferManager*           m_pConfigPacketManager;                 ///< Packet buffer manager
    CmdBufferManager*           m_pConfigCmdBufferManager;              ///< Cmd buffer manager
    CmdBuffer*                  m_pConfigCmdBuffer;                     ///< Config command buffer
    VOID*                       m_pRegSettings;                         ///< Pointer to register settings
    VOID*                       m_pExposureInfo;                        ///< Pointer to exposure info
    VOID*                       m_pExposureRegAddressInfo;              ///< Pointer to register addresses of exposure settings
    const EEPROMOTPData*        m_pOTPData;                             ///< Pointer to OTP data
    SensorSyncMode              m_currentSyncMode;                      ///< Dual Camera related params
    StaticSettings*             m_pStaticSettings;                      ///< Pointer to the static settings structure
    UINT                        m_cslDeviceCount;                       ///< The number of rt devices acquired by this node
    CSLDeviceHandle             m_hCSLDeviceHandles[CamxMaxDeviceIndex];///< Array of CSL device handle to be linked
    CSLLinkHandle               m_hCSLLinkHandle;                       ///< CSL link handle
    CSLDeviceHandle             m_hDevices[CamxMaxDeviceIndex];         ///< CSLDeviceHandles
    CmdBufferManager*           m_pUpdatePacketManager;                 ///< Update packet buffer manager
    CmdBufferManager*           m_pUpdateCmdBufferManager;              ///< Update cmd buffer manager
    SensorState                 m_sensorState;                          ///< Update cmd buffer manager
    CSLHandle                   m_hCSLSession;                          ///< CSL Session handle

};

CAMX_NAMESPACE_END

#endif // CAMXEXTERNALSENSOR_H