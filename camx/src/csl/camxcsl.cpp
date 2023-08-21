////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2016-2020 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  camxcsl.cpp
/// @brief CamxCSL Implementation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "camxcsl.h"
#include "camxcsljumptable.h"
#include "camxincs.h"
#include "camxmem.h"
#include "camxpacketdefs.h"
#include "camxhwenvironment.h"
#include "chi.h"
#include "g_camxsettings.h"

CAMX_STATIC_ASSERT_MESSAGE((sizeof(CSLPacket)                 % 8) == 0, "CSLPacket 64b-aligned");
CAMX_STATIC_ASSERT_MESSAGE((sizeof(CSLPacketHeader)           % 8) == 0, "CSLPacketHeader 64b-aligned");
CAMX_STATIC_ASSERT_MESSAGE((sizeof(CSLCmdMemDesc)             % 8) == 0, "CSLCmdMemDesc 64b-aligned");
CAMX_STATIC_ASSERT_MESSAGE((sizeof(CSLBufferIOConfig)         % 8) == 0, "CSLBufferIOConfig 64b-aligned");
CAMX_STATIC_ASSERT(static_cast<INT>(ChiCameraTitanSocInvalid)      == static_cast<INT>(CSLCameraTitanSocInvalid));
CAMX_STATIC_ASSERT(static_cast<INT>(ChiCameraTitanSocSDM845)       == static_cast<INT>(CSLCameraTitanSocSDM845));
CAMX_STATIC_ASSERT(static_cast<INT>(ChiCameraTitanSocSDM670)       == static_cast<INT>(CSLCameraTitanSocSDM670));
CAMX_STATIC_ASSERT(static_cast<INT>(ChiCameraTitanSocSDM855)       == static_cast<INT>(CSLCameraTitanSocSDM855));
CAMX_STATIC_ASSERT(static_cast<INT>(ChiCameraTitanSocSDM855P)      == static_cast<INT>(CSLCameraTitanSocSDM855P));
CAMX_STATIC_ASSERT(static_cast<INT>(ChiCameraTitanSocQCS605)       == static_cast<INT>(CSLCameraTitanSocQCS605));
CAMX_STATIC_ASSERT(static_cast<INT>(ChiCameraTitanSocSM6150)       == static_cast<INT>(CSLCameraTitanSocSM6150));
CAMX_STATIC_ASSERT(static_cast<INT>(ChiCameraTitanSocSDM865)       == static_cast<INT>(CSLCameraTitanSocSDM865));
CAMX_STATIC_ASSERT(static_cast<INT>(ChiCameraTitanSocSM7150)       == static_cast<INT>(CSLCameraTitanSocSM7150));
CAMX_STATIC_ASSERT(static_cast<INT>(ChiCameraTitanSocSM7150P)      == static_cast<INT>(CSLCameraTitanSocSM7150P));
CAMX_STATIC_ASSERT(static_cast<INT>(ChiCameraTitanSocSDM710)       == static_cast<INT>(CSLCameraTitanSocSDM710));
CAMX_STATIC_ASSERT(static_cast<INT>(ChiCameraTitanSocSXR1120)      == static_cast<INT>(CSLCameraTitanSocSXR1120));
CAMX_STATIC_ASSERT(static_cast<INT>(ChiCameraTitanSocSXR1130)      == static_cast<INT>(CSLCameraTitanSocSXR1130));
CAMX_STATIC_ASSERT(static_cast<INT>(ChiCameraTitanSocSDM712)       == static_cast<INT>(CSLCameraTitanSocSDM712));
CAMX_STATIC_ASSERT(static_cast<INT>(ChiCameraTitanSocSM7250)       == static_cast<INT>(CSLCameraTitanSocSM7250));
CAMX_STATIC_ASSERT(static_cast<INT>(ChiCameraTitanSocSM6250)       == static_cast<INT>(CSLCameraTitanSocSM6250));
CAMX_STATIC_ASSERT(static_cast<INT>(ChiCameraTitanSocQSM7250)      == static_cast<INT>(CSLCameraTitanSocQSM7250));
CAMX_STATIC_ASSERT(static_cast<INT>(ChiCameraTitanSocSM6350)       == static_cast<INT>(CSLCameraTitanSocSM6350));
CAMX_STATIC_ASSERT(static_cast<INT>(ChiCameraTitanSocSM7225)       == static_cast<INT>(CSLCameraTitanSocSM7225));

/// @brief  Helper object to initialize and expose the right CSL implementation.
CSLModeManager* g_pCSLModeManager;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CSLInitialize
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CSLInitialize(
    CSLInitializeParams* pInitializeParams)
{
    CAMX_ENTRYEXIT_SCOPE(CamxLogGroupCSL, CamX::SCOPEEventCSLInitialize);

    CAMX_STATIC_ASSERT(static_cast<INT>(CSLMode::CSLHwEnabled) ==
                       static_cast<INT>(CamX::CSLModeType::CSLModeHardware));
    CAMX_STATIC_ASSERT(static_cast<INT>(CSLMode::CSLIFHEnabled) ==
                       static_cast<INT>(CamX::CSLModeType::CSLModeIFH));
    CAMX_STATIC_ASSERT(static_cast<INT>(CSLMode::CSLPresilEnabled) ==
                       static_cast<INT>(CamX::CSLModeType::CSLModePresil));
    CAMX_STATIC_ASSERT(static_cast<INT>(CSLMode::CSLPresilRUMIEnabled) ==
                       static_cast<INT>(CamX::CSLModeType::CSLModePresilRUMI));

    if (NULL != g_pCSLModeManager)
    {
        CAMX_DELETE g_pCSLModeManager;
        g_pCSLModeManager = NULL;
    }

    g_pCSLModeManager = CAMX_NEW CSLModeManager(pInitializeParams);

    // Check g_pCSLModeManager before de-referencing it.
    if (NULL != g_pCSLModeManager)
    {
        CSLJumpTable* pJumpTable = g_pCSLModeManager->GetJumpTable();
        return pJumpTable->CSLInitialize();
    }
    else
    {
        CAMX_LOG_ERROR(CamxLogGroupCSL, "CSL not initialized");
        return CamxResultEFailed;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CSLUninitialize
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CSLUninitialize(void)
{
    CamxResult result = CamxResultSuccess;

    CAMX_ENTRYEXIT_SCOPE(CamxLogGroupCSL, CamX::SCOPEEventCSLUninitialize);

    CAMX_ASSERT_MESSAGE(NULL != g_pCSLModeManager, "CSL not initialized");
    if (NULL != g_pCSLModeManager)
    {
        CSLJumpTable* pJumpTable = g_pCSLModeManager->GetJumpTable();
        CAMX_ASSERT(NULL != pJumpTable);
        if (NULL != pJumpTable)
        {
            result = pJumpTable->CSLUninitialize();
        }

        CAMX_DELETE g_pCSLModeManager;
        g_pCSLModeManager = NULL;
    }
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CSLOpen
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CSLOpen(
    CSLHandle* phCSL)
{
    CAMX_ENTRYEXIT_SCOPE(CamxLogGroupCSL, CamX::SCOPEEventCSLOpen);

    CAMX_ASSERT_MESSAGE(NULL != g_pCSLModeManager, "CSL not initialized");
    CSLJumpTable* pJumpTable = g_pCSLModeManager->GetJumpTable();
    return pJumpTable->CSLOpen(phCSL);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CSLClose
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CSLClose(
    CSLHandle hCSL)
{
    CAMX_ENTRYEXIT_SCOPE(CamxLogGroupCSL, CamX::SCOPEEventCSLClose);

    CAMX_ASSERT_MESSAGE(NULL != g_pCSLModeManager, "CSL not initialized");
    CSLJumpTable* pJumpTable = g_pCSLModeManager->GetJumpTable();
    return pJumpTable->CSLClose(hCSL);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CSLAddReference
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CSLAddReference(
   CSLHandle hCSL)
{
    CAMX_ASSERT_MESSAGE(NULL != g_pCSLModeManager, "CSL not initialized");
    CSLJumpTable* pJumpTable = g_pCSLModeManager->GetJumpTable();
    return pJumpTable->CSLAddReference(hCSL);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CSLQueryCameraPlatform
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CSLQueryCameraPlatform(
    CSLCameraPlatform* pCameraPlatform)
{
    CAMX_ENTRYEXIT_SCOPE(CamxLogGroupCSL, CamX::SCOPEEventCSLQueryCameraPlatform);

    CAMX_ASSERT_MESSAGE(NULL != g_pCSLModeManager, "CSL not initialized");
    CSLJumpTable* pJumpTable = g_pCSLModeManager->GetJumpTable();
    return pJumpTable->CSLQueryCameraPlatform(pCameraPlatform);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CSLImageSensorProbe
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CSLImageSensorProbe(
    CSLMemHandle                hPacket,
    SIZE_T                      offset,
    CSLImageSensorProbeResult*  pProbeResult)
{
    CAMX_ENTRYEXIT_SCOPE(CamxLogGroupCSL, CamX::SCOPEEventCSLImageSensorProbe);

    CAMX_ASSERT_MESSAGE(NULL != g_pCSLModeManager, "CSL not initialized");
    CSLJumpTable* pJumpTable = g_pCSLModeManager->GetJumpTable();
    return pJumpTable->CSLImageSensorProbe(hPacket, offset, pProbeResult);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CSLSetDebugBuffer
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CSLSetDebugBuffer(
    CSLHandle               hCSL,
    CSLDebugBufferType      type,
    CSLMemHandle            hBuffer,
    SIZE_T                  offset,
    SIZE_T                  length,
    CSLDebugBufferResult*   pDebugBufferResult)
{
    CAMX_ENTRYEXIT_SCOPE(CamxLogGroupCSL, CamX::SCOPEEventCSLSetDebugBuffer);

    CAMX_ASSERT_MESSAGE(NULL != g_pCSLModeManager, "CSL not initialized");
    CSLJumpTable* pJumpTable = g_pCSLModeManager->GetJumpTable();
    return pJumpTable->CSLSetDebugBuffer(hCSL, type, hBuffer, offset, length, pDebugBufferResult);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CSLEnumerateDevices
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CSLEnumerateDevices(
    CSLDeviceDescriptor* pDeviceDescriptor)
{
    CAMX_ENTRYEXIT_SCOPE(CamxLogGroupCSL, CamX::SCOPEEventCSLEnumerateDevices);

    CAMX_ASSERT_MESSAGE(NULL != g_pCSLModeManager, "CSL not initialized");
    CSLJumpTable* pJumpTable = g_pCSLModeManager->GetJumpTable();
    return pJumpTable->CSLEnumerateDevices(pDeviceDescriptor);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CSLQueryDeviceCapabilities
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CSLQueryDeviceCapabilities(
    INT32   deviceIndex,
    VOID*   pDeviceData,
    SIZE_T  deviceDataSize)
{
    CAMX_ENTRYEXIT_SCOPE(CamxLogGroupCSL, CamX::SCOPEEventCSLQueryDeviceCapabilities);

    CAMX_ASSERT_MESSAGE(NULL != g_pCSLModeManager, "CSL not initialized");
    CSLJumpTable* pJumpTable = g_pCSLModeManager->GetJumpTable();
    return pJumpTable->CSLQueryDeviceCapabilities(deviceIndex, pDeviceData, deviceDataSize);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CSLAcquireDevice
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CSLAcquireDevice(
    CSLHandle           hCSL,
    CSLDeviceHandle*    phDevice,
    INT32               deviceIndex,
    CSLDeviceResource*  pDeviceResourceRequest,
    SIZE_T              numDeviceResources,
    CSLDeviceAttribute* pDeviceAttribute,
    SIZE_T              numDeviceAttributes,
    const CHAR*         pDeviceName)
{
    // If traces are disabled pDeviceName is not otherwise referenced. This doesnt hurt anything either way
    CAMX_UNREFERENCED_PARAM(pDeviceName);
    CAMX_TRACE_SYNC_BEGIN_F(CamxLogGroupCSL, "CSLAcquireDevice: %s", pDeviceName);

    CAMX_ASSERT_MESSAGE(NULL != g_pCSLModeManager, "CSL not initialized");
    CSLJumpTable* pJumpTable = g_pCSLModeManager->GetJumpTable();
    CamxResult result = pJumpTable->CSLAcquireDevice(hCSL,
                                        phDevice,
                                        deviceIndex,
                                        pDeviceResourceRequest,
                                        numDeviceResources,
                                        pDeviceAttribute,
                                        numDeviceAttributes,
                                        pDeviceName);
    CAMX_LOG_INFO(CamxLogGroupCore, "Acquiring - Device: %s, DeviceHandle: %d, Result: %d", pDeviceName, hCSL, result);
    CAMX_TRACE_SYNC_END(CamxLogGroupCSL);

    if (CamxResultSuccess != result)
    {
        *phDevice = CSLInvalidHandle;
        if (TRUE == CamX::HwEnvironment::GetInstance()->GetStaticSettings()->raisesigabrt)
        {
            CAMX_LOG_ERROR(CamxLogGroupCore, "FATAL ERROR: CSLAcquireDevice Fail, Raise SigAbort to debug the root cause");
#if !_WINDOWS
            raise(SIGABRT);
#endif // _WINDOWS
        }
    }
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CSLReleaseDevice
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CSLReleaseDevice(
    CSLHandle       hCSL,
    CSLDeviceHandle hDevice)
{
    CAMX_ENTRYEXIT_SCOPE(CamxLogGroupCSL, CamX::SCOPEEventCSLReleaseDevice);

    CAMX_ASSERT_MESSAGE(NULL != g_pCSLModeManager, "CSL not initialized");
    CSLJumpTable* pJumpTable = g_pCSLModeManager->GetJumpTable();
    CamxResult result        = pJumpTable->CSLReleaseDevice(hCSL, hDevice);
    CAMX_LOG_INFO(CamxLogGroupCore, "Releasing DeviceHandle: %d Result: %d", hCSL, result);
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CSLAcquireHardware
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CSLAcquireHardware(
    CSLHandle           hCSL,
    CSLDeviceHandle     hDevice,
    CSLDeviceResource*  pDeviceResourceRequest,
    const CHAR*         pDeviceName,
    UINT32              version)
{
    // If traces are disabled pDeviceName is not otherwise referenced. This doesnt hurt anything either way
    CAMX_UNREFERENCED_PARAM(pDeviceName);
    CAMX_TRACE_SYNC_BEGIN_F(CamxLogGroupCSL, "CSLAcquireHardware: %s", pDeviceName);

    CAMX_ASSERT_MESSAGE(NULL != g_pCSLModeManager, "CSL not initialized");
    CSLJumpTable* pJumpTable = g_pCSLModeManager->GetJumpTable();
    CamxResult result = pJumpTable->CSLAcquireHardware(hCSL,
                                                       hDevice,
                                                       pDeviceResourceRequest,
                                                       version);
    CAMX_TRACE_SYNC_END(CamxLogGroupCSL);

    if (CamxResultSuccess != result)
    {
        if (TRUE == CamX::HwEnvironment::GetInstance()->GetStaticSettings()->raisesigabrt)
        {
            CAMX_LOG_ERROR(CamxLogGroupCore, "FATAL ERROR: CSLAcquireHardware Fail, Raise SigAbort to debug the root cause");
            raise(SIGABRT);
        }
    }
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CSLReleaseHardware
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CSLReleaseHardware(
    CSLHandle       hCSL,
    CSLDeviceHandle hDevice)
{
    CAMX_ENTRYEXIT_SCOPE(CamxLogGroupCSL, CamX::SCOPEEventCSLReleaseHardware);

    CAMX_ASSERT_MESSAGE(NULL != g_pCSLModeManager, "CSL not initialized");
    CSLJumpTable* pJumpTable = g_pCSLModeManager->GetJumpTable();
    return pJumpTable->CSLReleaseHardware(hCSL, hDevice);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CSLLink
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CSLLink(
    CSLHandle           hCSL,
    CSLDeviceHandle*    phDevices,
    UINT                handleCount,
    CSLLinkHandle*      phLink)
{
    CAMX_ASSERT_MESSAGE(NULL != g_pCSLModeManager, "CSL not initialized");
    CSLJumpTable* pJumpTable = g_pCSLModeManager->GetJumpTable();
    return pJumpTable->CSLLink(hCSL, phDevices, handleCount, phLink);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CSLUnlink
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CSLUnlink(
    CSLHandle          hCSL,
    CSLLinkHandle*     phLink)
{
    CAMX_ASSERT_MESSAGE(NULL != g_pCSLModeManager, "CSL not initialized");
    CSLJumpTable* pJumpTable = g_pCSLModeManager->GetJumpTable();
    return pJumpTable->CSLUnlink(hCSL, phLink);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CSLSyncLinks
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CSLSyncLinks(
    CSLHandle           hCSL,
    CSLLinkHandle*      phLink,
    UINT                handleCount,
    CSLLinkHandle       hMasterLink,
    CSLSyncLinkMode     syncMode)
{
    CAMX_ASSERT_MESSAGE(NULL != g_pCSLModeManager, "CSL not initialized");
    CSLJumpTable* pJumpTable = g_pCSLModeManager->GetJumpTable();
    return pJumpTable->CSLSyncLinks(hCSL, phLink, handleCount, hMasterLink, syncMode);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CSLOpenRequest
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult  CSLOpenRequest(
    CSLHandle          hCSL,
    CSLLinkHandle      hLink,
    UINT64             requestId,
    BOOL               bubble,
    CSLSyncLinkMode    syncMode,
    UINT32             expectedExposureTimeInMs)
{
    CAMX_ASSERT_MESSAGE(NULL != g_pCSLModeManager, "CSL not initialized");
    CSLJumpTable* pJumpTable = g_pCSLModeManager->GetJumpTable();
    return pJumpTable->CSLOpenRequest(hCSL, hLink, requestId, bubble, syncMode, expectedExposureTimeInMs);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CSLCancelRequest
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult  CSLCancelRequest(
    CSLHandle           hCSL,
    const CSLFlushInfo& rCSLFlushInfo)
{
    CAMX_ASSERT_MESSAGE(NULL != g_pCSLModeManager, "CSL not initialized");
    CSLJumpTable* pJumpTable = g_pCSLModeManager->GetJumpTable();
    return pJumpTable->CSLCancelRequest(hCSL, rCSLFlushInfo);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CSLStreamOn
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CSLStreamOn(
    CSLHandle hCSL,
    CSLLinkHandle* phLink,
    CSLDeviceHandle* phDevices)
{
    CAMX_ENTRYEXIT_SCOPE(CamxLogGroupCSL, CamX::SCOPEEventCSLStreamOn);

    CAMX_ASSERT_MESSAGE(NULL != g_pCSLModeManager, "CSL not initialized");
    CSLJumpTable* pJumpTable = g_pCSLModeManager->GetJumpTable();
    return pJumpTable->CSLStreamOn(hCSL, phLink, phDevices);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CSLSingleDeviceStreamOn
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CSLSingleDeviceStreamOn(
    CSLHandle           hCSL,
    INT32               deviceIndex,
    CSLDeviceHandle*    phDevice)
{
    CAMX_ASSERT_MESSAGE(NULL != g_pCSLModeManager, "CSL not initialized");
    CSLJumpTable* pJumpTable = g_pCSLModeManager->GetJumpTable();
    return pJumpTable->CSLSingleDeviceStreamOn(hCSL, deviceIndex, phDevice);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CSLStreamOff
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CSLStreamOff(
    CSLHandle          hCSL,
    CSLLinkHandle*     phLink,
    CSLDeviceHandle*   phDevices,
    CSLDeactivateMode  mode)
{
    CAMX_ENTRYEXIT_SCOPE(CamxLogGroupCSL, CamX::SCOPEEventCSLStreamOff);

    CAMX_ASSERT_MESSAGE(NULL != g_pCSLModeManager, "CSL not initialized");
    CSLJumpTable* pJumpTable = g_pCSLModeManager->GetJumpTable();
    return pJumpTable->CSLStreamOff(hCSL, phLink, phDevices, mode);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CSLSingleDeviceStreamOff
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CSLSingleDeviceStreamOff(
    CSLHandle           hCSL,
    INT32               deviceIndex,
    CSLDeviceHandle*    phDevice)
{
    CAMX_ENTRYEXIT_SCOPE(CamxLogGroupCSL, CamX::SCOPEEventCSLStreamOff);

    CAMX_ASSERT_MESSAGE(NULL != g_pCSLModeManager, "CSL not initialized");
    CSLJumpTable* pJumpTable = g_pCSLModeManager->GetJumpTable();
    return pJumpTable->CSLSingleDeviceStreamOff(hCSL, deviceIndex, phDevice);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CSLSubmit
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CSLSubmit(
    CSLHandle           hCSL,
    CSLDeviceHandle     hDevice,
    CSLMemHandle        hPacket,
    SIZE_T              offset)
{
    CAMX_ENTRYEXIT_SCOPE(CamxLogGroupCSL, CamX::SCOPEEventCSLSubmit);

    CAMX_ASSERT_MESSAGE(NULL != g_pCSLModeManager, "CSL not initialized");
    CSLJumpTable* pJumpTable = g_pCSLModeManager->GetJumpTable();
    return pJumpTable->CSLSubmit(hCSL, hDevice, hPacket, offset);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CSLFlushLock
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CSLFlushLock(
    CSLHandle           hCSL,
    const CSLFlushInfo& rCSLFlushInfo)
{
    CAMX_ENTRYEXIT_SCOPE(CamxLogGroupCSL, CamX::SCOPEEventCSLFlushLock);

    CAMX_ASSERT_MESSAGE(NULL != g_pCSLModeManager, "CSL not initialized");
    CSLJumpTable* pJumpTable = g_pCSLModeManager->GetJumpTable();
    return pJumpTable->CSLFlushLock(hCSL, rCSLFlushInfo);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CSLFlushUnlock
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CSLFlushUnlock(
    CSLHandle hCSL)
{
    CAMX_ENTRYEXIT_SCOPE(CamxLogGroupCSL, CamX::SCOPEEventCSLFlush);

    CAMX_ASSERT_MESSAGE(NULL != g_pCSLModeManager, "CSL not initialized");
    CSLJumpTable* pJumpTable = g_pCSLModeManager->GetJumpTable();
    return pJumpTable->CSLFlushUnlock(hCSL);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CSLRegisterMessageHandler
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CSLRegisterMessageHandler(
    CSLHandle           hCSL,
    CSLLinkHandle       hCSLLinkHandle,
    CSLMessageHandler   messageHandler,
    VOID*               pUserData)
{
    CAMX_ENTRYEXIT_SCOPE(CamxLogGroupCSL, CamX::SCOPEEventCSLRegisterMessageHandler);

    CAMX_ASSERT_MESSAGE(NULL != g_pCSLModeManager, "CSL not initialized");
    CSLJumpTable* pJumpTable = g_pCSLModeManager->GetJumpTable();
    return pJumpTable->CSLRegisterMessageHandler(hCSL, hCSLLinkHandle, messageHandler, pUserData);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CSLRegisterSessionMessageHandler
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CSLRegisterSessionMessageHandler(
    CSLHandle                   hCSL,
    CSLSessionMessageHandler    sessionMessageHandler,
    VOID*                       pUserData)
{
    CAMX_ENTRYEXIT_SCOPE(CamxLogGroupCSL, CamX::SCOPEEventCSLRegisterMessageHandler);

    if (NULL == g_pCSLModeManager)
    {
        CAMX_LOG_ERROR(CamxLogGroupCore, "CSL not initialized");
        return CamxResultEFailed;
    }
    CSLJumpTable* pJumpTable = g_pCSLModeManager->GetJumpTable();
    return pJumpTable->CSLRegisterSessionMessageHandler(hCSL, sessionMessageHandler, pUserData);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CSLAlloc
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CSLAlloc(
    const CHAR*     pCallerName,
    CSLBufferInfo*  pBufferInfo,
    SIZE_T          bufferSize,
    SIZE_T          alignment,
    UINT32          flags,
    const INT32*    pDeviceIndices,
    UINT            deviceCount)
{
    CAMX_TRACE_SYNC_BEGIN_F(CamxLogGroupCSL, "CSLAlloc: %s %u", pCallerName, bufferSize);

    CAMX_ASSERT_MESSAGE(NULL != g_pCSLModeManager, "CSL not initialized");
    CSLJumpTable* pJumpTable = g_pCSLModeManager->GetJumpTable();
    CamxResult result = pJumpTable->CSLAlloc(
        pCallerName, pBufferInfo, bufferSize, alignment, flags, pDeviceIndices, deviceCount);

    CAMX_TRACE_SYNC_END(CamxLogGroupCSL);

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CSLMapBuffer
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CSLMapBuffer(
    CSLBufferInfo*  pBufferInfo,
    INT             bufferFD,
    SIZE_T          offset,
    SIZE_T          bufferLength,
    UINT32          flags,
    const INT32*    pDeviceIndices,
    UINT            deviceCount)
{
    CAMX_ENTRYEXIT_SCOPE(CamxLogGroupCSL, CamX::SCOPEEventCSLMapBuffer);

    CAMX_ASSERT_MESSAGE(NULL != g_pCSLModeManager, "CSL not initialized");
    CSLJumpTable* pJumpTable = g_pCSLModeManager->GetJumpTable();
    return pJumpTable->CSLMapBuffer(pBufferInfo, bufferFD, offset, bufferLength, flags, pDeviceIndices, deviceCount);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CSLMapNativeBuffer
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CSLMapNativeBuffer(
    CSLBufferInfo*          pBufferInfo,
    const CSLNativeHandle*  phNativeBuffer,
    SIZE_T                  offset,
    SIZE_T                  bufferSize,
    UINT32                  flags,
    const INT32*            pDeviceIndices,
    UINT                    deviceCount)
{
    CAMX_ENTRYEXIT_SCOPE(CamxLogGroupCSL, CamX::SCOPEEventCSLMapNativeBuffer);

    CAMX_ASSERT_MESSAGE(NULL != g_pCSLModeManager, "CSL not initialized");
    CSLJumpTable* pJumpTable = g_pCSLModeManager->GetJumpTable();
    return pJumpTable->CSLMapNativeBuffer(pBufferInfo,
                                          phNativeBuffer,
                                          offset,
                                          bufferSize,
                                          flags,
                                          pDeviceIndices,
                                          deviceCount);

}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CSLGetBufferInfo
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CSLGetBufferInfo(
    CSLMemHandle hBuffer,
    CSLBufferInfo*  pBufferInfo)
{
    CAMX_ENTRYEXIT_SCOPE(CamxLogGroupCSL, CamX::SCOPEEventCSLGetBufferInfo);

    CAMX_ASSERT_MESSAGE(NULL != g_pCSLModeManager, "CSL not initialized");
    CSLJumpTable* pJumpTable = g_pCSLModeManager->GetJumpTable();
    return pJumpTable->CSLGetBufferInfo(hBuffer, pBufferInfo);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CSLBufferCacheOp
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CSLBufferCacheOp(
    CSLMemHandle    hBuffer,
    BOOL            invalidate,
    BOOL            clean)
{
    CAMX_ENTRYEXIT_SCOPE(CamxLogGroupCSL, CamX::SCOPEEventCSLBufferCacheOp);

    CAMX_ASSERT_MESSAGE(NULL != g_pCSLModeManager, "CSL not initialized");
    CSLJumpTable* pJumpTable = g_pCSLModeManager->GetJumpTable();
    return pJumpTable->CSLBufferCacheOp(hBuffer, invalidate, clean);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CSLReleaseBuffer
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CSLReleaseBuffer(
    CSLMemHandle hBuffer)
{
    CAMX_ENTRYEXIT_SCOPE(CamxLogGroupCSL, CamX::SCOPEEventCSLReleaseBuffer);

    CAMX_ASSERT_MESSAGE(NULL != g_pCSLModeManager, "CSL not initialized");
    CSLJumpTable* pJumpTable = g_pCSLModeManager->GetJumpTable();
    return pJumpTable->CSLReleaseBuffer(hBuffer);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CSLReleaseBufferForced
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CSLReleaseBufferForced(
    CSLMemHandle hBuffer)
{
    CAMX_ENTRYEXIT_SCOPE(CamxLogGroupCSL, CamX::SCOPEEventCSLReleaseBuffer);

    CAMX_ASSERT_MESSAGE(NULL != g_pCSLModeManager, "CSL not initialized");
    CSLJumpTable* pJumpTable = g_pCSLModeManager->GetJumpTable();
    return pJumpTable->CSLReleaseBufferForced(hBuffer);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CSLCreatePrivateFence
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CSLCreatePrivateFence(
    const CHAR* pName,
    CSLFence*   phFenceOut)
{
    CAMX_ENTRYEXIT_SCOPE(CamxLogGroupCSL, CamX::SCOPEEventCSLCreatePrivateFence);

    CAMX_ASSERT_MESSAGE(NULL != g_pCSLModeManager, "CSL not initialized");
    CSLJumpTable* pJumpTable = g_pCSLModeManager->GetJumpTable();
    return pJumpTable->CSLCreatePrivateFence(pName, phFenceOut);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CSLCreateNativeFence
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CSLCreateNativeFence(
    CSLNativeFenceCreateDataPtr pCreateData,
    CSLFence*                   phFenceOut)
{
    CAMX_ENTRYEXIT_SCOPE(CamxLogGroupCSL, CamX::SCOPEEventCSLCreateNativeFence);

    CAMX_ASSERT_MESSAGE(NULL != g_pCSLModeManager, "CSL not initialized");
    CSLJumpTable* pJumpTable = g_pCSLModeManager->GetJumpTable();
    return pJumpTable->CSLCreateNativeFence(pCreateData, phFenceOut);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CSLMergeFences
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CSLMergeFences(
    CSLFence*   phFences,
    SIZE_T      fenceCount,
    CSLFence*   phFenceOut)
{
    CAMX_ENTRYEXIT_SCOPE(CamxLogGroupCSL, CamX::SCOPEEventCSLMergeFences);

    CAMX_ASSERT_MESSAGE(NULL != g_pCSLModeManager, "CSL not initialized");
    CSLJumpTable* pJumpTable = g_pCSLModeManager->GetJumpTable();
    return pJumpTable->CSLMergeFences(phFences, fenceCount, phFenceOut);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CSLGetFenceAttrib
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CSLGetFenceAttrib(
    CSLFence    hFence,
    UINT32      attrib,
    VOID*       pAttribVal,
    UINT32      valSize)
{
    CAMX_ENTRYEXIT_SCOPE(CamxLogGroupCSL, CamX::SCOPEEventCSLGetFenceAttrib);

    CAMX_ASSERT_MESSAGE(NULL != g_pCSLModeManager, "CSL not initialized");
    CSLJumpTable* pJumpTable = g_pCSLModeManager->GetJumpTable();
    return pJumpTable->CSLGetFenceAttrib(hFence, attrib, pAttribVal, valSize);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CSLFenceWait
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CSLFenceWait(
    CSLFence    hFence,
    UINT64      timeout)
{
    CAMX_ENTRYEXIT_SCOPE(CamxLogGroupCSL, CamX::SCOPEEventCSLFenceWait);

    CAMX_ASSERT_MESSAGE(NULL != g_pCSLModeManager, "CSL not initialized");
    CSLJumpTable* pJumpTable = g_pCSLModeManager->GetJumpTable();
    return pJumpTable->CSLFenceWait(hFence, timeout);
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CSLFenceWaitMultiple
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CSLFenceWaitMultiple(
    CSLFence*   phFences,
    BOOL*       pFenceSignaled,
    SIZE_T      fenceCount,
    UINT64      timeout)
{
    CAMX_ENTRYEXIT_SCOPE(CamxLogGroupCSL, CamX::SCOPEEventCSLFenceWaitMultiple);

    CAMX_ASSERT_MESSAGE(NULL != g_pCSLModeManager, "CSL not initialized");
    CSLJumpTable* pJumpTable = g_pCSLModeManager->GetJumpTable();
    return pJumpTable->CSLFenceWaitMultiple(phFences, pFenceSignaled, fenceCount, timeout);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CSLFenceAsyncWait
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CSLFenceAsyncWait(
    CSLFence        hFence,
    CSLFenceHandler handler,
    VOID*           pUserData)
{
    CAMX_ENTRYEXIT_SCOPE(CamxLogGroupCSL, CamX::SCOPEEventCSLFenceAsyncWait);
    CAMX_TRACE_ASYNC_BEGIN_F(CamxLogGroupCSL, static_cast<UINT32>(hFence), "CSL: FenceTrace");

    CAMX_ASSERT_MESSAGE(NULL != g_pCSLModeManager, "CSL not initialized");
    CSLJumpTable* pJumpTable = g_pCSLModeManager->GetJumpTable();
    CamxResult    result     = pJumpTable->CSLFenceAsyncWait(hFence, handler, pUserData);

    if ((CamxResultSuccess != result) &&
        (CamxResultEInvalidArg != result) &&
        (TRUE == CamX::HwEnvironment::GetInstance()->GetStaticSettings()->raisesigabrt))
    {
        raise(SIGABRT);
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CSLFenceAsyncCancel
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CSLFenceAsyncCancel(
    CSLFence        hFence,
    CSLFenceHandler handler,
    VOID*           pUserData)
{
    CAMX_ENTRYEXIT_SCOPE(CamxLogGroupCSL, CamX::SCOPEEventCSLFenceAsyncCancel);
    CAMX_TRACE_ASYNC_END_F(CamxLogGroupCSL, static_cast<UINT32>(hFence), "CSL: FenceTrace");

    CAMX_ASSERT_MESSAGE(NULL != g_pCSLModeManager, "CSL not initialized");
    CSLJumpTable* pJumpTable = g_pCSLModeManager->GetJumpTable();
    CamxResult    result     = pJumpTable->CSLFenceAsyncCancel(hFence, handler, pUserData);

    if ((CamxResultSuccess != result) &&
        (CamxResultEInvalidArg != result) &&
        (TRUE == CamX::HwEnvironment::GetInstance()->GetStaticSettings()->raisesigabrt))
    {
        raise(SIGABRT);
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CSLFenceSignal
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CSLFenceSignal(
    CSLFence        hFence,
    CSLFenceResult  fenceResult)
{
    CAMX_ENTRYEXIT_SCOPE(CamxLogGroupCSL, CamX::SCOPEEventCSLFenceSignal);
    CAMX_TRACE_ASYNC_END_F(CamxLogGroupCSL, static_cast<UINT32>(hFence), "CSL: FenceTrace");

    CAMX_ASSERT_MESSAGE(NULL != g_pCSLModeManager, "CSL not initialized");
    CSLJumpTable* pJumpTable = g_pCSLModeManager->GetJumpTable();
    CamxResult    result     = pJumpTable->CSLFenceSignal(hFence, fenceResult);

    if ((CamxResultSuccess != result) &&
        (CamxResultEInvalidArg != result) &&
        (TRUE == CamX::HwEnvironment::GetInstance()->GetStaticSettings()->raisesigabrt))
    {
        raise(SIGABRT);
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CSLReleaseFence
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CSLReleaseFence(
    CSLFence hFence)
{
    CAMX_ENTRYEXIT_SCOPE(CamxLogGroupCSL, CamX::SCOPEEventCSLReleaseFence);

    CAMX_ASSERT_MESSAGE(NULL != g_pCSLModeManager, "CSL not initialized");
    CSLJumpTable* pJumpTable = g_pCSLModeManager->GetJumpTable();
    return pJumpTable->CSLReleaseFence(hFence);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CSLDumpRequest
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult  CSLDumpRequest(
    CSLHandle              hCSL,
    CSLDumpRequestInfo*    pDumpRequestInfo,
    SIZE_T*                pFilledLength)
{
    CAMX_ENTRYEXIT_SCOPE(CamxLogGroupCSL, CamX::SCOPEEventCSLDumpRequest);

    CAMX_ASSERT_MESSAGE(NULL != g_pCSLModeManager, "CSL not initialized");
    CSLJumpTable* pJumpTable = g_pCSLModeManager->GetJumpTable();
    return pJumpTable->CSLDumpRequest(hCSL, pDumpRequestInfo, pFilledLength);
}
