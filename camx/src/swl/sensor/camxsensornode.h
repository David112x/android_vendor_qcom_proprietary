////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2016-2020 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  camxsensornode.h
/// @brief SensorNode class declarations
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef CAMXSENSORNODE_H
#define CAMXSENSORNODE_H

#include "camxcslsensordefs.h"

#include "camxhwdefs.h"
#include "camxmem.h"
#include "camxnode.h"
#include "camximagesensordata.h"
#include "camximagesensormoduledata.h"
#include "camxactuator.h"
#include "camxois.h"
#include "camxflash.h"
#include "camxpropertyblob.h"
#include "chiaecinterface.h"

// Forward declaration of CHI libraries
struct CHIPDLib;

CAMX_NAMESPACE_BEGIN

// Forward declaration
class  CPDLibHandler;
class  CSIPHYSubmodule;
class  HwContext;

/// Default maximum sensor pipeline delay
static const UINT32 MaxSensorPipelineDelay = 3;

/// @brief Enum describing sensor signal states
struct SensorSignalState
{
    Condition*  pWaitCondition; ///< Condition to wait on
    Mutex*      pMutex;         ///< Mutex to signal
};

/// @brief Enum describing sensor configuration status
enum class SensorConfigurationStatus
{
    SensorConfigurationStateUninitialized, ///< Sensor state uninitialized
    SensorInitializationInProgress,        ///< Sensor Initialization In Progress
    SensorInitializationComplete,          ///< Sensor Initialization Successful
    SensorInitializationFailed,            ///< Sensor Initialization Failed
    SensorConfigurationInProgress,         ///< Sensor Configuration In Progress
    SensorConfigurationComplete,           ///< Sensor Configuration Successful
    SensorConfigurationFailed,             ///< Sensor Configuration Failed
    SensorSubModuleCreateInProgress,       ///< Sensor Submodule creation In Progress
    SensorSubModuleCreateComplete,         ///< Sensor Submodule creation Successful
    SensorSubModuleCreateFailed,           ///< Sensor Submodule creation Failed
};

/// @brief Enum describing sensor OIS configuration status
enum class SensorOISConfigurationStatus
{
    SensorOISConfigurationStateUninitialized,   ///< Sensor state uninitialized
    SensorOISInitializationInProgress,          ///< Sensor OIS Initialization In Progress
    SensorOISInitializationComplete,            ///< Sensor OIS Initialization Successful
    SensorOISInitializationFailed               ///< Sensor OIS Initialization Failed
};


/// @brief Enum describing sensor post job commands
enum class SensorPostJobCommand
{
    InitializeSensor,   ///< Command to initialize the sensor
    InitializeOIS,      ///< Command to initialize the OIS
    ConfigureSensor,    ///< Command to configure the sensor
    SubModulesCreate,   ///< Command to create sensor submodules
    ReadRequest,        ///< Command to submit a read Request
    MaximumLimit        ///< Maximum limit of the commands
};

/// @brief Structure describing sensor post job parameters
struct SensorPostJob
{
    VOID*                 pSensor;            ///< Pointer to this sensor node
    SensorPostJobCommand  sensorJobCommand;   ///< Post job command to execute in callback
    VOID*                 pData;              ///< Additional data to be sent
};

/// @brief Details about the read data from device
struct SensorReadRequestFormat
{
    UINT   regAddr;          ///< Register address to read from
    UINT   regAddrType;      ///< Read register address type
    INT32  hCSLDeviceHandle; ///< CSL device handle of the device to be read
    UINT16 numOfBytes;       ///< Number of bytes to read from the device
    UINT64 pReadData;        ///< Address where the read data will be copied to
    UINT32 cameraID;         ///< Physical camera ID
};

/// @brief Structure describing sensor parameters
struct SensorParam
{
    FLOAT                   currentGain;             ///< Current gain value from AEC
    UINT64                  currentExposure;         ///< Current exposure time in ns from AEC
    UINT                    currentLineCount;        ///< Current line count calculated from current exposure time
    UINT32                  currentFrameLengthLines; ///< Current frame length lines applied based on exposure
    FLOAT                   previousGain;            ///< Previous gain value from AEC
    UINT64                  previousExposure;        ///< Previous exposure time in ns from AEC
    UINT                    previousLineCount;       ///< Previous line count calculated from previous exposure time
    UINT32                  previousFrameLengthLines;///< Previous frame length lines applied based on exposure
    AECAlgoAdditionalInfo*  pRegControlData;         ///< Sensor exposure register control data from external module
    FLOAT                   currentShortGain;        ///< Current short gain value from AEC
    UINT64                  currentShortExposure;    ///< Current short exposure time in ns from AEC
    UINT                    currentShortLineCount;   ///< Current short line count calculated from current short exposure time
    FLOAT                   previousShortGain;       ///< Previous short gain value from AEC
    UINT64                  previousShortExposure;   ///< Previous short exposure time in ns from AEC
    UINT                    previousShortLineCount;  ///< Previous short line count calculated from previous short exposure time
    FLOAT                   currentMiddleGain;       ///< Current middle gain value from AEC
    UINT64                  currentMiddleExposure;   ///< Current middle exposure time in ns from AEC
    UINT                    currentMiddleLineCount;  ///< Current middle line count calculated from current middle exposure time
    FLOAT                   previousMiddleGain;      ///< Previous middle gain value from AEC
    UINT64                  previousMiddleExposure;  ///< Previous middle exposure time in ns from AEC
    UINT                    previousMiddleLineCount; ///< Previous middle line count calculated from previous middle exposure
                                                     ///< time
    FLOAT                   currentAWBRGain;         ///< Current AWB R gain from AWB
    FLOAT                   currentAWBGGain;         ///< Current AWB G gain from AWB
    FLOAT                   currentAWBBGain;         ///< Current AWB B gain from AWB
    FLOAT                   previousAWBRGain;        ///< previous AWB R gain from AWB
    FLOAT                   previousAWBGGain;        ///< previous AWB G gain from AWB
    FLOAT                   previousAWBBGain;        ///< previous AWB B gain from AWB
    BOOL                    isFSModeCapture;         ///< Fast Shutter Capture Enabled
};

/// @brief structure describing parameters for a CSL device
struct SubDeviceProperty
{
    BOOL              isAcquired;         ///< bool to indicate if subdevice was cached
    INT32             deviceIndex;        ///< device index
    CSLDeviceHandle   hDevice;            ///< CSL device handle for the module
    UINT32            dataMask;           ///< Information specific to sub device data
};

enum SubDevice
{
    SensorHandle = 0,               ///< represents sensor module
    CSIPHYHandle,                   ///< represents CSIPHY module
    OISHandle,                      ///< represents OIS module
    ActuatorHandle,                 ///< represents Actuator module
    FlashHandle,                    ///< represents Flash module
    MaxSubDevices                   ///< Maximum subdevices supported
};

/// @brief structure defining possible CSL devices for sensor node
struct SensorSubDeviceHandles
{
    CSLHandle             hCSLSession[MaxRTSessionHandles];     ///< CSL Session handle
    UINT32                refCount;                             ///< reference count for each sensor

    SubDeviceProperty     subDevices[SubDevice::MaxSubDevices]; ///< Device property corresponding to the sub device
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief Class that implements the sensor node and subdevices cache class
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class SensorSubDevicesCache final
{
public:

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetInstance
    ///
    /// @brief  Creates a singleton instance of sensor node resource manager
    ///
    /// @return pointer to SensorSubDevicesCache
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static CAMX_INLINE SensorSubDevicesCache* GetInstance()
    {
        static SensorSubDevicesCache s_subDevicesCache;
        return &s_subDevicesCache;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetSubDevice
    ///
    /// @brief  Get the sensor device handle
    ///
    /// @param  cameraId  cameraId associated with the sensor handle
    /// @param  type      type of the subdevice handle
    ///
    /// @return SubDeviceProperty for requested device
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    SubDeviceProperty  GetSubDevice(
        UINT32 cameraId,
        SubDevice type) const;

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// SetSubDeviceIndex
    ///
    /// @brief  Get the sensor device handle
    ///
    /// @param  cameraId     cameraId associated with the sensor handle
    /// @param  deviceIndex  deviceIndex to be set
    /// @param  type         type of the subdevice handle
    ///
    /// @return CamxResult success if device index was set properly
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult SetSubDeviceIndex(
        UINT32    cameraId,
        INT32     deviceIndex,
        SubDevice type);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// SetSubDeviceData
    ///
    /// @brief  set the sensor sub device data
    ///
    /// @param  cameraId    cameraId associated with the sensor handle
    /// @param  data        sub device data to be set
    /// @param  type        type of the subdevice handle
    ///
    /// @return CamxResult success if device type was set properly
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult SetSubDeviceData(
        UINT32    cameraId,
        UINT32    data,
        SubDevice type);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// SetSubDeviceHandle
    ///
    /// @brief  Set the sensor device handle
    ///
    /// @param  hCSLSession  cSL Session handle
    /// @param  cameraId     cameraId associated with the sensor handle
    /// @param  handle       CSL handle to be set
    /// @param  type         type of the subdevice handle
    ///
    /// @return CamxResult success if device handle was set properly
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult SetSubDeviceHandle(
        CSLHandle       hCSLSession,
        UINT32          cameraId,
        CSLDeviceHandle handle,
        SubDevice       type);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// MustRelease
    ///
    /// @brief  Check if the subDevice must be released.
    ///
    /// @param  cameraId     cameraId associated with the sensor handle
    /// @param  type         Subdevice to release
    ///
    /// @return BOOL true if the device can be released
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    BOOL MustRelease(
        UINT32    cameraId,
        SubDevice type);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetSensorSubDeviceHandles
    ///
    /// @brief  Get Sensor Sub Device Handles
    ///
    /// @param  cameraID Physical camera ID
    ///
    /// @return pointer to SensorSubDeviceHandles
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CAMX_INLINE VOID* GetSensorSubDeviceHandles(
        UINT32 cameraID)
    {
        return static_cast<VOID*>(&m_sensorSubDevices[cameraID]);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ReleaseOneSubDevice
    ///
    /// @brief  Close all subdevices, release CSL handle and clear the cache for this cameraID
    ///
    /// @param  cameraID Physical camera ID
    /// @param  type     Subdevice to release
    ///
    /// @return CamxResultSuccess on success
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult ReleaseOneSubDevice(
        UINT32    cameraID,
        SubDevice type);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ReleaseAllSubDevices
    ///
    /// @brief  Close all subdevices, release CSL handle and clear the cache for this cameraID. Note that this API is intended
    ///         to be used by HAL/CHIOverride at the end of a HAL device session.
    ///
    /// @param  cameraID Physical camera ID
    ///
    /// @return CamxResultSuccess on success
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult ReleaseAllSubDevices(
        UINT32 cameraID);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// SetHWContextPointer
    ///
    /// @brief  Sets HW context pointer
    ///
    /// @param  pHwContext HW Context pointer
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CAMX_INLINE VOID SetHWContextPointer(
        HwContext* pHwContext)
    {
        m_pHwContext = pHwContext;
    }

private:
    SensorSubDeviceHandles            m_sensorSubDevices[MaxNumImageSensors]; ///< Sensor image device handle
    Mutex*                            m_pCacheLock;                           ///< mutex lock for the class
    BOOL                              m_bReleasehandle[MaxNumImageSensors];   ///< bool to indicate release of handles
    HwContext*                        m_pHwContext;                           ///< Pointer to the HW context

    SensorSubDevicesCache(const SensorSubDevicesCache&) = delete;             ///< Disallow the copy constructor.
    SensorSubDevicesCache& operator=(const SensorSubDevicesCache&) = delete;  ///< Disallow assignment operator.

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// SensorSubDevicesCache
    ///
    /// @brief  Constructor
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    SensorSubDevicesCache();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ~SensorSubDevicesCache
    ///
    /// @brief  Destructor
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    ~SensorSubDevicesCache();
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief Class that implements the sensor node class
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class SensorNode final : public Node
{
public:
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// Create
    ///
    /// @brief  Static method to create HWLSensorNode Object.
    ///
    /// @param  pCreateInputData  Node create input data
    /// @param  pCreateOutputData Node create output data
    ///
    /// @return Pointer to the concrete HWLSensorNode object
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static SensorNode* Create(
        const NodeCreateInputData* pCreateInputData,
        NodeCreateOutputData*      pCreateOutputData);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// Destroy
    ///
    /// @brief  This method destroys the derived instance of the interface
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual VOID Destroy();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// PublishPerFrameSensorMetaData
    ///
    /// @brief  Calculates the sensor metadata which might changes per-frame and publishes it to metadata pool
    ///
    /// @param  requestId   Current request ID being processed
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID PublishPerFrameSensorMetaData(
        UINT64 requestId);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// PublishISO100GainInformation
    ///
    /// @brief  Publish sensor ISO 100 Gain
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID PublishISO100GainInformation();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// PublishCameraModuleInformation
    ///
    /// @brief  Publish Camera Module Information
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID PublishCameraModuleInformation();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// PopulatePDAFInformation
    ///
    /// @brief  Obtain and populate sensor PDAF related data
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID PopulatePDAFInformation();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// PublishPDAFInformation
    ///
    /// @brief  Publish sensor PDAF related data
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID PublishPDAFInformation();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// IsStatsNodeEnabled
    ///
    /// @brief  Check if stats node is enabled or not in the pipeline
    ///
    /// @return Flag that indicates whether stats node is enabled or not
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    BOOL IsStatsNodeEnabled();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// PublishSensorProperties
    ///
    /// @brief  Publish Sensor Properties
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID PublishSensorProperties();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ReadSensorDeviceData
    ///
    /// @brief  Reads device data from specified address and publishes it to the usecase/property pool
    ///
    /// @param  cameraID     Camera's physical ID to read from
    /// @param  pDeviceName  Device Name - OIS, Sensor, Actuator
    /// @param  readDataAddr Address of the memory where the read data needs to copied to
    /// @param  readAddr     Register address to read from
    /// @param  readAddrType Register address type
    /// @param  numOfBytes   Number of bytes to read from this address
    ///
    /// @return CamxResultSuccess if successful
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult ReadSensorDeviceData(
        UINT32 cameraID,
        CHAR*  pDeviceName,
        UINT64 readDataAddr,
        UINT   readAddr,
        UINT   readAddrType,
        UINT16 numOfBytes);

protected:
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ProcessingNodeInitialize
    ///
    /// @brief  Initialize the hwl object
    ///
    /// @param  pCreateInputData  Node create input data
    /// @param  pCreateOutputData Node create output data
    ///
    /// @return CamxResultSuccess if successful
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual CamxResult ProcessingNodeInitialize(
        const NodeCreateInputData* pCreateInputData,
        NodeCreateOutputData*      pCreateOutputData);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// PrepareStreamOn
    ///
    /// @brief  virtual method to that will be called before streamOn command is sent to HW. HW nodes may use
    ///         this hook to do any preparation, or per-configure_stream one-time configuration.
    ///         This is generally called in FinalizePipeline, i.e within a lifetime of pipeline, this is called only once.
    ///         Actual StreamOn may happen much later based on Activate Pipeline. Nodes can use this to do any one time
    ///         setup that is needed before stream.
    ///
    /// @return CamxResultSuccess if successful
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual CamxResult PrepareStreamOn();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ExecuteProcessRequest
    ///
    /// @brief  Pure virtual method to trigger process request for the hwl node object.
    ///
    /// @param  pExecuteProcessRequestData Process request data
    ///
    /// @return CamxResultSuccess if successful and 0 dependencies, dependency information otherwise
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual CamxResult ExecuteProcessRequest(
        ExecuteProcessRequestData* pExecuteProcessRequestData);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// NotifyCSLMessage
    ///
    /// @brief  Pure virtual method to trigger process request for the hwl node object.
    ///
    /// @param  pCSLMessage pointer to the CSL message
    ///
    /// @return CamxResultSuccess if successful
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual CamxResult NotifyCSLMessage(
        CSLMessage* pCSLMessage);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ProcessingNodeFinalizeInitialization
    ///
    /// @brief  Method to finalize the created pipeline
    ///
    /// @param  pFinalizeInitializationData Finalize data
    ///
    /// @return CamxResultSuccess if successful
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual CamxResult ProcessingNodeFinalizeInitialization(
        FinalizeInitializationData* pFinalizeInitializationData);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// PostPipelineCreate
    ///
    /// @brief  virtual method to be called at NotifyTopologyCreated time; node should take care of updates and initialize
    ///         blocks that has dependency on other nodes in the topology at this time.
    ///
    /// @return CamxResultSuccess if successful
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual CamxResult PostPipelineCreate();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// UpdateStartupExposureSettings
    ///
    /// @brief  Update the initial aec settings for startup
    ///
    /// @param  statsNodeEnabled  Indicates whether stats node is enabled
    ///
    /// @return Success if applied successfully, Failure otherwise
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult UpdateStartupExposureSettings(
        BOOL statsNodeEnabled);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ProcessingNodeFinalizeInputRequirement
    ///
    /// @brief  Virtual method implemented by Sensor node to determine the final mode in which it will run. The selected mode
    ///         will be saved (and published later in FinalizeBufferProperties)
    ///
    /// @param  pBufferNegotiationData  Negotiation data for all output ports of a node
    ///
    /// @return Success if the negotiation was successful, Failure otherwise
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult ProcessingNodeFinalizeInputRequirement(
        BufferNegotiationData* pBufferNegotiationData)
    {
        m_pBufferNegotiationData = pBufferNegotiationData;

        return CamxResultSuccess;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// FinalizeBufferProperties
    ///
    /// @brief  Publish sensor mode to the usecase pool. Mode is selected in FinalizeInputRequirement()
    ///
    /// @param  pBufferNegotiationData Buffer negotiation data
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual VOID FinalizeBufferProperties(
        BufferNegotiationData* pBufferNegotiationData);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// QueryMetadataPublishList
    ///
    /// @brief  Method to query the publish list from the node
    ///
    /// @param  pPublistTagList List of tags published by the node
    ///
    /// @return CamxResultSuccess if successful
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual CamxResult QueryMetadataPublishList(
        NodeMetadataList* pPublistTagList);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// OnStreamOn
    ///
    /// @brief  virtual method to that will be called after streamOn command is sent to HW. HW nodes may use
    ///         this hook to do any stream on configuration. This is generally called everytime ActivatePipeline is called.
    ///         Nodes may use this to setup things that are required while streaming. For exa, any resources that are needed
    ///         only during streaming can be allocated here. Make sure to do light weight operations here as this might delay
    ///         processing of the first request.
    ///
    /// @return CamxResultSuccess if successful
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual CamxResult OnStreamOn();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// OnStreamOff
    ///
    /// @brief  virtual method to that will be called before streamOff command is sent to HW. HW nodes may use
    ///         this hook to do any preparation. This is generally called on every Deactivate Pipeline.
    ///         Nodes may use this to release things that are not required at the end of streaming. For exa, any resources
    ///         that are not needed after stream-on can be released here. Make sure to do light weight operations here as
    ///         releasing here may result in re-allocating resources in OnStreamOn.
    ///
    /// @param  modeBitmask Stream off mode bitmask
    ///
    /// @return CamxResultSuccess if successful
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual CamxResult OnStreamOff(
        CHIDEACTIVATEPIPELINEMODE modeBitmask);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// SensorNode
    ///
    /// @brief  Constructor
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    SensorNode();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ~SensorNode
    ///
    /// @brief  Destructor
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual ~SensorNode();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// UpdateDebugExposureSettings
    ///
    /// @brief  Update the sensor exposure debug settings based on debug settings set and override the AEC exposure settings
    ///
    /// @param  applyShortExposure   Indicates whether to apply short exposure
    /// @param  applyMiddleExposure  Indicates whether to apply middle exposure
    /// @param  pExposureParameters  Sensor exposure parameters to be updated
    /// @param  requestID            current request ID
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID UpdateDebugExposureSettings(
        BOOL              applyShortExposure,
        BOOL              applyMiddleExposure,
        SensorParam*      pExposureParameters,
        UINT64            requestID);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// UpdateSensorExposure
    ///
    /// @brief  Update the sensor exposure parameters from proper Exposure Index
    ///
    /// @param  applyShortExposure   Indicates whether to apply short exposure, IsZZHDR?
    /// @param  applyMiddleExposure  Indicates whether to apply middle exposure, Is3HDR?
    /// @param  pExposureParameters  Sensor exposure parameters to be updated
    /// @param  pAECOutput           Sensor AEC output coming from metadata
    /// @param  requestID            current request ID
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID UpdateSensorExposure(
        BOOL              applyShortExposure,
        BOOL              applyMiddleExposure,
        SensorParam*      pExposureParameters,
        AECFrameControl*  pAECOutput,
        UINT64            requestID);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// AcquireResources
    ///
    /// @brief  Method that is called by topology before streamOn. This generally happens in Activate Pipeline.
    ///
    /// @return CamxResultSuccess if successful
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual CamxResult AcquireResources();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ReleaseResources
    ///
    /// @brief  virtual method to that will be called after streamOff command is sent to HW. HW nodes may use
    ///         this hook to do any post stream off actions. This is generally called everytime De-activatePipeline is called.
    ///         Nodes may use this to release hardware.
    ///
    /// @param  modeBitmask Deactivate pipeline mode bitmask
    ///
    /// @return CamxResultSuccess if successful
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual CamxResult ReleaseResources(
       CHIDEACTIVATEPIPELINEMODE modeBitmask);

private:
    SensorNode(const SensorNode&) = delete;             ///< Disallow the copy constructor.
    SensorNode& operator=(const SensorNode&) = delete;  ///< Disallow assignment operator.

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
    /// CreateOISSubmodule
    ///
    /// @brief  Creates OIS submodule
    ///
    /// @return CamxResultSuccess
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult CreateOISSubmodule();

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
        UINT   regSettingIdx);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// LoadPDlibrary
    ///
    /// @brief  Helper method to load pdaf library
    ///
    /// @return CamxResultSuccess
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult LoadPDlibrary();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// CreatePerFrameUpdateResources
    ///
    /// @brief  Create packet and command buffer manager for per frame updates
    ///
    /// @return CamxResultSuccess, if SUCCESS
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult CreatePerFrameUpdateResources();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// SetDependencies
    ///
    /// @brief  Set dependencies for per frame request
    ///
    /// @param  pNodeProcessRequestData       Node per request data
    /// @param  hasExplicitDependencies       True if need to set stats dependancies
    /// @param  requestIdOffsetFromLastFlush  Request id offset from last flush
    ///
    /// @return CamxResultSuccess, if SUCCESS
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult SetDependencies(
        NodeProcessRequestData* pNodeProcessRequestData,
        BOOL                    hasExplicitDependencies,
        UINT64                  requestIdOffsetFromLastFlush);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// UpdateMultiCameraInfo
    ///
    /// @brief  Update multi-camera infomation
    ///
    /// @param  pExecuteProcessRequestData Process request data
    ///
    /// @return CamxResultSuccess, if SUCCESS
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult UpdateMultiCameraInfo(
        ExecuteProcessRequestData*  pExecuteProcessRequestData);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// InitializeMultiCameraInfo
    ///
    /// @brief  Initizalize multi-camera infomation
    ///
    /// @return CamxResultSuccess, if SUCCESS
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult InitializeMultiCameraInfo();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// AdjustSlaveCameraExposureInfo
    ///
    /// @brief  Adjust slave camera linecount and expsource time for multicamera usecase
    ///
    /// @param  pSensorParam              Parameters to be updated in sensor
    /// @param  pNodeProcessRequestData   Node process request data for per request
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID AdjustSlaveCameraExposureInfo(
        SensorParam* pSensorParam,
        NodeProcessRequestData* pNodeProcessRequestData);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// IsMasterCamera
    ///
    /// @brief  Check if current camera is master camera
    ///
    /// @return True if the camera is master camera, otherwise, it is slave camera
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    BOOL IsMasterCamera();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// PrepareSensorUpdate
    ///
    /// @brief  Prepare sensor update parameters from AEC published information
    ///
    /// @param  pSensorParam              Parameters to be updated in sensor
    /// @param  pNodeProcessRequestData   Node process request data for per request
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID PrepareSensorUpdate(
        SensorParam* pSensorParam,
        NodeProcessRequestData* pNodeProcessRequestData);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// HandleDelayInfo
    ///
    /// @brief  Handle the pipeline delay info
    ///
    /// @param  pSensorParam              Parameters to be updated in sensor
    /// @param  pNodeProcessRequestData   Node process request data for per request
    ///
    /// @return TRUE, if the delay info is processed.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    BOOL HandleDelayInfo(
        SensorParam* pSensorParam,
        NodeProcessRequestData* pNodeProcessRequestData);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// PrepareSensorPDAFUpdate
    ///
    /// @brief  Prepare sensor update parameters from PDAF published information
    ///
    /// @param  pPDAFWindowConfig  Parameters to be updated in sensor
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID PrepareSensorPDAFUpdate(
        PDLibWindowConfig* pPDAFWindowConfig);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// PrepareSensorWBGain
    ///
    /// @brief  Prepare sensor update parameters from AWB published information
    ///
    /// @param  pWGGainConfig  Sensor AWB gain parameters to be updated
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID PrepareSensorWBGain(
        AWBGainParams* pWGGainConfig);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetPDAFT3CAMIFPattern
    ///
    /// @brief  Get CAMIF Data Pattern for Type 3 PDAF from IFE
    ///
    /// @param  pCAMIFT3DataPattern  CAMIF Extracted decimated pattern to be passed to PD Lib during Initialization
    /// @param  pPDAFData            PDAF Module Data
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult GetPDAFT3CAMIFPattern(
        PDLibDataBufferInfo* pCAMIFT3DataPattern,
        PDAFData* pPDAFData);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetRDIBufferFormat
    ///
    /// @brief  Helper method to return the RDI format for LCR
    ///
    /// @param  pBufferFormat RDI buffer format for LCR feature
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID GetRDIBufferFormat(
        PDLibBufferFormat* pBufferFormat);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetOTPData
    ///
    /// @brief  Get the Sensor OTP data
    ///
    /// @return EEPROM calibration data pointer
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    const EEPROMOTPData* GetOTPData();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// PublishLensInformation
    ///
    /// @brief  Method to publish lens information to use-case pool
    ///
    /// @return CamxResultSuccess if successful or Failure code.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult PublishLensInformation();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// UpdateLensInformation
    ///
    /// @brief  Method to update the lens information.
    ///
    /// @param  pLensInfo pointer to the lens information to be filled in
    ///
    /// @return CamxResultSuccess if successful or Failure code.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult UpdateLensInformation(
        LensInfo* pLensInfo);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ApplySensorUpdate
    ///
    /// @brief  Create and submit sensor update packet from sensor parameters
    ///
    /// @param  requestId Requested ID
    ///
    /// @return CamxResultSuccess, if SUCCESS
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult ApplySensorUpdate(
        UINT64 requestId);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// CreateFlash
    ///
    /// @brief  Create flash object
    ///
    /// @return CamxResultSuccess, if SUCCESS
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult CreateFlash();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetCurrentFPS
    ///
    /// @brief  Helper method to calculate current FPS sensor is streaming
    ///
    /// @return current FPS
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    DOUBLE GetCurrentFPS();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetFpsDiv
    ///
    /// @brief  Helper method to calculate FPS divider
    ///
    /// @param  isFSCapture is current request fast shutter capture request
    ///
    /// @return FPS divider
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    DOUBLE GetFpsDiv(
              BOOL isFSCapture);

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
        StatsCameraInfo* pCameraInfo);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// AcquireDevice
    ///
    /// @brief  Helper method to acquire sensor device
    ///
    /// @return CamxResultSuccess
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult AcquireDevice();

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

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// IsPDAFEnabled
    ///
    /// @brief  Is PDAF Enabled in current sensor mode.
    ///
    /// @return TRUE if enabled
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    BOOL IsPDAFEnabled();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// CreateSubModules
    ///
    /// @brief  Creates sub modules
    ///
    /// @return CamxResultSuccess is creation of submodules successfull
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult CreateSubModules();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// CreateSensorStateSignals
    ///
    /// @brief  Creates sensor state signals
    ///
    /// @return CamxResultSuccess is creation of state signals is successfull
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult CreateSensorStateSignals();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// CreateActuatorResources
    ///
    /// @brief  Create command manager and packet manager for resources for Actuator.
    ///
    /// @return CamxResultSuccess
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult CreateActuatorResources();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// CreateFlashResources
    ///
    /// @brief  Create command manager and packet manager for flash for camera usecase so command buffer can be recycle.
    ///         Need similar function to be done in HAL for widget flash.
    ///
    /// @return CamxResultSuccess
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult CreateFlashResources();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// CreateOisResources
    ///
    /// @brief  Create command manager and packet manager for Ois for camera usecase so command buffer can be recycle.
    ///
    /// @return CamxResultSuccess
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult CreateOisResources();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// SensorThreadJobCallback
    ///
    /// @brief  This function is invoked by the worker thread in ThreadManager
    ///
    /// @param  pData Payload to offloaded thread
    ///               contains sensor node context, offloaded command and address of job information to be deleted
    ///
    /// @return CamxResultSuccess
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static VOID* SensorThreadJobCallback(
       VOID* pData);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// LoadSensorInitConfigCmd
    ///
    /// @brief   Load sensor init config cmd
    ///
    /// @return  CamxResultSuccess
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult LoadSensorInitConfigCmd();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// IsSensorPipelineStreamedOn
    ///
    /// @brief  Utility to check if the pipeline has been streamed on
    ///
    /// @return TRUE/FALSE based on stream on state
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CAMX_INLINE BOOL IsSensorPipelineStreamedOn()
    {
        return IsPipelineStreamedOn();
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetSensorModuleIndexes
    ///
    /// @brief  Get sensor module and submodule device indexes
    ///
    /// @param  pCSIPHYCapability pointer to capability of CSIPHY module
    /// @param  pFlashCapability  pointer to capability of Flash module
    /// @param  pActuatorCap      pointer to capability of Actuator module
    /// @param  pOisCap           pointer to capability of OIS module
    ///
    /// @return CamxResultSuccess
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult GetSensorModuleIndexes(
        CSLCSIPHYCapability*     pCSIPHYCapability,
        CSLFlashQueryCapability* pFlashCapability,
        CSLActuatorCapability*   pActuatorCap,
        CSLOISCapability*        pOisCap);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// UpdateSensitivityCorrectionFactor
    ///
    /// @brief  Update SensitivityCorrectionFactor
    ///
    /// @param  pTuningModeData  pointer to tuning mode data
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID UpdateSensitivityCorrectionFactor(
        ChiTuningModeParameter* pTuningModeData);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// EVCompensationISPGainAdjust
    ///
    /// @brief  Adjust ISP digital gain when run EV plus in 3HDR mode
    ///
    /// @param  pExposureInfo    pointer to exposure info
    /// @param  pTuningModeData  pointer to tuning mode data
    ///
    /// @return CamxResultSuccess
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult EVCompensationISPGainAdjust(
        SensorExposureInfo* pExposureInfo,
        ChiTuningModeParameter* pTuningModeData);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ProcessReadRequest
    ///
    /// @brief  Process read request for sensor submodules
    ///
    /// @param  pData   Pointer to the read request to be processed
    ///
    /// @return CamxResultSuccess
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult ProcessReadRequest(
        VOID* pData);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// IsFSModeEnabled
    ///
    /// @brief  Read vendor tag enable/disable FS mode
    ///
    /// @param  pIsFSModeEnabled Pointer to fill with FS mode enable value
    ///
    /// @return CamxResultSuccess if successful
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult IsFSModeEnabled(
        BOOL* pIsFSModeEnabled);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// IsFSCaptureRequest
    ///
    /// @brief  Checks if current EPR has capture request
    ///
    /// @return True if capture stream found
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    BOOL IsFSCaptureRequest();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetSensorSeamlessType
    ///
    /// @brief  Utility to get current sensor seamless type
    ///
    /// @param  inputInSensorControlState  input seamless in-sensor control state
    ///
    /// @return TRUE/FALSE based on stream on state
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CAMX_INLINE SensorSeamlessType GetSensorSeamlessType(
        SeamlessInSensorState inputInSensorControlState)
    {
        SensorSeamlessType currentSeamlessType = SensorSeamlessType::None;

        if ((SeamlessInSensorState::InSensorHDR3ExpStart   == inputInSensorControlState) ||
            (SeamlessInSensorState::InSensorHDR3ExpEnabled == inputInSensorControlState))
        {
            currentSeamlessType = SensorSeamlessType::SeamlessInSensorHDR3Exp;
        }

        return currentSeamlessType;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// IsApplyShortExposureNeeded
    ///
    /// @brief  Utility to check if we need apply short exposure or not for current seamless type
    ///
    /// @param  inputSeamlessType  Input seamless type for current sensor mode
    ///
    /// @return TRUE/FALSE based on input seamless type
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CAMX_INLINE BOOL IsApplyShortExposureNeeded(
        SensorSeamlessType inputSeamlessType)
    {
        return (SensorSeamlessType::SeamlessInSensorHDR3Exp == inputSeamlessType)? TRUE: FALSE;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// IsApplyMiddleExposureNeeded
    ///
    /// @brief  Utility to check if we need apply middle exposure or not for current seamless type
    ///
    /// @param  inputSeamlessType  Input seamless type for current sensor mode
    ///
    /// @return TRUE/FALSE based on input seamless type
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CAMX_INLINE BOOL IsApplyMiddleExposureNeeded(
        SensorSeamlessType inputSeamlessType)
    {
        return (SensorSeamlessType::SeamlessInSensorHDR3Exp == inputSeamlessType)? TRUE: FALSE;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetMaxCmdBufSize
    ///
    /// @brief  Helper function to get the maximum size from all I2C commands
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID GetMaxCmdBufSize();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// CreateCmdBuffers
    ///
    /// @brief  Helper method to initialize command manager and to allocate command buffers
    ///
    /// @return CamxResult
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult CreateCmdBuffers();

    SensorSubDevicesCache*       m_pSensorSubDevicesCache;           ///< pointer to singleton instance of sensor cache
    CmdBufferManager*            m_pConfigPacketManager;             ///< Packet buffer manager
    CmdBufferManager*            m_pConfigCmdBufferManager;          ///< Cmd buffer manager
    CmdBuffer*                   m_pConfigCmdBuffer;                 ///< Config command buffer
    Packet*                      m_pConfigPacket;                    ///< Config packet

    CmdBufferManager*            m_pUpdatePacketManager;             ///< Update packet buffer manager
    CmdBufferManager*            m_pUpdateCmdBufferManager;          ///< Update cmd buffer manager
    CmdBuffer*                   m_pUpdateCmds;                      ///< Update cmd buffer
    Packet*                      m_pUpdatePacket;                    ///< Update packet
    CmdBufferManager*            m_pActuatorPacketManager;           ///< Actuator packet buffer manager
    CmdBufferManager*            m_pActuatorInitializeCmdManager;    ///< Actuator init cmd buffer manager
    CmdBufferManager*            m_pActuatorI2CInfoCmdManager;       ///< Actuator I2CInfo cmd buffer manager
    CmdBufferManager*            m_pActuatorPowerCmdManager;         ///< Ois I2CInfo cmd buffer manager
    CmdBufferManager*            m_pActuatorMoveFocusCmdManager;     ///< Actuator move focus cmd buffer manager
    CmdBufferManager*            m_pOISInitializePacketManager;      ///< Ois init packet buffer manager
    CmdBufferManager*            m_pOISInitializeCmdManager;         ///< Ois init cmd buffer manager
    CmdBufferManager*            m_pOISModePacketManager;            ///< Ois Mode packet buffer manager
    CmdBufferManager*            m_pOISModeCmdManager;               ///< Ois Mode cmd buffer manager
    CmdBufferManager*            m_pOISI2CInfoCmdManager;            ///< Ois I2CInfo cmd buffer manager
    CmdBufferManager*            m_pOISPowerCmdManager;              ///< Ois I2CInfo cmd buffer manager
    CSLDeviceHandle              m_hSensorDevice;                    ///< Sensor image device handle
    UINT32                       m_cameraId;                         ///< CameraID corresponding to this sensor node
    HwContext*                   m_pHwContext;                       ///< Pointer to HW context
    SensorParam                  m_sensorParam;                      ///< Sensor hardware parameters for current request
    SensorParam                  m_appliedSensorParam;               ///< The applied hardware parameters for current request
    SensorParam*                 m_pSensorParamQueue;                ///< Pointer to sensor hardware parameters queue
    SensorFillPDAFData           m_sensorPdafData;                   ///< Pdaf sensor data
    SensorFillPDAFData           m_prevSensorPdafData;               ///< Pdaf sensor data for previous frame
    SensorFillWBGainData         m_sensorWBGainData;                 ///< Awb Gain sensor data
    SensorFillWBGainData         m_prevSensorWBGainData;             ///< Awb Gain sensor data for previous frame
    UINT32                       m_latestLTCRatioData;               ///< LTC Ratio sensor data for latest preview frame
    VOID*                        m_pExposureInfo;                    ///< Pointer to exposure info
    VOID*                        m_pRegSettings;                     ///< Pointer to register settings
    VOID*                        m_pPDAFSettings;                    ///< Pointer to PDAF register settings
    VOID*                        m_pWBGainSettings;                  ///< Pointer to WB gain register settings
    VOID*                        m_pLTCRatioSettings;                ///< Pointer to LTC ratio register settings
    VOID*                        m_pExposureRegAddressInfo;          ///< Pointer to register addresses of exposure settings
    UINT32                       m_currentResolutionIndex;           ///< Current sensor resolution index
    const ChiSensorModeInfo*     m_pCurrentSensorModeInfo;           ///< Current sensor mode Info
    Actuator*                    m_pActuator;                        ///< Pointer to actuator driver
    OIS*                         m_pOis;                             ///< Pointer to OIS driver
    ImageSensorModuleData*       m_pSensorModuleData;                ///< Pointer to ImageSensorModuleData for this camera
    CSIPHYSubmodule*             m_pCSIPHY;                          ///< Pointer to CSIPHY
    const BufferNegotiationData* m_pBufferNegotiationData;           ///< BufferNegotiationData
    CPDLibHandler*               m_pPDLibHandler;                    ///< Pointer to the instance of PD library handler
    CHIPDLib*                    m_pPDLib;                           ///< Pointer to the instance of PD library
    SensorPDAFInfo               m_sensorPDAFInfo;                   ///< Sensor PDAF related information
    Flash*                       m_pFlash;                           ///< Pointer to flash driver
    CHIPDLIBRARYCALLBACKS*       m_pPDCallback;                      ///< PD Library entry pointer
    BOOL                         m_isPDAFEnabled;                    ///< Is PDAF Enabled
    PDAFType                     m_pPDAFType;                        ///< PDAF Type supported by the sensor
    SensorSyncMode               m_currentSyncMode;                  ///< Dual Camera related params
    UINT32                       m_sensorSyncTag;                    ///< Sensor sync vendor tag
    JobHandle                    m_hJobFamilyHandle;                 ///< Handle for Worker Thread
    ThreadManager*               m_pThreadManager;                   ///< Thread Manager for this session
    SensorConfigurationStatus    m_sensorConfigStatus;               ///< Indicates the state of sensor config
    SensorSignalState            m_signalSensorInit;                 ///< Sensor init done signal
    SensorSignalState            m_signalSensorConfig;               ///< Sensor config done signal
    SensorSignalState            m_signalSensorSubModules;           ///< Sensor submodules creation done signal
    SensorSignalState            m_signalOISInit;                    ///< OIS init done signal
    JobHandle                    m_hJobFamilySubModuleOISHandle;     ///< Handle for Worker Thread
    SensorOISConfigurationStatus m_OISConfigStatus;                  ///< Indicates the state of sensor config
    FLOAT                        m_opt_wb_grgb;                      ///< AWB GRGB data read from eeprom
    const EEPROMOTPData*         m_pOTPData;                         ///< Pointer to OTP data
    BOOL                         m_initialConfigPending;             ///< Flag to track sensor initial config
    UINT                         m_peerPipelineId;                   ///< Peer pipeline id
    BOOL                         m_isFSModeEnabled;                  ///< Flag to indicate if the usecase is Fast Shutter
    BOOL                         m_isMultiCameraUsecase;             ///< Flag to indicate if the usecase is multi-camera
    BOOL                         m_isMasterCamera;                   ///< Flag to indicate if the camera is master camera
    BOOL                         m_isPdafUpdated;                    ///< Flag to indicate if the pdaf settings updated
    BOOL                         m_isVendorAECDataAvailable;         ///< Flag to indicate if Chi node publishes AEC data
    BOOL                         m_isWBGainUpdated;                  ///< Flag to indicate if the WB gain settings updated
    SeamlessInSensorState        m_seamlessInSensorState;            ///< Seamless in-sensor control state
    JobHandle                    m_hJobFamilyReadHandle;             ///< Handle for Worker Thread
    UINT64                       m_initialRequestId;                 ///< Initial request id after stream on
    UINT32                       m_maxSensorPipelineDelay;           ///< Max sensor pipeline delay
    BOOL                         m_reconfigResSetting;               ///< If reconfigure resolution setting is required
    UINT                         m_maxCmdBufSize;                    ///< Max Command Buffer size for this sensor
    SensorCropInfo               m_sensorCrop;                       ///< Sensor specific crop info required for PD Lib init
};

CAMX_NAMESPACE_END

#endif // CAMXSENSORNODE_H
