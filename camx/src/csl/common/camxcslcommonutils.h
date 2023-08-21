////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2017-2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  camxcslcommonutils.h
///
/// @brief CamxCSL common utilities header
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef CAMXCSLCOMMONUTILS_H
#define CAMXCSLCOMMONUTILS_H

#include "camxcsl.h"
#include "camxosutils.h"
#include "camxpacketdefs.h"
#include "camxthreadmanager.h"
#include "camxtypes.h"
#include "camxutils.h"

static const UINT  CSLMaxNumSessions        = 256;  ///< Max number of sessions allowed
static const UINT  CSLMaxNumAllocs          = 1024; ///< Max number of allocations allowed
static const UINT  CSLMaxNumResources       = 256;  ///< Max number of resources per device
static const UINT  CSLMaxNumDeviceHandles   = 512;  ///< Max number of device handles
static const UINT  CSLMaxNumFences          = 1024; ///< Max number of fences
static const UINT  CSLMaxNumDevices         = 16;   ///< Max number of devices

/// @todo (CAMX-366) Clean up management of device index for camera sensors.
static const INT32 CSLSensorDeviceIndex     = 8;   ///< The device index to be used for sensor device.

CAMX_STATIC_ASSERT(CSLxMaxDeviceIndex == CamxMaxDeviceIndex);

struct CSLJumpTable;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief Structure to track a CSL fence state
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
struct CSLFenceInfo
{
    enum class Type
    {
        Private,                            ///< Private fence type
        Native                              ///< Native (android) fence
    };

    enum class Status
    {
        Unsignaled = 0,                     ///< The status is unsignaled
        Signaled                            ///< The status is signaled
    };
    CSLFence                hFence;         ///< The handle representation of this fence object
    Type                    type;           ///< Type of the fence
    const CHAR*             pName;          ///< Name (if any)
    Status                  status;         ///< Fence status
    CamX::Mutex*            pMutex;         ///< Mutex protecting the condition variable
    CamX::Condition*        pCondition;     ///< Condition implementing the fence (if private)
                                            ///  This is only for the mini-implementation purposes.
    BOOL                    composite;      ///< Is this a merge of multiple fences?
    UINT                    fenceCount;     ///< The number of merged fences
    CSLFence*               phFences;       ///< Merged fences
    CSLFenceInfo*           pParent;        ///< Parent fence, if this is part of a merged fence
    UINT                    refCount;       ///< Reference counter
    CSLFenceHandler         fenceHandler;   ///< Async handler, if any (assumption is only one is supported)
    VOID*                   pHandlerData;   ///< Async handler data
    CSLFenceResult          fenceResult;    ///< Indicates the success/failure status of a signal
};

/// @brief  Debug buffer descriptor
struct CSLDebugBufferEntry
{
    CSLDebugBufferType  type;                   ///< Type of the debug buffer
    CSLMemHandle        hRegDumpBuffer;         ///< Handle to register dump debug buffer
    SIZE_T              offsetRegDumpBuffer;    ///< Start offset of the register dump buffer
    SIZE_T              lengthRegDumpBuffer;    ///< Length in bytes of the buffer
    SIZE_T              bytesWritten;           ///< Num bytes written to this buffer
};

/// @brief Aggregates session bookkeeping stuff.
struct CSLSession
{
    enum class State
    {
        Invalid = 0,                                ///< Initial state
        Opened,                                     ///< Session is open
        Closed                                      ///< Session is closed
    };

    CSLHandle            hSession;                           ///< Session handle
    State                state;                              ///< State of the session
    BOOL                 acquired[CSLMaxNumDevices];         ///< Acquired state for devices in this session
    BOOL                 acquiredHardware[CSLMaxNumDevices]; ///< Acquired state for device hardware in this session
    BOOL                 streamOn;                           ///< StreamOn
    CSLMessageHandler    messageHandler;                     ///< Callback for handling messages from KMD
    VOID*                pMessageHandlerUserData;            ///< User data for the callback
    CSLDebugBufferEntry  regDumpDebugBuffer;                 ///< Current debug buffer for register dump
    UINT32               refCount;                           ///< Count of client references for the CSL Session
};

/// @brief Aggregates device bookkeeping stuff.
struct CSLDeviceState
{
    INT32               index;                                      ///< Device index assigned to this device
    CSLDeviceType       type;                                       ///< The type of the device
    CSLVersion          hwVersion;                                  ///< The hardware version of the device.
    CSLVersion          driverVersion;                              ///< The version of the device driver
    VOID*               pCapabilities;                              ///< Pointer to device-specific CAPS
    UINT                numResources;                               ///< Number of resources
    CSLDeviceResource   resources[CSLMaxNumResources];              ///< Array of resources in this device
    BOOL                resourcesAcquired[CSLMaxNumResources];      ///< Used to keep track of acquired resources
    BOOL                shareable;                                  ///< Indicating if the device can be shared
    UINT                refCount;                                   ///< Number of sessions that have acquired this device
    INT32               hDevice;                                    ///< handle for the underlying device
    CSLSession*         pSession;                                   ///< Pointer to the session this device was aqcuired for
};

/// @brief Aggregates CSL bookkeeping stuff.
struct CSLState
{
    CamX::Mutex*            CSLLock;                                ///< Lock to synchronize access to global CSL state
    UINT                    numSessions;                            ///< Number of opened sessions
    CSLSession*             pSessions[CSLMaxNumSessions];           ///< Session store
    UINT                    memCount;                               ///< Number of active allocations
    CSLBufferInfo*          pMemHandles[CSLMaxNumAllocs];           ///< Memory handle store
    UINT                    numDeviceHandles;                       ///< Number of acquired device handles
    CSLDeviceState*         pDevicesState[CSLMaxNumDeviceHandles];  ///< An array of device state pointers used to track devices
    UINT                    fenceCount;                             ///< Number of fences created so far
    CSLFenceInfo*           pFences[CSLMaxNumFences];               ///< Array to store created fences
    BOOL                    uninitializing;                         ///< Used to let the thread end
    clock_t                 initializationTime;                     ///< The init time used as a relative start time
    UINT32                  sessionStateSize;                       ///< The size of session struct
    UINT32                  deviceStateSize;                        ///< the size of device struct
    CSLDeviceDescriptor*    pDeviceEntries;                         ///< Device registry with CSLMaxNumDevices entries
    VOID**                  ppDeviceCapabilities;                   ///< Device capabilities array
    CamX::ThreadManager*    fenceHandlerThreadManager;              ///< Thread manager for fence signal handlers
    CamX::JobHandle         hProcessFenceHandler;                   ///< Handle to the job for calling fence handlers
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// GetNextHandle
///
/// @brief  Get the next available handle slot from a handle store.
///
/// @param  ppHandleStore        Address of the handle store
/// @param  currentNumHandles    Current number of created handles
/// @param  maxNumHandles        Maximum number of handles
///
/// @return Handle to the available slot
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
INT GetNextHandle(
    VOID**  ppHandleStore,
    UINT    currentNumHandles,
    UINT    maxNumHandles);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// GetFromHandle
///
/// @brief  Get the next available handle slot from a handle store.
///
/// @param  ppHandleStore    Address of the handle store
/// @param  handle           Handle to retrieve
/// @param  maxNumHandles    Maximum number of handles
///
/// @return The stored pointer corresponding to the given handle
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID* GetFromHandle(
    VOID**  ppHandleStore,
    INT     handle,
    UINT    maxNumHandles);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// StoreHandle
///
/// @brief  Store the give pointer in the given handle store.
///
/// @param  ppHandleStore    Address of the handle store
/// @param  handle           Handle to store the pointer for
/// @param  pValue           Pointer to store for the handle
/// @param  maxNumHandles    Maximum number of handles
///
/// @return None
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID StoreHandle(
    VOID**  ppHandleStore,
    INT     handle,
    VOID*   pValue,
    UINT    maxNumHandles);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// GetFenceInfo
///
/// @brief  Get internal structure of the fence object from a fence handle
///
/// @param  pCSLState   CSL state
/// @param  hFence      Handle to a CSL fence
///
/// @return Pointer to the CSLFenceInfo structure of the given fence
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CSLFenceInfo* GetFenceInfo(
    CSLState*   pCSLState,
    CSLFence    hFence);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// GetDeviceState
///
/// @brief  Get internal structure of the device object from a device handle
///
/// @param  pCSLState   CSL state
/// @param  hDevice     Handle to a CSL device
///
/// @return Pointer to the CSLDeviceState structure of the given fence
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CSLDeviceState* GetDeviceState(
    CSLState*       pCSLState,
    CSLDeviceHandle hDevice);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// GetSession
///
/// @brief  Get internal structure of the session object from a session handle
///
/// @param  pCSLState   CSL state
/// @param  hCSL        Handle to a CSL session
///
/// @return Pointer to the CSLSession structure of the given session
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CSLSession* GetSession(
    CSLState*   pCSLState,
    CSLHandle   hCSL);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// GetBufferInfo
///
/// @brief  This is a helper function used to retrieve the allocation information of a given memory handle.
///
/// @param  pCSLState   CSL state
/// @param  hMem        Handle to memory
///
/// @return Pointer to the CSLBufferInfo entry in the allocation store
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CSLBufferInfo* GetBufferInfo(
    CSLState*       pCSLState,
    CSLMemHandle    hMem);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// GetCSLInitializedCommon
///
/// @brief  Get the global CSL state.
///
/// @return TRUE if CSL is initialized, FALSE otherwise.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL GetCSLInitializedCommon(void);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// SetCSLInitializedPresil
///
/// @brief  Set the global CSL state.
///
/// @param  initialized Initialization state
///
/// @return None
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID SetCSLInitializedPresil(
    BOOL initialized);

/// NOWHINE FILE DC002a: documentation for these is similar to the main entries so avoiding repetition here.
CamxResult CSLInitializeCommon(
    CSLState*               pCSLState,
    UINT32                  stateSize,
    CSLDeviceDescriptor*    pDevices,
    VOID**                  ppCapabilities);

CamxResult CSLUninitializeCommon(
    CSLState*   pCSLState);

CamxResult CSLOpenCommon(
    CSLState*   pCSLState,
    CSLHandle*  phCSL);

CamxResult CSLCloseCommon(
    CSLState*   pCSLState,
    CSLHandle   hCSL);

CamxResult CSLAddReferenceCommon(
    CSLState*   pCSLState,
    CSLHandle   hCSL);

CamxResult CSLQueryCameraPlatformCommon(
    CSLState*           pCSLState,
    CSLCameraPlatform*  pCameraPlatform);

CamxResult CSLImageSensorProbeCommon(
    CSLState*                   pCSLState,
    CSLMemHandle                hPacket,
    SIZE_T                      offset,
    CSLImageSensorProbeResult*  pProbeResult);

CamxResult CSLSetDebugBufferCommon(
    CSLState*               pCSLState,
    CSLHandle               hCSL,
    CSLDebugBufferType      type,
    CSLMemHandle            hBuffer,
    SIZE_T                  offset,
    SIZE_T                  length,
    CSLDebugBufferResult*   pDebugBufferResult);

CamxResult CSLEnumerateDevicesCommon(
    CSLState*               pCSLState,
    CSLDeviceDescriptor*    pDeviceDescriptor);

CamxResult CSLQueryDeviceCapabilitiesCommon(
    CSLState*   pCSLState,
    INT32       deviceIndex,
    VOID*       pDeviceData,
    SIZE_T      deviceDataSize);

CamxResult CSLAcquireDeviceCommon(
    CSLState*           pCSLState,
    CSLHandle           hCSL,
    CSLDeviceHandle*    phDevice,
    INT32               deviceIndex,
    CSLDeviceResource*  pDeviceResourceRequest,
    SIZE_T              numDeviceResources,
    CSLDeviceAttribute* pDeviceAttribute,
    SIZE_T              numDeviceAttributes);

CamxResult CSLReleaseDeviceCommon(
    CSLState*       pCSLState,
    CSLHandle       hCSL,
    CSLDeviceHandle hDevice);

CamxResult CSLAcquireHardwareCommon(
    CSLState*           pCSLState,
    CSLHandle           hCSL,
    CSLDeviceHandle     hDevice,
    CSLDeviceResource*  pDeviceResourceRequest);

CamxResult CSLReleaseHardwareCommon(
    CSLState*       pCSLState,
    CSLHandle       hCSL,
    CSLDeviceHandle hDevice);

CamxResult CSLStreamOnCommon(
    CSLState* pCSLState,
    CSLHandle hCSL);

CamxResult CSLStreamOffCommon(
    CSLState*   pCSLState,
    CSLHandle   hCSL);

CamxResult CSLSubmitCommon(
    CSLState*       pCSLState,
    CSLHandle       hCSL,
    CSLDeviceHandle hDevice,
    CSLMemHandle    hPacket,
    SIZE_T          offset);

CamxResult CSLRegisterMessageHandlerCommon(
    CSLState*           pCSLState,
    CSLHandle           hCSL,
    CSLMessageHandler   messageHandler,
    VOID*               pUserData);

CamxResult CSLCreatePrivateFenceCommon(
    CSLState*   pCSLState,
    const CHAR* pName,
    CSLFence*   phFenceOut);

CamxResult CSLMergeFencesCommon(
    CSLState*   pCSLState,
    CSLFence*   phFences,
    SIZE_T      fenceCount,
    CSLFence*   phFenceOut);

CamxResult CSLFenceWaitCommon(
    CSLState*   pCSLState,
    CSLFence    hFence,
    UINT64      timeout);

CamxResult CSLFenceAsyncWaitCommon(
    CSLState*       pCSLState,
    CSLFence        hFence,
    CSLFenceHandler handler,
    VOID*           pUserData);

CamxResult CSLFenceAsyncCancelCommon(
    CSLState*       pCSLState,
    CSLFence        hFence,
    CSLFenceHandler handler,
    VOID*           pUserData);

CamxResult CSLFenceSignalCommon(
    CSLState*       pCSLState,
    CSLFence        hFence,
    CSLFenceResult  fenceResult);

CamxResult CSLReleaseFenceCommon(
    CSLState*   pCSLState,
    CSLFence    hFence);


#endif // CAMXCSLCOMMONUTILS_H
