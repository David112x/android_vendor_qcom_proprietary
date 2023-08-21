////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2016-2020 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  camxcslhwinternal.h
/// @brief CamxCSL internal header file Version 1.0
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// NOWHINE FILE GR016: Converting CSL header to C, we need typedef
// NOWHINE FILE CF032: C-Style definitions for 0 parameters should be (VOID)
// NOWHINE FILE GR017: The ioctl type uses unsigned long

#ifndef CAMXCSLHWINTERNAL_H
#define CAMXCSLHWINTERNAL_H

#if ANDROID

#include <linux/media.h>
#include <media/cam_defs.h>
#include <media/cam_isp.h>
#include <media/cam_req_mgr.h>
#include <media/cam_sync.h>
#include <sys/ioctl.h>

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @todo  (CAMX-485) Move these headers to a different header file
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include <fcntl.h>
#include <unistd.h>

#include <poll.h>
#include <sys/mman.h>
#include "camxtypes.h"
#include "camxcsl.h"
#include "camxosutils.h"
#include "camxutils.h"
#include "camxlist.h"
#include "camxpacketdefs.h"
#include "camxthreadmanager.h"
#include "camxsyncmanager.h"

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Forward Declarations
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
typedef struct CSLHwDevice CSLHwDevice;

static const INT32  CSLHwMaxKMDNumDevices  = CAM_REQ_MGR_MAX_HANDLES_V2;   ///< Maximum number of KMD devices managed by CSL
static const INT32  CSLHwMaxNumSessions    = CAM_REQ_MGR_MAX_HANDLES_V2;   ///< Maximum number of sessions managed by CSL
static const INT32  CSLHwMaxLinksArray     = CSLHwMaxNumSessions;       ///< Maximum number of LinksArrays
static const INT32  CSLHwMaxSleepTime      = 100000;                    ///< Maximum sleep time in usec
static const INT32  CSLHwMaxDevOpenPolls   = 5;                         ///< Maximum device open poll attempts
static const INT32  EBUSY_CODE             = 114;                       ///< Error code number for operation already in progress
static const INT32  CSLMaxFences           = 1024;                      ///< Maximum fences that can be opened

// Max number of acquired devices is kept as same as CSLInternalMaxNumSessions because KMD handles
// are shared with sessions handles
static const INT32  CSLHwMaxNumAcquiredDevices   = CSLHwMaxNumSessions;
static const UINT   CSLHwMaxDevName              = 64;  ///< Max size of a KMD device name

/// @brief Pipe message types.
typedef enum
{
    CSLHwPipeMessageExitThread = 0,  ///< Pipe Exit message type
    CSLHwPipeMessageAddFd,           ///< Pipe Add new FD message type
    CSLHwPipeMessageDeleteFd         ///< Pipe Delete new FD message type
} CSLHwPipeType;

/// @brief Pipe message data structure.
typedef struct
{
    CSLHwPipeType type;  ///< CSLHwPipeType type of the message
    INT           fd;    ///< File descriptor associated to the message
} CSLInternalHwPipeMessage;

/// @brief Different states of the CSLHw Instance/Device.
typedef enum
{
    CSLHwInvalidState = 0,     ///< The CSLHw Instance/Device is in Invalid state
    CSLHwValidState,           ///< The CSLHw Instance/Device is in Valid state
    CSLHwDestroyingState,      ///< The CSLHw Instance/Device is getting destroyed
    CSLHwErrorState,           ///< The CSLHw Instance/Device is in Error state
    CSLHwFlushState,           ///< The CSLHw Instance/Device is in Flush state
                               ///  which is also a valid state
    CSLHwMaxState              ///< This is upper bound check value
} CSLHwInternalState;

/// @brief CSLHw KMD device classifications.
typedef enum
{
    CSLInternalHwVideodevice = 0,     ///< This CSLHw KMD device of type v4l video device
    CSLInternalHwVideoSubdevice,      ///< This CSLHw KMD device of type v4l subdev device
    CSLInternalHwVideoSubdeviceAll,   ///< This CSLHw KMD device of type v4l all subdev devices
    CSLInternalHwVideoInvalid         ///< This is upper bound check value
} CSLHwInternalHwEnumeration;

/// @brief CSLHw KMD Device type
typedef enum
{
    CSLHwDeviceSingleton,  ///< This CSLHw KMD device is singleton and do not allow multiple opens
    CSLHwDeviceMultiton    ///< This CSLHw KMD device is multiton and allow multiple opens
} CSLHwKMDDeviceType;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CSL Internal Device type Definitions
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
typedef enum
{
    CSLHwInvalidDevice        = CSLDeviceTypeInvalidDevice,    ///< Invalid device type
    CSLHwImageSensor          = CSLDeviceTypeImageSensor,      ///< Image sensor device.
    CSLHwLensActuator         = CSLDeviceTypeLensActuator,     ///< Lens actuator device.
    CSLHwCompanion            = CSLDeviceTypeCompanion,        ///< Sensor companion device.
    CSLHwEEPROM               = CSLDeviceTypeEEPROM,           ///< EEPROM to store the calibration data of ImageSensor.
    CSLHwCSIPHY               = CSLDeviceTypeCSIPHY,           ///< CSIPHY Camera Serial Interface Physical receiver.
    CSLHwOIS                  = CSLDeviceTypeOIS,              ///< OIS Optical image stabilization
    CSLHwFlash                = CSLDeviceTypeFlash,            ///< Flash device.
    CSLHwFD                   = CSLDeviceTypeFD,               ///< Face detection device.
    CSLHwJPEGE                = CSLDeviceTypeJPEGE,            ///< JPEG encoder device.
    CSLHwJPEGD                = CSLDeviceTypeJPEGD,            ///< JPEG decoder device.
    CSLHwVFE                  = CSLDeviceTypeVFE,              ///< VFE (Video Front End) device.
    CSLHwCPP                  = CSLDeviceTypeCPP,              ///< CPP (Camera Post Processor) device.
    CSLHwCSID                 = CSLDeviceTypeCSID,             ///< CSID Camera Serial Interface Decoder
    CSLHwISPIF                = CSLDeviceTypeISPIF,            ///< ISPIF Image Signal Processor InterFace
    CSLHwIFE                  = CSLDeviceTypeIFE,              ///< IFE (Image Front End) device.
    CSLHwICP                  = CSLDeviceTypeICP,              ///< ICP (Image Control Processor) device.
    CSLHwLRME                 = CSLDeviceTypeLRME,             ///< LRME (Low Resolution Motion Estimation) device.
    CSLHwCustom               = CSLDeviceTypeCustom,           ///< Custom HW
    CSLHwMAXDEVICE            = CSLDeviceTypeMaxDevice,        ///< MAX Device for UMD interactions
    CSLHwCPAS_TOP             = CSLHwMAXDEVICE + 1,            ///< CPAS_TOP device
    CSLHwRequestManager       = CSLHwMAXDEVICE + 2             ///< Camera Request Manager.
} CSLHwInternalDeviceType;

/// @brief The string name of the CSLHwInternalDeviceType. Must be in order of CSLHwInternalDeviceType enum.
#if __GNUC__
static const CHAR* CSLHwInternalDeviceTypeStrings[] __attribute__((unused)) =
#else // __GNUC__
static const CHAR* CSLHwInternalDeviceTypeStrings[] =
#endif // _GNUC__
{
    "CSLHwInvalidDevice",
    "CSLHwImageSensor",
    "CSLHwLensActuator",
    "CSLHwCompanion",
    "CSLHwEEPROM",
    "CSLHwCSIPHY",
    "CSLHwOIS",
    "CSLHwFlash",
    "CSLHwFD",
    "CSLHwJPEGE",
    "CSLHwJPEGD",
    "CSLHwVFE",
    "CSLHwCPP",
    "CSLHwCSID",
    "CSLHwISPIF",
    "CSLHwIFE",
    "CSLHwICP",
    "CSLHwLRME",
    "CSLHwCustom",
    "CSLHwMAXDEVICE",
    "CSLHwCPAS_TOP",
    "CSLHwRequestManager"
};

CAMX_STATIC_ASSERT(CAMX_ARRAY_SIZE(CSLHwInternalDeviceTypeStrings) ==
    (static_cast<UINT>(CSLHwInternalDeviceType::CSLHwRequestManager) + 1));

static const CHAR* CSLHwInternalDeviceStateStrings[] =
{
    "CSLHwInvalidState",
    "CSLHwValidState",
    "CSLHwDestroyingState",
    "CSLHwErrorState",
    "CSLHwFlushState",
    "CSLHwMaxState"
};

/// @brief CSLHw KMD IOMMU handles
typedef struct
{
    INT32 hSecureIOMMU;     ///< This CSLHw KMD device Secure IOMMU handle
    INT32 hNonSecureIOMMU;  ///< This CSLHw KMD device Non Secure IOMMU handle
} CSLHwIOMMUHandle;

/// @brief CSLHw KMD device ops.
typedef struct
{
    INT  (*Open)(const CHAR*  DeviceName, INT Flags);
    CamxResult (*Close)(INT Fd);
    // NOWHINE GR017: The ioctl type uses unsigned long
    CamxResult (*Ioctl)(const CSLHwDevice* pDevice, unsigned long request, VOID* arg);
    CamxResult (*Ioctl2)(const CSLHwDevice* pDevice, UINT request, VOID* arg, UINT32 hType, UINT32 size);
    CamxResult (*KMDQueryCap)(CSLHandle index);
    CamxResult (*UMDQueryCap)(INT32 deviceIndex, VOID* pDeviceData, SIZE_T deviceDataSize);
    CamxResult (*Acquire)(CSLHandle hCSL, CSLDeviceHandle* phDevice, INT32 deviceIndex,
        CSLDeviceResource* pDeviceResourceRequest, SIZE_T numDeviceResources);
    CamxResult (*Release)(CSLHandle hCSL, INT32 deviceIndex, CSLDeviceHandle hDevice);
    CamxResult (*StreamOn)(CSLHandle Sessionindex, CSLDeviceHandle hDevice, INT32 deviceIndex, CSLDeactivateMode mode);
    CamxResult (*StreamOff)(CSLHandle Sessionindex, CSLDeviceHandle hDevice, INT32 deviceIndex, CSLDeactivateMode mode);
    CamxResult (*Submit)(CSLHandle hCSL, CSLDeviceHandle hDevice, CSLMemHandle hPacket, SIZE_T offset, INT32 deviceIndex);
    CamxResult (*AcquireHardware)(CSLHandle hCSL, CSLDeviceHandle hDevice, INT32 deviceIndex,
                         CSLDeviceResource* pDeviceResourceRequest);
    CamxResult(*AcquireHardwareV2)(CSLHandle hCSL, CSLDeviceHandle hDevice, INT32 deviceIndex,
        CSLDeviceResource* pDeviceResourceRequest);
    CamxResult (*ReleaseHardware)(CSLHandle hCSL, CSLDeviceHandle hDevice, INT32 deviceIndex);
} CSLHwDeviceOps;

/// @brief CSLHw device
typedef struct CSLHwDevice
{
    INT32                       deviceIndex;                                   ///< Index of the KMD device table
    CamX::Mutex*                lock;                                          ///< Lock to protect CSL HW device
    INT32                       aState;                                        ///< state of CSL HW device
    CamX::Condition*            destroyCondition;                              ///< Condition used to destroy device
    CSLDeviceDescriptor         deviceDesc;                                    ///< CSL HW device descriptor
    // CSLDeviceDescriptor is having all below so use that
    CSLHwInternalDeviceType     deviceType;                                    ///< CSL HW device internal device type
    CSLVersion                  driverVersion;                                 ///< The version of the device driver
    CSLVersion                  hwVersion;                                     ///< CSL HW device NOT NEEDED
    CSLVersion                  hwPlatformVersion;                             ///< Device platform version NOT NEEDED
    // Till here
    CSLHwKMDDeviceType          mode;                                          ///< mode of KMD device node
    CHAR                        devName[CSLHwMaxDevName];                      ///< KMD device name
    INT                         deviceOrder;                                   ///< Device deviceOrder of stream ON for sequence
    INT                         fd;                                            ///< KMD Device File descriptor
    CSLDeviceHandle             hAcquired[CSLHwMaxNumAcquiredDevices];         ///< Acquired Handles of this CSL HW device
    UINT32                      kmdGroupId;                                    ///< KMD device discovery groupid
    CSLHwIOMMUHandle            hMapIOMMU;                                     ///< KMD HW Iommu handles for this CSL HW device
    CSLHwIOMMUHandle            hMapCDMIOMMU;                                  ///< KMD HW CDM Iommu handles
    UINT                        refCount;                                      ///< refCount for CSL HW device
    VOID*                       pUMDDeviceData;                                ///< Mostly NOT NEEDED
    SIZE_T                      umdDeviceDataSize;                             ///< Mostly NOT NEEDED
    VOID*                       pKMDDeviceData;                                ///< KMD Query cap data
    SIZE_T                      kmdDeviceDataSize;                             ///< Size of KMD Query cap data
    CSLHwDeviceOps              deviceOp;                                      ///< CSL HW ops associated this device
    CSLDeviceOperationMode      operationMode;                                 ///< mode of device Realtime/NonRealtime
    BOOL                        isActive;                                      ///< State to track whether device
                                                                               ///  is streamed on or off
} CSLHwDevice;

/// @brief CSLHw Acquired device
typedef struct
{
    CamX::Mutex*              lock;                         ///< Lock to protect CSLHw Acquired HW
    CamX::Condition*          destroyCondition;             ///< Condition used to destroy CSLHw Acquired HW
    INT32                     aState;                       ///< State of CSLHw Acquired HW
    CSLHwInternalDeviceType   deviceType;                   ///< CSLHw device internal device type
    CSLDeviceHandle           hAcquired;                    ///< Handle of this CSLHw Acquired HW
    CSLHandle                 hSession;                     ///< Session handle associated with this CSLHw Acquired HW
    UINT                      refCount;                     ///< refCount planning to use for session based operation
    INT                       fd;                           ///< KMD Device File descriptor associated with this CSLHw
                                                            ///< Acquired HW
    INT                       order;                        ///< Device deviceOrder of stream ON for sequence
    INT                       orderoff;                     ///< Device deviceOrder of stream OFF for sequence
    UINT                      cslDeviceIndex;               ///< Index of the CSLHw in KMD device table
    CSLHwKMDDeviceType        mode;                         ///< mode of KMD device node
    CHAR                      dev_name[CSLHwMaxDevName];    ///< KMD device name
    UINT                      acquiredPID;                  ///< PID of the this CSLHw Acquired HW
    UINT                      acquiredTID;                  ///< TID of the this CSLHw Acquired HW
    CSLDeviceOperationMode    operationMode;                ///< KMD device operate in Realtime or NonRealtime
    CHAR                      nodeName[MaxStringLength256]; ///< NodeIdentifier with pipelineIndex
} CSLHwAcquiredDevice;

/// @brief CSLHw Links
typedef struct
{
    CSLLinkHandle           hLinkIdentifier;                        ///< Handle for the link in a CSLHw session
    CSLDeviceHandle         hDevice[CSLHwMaxKMDNumDevices];         ///< Array of handles associated in this CSLHwLink
    INT32                   num_devices;                            ///< Number of valid handles in hDevice array above
    CSLDeactivateMode       currentMode;                            ///< Current mode
    BOOL                    isActive;                               ///< State to track whether link is active or not
} CSLHwLinks;

typedef struct
{
    CSLMessageHandler       messageHandler;                               ///< Message handler for events in a session
    VOID*                   pMessageData;                                 ///< User pdata for the callback handler
    CSLLinkHandle           hCSLLinkHandle;                               ///< CSL Link Handle
} CSLHwLinkHandleData;

/// @brief CSLHw Session
typedef struct
{
    CamX::Mutex*            lock;                                         ///< Lock to synchronize access to global CSL state
    CamX::Condition*        destroyCondition;                             ///< Condition used to destroy CSLHw session
    INT32                   aState;                                       ///< State of CSLHw session
    INT32                   index;                                        ///< Index of the CSLHw in KMD device table
    BOOL                    streamOn;                                     ///< TRUE for stream ON, FALSE for stream off
    UINT                    refCount;                                     ///< refCount for session based operations -
                                                                          ///  indicates if clients are actively performing
                                                                          ///  operations. Not to be confused for clientRefCount
    INT                     clientRefCount;                               ///< Indicates how many clients (Session, Pipelines)
                                                                          ///  are holding references to this CSL session.
                                                                          ///  This is used to ensure that this CSL session is
                                                                          ///  not closed until all clients remove references.
    CSLHandle               hSession;                                     ///< Generated by bridge driver for hw
    CSLHwAcquiredDevice     sessionDevices[CSLHwMaxNumAcquiredDevices];   ///< Array to hold handles in this CSLHw session
    CSLHwLinks              linkInfo[CSLHwMaxLinksArray];                 ///< Array of Links in this CSLHw session
    UINT                    linkCount;                                    ///< Number of Links in this CSLHw session
    CSLLinkHandle           hMasterLink;                                  ///< Master link handle in this session
    UINT                    acquiredPID;                                  ///< PID of the this CSLHw Acquired HW
    UINT                    acquiredTID;                                  ///< TID of the this CSLHw Acquired HW
    CSLMessageHandler       messageHandler;                               ///< Message handler for events in a session
    CSLSessionMessageHandler sessionMessageHandler;                       ///< Message handler for all Sessions
    VOID*                   pMessageData;                                 ///< User pdata for the callback handler
    VOID*                   pSessionMessageData;                          ///< Use  pdata for session callback
    CSLHwLinkHandleData     CSLLinkHandleData[CSLHwMaxLinksArray];        ///< CSL Link handle with pipeline

    CamX::LightweightDoublyLinkedList rtList;                             ///< Realtime device list
    CamX::LightweightDoublyLinkedList nrtList;                            ///< Non-Realtime device list
    BOOL                    bInFlush;                                     ///< Flush in progress
    CSLFlushInfo            flushInfo;                                    ///< Flush Info if flush is in progress
} CSLHwsession;

/// @brief CSLHw buffer info
typedef struct
{
    CSLBufferInfo* pBufferInfo;         ///< Pointer containing the CSL buffer information
    BOOL           isImported;          ///< Flag indicating if the buffer was imported
    UINT           refcount;            ///< refcount for number of links
} CSLHwMemBufferInfo;

/// @brief CSLHw Memory manager info
typedef struct
{
    CSLHwMemBufferInfo bufferInfo[CAM_MEM_BUFQ_MAX];          ///< Array to hold CSL buffer Info references
} CSLHwMemMgrInfo;

/// @brief CSLFenceInfo Instance structure
typedef struct
{
    BOOL isValid;                         ///< if the fence is valid or not
    CHAR fenceName[MaxStringLength256];   ///< char Array to hold CSL fence names
} CSLFenceInfo;

/// @brief CSLHw Instance structure
typedef struct
{
    CamX::Mutex*                lock;                                          ///< Lock to synchronize access to CSL state
    CamX::Condition*            destroyCondition;                              ///< Condition used to destroy CSLHw Instance
    INT32                       aState;                                        ///< State of CSLHw Instance
    CSLCameraPlatform           pCameraPlatform;                               ///< KMD Platform of this CSLHw Instance
    UINT                        refcount;                                      ///< refCount for Instance based operation
    UINT                        numSessions;                                   ///< Number of opened sessions
    CSLHwsession                sessionList[CSLHwMaxNumSessions];              ///< Session data in this CSLHw Instance
    INT32                       kmdDeviceCount;                                ///< Number of KMD devices
    UINT                        sensorSlotDeviceCount;                         ///< Number of KMD Sensor slot devices
    CSLHwDevice                 requestManager;                                ///< Camera request manger KMD device
    CSLHwDevice                 CPASDevice;                                    ///< CPAS KMD device
    CSLHwDevice                 CSLInternalKMDDevices[CSLHwMaxKMDNumDevices];  ///< Array to hold CSLHw KMD devices
    CSLHwDevice                 CSLHwSensorSlotDevices[CSLHwMaxKMDNumDevices]; ///< Array to hold CSLHw KMD Sensor slot devices
    CamX::OSThreadHandle        pollThreadHandle;                              ///< CSLHw poll thread handle
    INT                         pipeFd[2];                                     ///< CSLHw pipeFd  to interact with poll thread
    struct pollfd               pollFds[CSLHwMaxKMDNumDevices];                ///< Poll FD array to poll in CSLHw  poll thread
    nfds_t                      pollNumFds;                                    ///< Number of valid fd in pollFds array
    CamX::Mutex*                pollLock;                                      ///< Lock to synchronize access to poll fds
    CamX::SyncManager*          pSyncFW;                                       ///< Instance of the Sync Manager framework
    CSLHwMemMgrInfo             memManager;                                    ///< Instance of the CSL Memory Manager
    UINT                        acquiredPID;                                   ///< PID of the this CSLHw Instance
    UINT                        acquiredTID;                                   ///< TID of the this CSLHw Instance
    CamX::Mutex*                allocLock;                                     ///< CSL allocation lock
} CSLHwInstance;

extern CSLHwInstance g_CSLHwInstance;
extern CSLFenceInfo  g_CSLFenceInfo[CSLMaxFences];  ///< CSL Fence Information

extern CSLHwDeviceOps g_CSLHwDeviceDefaultOps;
extern CSLHwDeviceOps g_CSLHwDeviceCPASOps;
extern CSLHwDeviceOps g_CSLHwDeviceFDOps;
extern CSLHwDeviceOps g_CSLHwDeviceVFEOps;
extern CSLHwDeviceOps g_CSLHwDeviceLRMEOps;
extern CSLHwDeviceOps g_CSLHwDeviceIFEOps;
extern CSLHwDeviceOps g_CSLHwDeviceICPOps;
extern CSLHwDeviceOps g_CSLHwDeviceSensorOps;
extern CSLHwDeviceOps g_CSLHwDeviceActuatorOps;
extern CSLHwDeviceOps g_CSLHwDeviceCsiPhyOps;
extern CSLHwDeviceOps g_CSLHwDeviceJPEGOps;
extern CSLHwDeviceOps g_CSLHwDeviceEepromOps;
extern CSLHwDeviceOps g_CSLHwDeviceFlashOps;
extern CSLHwDeviceOps g_CSLHwDeviceOisOps;
extern CSLHwDeviceOps g_CSLHwDeviceCustomHWOps;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CSLHwIsHwInstanceValid
///
/// @brief  This api.checks the CSLHw instance is in valid state or not
///
/// @return boolean TRUE on success or FALSE on failure
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL CSLHwIsHwInstanceValid();

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CSLHwIsKmdGroupidCslInternalDevicetype
///
/// @brief  This api finds if the KMD groupid is an CSL internal device or not
///
/// @param  groupId     KMD groupid
///
/// @return boolean TRUE on success or FALSE on failure
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL CSLHwIsKmdGroupidCslInternalDevicetype(
    UINT32 groupId);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CSLHwIsHwRealtimeDevice
///
/// @brief  This api finds if the CSL internal device is a Realtime device or not
///
/// @param  type     CSL internal device type
///
/// @return boolean TRUE on success or FALSE on failure
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL CSLHwIsHwRealtimeDevice(
    CSLHwInternalDeviceType type);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CSLHwRtDeviceOderCompare
///
/// @brief  This api is used to sort the real time devices in stream ON order
///
/// @param  p1     Pointer to the Acquired HW
/// @param  p2     Pointer to the next Acquired HW in list to sort
///
/// @return INT   The difference of the given order of p1-p2
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
INT CSLHwRtDeviceOderCompare(
    const VOID* p1,
    const VOID* p2);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CSLHwRtDeviceStreamoffOrderCompare
///
/// @brief  This api is used to sort the real time devices in stream OFF order
///
/// @param  p1     Pointer to the Acquired HW
/// @param  p2     Pointer to the next Acquired HW in list to sort
///
/// @return INT   The difference of the given order of p1-p2
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
INT CSLHwRtDeviceStreamoffOrderCompare(
    const VOID* p1,
    const VOID* p2);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CSLHwGetDeviceTypeFromInternal
///
/// @brief  This api returns the CSL device type from the give CSL HW internal type.
///
/// @param  type     Pointer to the Acquired HW
///
/// @return CSLDeviceType returns valid CSL device type on success and CSLDeviceTypeInvalidDevice on failure
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CSLDeviceType CSLHwGetDeviceTypeFromInternal(
    CSLHwInternalDeviceType  type);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CSLHwGetHwDeviceOrder
///
/// @brief  This api gets the CSLW device order used for the streamOn
///
///         This method gets the CSLW device order used for the streamOn which is fixed based on the order of devices to
///         be turned ON for streamOn. If the device is not a realtime device and the default order is -1 and these devices
///         for now will be streamed ON only after the realtime devices stream ON and these devices can be streamedON parallel
///         to real time devices too.
///
/// @param  type     Type of the CSL HW Internal device
/// @param  pOrder   Output pointer to be updated with order of the given CSL HW device type
///
/// @return boolean  TRUE on success or FALSE on failure
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL CSLHwGetHwDeviceOrder(
    CSLHwInternalDeviceType  type,
    INT*                     pOrder);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CSLHwGetHwDeviceStreamOffOrder
///
/// @brief  This api gets the CSLW device order used for the streamOff
///
///         This method gets the CSLW device order used for the streamOn which is fixed based on the order of devices to
///         be turned ON for streamOn. If the device is not a realtime device and the default order is -1 and these devices
///         for now will be streamed ON only after the realtime devices stream ON and these devices can be streamedON parallel
///         to real time devices too.
///
/// @param  type     Type of the CSL HW Internal device
/// @param  pOrder   Output pointer to be updated with order of the given CSL HW device type
///
/// @return boolean  TRUE on success or FALSE on failure
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL CSLHwGetHwDeviceStreamOffOrder(
    CSLHwInternalDeviceType  type,
    INT*                     pOrder);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CSLHwInstanceSetState
///
/// @brief  This api sets the state of CSLHw instance.
///
/// @param  state  State which needs to be set for CSLHw instance.
///
/// @return boolean success or failure status
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL CSLHwInstanceSetState(
    CSLHwInternalState state);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CSLHwInstanceGetState
///
/// @brief  This api returns the current state of CSLHw instance.
///
/// @return INT32
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
INT32 CSLHwInstanceGetState();

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CSLHwInstanceGetRefCount
///
/// @brief  This api get a reference count on the current valid CSLHw instance.
///
/// @return boolean success or failure status
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL CSLHwInstanceGetRefCount();

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CSLHwInstancePutRefCount
///
/// @brief  This api puts back a reference count which was taken earlier using CSLHwInstanceGetRefCount
///
/// @return None
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID CSLHwInstancePutRefCount();

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CSLHwIsSessionStreamOn
///
/// @brief  This api checks the state of a CSLHw session and check whether the session is stream ON or not
///
/// @param  pSession  Pointer to the session to be operated on
///
/// @return boolean success if stream in ON or failure if stream is OFF or the session is in invalid state
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL CSLHwIsSessionStreamOn(
    CSLHwsession* pSession);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CSLHwGetSessionState
///
/// @brief  This api returns the current state of CSLHw Session.
///
/// @param  pSession  Pointer to the session to be operated on
///
/// @return INT32
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
INT32 CSLHwGetSessionState(
    CSLHwsession* pSession);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CSLHwSetSessionState
///
/// @brief  This api sets the state of CSLHw Session.
///
/// @param  pSession  Pointer to the session to be operated on
/// @param  state     State which needs to be set for CSLHw instance.
///
/// @return boolean success or failure status
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CSLHwSetSessionState(
    CSLHwsession*      pSession,
    CSLHwInternalState state);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CSLHwSessionGetRefCount
///
/// @brief  This api.get a reference count on the current valid CSLHw Session.
///
/// @param  pSession  Pointer to the session to be operated on
///
/// @return boolean success or failure status
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL CSLHwSessionGetRefCount(
    CSLHwsession* pSession);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CSLHwSessionPutRefCount
///
/// @brief  This api.puts back a reference count which was taken earlier using CSLHwSessionGetRefCount
///
/// @param  pSession  Pointer to the session to be operated on
///
/// @return None
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID CSLHwSessionPutRefCount(
    CSLHwsession* pSession);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CSLHwSessionWaitForRefcountZero
///
/// @brief  This api wait for the CSLHw session refcount to be zero
///
/// @param  pSession  Pointer to the session to be operated on
///
/// @return boolean success or failure status
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL CSLHwSessionWaitForRefcountZero(
    CSLHwsession* pSession);


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CSLHwKMDDeviceGetRefcount
///
/// @brief  This api get a reference count on the current valid CSLHwDevice.
///
/// @param  pHWDevice  Pointer to the CSLHwDevice to be operated on
///
/// @return boolean success or failure status
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL CSLHwKMDDeviceGetRefcount(
    CSLHwDevice* pHWDevice);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CSLHwKMDDevicePutRefcount
///
/// @brief  This api puts back a reference count which was taken earlier using CSLHwKMDDeviceGetRefcount
///
/// @param  pHWDevice  Pointer to the CSLHwDevice to be operated on
///
/// @return None
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID CSLHwKMDDevicePutRefcount(
    CSLHwDevice* pHWDevice);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CSLHwInternalCreateSession
///
/// @brief  Creates a CSLHw session.
///
/// @param  phCSL Output parameter. An handle to the CSLHw Session created by KMD camera request manager.
///
/// @return CamxResult indicating success or failure of the call.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CSLHwInternalCreateSession(
    CSLHandle* phCSL);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CSLHwInternalDestroySession
///
/// @brief  Close a CSLHw Session.
///
/// @param  hCSL Handle to the CSLHw session to be closed.
///
/// @return CamxResult indicating success or failure of the call.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CSLHwInternalDestroySession(
    CSLHandle hCSL);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CSLHwLinkKMDHardwares
///
/// @brief  Creates a link between the provided device handles in a KMD session for realtime devices.
///
/// @param  pSession    Pointer to the session to be operated on
/// @param  phDevices   Pointer to an array of device resources that needs to be linked.
/// @param  handleCount The size of the array pointed to by phDevices.
/// @param  phLink      Output handle to the link created.
///
/// @return CamxResult indicating success or failure of the call.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CSLHwLinkKMDHardwares(
    CSLHwsession*    pSession,
    CSLDeviceHandle* phDevices,
    UINT             handleCount,
    CSLLinkHandle*   phLink);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CSLHwSyncLinkKMDHardwares
///
/// @brief  Creates a synchronization object in the KMD for the links specified in phLink.
///
/// @param  pSession      Pointer to the session to be operated on
/// @param  phLink        Pointer to an array of link handles that need to be synchronized.
/// @param  handleCount   The size of the array pointed to by phLink.
/// @param  hMasterLink   Master link handle is the link handle which will drive timing for the synchronization between the
///                       two links. This link must be one of the links in phLink.
/// @param  syncMode      Sync mode for this Sync Links
///
/// @return CamxResult indicating success or failure of the call.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CSLHwSyncLinkKMDHardwares(
    CSLHwsession*   pSession,
    CSLLinkHandle*  phLink,
    UINT            handleCount,
    CSLLinkHandle   hMasterLink,
    CSLSyncLinkMode syncMode);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CSLHwScheduleRequest
///
/// @brief  Schedules a new process capture request with KMD for a CSL session.
///
/// @param  pSession                   Pointer to the session to be operated on
/// @param  hLink                      Link handle associated to a schedule request.
/// @param  requestId                  Input Capture? request id on which KMD will operate on subsequent SOFs.
/// @param  bubble                     Hint for KMD to decide whether to bubble or not
/// @param  syncMode                   Sync mode for this request
/// @param  expectedExposureTimeInMS   The request's expected exposure time in Milliseconds
///
/// @return CamxResult indicating success or failure of the call.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CSLHwScheduleRequest(
    CSLHwsession*   pSession,
    CSLLinkHandle   hLink,
    UINT64          requestId,
    BOOL            bubble,
    CSLSyncLinkMode syncMode,
    UINT32          expectedExposureTimeInMS);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CSLHwStreamOnKMDHardwares
///
/// @brief  This api triggers stream ON to all the device in the pList
///
/// @param hSessionIndex    Index pointing to the session in the session list of the CSLHw instance.
/// @param pList            List pointer holding list of CSLHwAcquiredDevices to be streamed ON
/// @param phDevices        Device handles
/// @param mode             Deactivate mode
///
/// @return CamxResult indicating success or failure of the call.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CSLHwStreamOnKMDHardwares(
    CSLHandle                          hSessionIndex,
    CamX::LightweightDoublyLinkedList* pList,
    CSLDeviceHandle*                   phDevices,
    CSLDeactivateMode                  mode);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CSLHwStreamOffKMDHardwares
///
/// @brief  This api triggers stream OFF to all the device in the pList
///
/// @param hSessionIndex    Index pointing to the session in the session list of the CSLHw instance.
/// @param pList            List pointer holding list of CSLHwAcquiredDevices to be streamed OFF
/// @param phDevices        Device handles
/// @param mode             Deactivate mode
///
/// @return CamxResult indicating success or failure of the call.

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CSLHwStreamOffKMDHardwares(
    CSLHandle                          hSessionIndex,
    CamX::LightweightDoublyLinkedList* pList,
    CSLDeviceHandle*                   phDevices,
    CSLDeactivateMode                  mode);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CSLHwSingleDeviceStreamOnKMDHardwares
///
/// @brief  This api triggers stream ON to a specific device
///
/// @param  hSessionIndex    Index pointing to the session in the session list of the CSLHw instance.
/// @param  deviceIndex      Device index
/// @param  phDevice         Device handle
/// @param  mode             Deactivate mode
///
/// @return CamxResult indicating success or failure of the call.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CSLHwSingleDeviceStreamOnKMDHardwares(
    CSLHandle                          hSessionIndex,
    INT32                              deviceIndex,
    CSLDeviceHandle*                   phDevice,
    CSLDeactivateMode                  mode);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CSLHwSingleDeviceStreamOffKMDHardwares
///
/// @brief  This api triggers stream OFF to a specific device
///
/// @param  hSessionIndex    Index pointing to the session in the session list of the CSLHw instance.
/// @param  deviceIndex      Device index
/// @param  phDevice         Device handle
/// @param  mode             Deactivate mode
///
/// @return CamxResult indicating success or failure of the call.

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CSLHwSingleDeviceStreamOffKMDHardwares(

    CSLHandle                          hSessionIndex,
    INT32                              deviceIndex,
    CSLDeviceHandle*                   phDevice,
    CSLDeactivateMode                  mode);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CSLHwEnumerateAndAddCSLHwDevice
///
/// @brief  This api detects KMD devices according to the class and adds to CSLHw device list.
///
/// @param  deviceType   Type of the hardware to be enumerated
/// @param  deviceClass  V4L2 Class of the KMD device
///
/// @return boolean success or failure status
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL CSLHwEnumerateAndAddCSLHwDevice(
    CSLHwInternalHwEnumeration deviceType,
    UINT32                     deviceClass);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CSLHwUnLinkKMDHardwares
///
/// @brief  Removes the specified link between real time devices in a session.
///
/// @param  pSession  Pointer to the session to be operated on
/// @param  hLink     Input handle to the Link to be unlinked.
///
/// @return CamxResult indicating success or failure of the call.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CSLHwUnLinkKMDHardwares(
    CSLHwsession* pSession,
    CSLLinkHandle hLink);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CSLHwRemoveHwDevice
///
/// @brief  This api removes the KMD devices according to the class from CSLHw device list.
///
/// @param  deviceType   Type of the hardware to be enumerated
/// @param  deviceClass  V4L2 Class of the KMD device
///
/// @return None
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID CSLHwRemoveHwDevice(
    CSLHwInternalHwEnumeration deviceType,
    UINT32                     deviceClass);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CSLHwInternalDefaultOpen
///
/// @brief  This api open a device node file of a KMD device with the given name and flags
///
/// @param  pDeviceName  Device node name to be opened
/// @param  flags        Flags used for the open
///
/// @return INT On success a Valid filedescriptor on failure a negative value
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
INT CSLHwInternalDefaultOpen(
    const CHAR*  pDeviceName,
    INT          flags);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CSLHwInternalDefaultClose
///
/// @brief  This api close the given File descriptor of the KMD device
///
/// @param  fd   File descriptor to be closed
///
/// @return boolean success or failure status
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL CSLHwInternalDefaultClose(
    INT fd);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CSLHwInternalFDIoctl
///
/// @brief  This api triggers a ioctl on a given file descriptor on the KMD device
///
/// @param  fd       File descriptor on which ioctl needs to be performed
/// @param  request  command/request associated with the ioctl
/// @param  pArg     Arguments of the ioctl
///
/// @return boolean success or failure status
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL CSLHwInternalFDIoctl(
    INT           fd,
    unsigned long request,
    VOID*         pArg);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CSLHwInternalDefaultIoctl
///
/// @brief  This api triggers a ioctl on a given CSLHwDevice
///
/// @param  pDevice  CSLHwDevice on which ioctl needs to be performed
/// @param  request  command/request associated with the ioctl
/// @param  pArg     Arguments of the ioctl
///
/// @return boolean success or failure status
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL CSLHwInternalDefaultIoctl(
    const CSLHwDevice*  pDevice,
    unsigned long       request,
    VOID*               pArg);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CSLHwInternalDefaultIoctl2
///
/// @brief  This api triggers a ioctl on a given file descriptor on the KMD device
///
/// @param  pDevice  CSLHwDevice on which ioctl needs to be performed
/// @param  opcode   command/request associated with the ioctl
/// @param  pArg     Arguments of the ioctl
/// @param  hType    Type
/// @param  size     Size
///
/// @return boolean success or failure status
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL CSLHwInternalDefaultIoctl2(
    const CSLHwDevice*  pDevice,
    UINT                opcode,
    VOID*               pArg,
    UINT32              hType,
    UINT32              size);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CSLHwInternalDefaultUMDQueryCap
///
/// @brief  This api provides the Device capabilities specific to each type of device from KMD query capabilities data
///
/// @param  deviceIndex     A CSLHw device index of the device to query.
/// @param  pDeviceData     Untyped pointer to a device specific structure describing the capabilities of that device.
/// @param  deviceDataSize  The size of the data structure pointed to by pDeviceData
///
/// @return boolean success or failure status
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL CSLHwInternalDefaultUMDQueryCap(
    INT32  deviceIndex,
    VOID*  pDeviceData,
    SIZE_T deviceDataSize);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CSLHwInternalDefaultSubmit
///
/// @brief  This api Submits a packet to a KMD device in CSLHw device list.
///
/// @param  hCSL        A handle to the CSL session.
/// @param  hDevice     A handle to a CSL Device instance.
/// @param  hPacket     A handle to packet memory.
/// @param  offset      Offset within memory described by hPacket where packet definition begins
/// @param  deviceIndex Device index of the CSLHw KMD device index.
///
/// @return CamxResult success or failure status
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CSLHwInternalDefaultSubmit(
    CSLHandle       hCSL,
    CSLDeviceHandle hDevice,
    CSLMemHandle    hPacket,
    SIZE_T          offset,
    INT32           deviceIndex);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CSLHwInternalKMDStreamOp
///
/// @brief  This performs a stream ON/OFF operation based on ops specified
///
/// @param  hSessionindex A handle to the CSL session.
/// @param  hDevice       A handle to a CSL Device instance.
/// @param  deviceIndex   Device index of the CSLHw KMD device index.
/// @param  op            TRUE for stream ON and FALSE for stream OFF.
///
/// @return boolean success or failure status
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL CSLHwInternalKMDStreamOp(
    CSLHandle       hSessionindex,
    CSLDeviceHandle hDevice,
    INT32           deviceIndex,
    BOOL            op);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CSLHwInternalKMDStreamOn
///
/// @brief  This performs a stream ON operation based on ops specified
///
/// @param  hSessionindex A handle to the CSL session.
/// @param  hDevice       A handle to a CSL Device instance.
/// @param  deviceIndex   Device index of the CSLHw KMD device index.
/// @param  mode          Deactivate mode
///
/// @return boolean success or failure status
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL CSLHwInternalKMDStreamOn(
    CSLHandle           hSessionindex,
    CSLDeviceHandle     hDevice,
    INT32               deviceIndex,
    CSLDeactivateMode   mode);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CSLHwInternalKMDStreamOff
///
/// @brief  This performs a stream ON operation based on ops specified
///
/// @param  hSessionindex A handle to the CSL session.
/// @param  hDevice       A handle to a CSL Device instance.
/// @param  deviceIndex   Device index of the CSLHw KMD device index.
/// @param  mode          Deactivate mode
///
/// @return boolean success or failure status
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL CSLHwInternalKMDStreamOff(
    CSLHandle           hSessionindex,
    CSLDeviceHandle     hDevice,
    INT32               deviceIndex,
    CSLDeactivateMode   mode);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CSLHwInternalKMDRelease
///
/// @brief  This api release KMD HW device acquired using private acquire ops in corresponding device index of the CSLHw
///
/// @param  hCSL          A handle to the CSL session.
/// @param  deviceIndex   Device index of the CSLHw KMD device index.
/// @param  hDevice       A handle to a CSL Device instance.
///
/// @return boolean success or failure status
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL CSLHwInternalKMDRelease(
    CSLHandle       hCSL,
    INT32           deviceIndex,
    CSLDeviceHandle hDevice);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CSLHwInternalKMDAcquireHardware
///
/// @brief  This api acquires device hardware
///
/// @param  hCSL                    A handle to the CSL session.
/// @param  hDevice                 Device index of the CSLHw KMD device index.
/// @param  deviceIndex             A handle to a CSL Device instance.
/// @param  pDeviceResourceRequest  Pointer to device resource
///
/// @return boolean success or failure status
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL CSLHwInternalKMDAcquireHardware(
    CSLHandle          hCSL,
    CSLDeviceHandle    hDevice,
    INT32              deviceIndex,
    CSLDeviceResource* pDeviceResourceRequest);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CSLHwInternalKMDAcquireHardwareV2
///
/// @brief  this api acquires device hardware for reflect version 2 API changes
///
/// @param  hCSL                    A handle to the CSL session.
/// @param  hDevice                 Device index of the CSLHw KMD device index.
/// @param  deviceIndex             A handle to a CSL Device instance.
/// @param  pDeviceResourceRequest  Pointer to device resource
///
/// @return boolean success or failure status
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL CSLHwInternalKMDAcquireHardwareV2(
    CSLHandle          hCSL,
    CSLDeviceHandle    hDevice,
    INT32              deviceIndex,
    CSLDeviceResource* pDeviceResourceRequest);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CSLHwInternalKMDReleaseHardware
///
/// @brief  This api releases device hardware
///
/// @param  hCSL          A handle to the CSL session.
/// @param  hDevice       A handle to a CSL Device instance.
/// @param  deviceIndex   Device index of the CSLHw KMD device index.
///
/// @return boolean success or failure status
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL CSLHwInternalKMDReleaseHardware(
    CSLHandle       hCSL,
    CSLDeviceHandle hDevice,
    INT32           deviceIndex);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CSLHwCancelRequest
///
/// @brief  This api deletes an existing process capture request with KMD for a CSL session or flush all the existing requests
///
/// @param  pSession       Pointer to the session to be operated on.
/// @param  rCSLFlushInfo  Reference to flush info
///
/// @return CamxResult success or failure status
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CSLHwCancelRequest(
    CSLHwsession*       pSession,
    const CSLFlushInfo& rCSLFlushInfo);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CSLHwLinkControl
///
/// @brief  This api toggles CRM link control based on op mode
///
/// @param  pSession    Pointer to the session to be operated on.
/// @param  hLink       link handle
/// @param  opMode      op mode
///
/// @return CamxResult success or failure status
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CSLHwLinkControl(
    CSLHwsession* pSession,
    CSLLinkHandle hLink,
    UINT32        opMode);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CSLGetBufferInfoHW
///
/// @brief  This api retrieves the CSLBufferInfo from CSLHw internal data using the provided CSLMemHandle
///
/// @param  hBuffer      CSLMemHandle of the buffer information to be retrieved.
/// @param  pBufferInfo  Pointer where the buffer info needs to be updated.
///
/// @return CamxResult success or failure status
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CSLGetBufferInfoHW(
    CSLMemHandle    hBuffer,
    CSLBufferInfo*  pBufferInfo);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CSLHwAddKMDDeviceToInstance
///
/// @brief  This api removes the KMD devices according to the class from CSLHw device list.
///
/// @param  pDeviceName   Device node name to be added to CSLHw KMD device list
/// @param  groupId       groupId of the KMD device
/// @param  pDeviceIndex  Pointer to update the CSLHw device index after added to  CSLHw KMD device list
/// @param  deviceFd      fd of an already opened device to be added, otherwise -1
///
/// @return boolean success or failure status
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL CSLHwAddKMDDeviceToInstance(
    CHAR*  pDeviceName,
    UINT32 groupId,
    INT32* pDeviceIndex,
    INT    deviceFd);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CSLHwAddFdToPoll
///
/// @brief  This api add the given fd to the poll thread to receive events
///
/// @param  fd   File descriptor to be added to the poll thread
///
/// @return boolean success or failure status
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL CSLHwAddFdToPoll(
    INT fd);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CSLHwRemoveFromPoll
///
/// @brief  This api removes the given fd from the poll thread
///
/// @param  fd   File descriptor to be removed from the poll thread
///
/// @return boolean success or failure status
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL CSLHwRemoveFromPoll(
    INT fd);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CSLHwInternalDefaultSubscribeEvents
///
/// @brief  This API subscribes to a V4L event
///
/// @param  fd    File descriptor of the device node that is subscribed
/// @param  id    V4L2 event id
/// @param  type  V4L2 event type

/// @return boolean success or failure status
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CSLHwInternalDefaultSubscribeEvents(
    INT    fd,
    UINT32 id,
    UINT32 type);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CSLHwRemoveSensorSlotDeviceFromInstance
///
/// @brief  This api removes the KMD Sensor Slot from the sensor slot list in CSLHw
///
/// @param  pHWDevice  Pointer to the CSLHwDevice to be operated on
///
/// @return None
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID CSLHwRemoveSensorSlotDeviceFromInstance(
    CSLHwDevice* pHWDevice);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CSLHwSensorSlotKmdQueryCapability
///
/// @brief  This api finds the available CSL device of class v4l2_subdev and returns the subdevice name.
///
/// @param  hIndex  Pointer to store the identified device node name
///
/// @return CamxResult success or failure status
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CSLHwSensorSlotKmdQueryCapability(
    CSLHandle hIndex);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CSLHwInternalProbeSensorHW
///
/// @brief  This api finds the matching slot on which the sensor probe needs to triggered and perform probe ioctl for sensor.
///
/// @param  hPacket       A handle to a probe packet memory.
/// @param  offset        Offset within memory described by hPacket where packet definition begins
/// @param  pDeviceIndex  Pointer to update the device index of the CSLHw KMD device index.
///
/// @return CamxResult indicating success or failure
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CSLHwInternalProbeSensorHW(
    CSLMemHandle hPacket,
    SIZE_T       offset,
    INT32*       pDeviceIndex);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CSLHwGetSyncHwDevice
///
/// @brief  This api finds the name of the sync device which is of class v4l2_dev and populates it in the passed parameter
///
/// @param  pSyncDeviceName  Pointer to CHAR where device name will be populated. This has to be allocated by caller.
/// @param  deviceNameLen    Length of pSyncDeviceName, in bytes
///
/// @return CamxResult indicating success or failure
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CSLHwGetSyncHwDevice(
    CHAR* pSyncDeviceName,
    INT32 deviceNameLen);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CSLHwPopulateMMUHandles
///
/// @brief  This api populates the MMU handles for the passed CSLHw KMD device indices
///
/// @param  pMMUHandles           Output pointer to hold the KMD IOMMU handles corresponding to device indices
/// @param  pNumMMUHandles        Output pointer to be updated on total devices to be mapped.
/// @param  pDeviceIndices        Pointer which points to array of device indexes to be mapped
/// @param  deviceCount           Number of device handles needed to be updated
/// @param  flags                 CSL flags used to identify secure or non secure type of buffer.
///
/// @return CamxResult success or failure status
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CSLHwPopulateMMUHandles(
    INT32*       pMMUHandles,
    UINT32*      pNumMMUHandles,
    const INT32* pDeviceIndices,
    UINT         deviceCount,
    UINT32       flags);


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CSLHwPopulateCDMMMUHandles
///
/// @brief  This api populates the CDM MMU handles for the passed CSLHw KMD device indices
///
/// @param  pMMUHandles           Output pointer to hold the KMD IOMMU handles corresponding to device indices
/// @param  pNumMMUHandles        Output pointer to be updated on total devices to be mapped.
/// @param  pDeviceIndices        Pointer which points to array of device indexes to be mapped
/// @param  deviceCount           Number of device handles needed to be updated
/// @param  CSLFlags              CSL flags used to identify secure or non secure type of buffer.
///
/// @return CamxResult success or failure status
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CSLHwPopulateCDMMMUHandles(
    INT32*       pMMUHandles,
    UINT32*      pNumMMUHandles,
    const INT32* pDeviceIndices,
    UINT         deviceCount,
    UINT32       CSLFlags);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CSLHwMapCSLAllocFlagsToKMD
///
/// @brief  This api maps CSL memory allocation flags to kernel memory allocation flags
///
/// @param  cslflags   Usage flags indicating to KMD how buffer will be used for proper allocation and mapping
/// @param  pKMDFlags  Pointer to KMDFlags derived from input cslflags
///
/// @return CamxResult success or failure status
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CSLHwMapCSLAllocFlagsToKMD(
    UINT32  cslflags,
    UINT32* pKMDFlags);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CSLHwAddBuffer
///
/// @brief   This api populates and adds the CSLBufferInfo information into the internal tracking data structures of CSL
///
/// @param   hMem              MemHandle of the CSL buffer to be added
/// @param   fd                File descriptor of the CSL buffer
/// @param   len               Total length of the CSL buffer
/// @param   ppCSLBufferInfo   Pointer of the CSLBufferInfo to be updated
/// @param   cslFlags          Usage flags indicating to KMD how buffer will be used for proper allocation and mapping.
/// @param   isImported        Flag indicating if buffer is imported or allocated by CSLHw
///
/// @return  CamxResult success or failure status
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CSLHwAddBuffer(
    CSLMemHandle    hMem,
    INT             fd,
    UINT64          len,
    CSLBufferInfo** ppCSLBufferInfo,
    UINT32          cslFlags,
    BOOL            isImported);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CSLHwReleaseBufferInKernel
///
/// @brief  This api makes a call to the kernel to release a buffer
///
/// @param  hMem  MemHandle of the CSL buffer to be released
///
/// @return CamxResult success or failure status
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CSLHwReleaseBufferInKernel(
    CSLMemHandle hMem);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CSLHwParseDeviceAttribute
///
/// @brief   This api parse and set the attribute in CSLHwDevice structure
///
/// @param   pHWDevice           Pointer to CSLHwDevice structure
/// @param   pDeviceAttribute    Pointer to an array of device attributes
/// @param   numDeviceAttributes Size of the array of device attrubutes
///
/// @return  CamxResult success or failure status
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CSLHwParseDeviceAttribute(
    CSLHwDevice*        pHWDevice,
    CSLDeviceAttribute* pDeviceAttribute,
    SIZE_T              numDeviceAttributes);


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CSLHwDumpRequest
///
/// @brief   This api dumps the information for the error request
///
/// @param   pSession            Pointer to the session to be operated on.
/// @param   pDumpRequestInfo    Dump requset info
/// @param   pFilledLength       OUT size of the buffer dumped
///
/// @return  CamxResult success or failure status
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CSLHwDumpRequest(
    CSLHwsession*          pSession,
    CSLDumpRequestInfo*    pDumpRequestInfo,
    SIZE_T*                pFilledLength);
#endif // ANDROID

#endif // CAMXCSLHWINTERNAL_H
