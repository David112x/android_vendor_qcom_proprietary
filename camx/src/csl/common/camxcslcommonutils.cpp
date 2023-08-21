////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2017-2018, 2020 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  camxcslcommonutils.cpp
///
/// @brief CamxCSL common utilities Implementation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include <ctime>
#include "camxcsl.h"
#include "camxcslcommonutils.h"
#include "camxincs.h"
#include "camxlist.h"
#include "camxmem.h"
#include "camxpacketdefs.h"
#include "camxutils.h"

/// @brief  Keeping track of CSL initialization
// The reason this is not part of g_CSLState is we want to track whether g_CSLState is initialized or not and cannot use
// a member of itself to do that (chicken-and-egg problem.) Technically, we could, because g_CSLState is set to 0 which
// will implicitly set any included boolean to FALSE; still, logically, this should be factored out.
// This global variable does not need synchronization as only one caller is supposed to call CSLInitialize; calling
// CSLInitialize from multiple threads would result in undefined behavior (due to the race condition checking g_initialized).
static BOOL g_initialized = FALSE;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// GetCSLInitializedCommon
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL GetCSLInitializedCommon(void)
{
    return g_initialized;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// SetCSLInitializedPresil
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID SetCSLInitializedPresil(
    BOOL initialized)
{
    g_initialized = initialized;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// GetNextHandle
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
INT GetNextHandle(
    VOID**  ppHandleStore,
    UINT    currentNumHandles,
    UINT    maxNumHandles)
{
    INT index = -1;

    if (currentNumHandles < maxNumHandles)
    {
        index = currentNumHandles;
        // This loop must find at least one available handle slot since currentNumHandles < maxNumHandles.
        // Wrap around if necessary. Use a slot unless it's being currently used (session open.)
        UINT counter = 0;
        while ((NULL != ppHandleStore[index]) && (counter < maxNumHandles))
        {
            index = (index + 1) % maxNumHandles;
            counter++;
        }
        if (maxNumHandles == counter)
        {
            CAMX_ASSERT_ALWAYS_MESSAGE("Invalid CSL state: breaking out of an otherwise infinite loop in GetNextHandle.");
            index = -1;
        }
    }
    else
    {
        CAMX_ASSERT(currentNumHandles < maxNumHandles);
    }
    // Map index (beginning from 0) to the valid handle range (beginning from 1)
    return index + 1;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// GetFromHandle
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID* GetFromHandle(
    VOID**  ppHandleStore,
    INT     handle,
    UINT    maxNumHandles)
{
    VOID*   pValue  = NULL;
    // Map handle back to index domain
    INT     index   = handle - 1;

    if ((index >= 0) && (static_cast<UINT>(index) < maxNumHandles))
    {
        pValue = ppHandleStore[index];
    }
    else
    {
        CAMX_ASSERT((index >= 0) && (static_cast<UINT>(index) < maxNumHandles));
    }
    return pValue;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// StoreHandle
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID StoreHandle(
    VOID**  ppHandleStore,
    INT     handle,
    VOID*   pValue,
    UINT    maxNumHandles)
{
    // Map handle back to index domain
    INT     index = handle - 1;

    if ((index >= 0) && (static_cast<UINT>(index) < maxNumHandles))
    {
        ppHandleStore[index] = pValue;
    }
    else
    {
        CAMX_ASSERT((index >= 0) && (static_cast<UINT>(index) < maxNumHandles));
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CreateFence
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static CSLFence CreateFence(
    CSLState*       pCSLState,
    CSLFenceInfo**  ppFence)
{
    INT           handle     = CSLInvalidHandle;
    CSLFenceInfo* pFenceInfo = static_cast<CSLFenceInfo*>(CAMX_CALLOC(sizeof(CSLFenceInfo)));

    if (NULL != pFenceInfo)
    {
        // Can we get an available slot?
        handle = GetNextHandle(reinterpret_cast<VOID**>(&pCSLState->pFences[0]),
                               pCSLState->fenceCount,
                               CSLMaxNumFences);

        if (CSLInvalidHandle != handle)
        {
            pFenceInfo->hFence = handle;
            // Now store the pointer in that slot
            pCSLState->fenceCount++;
            StoreHandle(reinterpret_cast<VOID**>(&pCSLState->pFences[0]), handle, pFenceInfo, CSLMaxNumFences);
            if (NULL != ppFence)
            {
                *ppFence = pFenceInfo;
            }
        }
        else
        {
            CAMX_FREE(pFenceInfo);
            pFenceInfo = NULL;
        }
    }
    // Use the index as handle
    return static_cast<CSLFence>(handle);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// GetFenceInfo
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CSLFenceInfo* GetFenceInfo(
    CSLState*   pCSLState,
    CSLFence    hFence)
{
    CSLFenceInfo* pFenceInfo = NULL;

    if ((CSLInvalidHandle != hFence) && (static_cast<UINT>(hFence) <= CSLMaxNumFences))
    {
        pFenceInfo = reinterpret_cast<CSLFenceInfo*>(GetFromHandle(reinterpret_cast<VOID**>(&pCSLState->pFences[0]),
                                                                   hFence,
                                                                   CSLMaxNumFences));
    }
    else
    {
        CAMX_ASSERT((CSLInvalidHandle != hFence) && (static_cast<UINT>(hFence) <= CSLMaxNumFences));
    }
    return pFenceInfo;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// DestroyFence
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static VOID DestroyFence(
    CSLState*   pCSLState,
    CSLFence    hFence)
{
    CSLFenceInfo* pFenceInfo = GetFenceInfo(pCSLState, hFence);

    if (NULL != pFenceInfo)
    {
        if (CSLFenceInfo::Type::Private == pFenceInfo->type)
        {
            if (NULL != pFenceInfo->phFences)
            {
                for (UINT i = 0; i < pFenceInfo->fenceCount; i++)
                {
                    CSLReleaseFence(pFenceInfo->phFences[i]);
                }
                CAMX_FREE(pFenceInfo->phFences);
                pFenceInfo->phFences = NULL;
            }
            CAMX_ASSERT(NULL != pFenceInfo->pCondition);
            CAMX_ASSERT(NULL != pFenceInfo->pMutex);
            pFenceInfo->pCondition->Destroy();
            pFenceInfo->pMutex->Destroy();
            pFenceInfo->pCondition = NULL;
            pFenceInfo->pMutex = NULL;
        }
        else
        {
            CAMX_NOT_IMPLEMENTED();
        }
        // Make sure we free up the handle store slot
        pCSLState->CSLLock->Lock();
        pCSLState->fenceCount--;
        StoreHandle(reinterpret_cast<VOID**>(&pCSLState->pFences[0]), hFence, NULL, CSLMaxNumFences);
        pCSLState->CSLLock->Unlock();
        CAMX_FREE(pFenceInfo);
        pFenceInfo = NULL;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// GetDeviceState
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CSLDeviceState* GetDeviceState(
    CSLState*       pCSLState,
    CSLDeviceHandle hDevice)
{
    CSLDeviceState* pDevice = NULL;

    if ((CSLInvalidHandle != hDevice) && (static_cast<UINT>(hDevice) <= CSLMaxNumDeviceHandles))
    {
        UINT index = hDevice - 1;   // map back to index
        pDevice = pCSLState->pDevicesState[index];
    }
    else
    {
        CAMX_ASSERT((CSLInvalidHandle != hDevice) && (static_cast<UINT>(hDevice) <= CSLMaxNumDeviceHandles));
    }
    return pDevice;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ConstructSession
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static CSLHandle ConstructSession(
    CSLState*       pCSLState,
    CSLSession**    ppSession)
{
    CSLHandle   hSession = CSLInvalidHandle;
    CSLSession* pSession = static_cast<CSLSession*>(CAMX_CALLOC(pCSLState->sessionStateSize));

    if (NULL != pSession)
    {
        hSession = GetNextHandle(reinterpret_cast<VOID**>(&pCSLState->pSessions[0]), pCSLState->numSessions, CSLMaxNumSessions);

        if (CSLInvalidHandle != hSession)
        {
            StoreHandle(reinterpret_cast<VOID**>(&pCSLState->pSessions[0]), hSession, pSession, CSLMaxNumSessions);
            pSession->state = CSLSession::State::Opened;
            pSession->hSession = hSession;
            pSession->streamOn = FALSE;
            pCSLState->numSessions++;
            if (NULL != ppSession)
            {
                *ppSession = pSession;
            }
        }
        else
        {
            CAMX_FREE(pSession);
            pSession = NULL;
        }
    }
    return hSession;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// GetSession
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CSLSession* GetSession(
    CSLState* pCSLState,
    CSLHandle hCSL)
{
    CSLSession* pSession    = NULL;

    if ((CSLInvalidHandle != hCSL) && (static_cast<UINT>(hCSL) <= CSLMaxNumSessions))
    {
        pSession = reinterpret_cast<CSLSession*>(GetFromHandle(reinterpret_cast<VOID**>(&pCSLState->pSessions[0]),
                                                               hCSL,
                                                               CSLMaxNumSessions));
    }
    else
    {
        CAMX_ASSERT((CSLInvalidHandle != hCSL) && (static_cast<UINT>(hCSL) <= CSLMaxNumSessions));
    }
    return pSession;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// DestructSession
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static VOID DestructSession(
    CSLState*   pCSLState,
    CSLHandle   hCSL,
    CSLSession* pSession)
{

    if ((CSLInvalidHandle != hCSL) && (NULL != pSession) && (CSLSession::State::Closed != pSession->state))
    {
        pSession->state = CSLSession::State::Closed;
        pCSLState->numSessions--;
        CAMX_FREE(pSession);
        pSession = NULL;
        StoreHandle(reinterpret_cast<VOID**>(&pCSLState->pSessions[0]), hCSL, NULL, CSLMaxNumSessions);
    }
    else
    {
        CAMX_ASSERT((CSLInvalidHandle != hCSL) && (NULL != pSession) && (CSLSession::State::Closed != pSession->state));
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ConstructDevice
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static CSLHandle ConstructDevice(
    CSLState*           pCSLState,
    CSLDeviceState**    ppDevice)
{
    CSLHandle       hDevice = CSLInvalidHandle;
    CSLDeviceState* pDevice = static_cast<CSLDeviceState*>(CAMX_CALLOC(pCSLState->deviceStateSize));

    if (NULL != pDevice)
    {
        hDevice = GetNextHandle(reinterpret_cast<VOID**>(&pCSLState->pDevicesState[0]),
                                pCSLState->numDeviceHandles,
                                CSLMaxNumDeviceHandles);

        if (CSLInvalidHandle != hDevice)
        {
            StoreHandle(reinterpret_cast<VOID**>(&pCSLState->pDevicesState[0]), hDevice, pDevice, CSLMaxNumDeviceHandles);
            pCSLState->numDeviceHandles++;
            if (NULL != ppDevice)
            {
                *ppDevice = pDevice;
            }
        }
        else
        {
            CAMX_FREE(pDevice);
            pDevice = NULL;
        }
    }
    return hDevice;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// GetDevice
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CSLDeviceState* GetDevice(
    CSLState*       pCSLState,
    CSLDeviceHandle hDevice)
{
    CSLDeviceState* pDevice = NULL;

    if ((CSLInvalidHandle != hDevice) && (static_cast<UINT>(hDevice) <= CSLMaxNumDeviceHandles))
    {
        pDevice = reinterpret_cast<CSLDeviceState*>(GetFromHandle(reinterpret_cast<VOID**>(&pCSLState->pDevicesState[0]),
                                                                  hDevice,
                                                                  CSLMaxNumDeviceHandles));
    }
    else
    {
        CAMX_ASSERT((NULL != pDevice) && (static_cast<UINT>(hDevice) <= CSLMaxNumDeviceHandles));
    }
    return pDevice;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// DestructDevice
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static VOID DestructDevice(
    CSLState*       pCSLState,
    CSLDeviceHandle hDevice,
    CSLDeviceState* pDevice)
{

    if ((CSLInvalidHandle != hDevice) && (NULL != pDevice) && (0 == pDevice->refCount))
    {
        pCSLState->numDeviceHandles--;
        CAMX_FREE(pDevice);
        pDevice = NULL;
        StoreHandle(reinterpret_cast<VOID**>(&pCSLState->pDevicesState[0]), hDevice, NULL, CSLMaxNumDeviceHandles);
    }
    else
    {
        CAMX_ASSERT((CSLInvalidHandle != hDevice) && (NULL != pDevice) && (0 == pDevice->refCount));
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// GetBufferInfo
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CSLBufferInfo* GetBufferInfo(
    CSLState*       pCSLState,
    CSLMemHandle    hMem)
{
    CSLBufferInfo* pBufferInfo = NULL;

    if ((CSLInvalidHandle != hMem) && (static_cast<UINT>(hMem) <= CSLMaxNumAllocs))
    {
        pBufferInfo = reinterpret_cast<CSLBufferInfo*>(GetFromHandle(reinterpret_cast<VOID**>(&pCSLState->pMemHandles[0]),
                                                          hMem,
                                                          CSLMaxNumAllocs));
    }
    else
    {
        CAMX_ASSERT((CSLInvalidHandle != hMem) && (static_cast<UINT>(hMem) <= CSLMaxNumAllocs));
    }

    return pBufferInfo;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ProcessFenceHandler
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static VOID* ProcessFenceHandler(
    VOID* pData)
{
    CSLFenceInfo* pFenceInfo = reinterpret_cast<CSLFenceInfo*>(pData);
    CAMX_ASSERT(NULL != pFenceInfo);

    if (NULL != pFenceInfo->fenceHandler)
    {
        pFenceInfo->fenceHandler(pFenceInfo->pHandlerData, pFenceInfo->hFence, pFenceInfo->fenceResult);
    }

    return NULL;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CSLInitializeCommon
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CSLInitializeCommon(
    CSLState*               pCSLState,
    UINT32                  stateSize,
    CSLDeviceDescriptor*    pDevices,
    VOID**                  ppCapabilities)
{
    CamxResult result = CamxResultSuccess;

    CAMX_ASSERT(NULL != pCSLState);

    // Protect against calling this more than once.
    if (GetCSLInitializedCommon() == TRUE)
    {
        result = CamxResultEFailed;
        CAMX_LOG_ERROR(CamxLogGroupCSL, "CSL is already initialized.")
    }
    else
    {
        CamX::Utils::Memset(pCSLState, 0, stateSize);
        pCSLState->CSLLock              = CamX::Mutex::Create("CSLState");
        pCSLState->sessionStateSize     = sizeof(CSLSession);
        pCSLState->deviceStateSize      = sizeof(CSLDeviceState);
        pCSLState->pDeviceEntries       = pDevices;
        pCSLState->ppDeviceCapabilities = ppCapabilities;
        pCSLState->initializationTime   = clock();

        result = CamX::ThreadManager::Create(&pCSLState->fenceHandlerThreadManager, "FenceHandlerThreadPool", 1);
        if (CamxResultSuccess == result)
        {
            result = pCSLState->fenceHandlerThreadManager->RegisterJobFamily(ProcessFenceHandler,
                                                                             "ProcessFenceHandler",
                                                                             NULL,
                                                                             CamX::JobPriority::Normal,
                                                                             TRUE,
                                                                             &pCSLState->hProcessFenceHandler);
        }
    }

    if (CamxResultSuccess != result)
    {
        g_initialized = FALSE;
    }
    else
    {
        g_initialized = TRUE;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CSLUninitializeCommon
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CSLUninitializeCommon(
    CSLState*   pCSLState)
{
    CamxResult result = CamxResultSuccess;

    CAMX_ASSERT(NULL != pCSLState);
    CAMX_ASSERT(NULL != pCSLState->fenceHandlerThreadManager);

    /// @note Since CSL uninitialize is call when HAL3 module is unloaded, by the time this line is hit,
    ///       program threads are already released which causes the synchronization logic in thread core
    ///       to block; hence, leaking the thread manager below.
    /// @todo (CAMX-972) Revisit this along with the overall static destruction order.
    // pCSLState->fenceHandlerThreadManager->Destroy();

    // Free up any unfreed fence object in the store
    for (UINT i = 0; i < pCSLState->fenceCount; i++)
    {
        // clients should've released the buffer!
        if (NULL != pCSLState->pFences[i])
        {
            CAMX_ASSERT_ALWAYS_MESSAGE("CSL hFence not released by client: handle: %d, vAddr: %p",
                                       i + 1, pCSLState->pFences[i]);

            CAMX_FREE(pCSLState->pFences[i]);
            pCSLState->pFences[i] = NULL;
        }
    }

    CamX::Utils::Memset(&pCSLState->pFences[0], 0, sizeof(pCSLState->pFences));

    // Free up any unfreed session object in the store
    for (UINT i = 0; i < pCSLState->numSessions; i++)
    {
        // clients should've released the buffer!
        if (NULL != pCSLState->pSessions[i])
        {
            // Stream-off and close the session if not done by client
            if (CSLSession::State::Opened == pCSLState->pSessions[i]->state)
            {
                CAMX_LOG_ERROR(CamxLogGroupCSL, "CSL session still open when CSLUninitialize called: handle: %d, vAddr: %p",
                               i + 1, pCSLState->pSessions[i]);

                if (TRUE == pCSLState->pSessions[i]->streamOn)
                {
                    CAMX_LOG_ERROR(CamxLogGroupCSL,
                                   "CSL session in stream-on mode when CSLUninitialize called: handle: %d, vAddr: %p",
                                   i + 1, pCSLState->pSessions[i]);
                    if (CamxResultSuccess == result)
                    {
                        result = CSLStreamOff(pCSLState->pSessions[i]->hSession, NULL, NULL, CSLDeativateModeDefault);
                        if (CamxResultSuccess != result)
                        {
                            CAMX_LOG_ERROR(CamxLogGroupCSL, "Stream off failed: handle: %d, vAddr: %p",
                                                            i + 1, pCSLState->pSessions[i]);
                        }
                    }
                    else
                    {
                        // do not overwrite the error result, but continue with Stream Off
                        if (CamxResultSuccess !=
                            CSLStreamOff(pCSLState->pSessions[i]->hSession, NULL, NULL, CSLDeativateModeDefault))
                        {
                            CAMX_LOG_ERROR(CamxLogGroupCSL, "Stream off failed: handle: %d, vAddr: %p",
                                                            i + 1, pCSLState->pSessions[i]);
                        }
                    }
                }
                CSLClose(pCSLState->pSessions[i]->hSession);
            }
            CAMX_FREE(pCSLState->pSessions[i]);
            pCSLState->pSessions[i] = NULL;
            CAMX_LOG_ERROR(CamxLogGroupCSL, "CSL session not released by client: handle: %d, vAddr: %p",
                           i + 1, pCSLState->pSessions[i]);
            CAMX_ASSERT_ALWAYS();
        }
    }

    CamX::Utils::Memset(&pCSLState->pSessions[0], 0, CSLMaxNumSessions * sizeof(pCSLState->pSessions[0]));

    // Free up any unfreed memory object in the store
    // It is expected that by the time Uninitialize is called, everything be released already so this is
    // mostly to allow catching driver bugs.
    for (UINT i = 0; i < pCSLState->memCount; i++)
    {
        // clients should've released the buffer!
        if (NULL != pCSLState->pMemHandles[i])
        {
            if (NULL != pCSLState->pMemHandles[i]->pVirtualAddr)
            {
                CAMX_LOG_ERROR(CamxLogGroupCSL, "CSL memory not released by client: handle: %d, vAddr: %p",
                               pCSLState->pMemHandles[i]->hHandle,
                               pCSLState->pMemHandles[i]->pVirtualAddr);
                CAMX_FREE(pCSLState->pMemHandles[i]->pVirtualAddr);
                pCSLState->pMemHandles[i]->pVirtualAddr = NULL;
                CAMX_ASSERT_ALWAYS();
            }

            CAMX_FREE(pCSLState->pMemHandles[i]);
            pCSLState->pMemHandles[i] = NULL;
        }
    }

    CamX::Utils::Memset(&pCSLState->pMemHandles[0], 0, CSLMaxNumAllocs * sizeof(pCSLState->pMemHandles[0]));

    if (NULL == pCSLState->CSLLock)
    {
        CAMX_LOG_ERROR(CamxLogGroupCSL, "CSL state lock has already been destroyed (or never created!)");
    }
    else
    {
        pCSLState->CSLLock->Destroy();
        pCSLState->CSLLock = NULL;
    }
    g_initialized = FALSE;

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CSLOpenCommon
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CSLOpenCommon(
    CSLState*   pCSLState,
    CSLHandle*  phCSL)
{
    CamxResult result = CamxResultSuccess;

    if (NULL != phCSL)
    {
        *phCSL = CSLInvalidHandle;

        if (GetCSLInitializedCommon() != TRUE)
        {
            result = CamxResultEInvalidState;
        }
        else if ((pCSLState->numSessions + 1) > CSLMaxNumSessions)
        {
            result = CamxResultENoMore;
            CAMX_LOG_ERROR(CamxLogGroupCSL, "No more sessions available.");
        }
        else
        {
            CSLSession* pSession = NULL;
            pCSLState->CSLLock->Lock();
            CSLHandle hSession = ConstructSession(pCSLState, &pSession);
            pCSLState->CSLLock->Unlock();
            CAMX_ASSERT((NULL == pSession) == (CSLInvalidHandle == hSession));

            if (NULL != pSession)
            {
                *phCSL             = hSession;
                pSession->refCount = 1;
            }
            else
            {
                result = CamxResultEFailed;
            }
        }
    }
    else
    {
        result = CamxResultEInvalidArg;
        CAMX_LOG_ERROR(CamxLogGroupCSL, "Invalid argument (NULL pointer).");
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CSLCloseCommon
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CSLCloseCommon(
    CSLState*   pCSLState,
    CSLHandle   hCSL)
{
    CamxResult result = CamxResultEFailed;

    if (GetCSLInitializedCommon() != TRUE)
    {
        result = CamxResultEInvalidState;
    }
    else
    {
        pCSLState->CSLLock->Lock();

        CSLSession* pSession = GetSession(pCSLState, hCSL);
        CAMX_ASSERT(NULL != pSession);

        if (NULL != pSession)
        {
            if (CSLSession::State::Opened != pSession->state)
            {
                result = CamxResultEInvalidArg;
                CAMX_LOG_ERROR(CamxLogGroupCSL, "Session is not open.");
            }
            else
            {
                /// @todo (CAMX-75) Decide if we should do an stream-off here to client must do it.
                ///       For now, assert that it's off.
                CAMX_ASSERT(FALSE == pSession->streamOn);
                CAMX_ASSERT(0 < pSession->refCount);

                if (0 < pSession->refCount)
                {
                    pSession->refCount--;
                    if (0 == pSession->refCount)
                    {
                        DestructSession(pCSLState, hCSL, pSession);
                    }
                    result = CamxResultSuccess;
                }
            }
        }
        pCSLState->CSLLock->Unlock();
    }
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CSLAddReferenceCommon
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CSLAddReferenceCommon(
    CSLState*   pCSLState,
    CSLHandle   hCSL)
{
    CamxResult result = CamxResultEFailed;

    if (GetCSLInitializedCommon() != TRUE)
    {
        result = CamxResultEInvalidState;
    }
    else
    {
        pCSLState->CSLLock->Lock();

        CSLSession* pSession = GetSession(pCSLState, hCSL);
        CAMX_ASSERT(NULL != pSession);

        if (NULL != pSession)
        {
            if (CSLSession::State::Opened != pSession->state)
            {
                result = CamxResultEInvalidArg;
                CAMX_LOG_ERROR(CamxLogGroupCSL, "Session is not open.");
            }
            else
            {
                pSession->refCount++;
                result = CamxResultSuccess;
            }
        }
        pCSLState->CSLLock->Unlock();
    }
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CSLQueryCameraPlatformCommon
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CSLQueryCameraPlatformCommon(
    CSLState*           pCSLState,
    CSLCameraPlatform*  pCameraPlatform)
{
    CAMX_UNREFERENCED_PARAM(pCSLState);

    CamxResult result = CamxResultEFailed;

    if (GetCSLInitializedCommon() != TRUE)
    {
        result = CamxResultEInvalidState;
    }
    else if (NULL == pCameraPlatform)
    {
        result = CamxResultEInvalidArg;
    }
    else
    {
        // Hardcoded to Titan camera (Napali v2 Camera Titan : Platform 1.7.0, cpas 1.1.0)
        pCameraPlatform->family                         = CSLCameraFamilyTitan;
        pCameraPlatform->platformVersion.majorVersion   = 1;
        pCameraPlatform->platformVersion.minorVersion   = 7;
        pCameraPlatform->platformVersion.revVersion     = 0;
        pCameraPlatform->CPASVersion.majorVersion       = 1;
        pCameraPlatform->CPASVersion.minorVersion       = 1;
        pCameraPlatform->CPASVersion.revVersion         = 0;
        result = CamxResultSuccess;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CSLImageSensorProbeCommon
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CSLImageSensorProbeCommon(
    CSLState*                   pCSLState,
    CSLMemHandle                hPacket,
    SIZE_T                      offset,
    CSLImageSensorProbeResult*  pProbeResult)
{
    CamxResult result = CamxResultEInvalidArg;

    CAMX_UNREFERENCED_PARAM(offset);
    CAMX_UNREFERENCED_PARAM(pCSLState);

    if ((NULL != pProbeResult) && (CSLInvalidHandle != hPacket))
    {
        pProbeResult->detected = TRUE;
        // For now device index CSLSensorDeviceIndex is the image sensor.
        // Real implementation should assign a unique index for all successfully probed cameras.
        pProbeResult->deviceIndex = CSLSensorDeviceIndex;
        result = CamxResultSuccess;
    }
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CSLSetDebugBufferCommon
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CSLSetDebugBufferCommon(
    CSLState*               pCSLState,
    CSLHandle               hCSL,
    CSLDebugBufferType      type,
    CSLMemHandle            hBuffer,
    SIZE_T                  offset,
    SIZE_T                  length,
    CSLDebugBufferResult*   pDebugBufferResult)
{
    CamxResult  result    = CamxResultSuccess;

    CAMX_ASSERT(NULL != pCSLState);
    CAMX_ASSERT(NULL != pCSLState->CSLLock);
    pCSLState->CSLLock->Lock();

    CSLSession* pSession = GetSession(pCSLState, hCSL);
    CAMX_ASSERT(NULL != pSession);

    if (NULL != pSession)
    {
        if (NULL != pDebugBufferResult)
        {
            if (CSLDebugBufferTypeRegDump == type)
            {
                pDebugBufferResult->hHandle         = pSession->regDumpDebugBuffer.hRegDumpBuffer;
                pDebugBufferResult->bytesWritten    = pSession->regDumpDebugBuffer.bytesWritten;
                pDebugBufferResult->offset          = pSession->regDumpDebugBuffer.offsetRegDumpBuffer;
            }
            else
            {
                pDebugBufferResult->hHandle         = CSLInvalidHandle;
            }

        }

        if (CSLDebugBufferTypeRegDump == type)
        {
            pSession->regDumpDebugBuffer.type                   = type;
            pSession->regDumpDebugBuffer.hRegDumpBuffer         = hBuffer;
            pSession->regDumpDebugBuffer.offsetRegDumpBuffer    = offset;
            pSession->regDumpDebugBuffer.lengthRegDumpBuffer    = length;
            pSession->regDumpDebugBuffer.bytesWritten           = 0;
        }
    }
    else
    {
        result = CamxResultEInvalidPointer;
        CAMX_LOG_ERROR(CamxLogGroupCSL, "hCSL: %d, pSession is NULL", hCSL);
    }
    pCSLState->CSLLock->Unlock();

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CSLEnumerateDevicesCommon
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CSLEnumerateDevicesCommon(
    CSLState*               pCSLState,
    CSLDeviceDescriptor*    pDeviceDescriptor)
{
    CAMX_UNREFERENCED_PARAM(pCSLState);

    CamxResult result = CamxResultEFailed;

    if (NULL == pDeviceDescriptor)
    {
        result = CamxResultEInvalidArg;
        CAMX_LOG_ERROR(CamxLogGroupCSL, "Invalid argument (NULL pointer).");
    }
    else if (pDeviceDescriptor->deviceIndex < 0)
    {
        result = CamxResultEInvalidArg;
        CAMX_LOG_ERROR(CamxLogGroupCSL, "Invalid argument (device index is negative).");
    }
    else if (static_cast<UINT32>(pDeviceDescriptor->deviceIndex) >= CSLMaxNumDevices)
    {
        result = CamxResultENoMore;
        CAMX_LOG_ERROR(CamxLogGroupCSL, "No more devices available (index: %d).", pDeviceDescriptor->deviceIndex);
    }
    else
    {
        pDeviceDescriptor->deviceType       = pCSLState->pDeviceEntries[pDeviceDescriptor->deviceIndex].deviceType;
        pDeviceDescriptor->driverVersion    = pCSLState->pDeviceEntries[pDeviceDescriptor->deviceIndex].driverVersion;
        pDeviceDescriptor->hwVersion        = pCSLState->pDeviceEntries[pDeviceDescriptor->deviceIndex].hwVersion;
        result                              = CamxResultSuccess;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CSLQueryDeviceCapabilitiesCommon
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CSLQueryDeviceCapabilitiesCommon(
    CSLState*   pCSLState,
    INT32       deviceIndex,
    VOID*       pDeviceData,
    SIZE_T      deviceDataSize)
{
    CamxResult result = CamxResultSuccess;

    if ((NULL == pDeviceData) || (deviceIndex < 0) || (static_cast<UINT32>(deviceIndex) >= CSLMaxNumDevices))
    {
        result = CamxResultEInvalidArg;
        CAMX_LOG_ERROR(CamxLogGroupCSL, "Invalid arguments.");
    }
    else
    {
        if (NULL != pCSLState->ppDeviceCapabilities[deviceIndex])
        {
            // In real implementation, must verify the deviceDataSize is the same as the pCaps size.
            CamX::Utils::Memcpy(pDeviceData, pCSLState->ppDeviceCapabilities[deviceIndex], deviceDataSize);
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CSLAcquireDeviceCommon
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CSLAcquireDeviceCommon(
    CSLState*           pCSLState,
    CSLHandle           hCSL,
    CSLDeviceHandle*    phDevice,
    INT32               deviceIndex,
    CSLDeviceResource*  pDeviceResourceRequest,
    SIZE_T              numDeviceResources,
    CSLDeviceAttribute* pDeviceAttribute,
    SIZE_T              numDeviceAttributes)
{
    CamxResult result       = CamxResultSuccess;

    CAMX_UNREFERENCED_PARAM(pDeviceAttribute);
    CAMX_UNREFERENCED_PARAM(numDeviceAttributes);

    if (GetCSLInitializedCommon() != TRUE)
    {
        result = CamxResultEInvalidState;
        CAMX_LOG_ERROR(CamxLogGroupCSL, "Invalid state");
    }
    else
    {
        pCSLState->CSLLock->Lock();

        CSLSession* pSession = GetSession(pCSLState, hCSL);
        CAMX_ASSERT(NULL != pSession);

        if (NULL == pSession)
        {
            result = CamxResultEInvalidPointer;
            CAMX_LOG_ERROR(CamxLogGroupCSL, "hCSL: %d, pSession is NULL", hCSL);
        }
        else if ((deviceIndex < 0) ||
            (static_cast<UINT32>(deviceIndex) >= CSLMaxNumDevices))
        {
            result = CamxResultEInvalidArg;
            CAMX_LOG_ERROR(CamxLogGroupCSL, "Invalid arguments.");
        }
        else if (CSLSession::State::Opened != pSession->state)
        {
            result = CamxResultEInvalidState;
            CAMX_LOG_ERROR(CamxLogGroupCSL, "Session not open.");
        }
        else
        {
            CSLDeviceState* pDevice = NULL;
            CSLDeviceHandle hDevice = ConstructDevice(pCSLState, &pDevice);

            CAMX_ASSERT((NULL == pDevice) == (CSLInvalidHandle == hDevice));

            if (NULL != pDevice)
            {
                pDevice->type = pCSLState->pDeviceEntries[deviceIndex].deviceType;
                pDevice->index = deviceIndex;
                // Currently, allow all devices to be shared. Assume infinite resources on all devices.
                pDevice->shareable = TRUE;

                if (FALSE == pSession->acquired[deviceIndex])
                {
                    // If some other session has acquired this device before, fail if the device is not shareable
                    if ((pDevice->refCount > 0) && (FALSE == pDevice->shareable))
                    {
                        result = CamxResultEBusy;
                        CAMX_LOG_ERROR(CamxLogGroupCSL,
                                       "Device (type: %d, index: %d) is acquired by other session and is not shareable.",
                                       pDevice->type,
                                       pDevice->index);
                    }
                }

                // In real implementation, we should check the resource Ids to make sure we can
                // they are available. Here though we assume the right combination is asked.

                // If everything is available, then set acquire them.
                if (CamxResultSuccess == result)
                {
                    pSession->acquired[deviceIndex] = TRUE;
                    pDevice->refCount++;
                    for (UINT j = 0; j < numDeviceResources; j++)
                    {
                        pDevice->resourcesAcquired[pDeviceResourceRequest[j].resourceID] = TRUE;
                    }

                    *phDevice                               = hDevice;
                    pCSLState->pDevicesState[hDevice - 1]   = pDevice;
                    pDevice->pSession                       = pSession;
                }
                else
                {
                    *phDevice = CSLInvalidHandle;
                    DestructDevice(pCSLState, hDevice, pDevice);
                }
            }
            else
            {
                result = CamxResultENoMemory;
            }
        }
        pCSLState->CSLLock->Unlock();
    }
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CSLReleaseDeviceCommon
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CSLReleaseDeviceCommon(
    CSLState*       pCSLState,
    CSLHandle       hCSL,
    CSLDeviceHandle hDevice)
{
    CamxResult result = CamxResultSuccess;

    CAMX_ASSERT((CSLInvalidHandle != hCSL) && (CSLInvalidHandle != hDevice));

    if (GetCSLInitializedCommon() != TRUE)
    {
        result = CamxResultEInvalidState;
        CAMX_LOG_ERROR(CamxLogGroupCSL, "Invalid state");
    }
    else
    {
        pCSLState->CSLLock->Lock();

        CSLSession*     pSession = GetSession(pCSLState, hCSL);
        CSLDeviceState* pDevice  = GetDeviceState(pCSLState, hDevice);

        CAMX_ASSERT((NULL != pSession) && (NULL != pDevice));

        if ((NULL == pSession) || (NULL == pDevice))
        {
            result = CamxResultEInvalidPointer;
            CAMX_LOG_ERROR(CamxLogGroupCSL, "hCSL: %d, hDevice: %d, pSession: %p, pDevice: %p",
                            hCSL, hDevice, pSession, pDevice);
        }
        else if (CSLSession::State::Opened != pSession->state)
        {
            result = CamxResultEInvalidState;
            CAMX_LOG_ERROR(CamxLogGroupCSL, "Session not open.");
        }
        else if (FALSE == pSession->acquired[pDevice->index])
        {
            result = CamxResultEInvalidArg;
            CAMX_LOG_ERROR(CamxLogGroupCSL, "Device not acquired before.");
        }
        else
        {
            CAMX_ASSERT(pDevice->refCount > 0);

            for (UINT j = 0; j < pDevice->numResources; j++)
            {
                // If the resource is acquired by the current session
                if (TRUE == pDevice->resourcesAcquired[j])
                {
                    pDevice->resourcesAcquired[j] = FALSE;
                }
            }

            pDevice->refCount--;
            pSession->acquired[pDevice->index] = FALSE;
            pCSLState->pDevicesState[hDevice - 1] = NULL;
            DestructDevice(pCSLState, hDevice, pDevice);
        }
        pCSLState->CSLLock->Unlock();
    }
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CSLAcquireHardwareCommon
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CSLAcquireHardwareCommon(
    CSLState*           pCSLState,
    CSLHandle           hCSL,
    CSLDeviceHandle     hDevice,
    CSLDeviceResource*  pDeviceResourceRequest)
{
    CamxResult result = CamxResultSuccess;
    CAMX_UNREFERENCED_PARAM(pDeviceResourceRequest);

    CAMX_ASSERT((CSLInvalidHandle != hCSL) && (CSLInvalidHandle != hDevice));

    if (GetCSLInitializedCommon() != TRUE)
    {
        result = CamxResultEInvalidState;
        CAMX_LOG_ERROR(CamxLogGroupCSL, "Invalid state");
    }
    else
    {
        pCSLState->CSLLock->Lock();

        CSLSession*     pSession = GetSession(pCSLState, hCSL);
        CSLDeviceState* pDevice  = GetDeviceState(pCSLState, hDevice);

        CAMX_ASSERT((NULL != pSession) && (NULL != pDevice));

        if ((NULL != pSession) && (NULL != pDevice))
        {
            if (CSLSession::State::Opened != pSession->state)
            {
                result = CamxResultEInvalidState;
                CAMX_LOG_ERROR(CamxLogGroupCSL, "Session not open.");
            }
            else if (FALSE == pSession->acquired[pDevice->index])
            {
                result = CamxResultEInvalidArg;
                CAMX_LOG_ERROR(CamxLogGroupCSL, "Device not acquired before.");
            }
            else
            {
                pSession->acquiredHardware[pDevice->index] = TRUE;
            }
        }

        pCSLState->CSLLock->Unlock();
    }
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CSLReleaseHardwareCommon
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CSLReleaseHardwareCommon(
    CSLState*       pCSLState,
    CSLHandle       hCSL,
    CSLDeviceHandle hDevice)
{
    CamxResult result = CamxResultSuccess;

    CAMX_ASSERT((CSLInvalidHandle != hCSL) && (CSLInvalidHandle != hDevice));

    if (GetCSLInitializedCommon() != TRUE)
    {
        result = CamxResultEInvalidState;
        CAMX_LOG_ERROR(CamxLogGroupCSL, "Invalid state");
    }
    else
    {
        pCSLState->CSLLock->Lock();

        CSLSession*     pSession = GetSession(pCSLState, hCSL);
        CSLDeviceState* pDevice  = GetDeviceState(pCSLState, hDevice);

        CAMX_ASSERT((NULL != pSession) && (NULL != pDevice));

        if ((NULL == pSession) || (NULL == pDevice))
        {
            result = CamxResultEInvalidPointer;
            CAMX_LOG_ERROR(CamxLogGroupCSL, "hCSL: %d, hDevice: %d, pSession: %p, pDevice: %p",
                            hCSL, hDevice, pSession, pDevice);
        }
        else if (CSLSession::State::Opened != pSession->state)
        {
            result = CamxResultEInvalidState;
            CAMX_LOG_ERROR(CamxLogGroupCSL, "Session not open.");
        }
        else if (FALSE == pSession->acquired[pDevice->index])
        {
            result = CamxResultEInvalidArg;
            CAMX_LOG_ERROR(CamxLogGroupCSL, "Device not acquired before.");
        }
        else if (FALSE == pSession->acquiredHardware[pDevice->index])
        {
            result = CamxResultEInvalidArg;
            CAMX_LOG_ERROR(CamxLogGroupCSL, "Hardware not acquired before.");
        }
        else
        {
            pSession->acquiredHardware[pDevice->index] = FALSE;
        }
        pCSLState->CSLLock->Unlock();
    }
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CSLStreamOnCommon
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CSLStreamOnCommon(
    CSLState* pCSLState,
    CSLHandle hCSL)
{
    CamxResult result = CamxResultSuccess;

    CAMX_ASSERT(CSLInvalidHandle != hCSL);

    if (GetCSLInitializedCommon() != TRUE)
    {
        result = CamxResultEInvalidState;
    }
    else
    {
        pCSLState->CSLLock->Lock();
        CSLSession* pSession = GetSession(pCSLState, hCSL);
        CAMX_ASSERT(NULL != pSession);

        if (NULL != pSession)
        {
            pSession->streamOn = TRUE;
        }
        else
        {
            result = CamxResultEInvalidPointer;
            CAMX_LOG_ERROR(CamxLogGroupCSL, "hCSL: %d, pSession is NULL", hCSL);
        }
        pCSLState->CSLLock->Unlock();
    }
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CSLStreamOffCommon
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CSLStreamOffCommon(
    CSLState*   pCSLState,
    CSLHandle   hCSL)
{
    CamxResult result = CamxResultSuccess;

    CAMX_ASSERT(CSLInvalidHandle != hCSL);

    if (GetCSLInitializedCommon() != TRUE)
    {
        result = CamxResultEInvalidState;
    }
    else
    {
        pCSLState->CSLLock->Lock();
        CSLSession* pSession = GetSession(pCSLState, hCSL);
        CAMX_ASSERT(NULL != pSession);

        if (NULL != pSession)
        {
            pSession->streamOn = FALSE;
        }
        else
        {
            result = CamxResultEInvalidPointer;
            CAMX_LOG_ERROR(CamxLogGroupCSL, "hCSL: %d, pSession is NULL", hCSL);
        }
        pCSLState->CSLLock->Unlock();
    }
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CSLRegisterMessageHandlerCommon
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CSLRegisterMessageHandlerCommon(
    CSLState*           pCSLState,
    CSLHandle           hCSL,
    CSLMessageHandler   messageHandler,
    VOID*               pUserData)
{
    CamxResult result = CamxResultSuccess;

    CAMX_ASSERT(CSLInvalidHandle != hCSL);

    if (GetCSLInitializedCommon() != TRUE)
    {
        result = CamxResultEInvalidState;
    }
    else if (NULL == messageHandler)
    {
        result = CamxResultEInvalidArg;
        CAMX_LOG_ERROR(CamxLogGroupCSL, "Invalid arguments: NULL == pMessageHandler");
    }
    else
    {
        pCSLState->CSLLock->Lock();
        CSLSession* pSession = GetSession(pCSLState, hCSL);

        CAMX_ASSERT(NULL != pSession);

        if (NULL != pSession)
        {
            if (CSLSession::State::Opened != pSession->state)
            {
                result = CamxResultEInvalidState;
                CAMX_LOG_ERROR(CamxLogGroupCSL, "Session not open.");
            }
            else
            {
                pSession->pMessageHandlerUserData = pUserData;
                pSession->messageHandler = messageHandler;
            }
        }
        else
        {
            CAMX_LOG_ERROR(CamxLogGroupCSL, "pSession is NULL (should not have happened!)");
            result = CamxResultEFailed;
        }

        pCSLState->CSLLock->Unlock();
    }
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CSLCreatePrivateFenceCommon
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CSLCreatePrivateFenceCommon(
    CSLState*   pCSLState,
    const CHAR* pName,
    CSLFence*   phFenceOut)
{
    CamxResult      result      = CamxResultSuccess;
    CSLFenceInfo*   pFenceInfo  = NULL;
    CSLHandle       hFence      = CSLInvalidHandle;

    if (NULL == phFenceOut)
    {
        result = CamxResultEInvalidArg;
        CAMX_LOG_ERROR(CamxLogGroupCSL, "phFenceOut is NULL");
    }
    else if (GetCSLInitializedCommon() == TRUE)
    {
        pCSLState->CSLLock->Lock();
        hFence = CreateFence(pCSLState, &pFenceInfo);
        pCSLState->CSLLock->Unlock();
        // Either both must be invalid or both valid.
        if ((CSLInvalidHandle != hFence) && (NULL != pFenceInfo))
        {
            pFenceInfo->pName       = pName;
            pFenceInfo->type        = CSLFenceInfo::Type::Private;
            pFenceInfo->pCondition  = CamX::Condition::Create("CSLFence");
            pFenceInfo->pMutex      = CamX::Mutex::Create("CSLFence");
            pFenceInfo->status      = CSLFenceInfo::Status::Unsignaled;
            pFenceInfo->fenceResult = CSLFenceResultSuccess;
            pFenceInfo->refCount    = 1;
        }
        else
        {
            CAMX_LOG_ERROR(CamxLogGroupCSL, "Failed to create hFence.");
            result = CamxResultEFailed;
        }
    }

    if (NULL != phFenceOut)
    {
        *phFenceOut = hFence;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ShouldSignalMerged
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static BOOL ShouldSignalMerged(
    CSLState*       pCSLState,
    CSLFenceInfo*   pFenceInfo)
{
    BOOL result = TRUE;

    CAMX_ASSERT(NULL != pCSLState);
    CAMX_ASSERT(NULL != pFenceInfo);

    pFenceInfo->pMutex->Lock();
    for (UINT i = 0; i < pFenceInfo->fenceCount; i++)
    {
        CSLFenceInfo* pChildFenceInfo = GetFenceInfo(pCSLState, pFenceInfo->phFences[i]);
        CAMX_ASSERT(NULL != pChildFenceInfo);

        if (CSLFenceInfo::Status::Signaled != pChildFenceInfo->status)
        {
            result = FALSE;
            break;
        }
    }
    pFenceInfo->pMutex->Unlock();

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// SignalFence
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static VOID SignalFence(
    CSLState*       pCSLState,
    CSLFenceInfo*   pFenceInfo,
    CSLFenceResult  fenceResult)
{
    CAMX_ASSERT(NULL != pCSLState);
    CAMX_ASSERT(NULL != pFenceInfo);
    CAMX_ASSERT((NULL != pFenceInfo->pCondition) && (NULL != pFenceInfo->pMutex));

    pFenceInfo->pMutex->Lock();
    pFenceInfo->status = CSLFenceInfo::Status::Signaled;
    pFenceInfo->fenceResult = fenceResult;
    pFenceInfo->pCondition->Broadcast();
    pFenceInfo->pMutex->Unlock();

    CAMX_ASSERT(NULL != pCSLState->fenceHandlerThreadManager);
    CAMX_ASSERT(0 != pCSLState->hProcessFenceHandler);

    pCSLState->fenceHandlerThreadManager->PostJob(pCSLState->hProcessFenceHandler,
                                                  NULL,
                                                  reinterpret_cast<VOID**>(&pFenceInfo),
                                                  FALSE,
                                                  FALSE);

    if (NULL != pFenceInfo->pParent)
    {
        CAMX_ASSERT(CSLFenceInfo::Status::Signaled != pFenceInfo->pParent->status);

        if (TRUE == ShouldSignalMerged(pCSLState, pFenceInfo->pParent))
        {
            SignalFence(pCSLState, pFenceInfo->pParent, fenceResult);
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CSLMergeFencesCommon
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CSLMergeFencesCommon(
    CSLState*   pCSLState,
    CSLFence*   phFences,
    SIZE_T      fenceCount,
    CSLFence*   phFenceOut)
{
    CamxResult      result      = CamxResultSuccess;
    CSLFenceInfo*   pFenceInfo  = NULL;
    CSLHandle       hFence      = CSLInvalidHandle;

    if (GetCSLInitializedCommon() != TRUE)
    {
        result = CamxResultEInvalidState;
    }
    else if ((NULL == phFences) || (0 == fenceCount) || (NULL == phFenceOut))
    {
        result = CamxResultEInvalidArg;
        CAMX_LOG_ERROR(CamxLogGroupCSL, "Invalid arguments.");
    }
    else
    {
        result = CSLCreatePrivateFenceCommon(pCSLState, "composite", &hFence);
        CAMX_ASSERT(CamxResultSuccess == result);
        pFenceInfo = GetFenceInfo(pCSLState, hFence);
        // Either both must be invalid or both valid.
        CAMX_ASSERT((CSLInvalidHandle == hFence) == (NULL == pFenceInfo));
        if (NULL != pFenceInfo)
        {
            pFenceInfo->fenceCount = static_cast<UINT>(fenceCount);
            pFenceInfo->composite = TRUE;
            pFenceInfo->phFences = static_cast<CSLFence*>(CAMX_CALLOC(fenceCount * sizeof(CSLFence)));

            if (NULL != pFenceInfo->phFences)
            {
                for (UINT i = 0; i < fenceCount; i++)
                {
                    pFenceInfo->phFences[i] = phFences[i];
                    CSLFenceInfo* pChildFence = GetFenceInfo(pCSLState, phFences[i]);
                    if ((NULL != pChildFence) && (0 != pChildFence->refCount))
                    {
                        // Currently only support fence to be part of one merge
                        CAMX_ASSERT(NULL == pChildFence->pParent);
                        pChildFence->pParent = pFenceInfo;

                        CamX::CamxAtomicIncU(&pChildFence->refCount);
                    }
                    else
                    {
                        result = CamxResultEInvalidPointer;
                        CAMX_LOG_ERROR(CamxLogGroupCSL, "Invalid pChildFence");
                    }
                }
            }
            else
            {
                result = CamxResultENoMemory;
                CAMX_LOG_ERROR(CamxLogGroupCSL, "Out of memory (CAMX_CALLOC failed).");
            }
        }
        else
        {
            result = CamxResultENoMemory;
            CAMX_LOG_ERROR(CamxLogGroupCSL, "Out of memory (CAMX_CALLOC failed).");
        }
    }

    if (CamxResultSuccess != result)
    {
        if (NULL != pFenceInfo)
        {
            if (NULL != pFenceInfo->phFences)
            {
                CAMX_FREE(pFenceInfo->phFences);
                pFenceInfo->phFences = NULL;
            }
            CAMX_FREE(pFenceInfo);
            pFenceInfo = NULL;
        }
        hFence = CSLInvalidHandle;
    }

    if (NULL != phFenceOut)
    {
        if ((NULL != pFenceInfo) && (TRUE == ShouldSignalMerged(pCSLState, pFenceInfo)))
        {
            pFenceInfo->pMutex->Lock();
            pFenceInfo->status = CSLFenceInfo::Status::Signaled;
            pFenceInfo->fenceResult = CSLFenceResult::CSLFenceResultSuccess;
            pFenceInfo->pMutex->Unlock();
        }
        *phFenceOut = hFence;
    }
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CSLFenceWaitCommon
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CSLFenceWaitCommon(
    CSLState*   pCSLState,
    CSLFence    hFence,
    UINT64      timeout)
{
    CamxResult result = CamxResultSuccess;

    if (GetCSLInitializedCommon() != TRUE)
    {
        result = CamxResultEInvalidState;
    }
    else if (CSLInvalidHandle == hFence)
    {
        result = CamxResultEInvalidArg;
        CAMX_LOG_ERROR(CamxLogGroupCSL, "Invalid argument (hFence handle).");
    }
    else
    {
        CSLFenceInfo* pTopFenceInfo = GetFenceInfo(pCSLState, hFence);
        CAMX_ASSERT(NULL != pTopFenceInfo);

        CamX::LightweightDoublyLinkedList* pFences = CAMX_NEW CamX::LightweightDoublyLinkedList();

        if (NULL != pFences)
        {
            CamX::LDLLNode* pNode = static_cast<CamX::LDLLNode*>(CAMX_CALLOC(sizeof(CamX::LDLLNode)));

            if (NULL != pNode)
            {
                pNode->pData = pTopFenceInfo;
                pFences->InsertToHead(pNode);
            }
            else
            {
                result = CamxResultENoMemory;
            }

            while ((CamxResultSuccess == result) && (0 != pFences->NumNodes()))
            {
                pNode = pFences->RemoveFromHead();

                CSLFenceInfo* pFenceInfo = reinterpret_cast<CSLFenceInfo*>(pNode->pData);

                CAMX_FREE(pNode);
                pNode = NULL;

                if (CSLFenceInfo::Type::Private == pFenceInfo->type)
                {
                    if (pFenceInfo->composite)
                    {
                        if (CSLFenceInfo::Status::Signaled != pFenceInfo->status)
                        {
                            CAMX_ASSERT(NULL != pFenceInfo->phFences);

                            for (UINT i = 0; i < pFenceInfo->fenceCount; i++)
                            {
                                CSLFenceInfo* pTempFenceInfo = GetFenceInfo(pCSLState, pFenceInfo->phFences[i]);
                                CAMX_ASSERT(NULL != pTempFenceInfo);
                                CamX::LDLLNode* pTempNode = static_cast<CamX::LDLLNode*>(CAMX_CALLOC(sizeof(CamX::LDLLNode)));

                                if (NULL != pTempNode)
                                {
                                    pTempNode->pData = pTempFenceInfo;

                                    pFences->InsertToHead(pTempNode);
                                }
                            }
                        }
                    }
                    else
                    {
                        CAMX_ASSERT((NULL != pFenceInfo->pCondition) && (NULL != pFenceInfo->pMutex));

                        pFenceInfo->pMutex->Lock();
                        if (CSLFenceInfo::Status::Signaled != pFenceInfo->status)
                        {
                            result = pFenceInfo->pCondition->TimedWait(pFenceInfo->pMutex->GetNativeHandle(),
                                static_cast<UINT>(timeout));

                            // Regardless of what happened in the wait, if the condition is signaled, then it's a success!
                            if (CSLFenceInfo::Status::Signaled == pFenceInfo->status)
                            {
                                result = CamxResultSuccess;
                            }
                            else
                            {
                                CAMX_LOG_ERROR(CamxLogGroupCSL,
                                    "Condition::TimedWait timed out, failed or ended unexpectedly: %d",
                                    result);
                            }
                        }
                        pFenceInfo->pMutex->Unlock();
                    }
                }
                else
                {
                    result = CamxResultEFailed;
                    CAMX_NOT_IMPLEMENTED();
                }
            } // end of while

            if (CamxResultSuccess == result)
            {
                pTopFenceInfo->status = CSLFenceInfo::Status::Signaled;
            }
            else
            {
                pNode = pFences->RemoveFromHead();
                while (NULL != pNode)
                {
                    CAMX_FREE(pNode);
                    pNode = pFences->RemoveFromHead();
                }
            }

            CAMX_DELETE pFences;
            pFences = NULL;
        }
    }
    return result;
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CSLFenceAsyncWaitCommon
/// @note Currently only supports one handler/data per fence
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CSLFenceAsyncWaitCommon(
    CSLState*       pCSLState,
    CSLFence        hFence,
    CSLFenceHandler handler,
    VOID*           pUserData)
{
    CamxResult result = CamxResultSuccess;

    if (GetCSLInitializedCommon() != TRUE)
    {
        result = CamxResultEInvalidState;
    }
    else if ((CSLInvalidHandle == hFence) || (NULL == handler))
    {
        result = CamxResultEInvalidArg;
        CAMX_LOG_ERROR(CamxLogGroupCSL, "Invalid arguments.");
    }
    else
    {
        CSLFenceInfo* pFenceInfo = GetFenceInfo(pCSLState, hFence);

        if (pFenceInfo != NULL)
        {
            BOOL alreadySignaled = FALSE;

            pFenceInfo->pMutex->Lock();
            pFenceInfo->fenceHandler = handler;
            pFenceInfo->pHandlerData = pUserData;
            if (CSLFenceInfo::Status::Signaled == pFenceInfo->status)
            {
                alreadySignaled = TRUE;
            }
            pFenceInfo->pMutex->Unlock();

            if ((CamxResultSuccess == result) && (TRUE == alreadySignaled))
            {
                handler(pUserData, hFence, pFenceInfo->fenceResult);
            }
        }
        else
        {
            result = CamxResultEInvalidArg;
            CAMX_LOG_ERROR(CamxLogGroupCSL, "Invalid arguments.");

        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CSLFenceAsyncCancelCommon
/// @note Currently only supports one handler/data per fence
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CSLFenceAsyncCancelCommon(
    CSLState*       pCSLState,
    CSLFence        hFence,
    CSLFenceHandler handler,
    VOID*           pUserData)
{
    CamxResult result = CamxResultSuccess;

    if (GetCSLInitializedCommon() != TRUE)
    {
        result = CamxResultEInvalidState;
    }
    else if (CSLInvalidHandle == hFence)
    {
        result = CamxResultEInvalidArg;
        CAMX_LOG_ERROR(CamxLogGroupCSL, "Invalid arguments.");
    }
    else
    {
        CSLFenceInfo* pFenceInfo = GetFenceInfo(pCSLState, hFence);

        if (NULL != pFenceInfo)
        {
            // Currently, IFH only supports one handler/data per fence.
            CAMX_ASSERT(pFenceInfo->fenceHandler == handler);
            CAMX_ASSERT(pFenceInfo->pHandlerData == pUserData);

            pFenceInfo->fenceHandler = NULL;
            pFenceInfo->pHandlerData = NULL;
        }
        else
        {
            result = CamxResultEInvalidArg;
            CAMX_LOG_ERROR(CamxLogGroupCSL, "Invalid arguments.");
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CSLFenceSignalCommon
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CSLFenceSignalCommon(
    CSLState*       pCSLState,
    CSLFence        hFence,
    CSLFenceResult  fenceResult)
{
    CamxResult result = CamxResultSuccess;

    if (GetCSLInitializedCommon() != TRUE)
    {
        result = CamxResultEInvalidState;
    }
    else if (CSLInvalidHandle == hFence)
    {
        result = CamxResultEInvalidArg;
        CAMX_LOG_ERROR(CamxLogGroupCSL, "Invalid arguments.");
    }
    else
    {
        CSLFenceInfo* pFenceInfo = GetFenceInfo(pCSLState, hFence);
        if (NULL != pFenceInfo)
        {
            if (CSLFenceInfo::Status::Signaled != pFenceInfo->status)
            {
                if (CSLFenceInfo::Type::Private == pFenceInfo->type)
                {
                    if (TRUE == pFenceInfo->composite)
                    {
                        // Merged fences are not supposed to be signaled directly but are signaled automatically when all their
                        // child fences signal.
                        result = CamxResultEUnsupported;
                        CAMX_LOG_ERROR(CamxLogGroupCSL, "Invalid operation: merged fences cannot be signaled directly.");
                    }
                    else
                    {
                        SignalFence(pCSLState, pFenceInfo, fenceResult);
                    }
                }
                else
                {
                    result = CamxResultEFailed;
                    CAMX_NOT_IMPLEMENTED();
                }
            }
        }
        else
        {
            result = CamxResultEInvalidArg;
            CAMX_LOG_ERROR(CamxLogGroupCSL, "Invalid arguments.");
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CSLReleaseFenceCommon
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CSLReleaseFenceCommon(
    CSLState*   pCSLState,
    CSLFence    hFence)
{
    CamxResult result = CamxResultSuccess;

    if (GetCSLInitializedCommon() != TRUE)
    {
        result = CamxResultEInvalidState;
    }
    else if (CSLInvalidHandle == hFence)
    {
        result = CamxResultEInvalidArg;
        CAMX_LOG_ERROR(CamxLogGroupCSL, "Invalid arguments.");
    }
    else
    {
        // Lock the fence list lock instead of individual fence.
        CSLFenceInfo* pFenceInfo = GetFenceInfo(pCSLState, hFence);
        if (NULL != pFenceInfo)
        {
            CAMX_ASSERT(0 != pFenceInfo->refCount);

            if (0 == CamX::CamxAtomicDecU(&pFenceInfo->refCount))
            {
                DestroyFence(pCSLState, hFence);
            }
        }
        else
        {
            result = CamxResultEInvalidArg;
            CAMX_LOG_ERROR(CamxLogGroupCSL, "Invalid arguments.");
        }
    }
    return result;
}
