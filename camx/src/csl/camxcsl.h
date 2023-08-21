////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2016-2020 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  camxcsl.h
/// @brief CamxCSL interface Version 1.0
/// A simple C API is provided for the UMD to interact with the KMD. This layer is called the Camera Service Layer or CSL.
/// The CSL will provide for the following:
/// - Session management
/// - Device operations including enumerating devices, querying their capabilities, and acquiring and releasing HW and
///   their associated resources.
/// - Memory allocation and mapping
/// - Packet submission and management
/// - Stream on/off of camera operation
/// - Kernel to user messaging
/// - Fence objects
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// NOWHINE FILE GR016: Converting CSL header to C, we need typedef
// NOWHINE FILE CF032: This is a C function so void should be used for empty parameter list.

#ifndef CAMXCSL_H
#define CAMXCSL_H

#include "camxtypes.h"

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Forward Declarations
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
struct CSLNativeHandle;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CSL Handle Definitions
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/// @brief A Handle to a CSL Session
typedef INT32 CSLHandle;

/// @brief Handle to a device within the CAMSS that is exposed to the UMD
typedef INT32 CSLDeviceHandle;

/// @brief Type to hold addresses mapped to the camera HW.
typedef UINT32 CSLDeviceAddress;

/// @brief Handle to memory that is managed by the CSL
typedef INT32 CSLMemHandle;

/// @brief Handle to a CSL Fence object used for synchronization.
typedef INT32 CSLFence;

/// @brief Handle to a link of real time devices in a session
typedef INT32 CSLLinkHandle;

/// @brief  The invalid handle constant
static const INT32 CSLInvalidHandle = 0;

/// @brief  The invalid Fence constant
static const INT32 CSLInvalidFence = -1;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CSL Device Definitions
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/// @brief Maximum device index count
static const UINT CSLxMaxDeviceIndex = 16;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CSL type Definitions
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/// @brief  Enumeration of device types that CSL may manage. Enumeration of all possible device types that may be accessed
///         through CSL. The exact set of devices available is platform specific.
typedef enum
{
    CSLDeviceTypeInvalidDevice = 0,  ///< Invalid device type
    CSLDeviceTypeImageSensor,        ///< Image sensor device.
    CSLDeviceTypeLensActuator,       ///< Lens actuator device.
    CSLDeviceTypeCompanion,          ///< Sensor companion device.
    CSLDeviceTypeEEPROM,             ///< EEPROM to store the calibration data of ImageSensor.
    CSLDeviceTypeCSIPHY,             ///< CSIPHY Camera Serial Interface Physical receiver.
    CSLDeviceTypeOIS,                ///< OIS Optical image stabilization
    CSLDeviceTypeFlash,              ///< Flash device.
    CSLDeviceTypeFD,                 ///< Face detection device.
    CSLDeviceTypeJPEGE,              ///< JPEG device.
    CSLDeviceTypeJPEGD,              ///< JPEG decoder device.
    CSLDeviceTypeVFE,                ///< VFE (Video Front End) device.
    CSLDeviceTypeCPP,                ///< CPP (Camera Post Processor) device.
    CSLDeviceTypeCSID,               ///< CSID Camera Serial Interface Decoder
    CSLDeviceTypeISPIF,              ///< ISPIF Image Signal Processor InterFace
    CSLDeviceTypeIFE,                ///< IFE (Image Front End) device.
    CSLDeviceTypeICP,                ///< ICP (Image Control Processor) device.
    CSLDeviceTypeLRME,               ///< LRME (Low Resolution Motion Estimation) device.
    CSLDeviceTypeCustom,             ///< Custom HW
    CSLDeviceTypeMaxDevice           ///< Maxdevice upper bound limit macro
} CSLDeviceType;

/// @brief Enumeration of the supported camera architecture families.
typedef enum
{
    CSLCameraFamilyCameraSS,   ///< Camera Subsystem present in Bear, Badger and HoneyBadger.
    CSLCameraFamilyTitan       ///< Titan camera architecture.
} CSLCameraFamily;

/// @brief Camera Family SOC Information
typedef enum
{
    CSLCameraTitanSocInvalid  = 0,     ///< Invalid SOC Id
    CSLCameraTitanSocSDM845   = 321,   ///< SDM845 SOC Id
    CSLCameraTitanSocSDM670   = 336,   ///< SDM670 SOC Id
    CSLCameraTitanSocSDM855   = 339,   ///< SDM855 SOC Id
    CSLCameraTitanSocSDM855P  = 361,   ///< SDM855P SOC Id
    CSLCameraTitanSocQCS605   = 347,   ///< QCS605 SOC Id
    CSLCameraTitanSocSM6150   = 355,   ///< SM6150 SOC Id
    CSLCameraTitanSocSDM865   = 356,   ///< SDM865 SOC Id
    CSLCameraTitanSocSM7150   = 365,   ///< SM7150 SOC Id
    CSLCameraTitanSocSM7150P  = 366,   ///< SM7150P SOC Id
    CSLCameraTitanSocSDM710   = 360,   ///< SDM710 SOC Id
    CSLCameraTitanSocSXR1120  = 370,   ///< SXR1120 SOC Id
    CSLCameraTitanSocSXR1130  = 371,   ///< SXR1130 SOC Id
    CSLCameraTitanSocSDM712   = 393,   ///< SDM712 SOC Id
    CSLCameraTitanSocSM7250   = 400,   ///< SM7250 SOC Id
    CSLCameraTitanSocSM6250   = 407,   ///< SM6250 SOC Id
    CSLCameraTitanSocQSM7250  = 440,   ///< QSM7250 SOC Id
    CSLCameraTitanSocSM6350   = 434,   ///< SM6350 SOC Id
    CSLCameraTitanSocSM7225   = 459,   ///< SM6350 AB SOC Id
} CSLCameraFamilySoc;

/// @brief Camera Titan family Information
typedef enum
{
    CSLTitan175 = 0x00010705,    ///< Titan version
    CSLTitan170 = 0x00010700,    ///< Titan version
    CSLTitan160 = 0x00010600,    ///< Titan version
    CSLTitan150 = 0x00010500,    ///< Titan version
    CSLTitan480 = 0x00040800,    ///< Spectrum 480
} CSLCameraTitanVersion;

/// @brief ISP Titan HW Chip version
typedef enum
{
    CSLTitanNone    = 0,                  ///< Invalid Titan HW version
    CSLTitan150V100 = 0x150100,           ///<  Titan HW version for SDM640 (SM6150)
    CSLTitan150V110 = 0x150110,           ///< Titan HW version for SM6250 V110
    CSLTitan170V100 = 0x170100,           ///< Titan HW version for SDM845/SDM710/SDM670 V100
    CSLTitan170V110 = 0x170110,           ///< Titan HW version for SDM845/SDM710/SDM670 V110
    CSLTitan170V120 = 0x170120,           ///< Titan HW version for SDM845/SDM710/SDM670 V110
    CSLTitan170V200 = 0x170200,           ///< Titan HW version for SM6350
    CSLTitan175V100 = 0x175100,           ///< Titan HW version for SDM855 (SM8150) V100
    CSLTitan175V101 = 0x175101,           ///< Titan HW version for SDM855 (SM8150) V101
    CSLTitan175V120 = 0x175120,           ///< Titan HW version for SM7150/SM7150P V100
    CSLTitan175V130 = 0x175130,           ///< Titan HW version for SM7250/SM7250P V130
    CSLTitan480V100 = 0x480100,           ///< Titan HW version for SDM865 (SM8250) V100
    CSLTitanMax     = 0xFFFFFF,           ///< Titan MAX
}CSLCameraTitanChipVersion;

/// @brief Structure to define version of HW and drivers
typedef struct
{
    UINT32 majorVersion; ///< Major version number. Major version numbers are updated with major architectural changes.
    UINT32 minorVersion; ///< Minor version numbers. Minor version numbers are updated on any change in interface. Changes
                         ///  are backward compatible.
    UINT32 revVersion;   ///< Revision version number. The revision version number is updated when changes are made that do
                         ///  not impact the interface in any way.
    UINT32 reserved;     ///< Reserved field to force size as a multiple of 64bits
} CSLVersion;

/// @brief Structure to identify the camera platform.
typedef struct
{
    CSLCameraFamily    family;             ///<< The camera architecture family.
    CSLVersion         platformVersion;    ///<< The version of the camera platform.
    CSLVersion         CPASVersion;        ///<< The version of the camera cpas.
    CSLCameraFamilySoc socId;              ///<< SOC ID of a specific camera architecture family.
    UINT32             CPASHardwareCaps;   ///<< CPAS hardware capabilities
} CSLCameraPlatform;

/// @brief Structure to define basic identifying properties of a device.
typedef struct
{
    INT32           deviceIndex;    ///< The index for devices can be discovered with CSLEnumerateDevices. The device
                                    ///  index is guaranteed to remain constant for the lifetime of CSL.
    CSLDeviceType   deviceType;     ///< The device type.
    CSLVersion      hwVersion;      ///< The hardware version of the device. For IP cores in CAMSS the version number is
                                    ///  the version number described in IPCatalog.
    CSLVersion      driverVersion;  ///< The version of the device driver. The version of the device driver is used to
                                    ///  indicate the opcodes supported by each device.
} CSLDeviceDescriptor;

/// @todo (CAMX-360) Determine if additional result parameters are required. Potentially CSID "slot" and mount info?
/// @brief Structure to define the result data from CSLSensorProbe.
typedef struct
{
    BOOL    detected;       ///< TRUE for a detected device, FALSE otherwise.
    INT32   deviceIndex;    ///< The device index assigned to this device if detected.
} CSLImageSensorProbeResult;

/// @brief Device resource request structure.
typedef struct
{
    UINT32  resourceID;                 ///< The resourceID of the device resource to acquire.
    VOID*   pDeviceResourceParam;       ///< Untyped pointer to the resource specific structure, structure can be in/out.
    SIZE_T  deviceResourceParamSize;    ///< The size of the data structure pointed to be pDeviceResourceParam. The size is
                                        ///  used to validate the correct structure is passed.
    VOID*   pReturnParams;              ///< Return resource params
} CSLDeviceResource;

/// @brief Device attribute request structure.
typedef struct
{
    UINT32  attributeID;                ///< The attributeID of the device resource to acquire.
    VOID*   pDeviceAttributeParam;      ///< Untyped pointer to the attribute specific structure, structure can be in/out.
    SIZE_T  deviceAttributeParamSize;   ///< The size of the data structure pointed to be pDeviceAttributeParam. The size is
                                        ///  used to validate the correct structure is passed.
} CSLDeviceAttribute;

/// @brief  CSL message types. CSL provides the ability for the KMD to pass asynchronous messages back to the UMD. This enum
///         describes the different kinds of messages.
typedef enum
{
    CSLMessageTypeFrame,  ///< Frame message. Frame messages have a payload of type CSLFrameMsg
    CSLMessageTypeError   ///< Error message. Error message have a payload of type CSLErrorMsg
} CSLMessageType;

/// @brief  CSL debug buffer types. CSL provides the ability to pass KMD buffers from usermode which it can use for debugging
///         information which is shared with usermode.
typedef enum
{
    CSLDebugBufferTypeSCOPE,    ///< The buffer is filled with SCOPE data
    CSLDebugBufferTypeRegDump   ///< The buffer is filled with register dump
} CSLDebugBufferType;

/// @brief  CSL debug buffer result. Reports the previously set debug buffer (if any) and the number of bytes written to it.
typedef struct
{
    CSLMemHandle    hHandle;        ///< Handle to the previous debug buffer
    SIZE_T          offset;         ///< The offset of the buffer in the handle
    SIZE_T          bytesWritten;   ///< Number of bytes written to the previous debug buffer
} CSLDebugBufferResult;

/// @brief CSL time stamp type returned by kernel
typedef enum
{
    CSLTimestampTypeQtimer,     ///< timestamp type is Qtimer
    CSLTimestampTypeMono        ///< timestamp type is boot time
} CSLTimestampType;

/// @brief  CSL frame message. Frame messages are used to communicate the status of the capture of frames. The KMD should send
///         the frame message as early as possible after the hardware observes the start of the frame streaming, or SOF. UMD
///         must not rely on the timing of the frame message arrival.
typedef struct
{
    UINT64           requestID;     ///< The request ID associated with this frame. If no request ID is associated with this
                                    ///< frame, meaning the frame is dropped, the requestID will be 0.
    UINT64           frameCount;    ///< The frame count of the frame. The frame count is the absolute frame count observed by
                                    ///< the hardware since the last call to CSLStreamOn.
    UINT64           timestamp;     ///< The time stamp of the frame. The time stamp is the time HW observed the start of
                                    ///  streaming for the frame. The time is nanoseconds since boot.
    INT32            link_hdl;      ///< The link handle represent real time link
    CSLTimestampType timestampType; ///< The timestamp type in this message
    BOOL             bNotify;       ///< need send notificaiton on this message
} CSLFrameMessage;

/// @brief CSL error message types. Error messages are used to communicate errors for asynchronous events.
typedef enum
{
    CSLErrorMessageCodeDevice,      ///< Device has generated an error. Device errors are considered fatal to the camera
                                    ///  session.
    CSLErrorMessageCodeRequest,     ///< An error occurred for a single request. Request errors are not fatal. Pending and
                                    ///  future
                                    ///  request should be expected to be successfully processed.
    CSLErrorMessageCodeBuffer,      ///< A buffer was not filled. Other buffers associated with the request may have been
                                    ///  successfully processed. Buffer errors are not fatal.
    CSLErrorMessageCodeRecovery,    ///< Recovery event from KMD
    CSLErrorMessageCodeSOFFreeze,   ///< SOF Freeze event from KMD
    CSLErrorMessageCodeFullRecovery ///< Full Recovery event from KMD. Need to recover whole system including sensor and phy.
} CSLErrorMessageCode;

/// @brief CSL error message payload. Defines the parameters of the CSL error message.
typedef struct
{
    CSLErrorMessageCode errorType;      ///< The error type.
    UINT64              requestID;      ///< The request ID, if any, associated with the error.
    CSLDeviceHandle     hDevice;        ///< The device instance handle, if any, associated with the error.
    SIZE_T              resourceIndex;  ///< The resource index, if any, associated with the error.
    INT32               link_hdl;       ///< The link handle represent real time link
} CSLErrorMessage;

/// @brief CSL message
typedef struct
{
    CSLMessageType type;   ///< The message type. The message type is used to determine the valid member of the union message.
    union
    {
        CSLErrorMessage errorMessage;   ///< Error Message.
        CSLFrameMessage frameMessage;   ///< Frame Message.
    } message;                          ///< Union of available message types.
} CSLMessage;

/// @brief CSL buffer descriptor.
typedef struct
{
    CSLMemHandle        hHandle;        ///< Handle to the buffer managed by CSL.
    VOID*               pVirtualAddr;   ///< Virtual address mapped in the UMD for the buffer. If the buffer is not mapped,
                                        ///  pVa will be set to NULL.
    CSLDeviceAddress    deviceAddr;     ///< The mapped HW device address for the buffer. If the buffer is not mapped to HW,
                                        ///  or the KMD does not expose the device address to the UMD, it will be set to 0.
    INT                 fd;             ///< The file descriptor of the buffer. If no valid file descriptor is available for
                                        ///  the buffer the fd will be set to 0.
    SIZE_T              offset;         ///< The offset of the buffer in the file. This offset is only valid relative to fd.
    SIZE_T              size;           ///< The size in bytes of the buffer.
    UINT32              flags;          ///< Any usage flags set for this buffer.
    /// @todo (CAMX-428) Remove the bufferImported member in CSLBufferInfo.
    UINT32              bufferImported; ///< Set if this buffer is imported and not allocated by CSL.
} CSLBufferInfo;

/// @brief CSL dump req descriptor.
typedef struct
{
    UINT64             issueRequestId;    ///< issue request id.
    UINT64             issueSyncId;       ///< issue sync id.
    UINT64             lastRequestId;     ///< last submitted request id. Reserved for future use.
    UINT64             lastSyncId;        ///< last sync id. Reserved for future use.
    UINT32             hBuf;              ///< buffer handle in which data to be dumped.
    UINT32             errorType;         ///< to be used in future to specify error type triggerring LDAR.
    SIZE_T             offset;            ///< offset of the buffer.
} CSLDumpRequestInfo;

/// @brief Pointer type for native fence creation data (platform-specific structures need to be type-cast to this.)
typedef VOID* CSLNativeFenceCreateDataPtr;

/// @brief CSL fence attribute identifier (platform-specific)
typedef UINT32 CSLFenceAttrib;

/// @brief Define CSL fence signal status codes
typedef enum
{
    CSLFenceResultSuccess = 0,  ///< Fence signaled with success
    CSLFenceResultFailed,       ///< Fence signaled with failure
    CSLFenceResultCanceled      ///< Fence signaled with cancelation
} CSLFenceResult;

/// @brief CSL Modes
typedef enum
{
    CSLHwEnabled,           ///< HW Enabled
    CSLIFHEnabled,          ///< IFH Enabled
    CSLPresilEnabled,       ///< Presil CSIM Enabled
    CSLPresilRUMIEnabled,   ///< Presil RUMI Enabled
} CSLMode;

/// @todo (CAMX-879) Clarify presil only setting mechanism: for now initializations
///                  parameters applies to both device and presil environments,
///                  we should verify if this mechanism will work for presil only
///                  configuration/setup options too.

/// @brief CSL Initialize Parameters
typedef struct
{
    BIT   enableSensorSimulation    : 1;    ///< Sensor simulation enabled or not
    BIT   dumpSensorEmulationOutput : 1;    ///< Dump sensor output or not
    CHAR  sensorEmulatorPath[512];          ///< Emulated sensor application path
    CHAR  sensorEmulator[512];              ///< Emulated sensor application name
} CSLEmulatedSensorParams;

typedef struct
{
    UINT                    mode;                   ///< CSL Mode
    CSLEmulatedSensorParams emulatedSensorParams;   ///< Emulated Sensor Parameter
} CSLInitializeParams;

/// @brief  Helper object to initialize and expose the right CSL implementation.
class  CSLModeManager;
// NOWHINE NC003: g should override name
extern CSLModeManager* g_pCSLModeManager;

/// @brief CSL Buffer Heaps
typedef enum
{
    CSLBufferHeapSystem = 0,        ///< System Heap
    CSLBufferHeapIon    = 1,        ///< Ion Heap
    CSLBufferHeapDSP    = 2,        ///< DSP Heap
    CSLBufferHeapEGL    = 3         ///< EGL Heap
} CSLBufferHeap;

/// @brief CSL mode of operation
typedef enum
{
    CSLRealtimeOperation,    ///< This CSL KMD device is realtime device
    CSLNonRealtimeOperation, ///< This CSL KMD device is Non realtime device
} CSLDeviceOperationMode;

/// @brief CSL sync link mode, i.e sync mode for multi camera use cases
typedef enum
{
    CSLSyncLinkModeNoSync,      ///< No Sync mode for SyncLink
    CSLSyncLinkModeSync,        ///< Sync mode for SyncLink
} CSLSyncLinkMode;



/// @brief CSL stream off Deactivate mode
typedef UINT32 CSLDeactivateMode;
static const CSLDeactivateMode CSLDeativateModeDefault               = (1 << 0); ///< Default mode, deactivate all nodes
                                                                                 ///  in pipeline
static const CSLDeactivateMode CSLDeativateModeSensorStandBy         = (1 << 1); ///< Sensor stand by mode
static const CSLDeactivateMode CSLDeativateModeSensorRealtimeDevices = (1 << 2); ///< Stream off and release IFE only so
                                                                                 ///  it can be reused. eg. Triple camera
static const CSLDeactivateMode CSLDeativateModeReleaseBuffer         = (1 << 3); ///< Release cslalloc buffers of all nodes
                                                                                 ///  in pipeline
static const CSLDeactivateMode CSLDeactivateModeUnlinkPipeline       = (1 << 4); ///< Stream Off and Unlink pipeline

/// @FlushType
typedef enum
{
    CSLFlushAll,               ///< Flush all requests and stop after lastFlushedRequest.
                               ///  Flush all including the actively processing requests in RT
                               ///  devices such as Sensor, IFE, Actuator. Will not wait at
                               ///  frame boundary, but stop CSID immediately.
    CSLFlushRequest            ///< Flush cancel request
} CSLFlushType;

/// @brief The string name of the type of Flush. Must be in order of FlushType codes.
#if __GNUC__
static const CHAR* CamxFlushTypeStrings[] __attribute__((unused)) =
#else // __GNUC__
static const CHAR* CamxFlushTypeStrings[] =
#endif // _GNUC__
{
    "CSLFlushAll",
    "CSLFlushRequest"
};

/// @brief CSL Flush Information
typedef struct
{
    CSLFlushType     flushType;      ///< Type of flush -- Flush all, flush range etc.
    UINT64           firstCSLSyncId; ///< First CSL Sync Id that needs to be cancelled for realtime
    UINT64           firstRequestId; ///< First request Id that needs to be cancelled for offline
    UINT64           lastCSLSyncId;  ///< Last CSL Sync Id that needs to be cancelled for realtime
    UINT64           lastRequestId;  ///< Last request Id that needs to be cancelled for offline
    UINT             numDevices;     ///< Number of devices
    CSLDeviceHandle* phDevices;      ///< Pointer to list of devices
    UINT             numSyncLinks;   ///< Current assumption for numSyncLinks is always 1 based on
                                     ///  existing usecases. In case there is need for flushing more than
                                     ///  1 syncLink in the future, appropriate changes need to be made in
                                     ///  CSLHwCancelRequest
    CSLLinkHandle*   phSyncLink;     ///< sync link handle
} CSLFlushInfo;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CSL Buffer Usage flags
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/// Buffer is accessed by Hardware.
static const UINT32 CSLMemFlagHw           = 0x00000001;

/// Buffer is protected and only accessible by content protection enabled paths.
static const UINT32 CSLMemFlagProtected    = 0x00000002;

/// Buffer is a command buffer.
static const UINT32 CSLMemFlagCmdBuffer    = 0x00000004;

/// Buffer is to be accessed by the UMD and is mapped into the UMD virtual address space.
static const UINT32 CSLMemFlagUMDAccess    = 0x00000008;

/// Buffer is cached.
static const UINT32 CSLMemFlagCache        = 0x00000010;

/// Buffer is a packet buffer.
static const UINT32 CSLMemFlagPacketBuffer = 0x00000020;

// Buffer is going to be accessed only in KMD
static const UINT32 CSLMemFlagKMDAccess    = 0x00000040;

// Buffer is going to be allocated in the shared memory region in HW
static const UINT32 CSLMemFlagSharedAccess = 0x00000080;

// Buffer is not going to mapped to HW or UMD but will manitain the FDs
static const UINT32 CSLMemFlagProtectedUMDAccess  = 0x00000100;

// Buffer is going to mapped to IOD CDSP and we  will manitain the FDs
static const UINT32 CSLMemFlagDSPSecureAccess     = 0x00000400;

// Buffer is going to mapped to SMMU without Delayed Unmap feature
static const UINT32 CSLMemFlagDisableDelayedUnmap = 0x00000800;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CSL Device attributes
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// Configure devices as realtime or non-realtime device, and setup link in command manager
// Without specified operation attribute, CSL will use predefined operation found in CSLHwIsHwRealtimeDevice()
static const UINT32 CSLDeviceAttributeRealtimeOperation    = 0;
static const UINT32 CSLDeviceAttributeNonRealtimeOperation = 1;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CSL Callback method
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief  CSL message callback.
///
/// @param  pUserData Opaque pointer provided by client to CSLRegisterMessageHandler.
/// @param  pMessage The CSL message
///
/// @return None
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
typedef VOID (*CSLMessageHandler)(
    VOID*       pUserData,
    CSLMessage* pMessage);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief  CSLSessionMessageHandler callback.
///
/// @param  pUserData Opaque pointer provided by client to CSLRegisterMessageHandler.
/// @param  pMessage The CSL message
///
/// @return None
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
typedef VOID (*CSLSessionMessageHandler)(
    VOID*       pUserData,
    CSLMessage* pMessage);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief  CSL callback method for asynchronous fence waits.
///
/// @param  pUserData Opaque pointer provided by client to CSLFenceAsyncWait.
/// @param  syncFence The CSL fence that triggered the callback.
/// @param  result    Indicates the success/failure status of the signal
///
/// @return None
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
typedef VOID (*CSLFenceHandler)(
    VOID*           pUserData,
    CSLFence        syncFence,
    CSLFenceResult  result);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CSL Method Declarations
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CSLInitialize
///
/// @brief  Initialize the CSL layer
///
///         This method initializes the CSL layer and is intended to be called once at boot or other such early time.
///
/// @param  pInitializeParams   Initialization parameters
///
/// @return CamxResult indicating success or failure of the call.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CSLInitialize(
    CSLInitializeParams* pInitializeParams);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CSLUninitialize
///
/// @brief  Uninitialize the CSL layer
///
///         This method will uninitialize the CSL layer and cleans up any resources. This method is intended to be called once
///         at device shutdown.
///
/// @return CamxResult indicating success or failure of the call.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CSLUninitialize(void);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CSLOpen
///
/// @brief  Creates a CSL session.
///
///         A session and a HAL Camera device have a one to one relationship. This method provides an opaque handle for the
///         session.  Many CSL operations occur within the context of a session and will be designated in their documentation
///         as well as taking a CSL session handle as an argument.
///
/// @param  phCSL Output parameter. An opaque handle to the CSL session.
///
/// @return CamxResult indicating success or failure of the call.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CSLOpen(
    CSLHandle* phCSL);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CSLClose
///
/// @brief  Close a CSL session.
///
///         Closing a CSL session should attempt to free any acquired devices or resources during the CSL session. However,
///         clients should not rely on automatic clean up and explicitly release and acquired devices or resources.
///
/// @param  hCSL Handle to the CSL session to be closed.
///
/// @return CamxResult indicating success or failure of the call.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CSLClose(
    CSLHandle hCSL);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CSLAddReference
///
/// @brief  Indicate that another client is sharing the same CSL session by addiing a reference to it. Calling
///         CSLOpen() set the count 1. CSLAddReference() will increment the reference count. CSLClose() will decrement and
///         release the session when the count drops to zero.
///
/// @param  hCSL Handle to the CSL session to be closed.
///
/// @return CamxResult indicating success or failure of the call.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CSLAddReference(
   CSLHandle hCSL);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CSLQueryCameraPlatform
///
/// @brief  Query the camera platform information including the architecture and version.
///
/// @param  pCameraPlatform Pointer to CSLCameraPlatform structure to populate with the platform information
///
/// @return CamxResult indicating success or failure of the call.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CSLQueryCameraPlatform(
    CSLCameraPlatform* pCameraPlatform);

/// @todo (CAMX-361) Define CSL probe packet.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CSLImageSensorProbe
///
/// @brief  Submit a image sensor probe packet to discover an image sensor. This method may be called anytime after
///         CSLInitialize. The client may then use other CSL methods to interact with the image sensor device.
///
/// @param  hPacket         A handle to a probe packet memory.
/// @param  offset          Offset within memory described by hPacket where packet definition begins
/// @param  pProbeResult    Result of the image sensor probe.
///
/// @return CamxResult indicating success or failure of the call.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CSLImageSensorProbe(
    CSLMemHandle                hPacket,
    SIZE_T                      offset,
    CSLImageSensorProbeResult*  pProbeResult);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CSLSetDebugBuffer
///
/// @brief  Sets the given buffer as the active debug buffer for that type. All outstanding writes to the previous buffer
///         should be completed before the return of this function so the previous buffer can be immediately freed.
///
/// @param  hCSL                A handle to the CSL session
/// @param  type                The type of debug buffer to set
/// @param  hBuffer             The buffer to set, or CSLInvalidHandle to set to no buffer available
/// @param  offset              Offset within memory described by hBuffer where the buffer begins
/// @param  length              Length of the buffer
/// @param  pDebugBufferResult  Handle to the previously set buffer and number of bytes written to that buffer
///
/// @return CamxResult indicating success or failure of the call.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CSLSetDebugBuffer(
    CSLHandle               hCSL,
    CSLDebugBufferType      type,
    CSLMemHandle            hBuffer,
    SIZE_T                  offset,
    SIZE_T                  length,
    CSLDebugBufferResult*   pDebugBufferResult);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CSLEnumerateDevices
///
/// @brief  Enumerate devices managed by CSL.
///
///         CSLEnumerate takes a CSLDeviceDesciptor pointer as an input/output parameter. The CSLDeviceDescriptor will contain a
///         device index, an enum indicating the device type. A client will fill in the index of the CSLDeviceDescriptor and
///         call the CSL enumerate method. The CSL will then populate the other fields in the structure. The device index is
///         guaranteed to remain constant for any particular device for the life time of CSL.
///
/// @param  pDeviceDescriptor Input/output device descriptor.
///
/// @return CamxResult indicating success or failure of the call.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CSLEnumerateDevices(
    CSLDeviceDescriptor* pDeviceDescriptor);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CSLQueryDeviceCapabilities
///
/// @brief  Query device specific capabilities.
///
///         Device capabilities are specific to each type of device. The caller must pass the appropriately sized buffer to
///         receive the device capabilities.
///
/// @param  deviceIndex    A device index of the device to query.
/// @param  pDeviceData    Untyped pointer to a device specific structure describing the capabilities of that device.
/// @param  deviceDataSize The size of the data structure pointed to by pDevice. The size is used to verify the correct
///                        structure is to be populated.
///
/// @return CamxResult indicating success or failure of the call.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CSLQueryDeviceCapabilities(
    INT32   deviceIndex,
    VOID*   pDeviceData,
    SIZE_T  deviceDataSize);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CSLAcquireDevice
///
/// @brief  Acquires a device and associated resources within the context of a CSL session.
///
///         This method will acquire an opaque handle to a device. The client may then acquire resources specific to this
///         device as well as submit packets to the device. If a device is already in use and cannot be shared, this method
///         will return an error indicating that the device is busy. Multiple sessions will likely acquire unique instances of
///         the same physical device if supported by the HW and kernel implementation. It is expected that only a single
///         instance of a device will be acquired within a session, however in the future there may be cases or classes of
///         devices where multiple instances (of the same physical HW) are acquired within the same session.
///
///         Device resources are specific to each type of device in the camera system. For each device type, a unique range of
///         resource IDs and a device specific structure to describe the device specific resources will be defined.
///
/// @param  hCSL                    A handle to the CSL session.
/// @param  phDevice                Output handle to the acquired device.
/// @param  deviceIndex             Device index of the requested device.
/// @param  pDeviceResourceRequest  Pointer to an array of device resource request.
/// @param  numDeviceResources      The number of elements in the array pointed to by pDeviceResourceRequest.
/// @param  pDeviceAttribute        Pointer to an array of device attribute.
/// @param  numDeviceAttributes     The number of elements in the array pointed to by pDeviceAttribute.
/// @param  pDeviceName             Device name string
///
/// @return CamxResult indicating success or failure of the call.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CSLAcquireDevice(
    CSLHandle           hCSL,
    CSLDeviceHandle*    phDevice,
    INT32               deviceIndex,
    CSLDeviceResource*  pDeviceResourceRequest,
    SIZE_T              numDeviceResources,
    CSLDeviceAttribute* pDeviceAttribute,
    SIZE_T              numDeviceAttributes,
    const CHAR*         pDeviceName);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CSLReleaseDevice
///
/// @brief  Release an acquired device.
///
///         The caller should stream off and release any acquired resources prior to releasing a device. The KMD should attempt
///         to clean up in case the device is not properly shutdown.
///
/// @param  hCSL    A handle to the CSL session.
/// @param  hDevice The device instance to release.
///
/// @return CamxResult indicating success or failure of the call.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CSLReleaseDevice(
    CSLHandle       hCSL,
    CSLDeviceHandle hDevice);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CSLAcquireHardware
///
/// @brief  Acquires the device HW and associated resources within the context of a CSL session.
///
///         This method will acquire the device hardware using the device handle acquired during acquire device.
///         This method can be used to acquire hardware dynamically to switch hardware context.
///
/// @param  hCSL                    A handle to the CSL session.
/// @param  hDevice                 Handle to the acquired device HW.
/// @param  pDeviceResourceRequest  Pointer to an array of device resource request.
/// @param  pDeviceName             Device name string
/// @param  version                 Version number of the Acquired HW structure
///
/// @return CamxResult indicating success or failure of the call.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CSLAcquireHardware(
    CSLHandle           hCSL,
    CSLDeviceHandle     hDevice,
    CSLDeviceResource*  pDeviceResourceRequest,
    const CHAR*         pDeviceName,
    UINT32              version);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CSLReleaseHardware
///
/// @brief  Release an acquired hardware.
///
///         The caller should stream off and release any acquired hardware prior to releasing a device. The KMD should attempt
///         to clean up in case the device is not properly shutdown.
///
/// @param  hCSL    A handle to the CSL session.
/// @param  hDevice The device instance to release.
///
/// @return CamxResult indicating success or failure of the call.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CSLReleaseHardware(
    CSLHandle       hCSL,
    CSLDeviceHandle hDevice);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CSLLink
///
/// @brief  Creates a link between the provided device handles in a CSL session. More than two devices may be linked with a
///         single link handle.
///
///         This method creates a Link with the provided device handles in a CSL session.  The array of device handles must only
///         contain real-time device handles. All links (i.e. calls to CSLLink) for a session must be established before calling
///         CSLStreamOn.
///
/// @param  hCSL        A handle to the CSL session.
/// @param  phDevices   Pointer to an array of device resources that needs to be linked.
/// @param  handleCount The size of the array pointed to by phDevices.
/// @param  phLink      Output handle to the link created.
///
/// @return CamxResult indicating success or failure of the call.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CSLLink(
    CSLHandle           hCSL,
    CSLDeviceHandle*    phDevices,
    UINT                handleCount,
    CSLLinkHandle*      phLink);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CSLUnlink
///
/// @brief  Removes the specified link between real time devices.
///
/// @param  hCSL   A handle to the CSL session.
/// @param  phLink Input handle to the Link to be unlinked.
///
/// @return CamxResult indicating success or failure of the call.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CSLUnlink(
    CSLHandle          hCSL,
    CSLLinkHandle*     phLink);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CSLSyncLinks
///
/// @brief  Creates a synchronization object in the KMD for the links specified in phLink.
///
///         This method creates a synchronization object in the KMD for the links specified in phLink.
///         Synchronization will be used primarily for dual camera in a single session. All links (i.e. calls to CSLLink)
///         for a session must be established before calling CSLStreamOn.
///
/// @param  hCSL          A handle to the CSL session.
/// @param  phLink        Pointer to an array of link handles that need to be synchronized.
/// @param  handleCount   The size of the array pointed to by phLink.
/// @param  hMasterhLink  Master link handle is the link handle which will drive timing for the synchronization between the
///                       two links. This link must be one of the links in phLink.
/// @param  syncMode      Sync mode for this Sync Links
///
/// @return CamxResult indicating success or failure of the call.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CSLSyncLinks(
    CSLHandle           hCSL,
    CSLLinkHandle*      phLink,
    UINT                handleCount,
    CSLLinkHandle       hMasterhLink,
    CSLSyncLinkMode     syncMode);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CSLOpenRequest
///
/// @brief  Creates a new process capture request with CSL for a CSL session.
///
///         This method creates a new process capture request with KMD for a CSL session. A hint from the UMD is provided
///         to the KMD to help decide whether to bubble the specified request if it is believed the KMD cannot service
///         the request in a single subsequent SOF.
///
/// @param  hCSL                   A handle to the CSL session.
/// @param  hLink                  link handle on which request to be created
/// @param  requestId              Input Capture? request id on which KMD will operate on subsequent SOFs.
/// @param  bubble                 Hint for KMD to decide whether to bubble or not
/// @param  syncMode               Sync mode for this request
/// @param  expectedExposureTime   How long do we expect the request's exposure time to be (In Milliseconds)
///
/// @return CamxResult indicating success or failure of the call.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult  CSLOpenRequest(
    CSLHandle          hCSL,
    CSLLinkHandle      hLink,
    UINT64             requestId,
    BOOL               bubble,
    CSLSyncLinkMode    syncMode,
    UINT32             expectedExposureTime);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CSLCancelRequest
///
/// @brief  Deletes an existing process capture request with CSL for a CSL session.
///
///         This method Deletes an existing process capture request previously registered with KMD for a CSL session.
///         If KMD request manager already started processing the request once all the KMD realtime devices has received
///         the requested setting then this API will return failure.
///
/// @param  hCSL       A handle to the CSL session.
/// @param  rFlushInfo Reference to CSL Flush Information.
///
/// @return CamxResult indicating success or failure of the call.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult  CSLCancelRequest(
    CSLHandle           hCSL,
    const CSLFlushInfo& rFlushInfo);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CSLStreamOn
///
/// @brief  Enable underlying devices to receive and process packets.
///
///         CSLStreamOn controls the state of the underlying devices. After a call to CSLStreamOn devices will be powered on,
///         all resources fully committed and ready to accept processing of packets. This call will block until the stream on
///         operations is complete.
///
/// @param  hCSL      A handle to the CSL session.
/// @param  phLink    Link handle
/// @param  phDevices Pointer to devices.
///
/// @return CamxResult indicating success or failure of the call.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CSLStreamOn(
    CSLHandle           hCSL,
    CSLLinkHandle*      phLink,
    CSLDeviceHandle*    phDevices);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CSLSingleDeviceStreamOn
///
/// @brief  Enable specific device to receive and process packets.
///
///         CSLSingleDeviceStreamOn controls the state of a specific device.
///         After a call to CSLSingleDeviceStreamOn device will be powered on,
///         all resources fully committed and ready to accept processing of packets.
///         This call will block until the stream on
///         operations is complete.
///
/// @param  hCSL        A handle to the CSL session.
/// @param  deviceIndex Device index.
/// @param  phDevice    Pointer to device.
///
/// @return CamxResult indicating success or failure of the call.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CSLSingleDeviceStreamOn(
    CSLHandle           hCSL,
    INT32               deviceIndex,
    CSLDeviceHandle*    phDevice);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CSLStreamOff
///
/// @brief  Disable underlying devices.
///
///         After a call to CSLStreamOff no packets can be processed. The call will block until all outstanding packets are
///         processed by the KMD. A client should call CSLFlush prior to CSLStreamOff to immediately terminate a use case.
///
/// @param  hCSL      A handle to the CSL session.
/// @param  phLink    Link handle
/// @param  phDevices Pointer to devices.
/// @param  mode      Stream off mode.
///
/// @return CamxResult indicating success or failure of the call.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CSLStreamOff(
    CSLHandle           hCSL,
    CSLLinkHandle*      phLink,
    CSLDeviceHandle*    phDevices,
    CSLDeactivateMode   mode);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CSLSingleDeviceStreamOff
///
/// @brief  Disable single device.
///
///         After a call to CSLSingleDeviceStreamOff no packets can be processed. The call will block until all outstanding
///         packets are processed by the KMD. A client should call CSLFlush prior to CSLStreamOff to immediately terminate a
///         use case.
///
/// @param  hCSL        A handle to the CSL session.
/// @param  deviceIndex Device index.
/// @param  phDevice    Pointer to device.
///
/// @return CamxResult indicating success or failure of the call.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CSLSingleDeviceStreamOff(
    CSLHandle           hCSL,
    INT32               deviceIndex,
    CSLDeviceHandle*    phDevice);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CSLSubmit
///
/// @brief  Submit a packet to a device.
///
/// @param  hCSL        A handle to the CSL session.
/// @param  hDevice     A handle to a CSL Device instance.
/// @param  hPacket     A handle to packet memory.
/// @param  offset      Offset within memory described by hPacket where packet definition begins
///
/// @return CamxResult indicating success or failure of the call.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CSLSubmit(
    CSLHandle           hCSL,
    CSLDeviceHandle     hDevice,
    CSLMemHandle        hPacket,
    SIZE_T              offset);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CSLFlushLock
///
/// @brief  Obtain lock for the devices to prevent any new packet submission or open a new request
///
///         This method will prevent any new packet submission to KMD for all the devices of the given session until
///         CSLFlushUnlock is called. This will help to skip any new packet submission to the device during the flush call.
///         If a device tries to submit a new packet after calling CSLFlushLock, that submission will immediately
///         return failure. Caller is expected to handle that.
///
/// @param  hCSL       A handle to the CSL session.
/// @param  rFlushInfo Reference to CSL Flush Information.
///
/// @return CamxResult indicating success or failure of the call.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CSLFlushLock(
    CSLHandle           hCSL,
    const CSLFlushInfo& rFlushInfo);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CSLFlushUnlock
///
/// @brief  Release lock for the devices to allow new packet submission or open a new request
///
///         This method will unblock packet submission for all of the devices of the given session.
///         Caller is expected to call this to release the lock for packet submission once it knows that all the outstanding
///         requests for the devices have either completed successfully or cleared during flush call.
///
/// @param  hCSL A handle to the CSL session.
///
/// @return CamxResult indicating success or failure of the call.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CSLFlushUnlock(
    CSLHandle hCSL);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CSLRegisterMessageHandler
///
/// @brief  Register a message handler callback with CSL.
///
///         CSL provides the ability for the kernel mode driver to pass messages back to the user mode driver. Two types of
///         messages are defined, error messages and frame messages.
///
///         Error messages are used to return error status of asynchronous events. Frame messages are used to communicate the
///         status of frames through the system. Frame messages should be sent by the KMD as early as possible after the SOF.
///         UMD must not depend on receiving the event.
///
///         CSLRegisterMessageHandler allows the client to register a message handler callback with CSL. Only a single message
///         handler may be registered with CSL at any time.
///
/// @param  hCSL            A handle to the CSL session.
/// @param  hCSLLinkHandle  CSL link handle
/// @param  handler         Callback method to call when a new message is available. A NULL pointer is used to disable
///                         callbacks to the UMD.
/// @param  pUserData       Opaque pointer provided by the client to be passed back in the callback to handler.
///
/// @return CamxResult indicating success or failure of the call.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CSLRegisterMessageHandler(
    CSLHandle           hCSL,
    CSLLinkHandle       hCSLLinkHandle,
    CSLMessageHandler   handler,
    VOID*               pUserData);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CSLRegisterSessionMessageHandler
///
/// @brief  Register a message handler callback with CSL.
///
/// @param  hCSL                 A handle to the CSL session.
/// @param  sesMessageHandler    Callback method to call when a new message is available. A NULL pointer is used to disable
///                              callbacks to the UMD.
/// @param  pUserData            Opaque pointer provided by the client to be passed back in the callback to handler.
///
/// @return CamxResult indicating success or failure of the call.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

CamxResult CSLRegisterSessionMessageHandler(
    CSLHandle                   hCSL,
    CSLSessionMessageHandler    sesMessageHandler,
    VOID*                       pUserData);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CSLAlloc
///
/// @brief  Allocate a buffer that can be shared between UMD, KMD, and FW/HW.
///
/// @param  pStr            A string tag indicates who is calling CSLAlloc.
/// @param  pBufferInfo     Output parameter for details of the allocated buffer.
/// @param  bufferSize      The requested size of the allocated buffer.
/// @param  alignment       The required alignment of the buffer specified in bytes.
/// @param  flags           Usage flags indicating to KMD how buffer will be used for proper allocation and mapping.
/// @param  pDeviceIndices  Pointer to array of device indices that may access this buffer.
/// @param  deviceCount     The number of valid entries in the array pointed to by pDeviceIndices.
///
/// @return CamxResult indicating success or failure of the call.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CSLAlloc(
    const CHAR*     pStr,
    CSLBufferInfo*  pBufferInfo,
    SIZE_T          bufferSize,
    SIZE_T          alignment,
    UINT32          flags,
    const INT32*    pDeviceIndices,
    UINT            deviceCount);

/// @todo (CAMX-771)   Deprecate!
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CSLMapBuffer
///
/// @brief  Map an already allocated buffer that can be shared between UMD, KMD, and FW/HW.
///
/// @param  pBufferInfo     Output parameter for details of the allocated buffer.
/// @param  bufferFD        File descriptor of the buffer to map.
/// @param  offset          The offset in the file at which the buffer to be mapped begins.
/// @param  bufferLength    The size of the buffer to be mapped.
/// @param  flags           Usage flags indicating to KMD how buffer will be used for proper allocation and mapping. Multiple
///                         usage flags may be bitwise OR together.
/// @param  pDeviceIndices  Pointer to array of device indexes that may access this buffer.
/// @param  deviceCount     The number of valid entries in the array pointed to by pDeviceIndices.
///
/// @return CamxResult indicating success or failure of the call.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CSLMapBuffer(
    CSLBufferInfo*  pBufferInfo,
    INT             bufferFD,
    SIZE_T          offset,
    SIZE_T          bufferLength,
    UINT32          flags,
    const INT32*    pDeviceIndices,
    UINT            deviceCount);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CSLMapNativeBuffer
///
/// @brief  Map an already allocated buffer that can be shared between UMD, KMD, and FW/HW.
///
/// @param  pBufferInfo     Output parameter for details of the allocated buffer.
/// @param  phNativeBuffer  Handle to native buffer to map.
/// @param  offset          The offset in the file at which the buffer to be mapped begins.
/// @param  bufferLength    The size of the buffer to be mapped.
/// @param  flags           Usage flags indicating to KMD how buffer will be used for proper allocation and mapping. Multiple
///                         usage flags may be bitwise OR together.
/// @param  pDeviceIndices  Pointer to array of device indexes that may access this buffer.
/// @param  deviceCount     The number of valid entries in the array pointed to by pDeviceIndices.
///
/// @return CamxResult indicating success or failure of the call.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CSLMapNativeBuffer(
    CSLBufferInfo*          pBufferInfo,
    const CSLNativeHandle*  phNativeBuffer,
    SIZE_T                  offset,
    SIZE_T                  bufferLength,
    UINT32                  flags,
    const INT32*            pDeviceIndices,
    UINT                    deviceCount);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CSLGetBufferInfo
///
/// @brief  Retrieve the properties of a buffer managed by CSL.
///
/// @param  hBuffer      Handle of a buffer to retrieve buffer properties for.
/// @param  pBufferInfo  Output parameter for details of the buffer.
///
/// @return CamxResult indicating success or failure of the call.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CSLGetBufferInfo(
    CSLMemHandle    hBuffer,
    CSLBufferInfo*  pBufferInfo);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CSLBufferCacheOp
///
/// @brief  Allows UMD to invalidate and/or clean the cache.
///
/// @param  hBuffer     Handle of a buffer to perform cache operations on.
/// @param  invalidate  Set to TRUE to invalidate the cache.
/// @param  clean       Set to TRUE to clean the cache.
///
/// @return CamxResult
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CSLBufferCacheOp(
    CSLMemHandle    hBuffer,
    BOOL            invalidate,
    BOOL            clean);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CSLReleaseBuffer
///
/// @brief  Release a buffer from CSL.
///
///         This method will remove any mappings from UM, KMD and/or HW made by CSL. If the buffer was allocated by CSL the
///         underlying memory is also freed.
///
/// @param  hBuffer Handle of a buffer to release.
///
/// @return CamxResult
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CSLReleaseBuffer(
    CSLMemHandle hBuffer);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CSLReleaseBufferForced
///
/// @brief  Release a buffer from CSL.
///
///         This method will remove any mappings from UM, KMD and/or HW made by CSL. If the buffer was allocated by CSL the
///         underlying memory is also freed.
///
/// @param  hBuffer Handle of a buffer to release.
///
/// @return CamxResult
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CSLReleaseBufferForced(
    CSLMemHandle hBuffer);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CSLCreatePrivateFence
///
/// @brief  Create a CSL fence object that will be used only within the CamX UMD and KMD.
///
///         Fence objects represent synchronization points in the pipeline (HW/SW) that can be used in the dependency list of
///         packets or be waited on explicitly in the UMD to be signaled. Fence objects are not specific to a CSL session and
///         can be shared across sessions.
///
///         Private fences are not usable by entities outside of the CamX UMD and KMD. If a fence needs to be shared the
///         client must create a fence from an existing native fence with the CSLCreateFenceFromNativeFence.
///
/// @param  pName       A string name for the fence. This parameter is useful for debugging. Passing NULL is valid.
/// @param  phFenceOut  The resulting CSL fence representing the created fence.
///
/// @return CamxResult
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CSLCreatePrivateFence(
    const CHAR* pName,
    CSLFence*   phFenceOut);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CSLCreateNativeFence
///
/// @brief  Create or import a native fence. CSL may import or create a native fence (platform specific) and allow
///         KMD/HW to signal fences that may be used by entities outside of the CamX UMD/KMD, or to wait on such fences.
///         The bahavior of this function is platform-specific, but its intent is the same across all platforms (that is, to
///         allow CamX to interact with the rest of the platform through native fences.)
///
/// @param  pCreateData Input data needed to execute the correct platform-specific behavior for creating or importing a fence.
/// @param  phFenceOut  The resulting CSL fence representing the created/imported native fence.
///
/// @return CamxResult
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CSLCreateNativeFence(
    CSLNativeFenceCreateDataPtr pCreateData,
    CSLFence*                   phFenceOut);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CSLGetFenceAttrib
///
/// @brief  Get the requested attribute of the give fence.
///
/// @param  hFence      Handle to a fence.
/// @param  attrib      An attribute identifier to be retrieved.
/// @param  pAttribVal  Pointer to where the attribute name will be written.
/// @param  valSize     Size of the value type.
///
/// @return CamxResult
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CSLGetFenceAttrib(
    CSLFence    hFence,
    UINT32      attrib,
    VOID*       pAttribVal,
    UINT32      valSize);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CSLMergeFences
///
/// @brief  Merge multiple fences to a new fence that is a logical AND of the merged fences.
///
/// @param  phFences    Pointer to an array of fences handles.
/// @param  fenceCount  The number of fence handles in the array pointed to by phFences.
/// @param  phFenceOut  The address where resulting CSL fence handle will be written.
///
/// @return CamxResult
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CSLMergeFences(
    CSLFence*   phFences,
    SIZE_T      fenceCount,
    CSLFence*   phFenceOut);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CSLFenceWait
///
/// @brief  Wait on a CSL fence.
///
///         This method will block until the fence is signaled or the timeout expires.
///
/// @param  hFence  The handle of the fence to wait on.
/// @param  timeout Timeout for the wait in milliseconds.
///
/// @return CamxResult indicating success or failure of the call.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CSLFenceWait(
    CSLFence    hFence,
    UINT64      timeout);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CSLFenceWaitMultiple
///
/// @brief  Wait on multiple fences.
///
///         This method will block until any (logical OR) of the waited on fences are signaled or the timeout expires.
///
/// @param  phFences        Pointer to an array of fences handles.
/// @param  pFenceSignaled  Pointer to array of BOOLs. Entries will be set to TRUE or FALSE to indicate which fence in
///                         phFences was signaled at the same index.
/// @param  fenceCount      The number of entries in the array pointed to by phFences and pFenceSignaled.
/// @param  timeout         Timeout for the wait in milliseconds.
///
/// @return CamxResult indicating success or failure of the call.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CSLFenceWaitMultiple(
    CSLFence*   phFences,
    BOOL*       pFenceSignaled,
    SIZE_T      fenceCount,
    UINT64      timeout);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CSLFenceAsyncWait
///
/// @brief  Asynchronously wait on a fence.
///
///         Register a callback method to be called by CSL when a fence is signaled. The callback will be executed in the
///         context of a CSL thread. Clients should not perform any significant work or blocking calls in the context of the
///         callback.
///
/// @param  hFence      Handle to the fence to wait for notifications on.
/// @param  handler     Callback method to be called when the fence is signaled.
/// @param  pUserData   Opaque pointer to pass back in the callback call.
///
/// @return CamxResult indicating success or failure of the call.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CSLFenceAsyncWait(
    CSLFence        hFence,
    CSLFenceHandler handler,
    VOID*           pUserData);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CSLFenceAsyncCancel
///
/// @brief  Cancel an asynchronous wait.
///
/// @param  hFence      Handle of a fence that is being waited on by CSLFenceAsyncWait.
/// @param  handler     Callback method that is registered with CSLFenceAsyncWait.
/// @param  pUserData   Opaque pointer that was registered with CSLFenceAsyncWait.
///
/// @return CamxResult indicating success or failure of the call.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CSLFenceAsyncCancel(
    CSLFence        hFence,
    CSLFenceHandler handler,
    VOID*           pUserData);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CSLFenceSignal
///
/// @brief  Signal a fence from UMD.
///
///         UMD signal of a fence is only supported for private CSL fences. This method will fail if attempting to signal any
///         other fences.
///
/// @param  hFence Handle of a fence to signal.
/// @param  result Status to indicate success or failure
///
/// @return CamxResult indicating success or failure of the call.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CSLFenceSignal(
    CSLFence        hFence,
    CSLFenceResult  result);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CSLReleaseFence
///
/// @brief  Release a CSL fence.
///
/// @param  hFence Handle of the fence to release.
///
/// @return CamxResult indicating success or failure of the call.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CSLReleaseFence(
    CSLFence hFence);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CSLDumpRequest
///
/// @brief  Dumps the information for the error request
///
/// @param  hCSL                CSL handle
/// @param  pDumpRequestInfo    Input parameters to dump the request
/// @param  pFilledLength       OUT size of the buffer dumped
///
/// @return CamxResult          Indicating success or failure of the call.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult  CSLDumpRequest(
    CSLHandle           hCSL,
    CSLDumpRequestInfo* pDumpRequestInfo,
    SIZE_T*             pFilledLength);

#ifdef __cplusplus
}
#endif // __cplusplus

#endif // CAMXCSL_H
