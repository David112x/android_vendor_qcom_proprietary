////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2016-2020 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  camxcslifh.cpp
///
/// @brief CamxCSL Implementation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include <ctime>
#include "camxatomic.h"
#include "camxcsl.h"
#include "camxcslicpdefs.h"
#include "camxcsljumptable.h"
#include "camxcslcommonutils.h"
#include "camxcslresourcedefs.h"
#include "camxincs.h"
#include "camxlist.h"
#include "camxmem.h"
#include "camxpacketdefs.h"

/// @brief  Define all the supported devices; one entry per instance (e.g. 2 entries for 2 VFEs etc.)
/// @todo   (CAMX-44) Incrementally fill in the definitions with more info
static CSLDeviceDescriptor g_devices[CSLMaxNumDevices] =
{
    {
        0,
        CSLDeviceTypeVFE,
        {0},
        {0}
    },
    {
        1,
        CSLDeviceTypeVFE,
        {0},
        {0}
    },
    {
        2,
        CSLDeviceTypeCPP,
        {0},
        {0}
    },
    {
        3,
        CSLDeviceTypeIFE,
        {0},
        {0}
    },
    {
        4,
        CSLDeviceTypeFD,
        {0},
        {0}
    },
    {
        5,
        CSLDeviceTypeLRME,
        {0},
        {0}
    },
    {
        6,
        CSLDeviceTypeJPEGD,
        {0},
        {0}
    },
    {
        7,
        CSLDeviceTypeJPEGE,
        {0},
        {0}
    },

    //  In real implementation, the number of sensors must be probed and added; we just use one sensor here.
    //  Will define more if needed.
    {
        8,
        CSLDeviceTypeImageSensor,
        {0},
        {0}
    },
    {
        9,
        CSLDeviceTypeLensActuator,
        {0},
        {0}
    },
    {
        10,
        CSLDeviceTypeFlash,
        {0},
        {0}
    },
    {
        11,
        CSLDeviceTypeCSID,
        {0},
        {0}
    },
    {
        12,
        CSLDeviceTypeCSIPHY,
        {0},
        {0}
    },
    {
        13,
        CSLDeviceTypeEEPROM,
        {0},
        {0}
    },
    {
        14,
        CSLDeviceTypeICP,
        {0},
        {0}
    },
    {
        15,
        CSLDeviceTypeCustom,
        { 0 },
        { 0 }
    },
};

/// @brief  Define devices caps
static VOID* g_pCapabilities[CSLMaxNumDevices] = { 0 };

CAMX_STATIC_ASSERT(8 == CSLSensorDeviceIndex);

/// @brief  Global CSL state
static CSLState g_CSLState = { 0 };

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ConstructMemAlloc
///
/// @brief  This is a helper function used to initialize a memory allocation struct
///
/// @param  ppBufferInfo    Address of a CSLBufferInfo pointer that will receive the corresponding buffer info
/// @param  bufferSize      Size of the buffer
/// @param  flags           Usage flags for the memory allocation
///
/// @return CSLMemHandle
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static CSLMemHandle ConstructMemAlloc(
    CSLState*       pCSLState,
    CSLBufferInfo** ppBufferInfo,
    SIZE_T          bufferSize,
    UINT32          flags)
{
    CAMX_ASSERT(NULL != ppBufferInfo);
    INT             handle       = CSLInvalidHandle;
    CSLBufferInfo*  pBufferInfo  = static_cast<CSLBufferInfo*>(CAMX_CALLOC(sizeof(CSLBufferInfo)));
    handle = GetNextHandle(reinterpret_cast<VOID**>(&pCSLState->pMemHandles[0]), pCSLState->memCount, CSLMaxNumAllocs);

    // If memory allocated successfully and a free handle slot was available
    if ((NULL != pBufferInfo) && (CSLInvalidHandle != handle))
    {
        if (CSLMemFlagKMDAccess == flags)
        {
            pBufferInfo->pVirtualAddr = NULL;
        }
        else
        {
            // Allocate heap memory as if we have mapped physical memory
            pBufferInfo->pVirtualAddr = CAMX_CALLOC(bufferSize);
            if (NULL == pBufferInfo->pVirtualAddr)
            {
                CAMX_FREE(pBufferInfo);
                pBufferInfo = NULL;
                handle      = CSLInvalidHandle;
            }
        }

        if (CSLInvalidHandle != handle)
        {
            pBufferInfo->size           = bufferSize;
            pBufferInfo->flags          = flags;
            pBufferInfo->bufferImported = 0;

            pCSLState->memCount++;

            // Store the pointer in the corresponding handle slot
            StoreHandle(reinterpret_cast<VOID**>(&pCSLState->pMemHandles[0]), handle, pBufferInfo, CSLMaxNumAllocs);

            pBufferInfo->hHandle = handle;

            if (NULL != ppBufferInfo)
            {
                *ppBufferInfo = pBufferInfo;
            }
        }
    }

    return handle;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ImportMemAlloc
///
/// @brief  This is a helper function used to initialize a memory allocation struct
///
/// @param  ppBufferInfo    Address of a CSLBufferInfo pointer that will receive the corresponding buffer info
/// @param  bufferFD        File descriptor of the buffer to be imported
/// @param  bufferLength    Total length that of the buffer
/// @param  offset          Offset of the address which valid to map and access
///
/// @return CSLMemHandle    Handle to memory that is mapped by the CSL
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static CSLMemHandle ImportMemAlloc(
    CSLState*       pCSLState,
    CSLBufferInfo** ppBufferInfo,
    INT             bufferFD,
    SIZE_T          bufferLength,
    SIZE_T          offset)
{
    CAMX_ASSERT(NULL != ppBufferInfo);
    INT             handle       = CSLInvalidHandle;
    CSLBufferInfo*  pBufferInfo  = static_cast<CSLBufferInfo*>(CAMX_CALLOC(sizeof(CSLBufferInfo)));
    handle = GetNextHandle(reinterpret_cast<VOID**>(&pCSLState->pMemHandles[0]), pCSLState->memCount, CSLMaxNumAllocs);

    // If memory allocated successfully and a free handle slot was available
    if ((NULL != pBufferInfo) && (CSLInvalidHandle != handle))
    {
        // Allocate heap memory as if we have mapped physical memory
        // We don't care about the rest of the fields for now
        /// @todo   (CAMX-427) If secure buffer this is going to be NULL - need to take care of that
        pBufferInfo->pVirtualAddr = CamX::OsUtils::MemMap(bufferFD, bufferLength, offset);
        if (NULL != pBufferInfo->pVirtualAddr)
        {
            pBufferInfo->size = bufferLength;
            pBufferInfo->bufferImported = 1;
            pBufferInfo->fd = bufferFD;
            pBufferInfo->offset = offset;
            pCSLState->memCount++;
            // Store the pointer in the corresponding handle slot
            StoreHandle(reinterpret_cast<VOID**>(&pCSLState->pMemHandles[0]), handle, pBufferInfo, CSLMaxNumAllocs);
            pBufferInfo->hHandle = handle;
            if (NULL != ppBufferInfo)
            {
                *ppBufferInfo = pBufferInfo;
            }
        }
        else
        {
            CAMX_FREE(pBufferInfo);
            pBufferInfo = NULL;
            handle      = CSLInvalidHandle;
        }
    }

    return handle;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// DestructMemAlloc
///
/// @brief  This is a helper function used to destruct a memory allocation struct
///
/// @param  pBuffer  Pointer to the buffer that will be initialized
///
/// @return N/A
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static VOID DestructMemAlloc(
    CSLState*       pCSLState,
    CSLMemHandle    hMem)
{
    if ((hMem > 0) && (static_cast<UINT>(hMem) <= CSLMaxNumAllocs))
    {
        VOID* pValue = GetFromHandle(reinterpret_cast<VOID**>(&pCSLState->pMemHandles[0]), hMem, CSLMaxNumAllocs);
        CSLBufferInfo* pBufferInfo = reinterpret_cast<CSLBufferInfo*>(pValue);

        CAMX_ASSERT((NULL != pBufferInfo) && (NULL != pBufferInfo->pVirtualAddr));

        if (NULL != pBufferInfo)
        {
            if (pBufferInfo->bufferImported)
            {
                CamX::OsUtils::MemUnmap(pBufferInfo->pVirtualAddr, pBufferInfo->size);
            }
            else
            {
                CAMX_FREE(pBufferInfo->pVirtualAddr);
                pBufferInfo->pVirtualAddr = NULL;
            }
            CAMX_FREE(pBufferInfo);
            pBufferInfo = NULL;
            pCSLState->memCount--;
            // Free up the slot in the handle store
            StoreHandle(reinterpret_cast<VOID**>(&pCSLState->pMemHandles[0]), hMem, NULL, CSLMaxNumAllocs);
        }
    }
    else
    {
        CAMX_ASSERT((hMem >= 0) && (static_cast<UINT>(hMem) < CSLMaxNumAllocs));
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CSLInitializeIFH
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CSLInitializeIFH(void)
{
    return CSLInitializeCommon(&g_CSLState, sizeof(CSLState), g_devices, g_pCapabilities);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CSLUninitializeIFH
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CSLUninitializeIFH(void)
{
    return CSLUninitializeCommon(&g_CSLState);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CSLOpenIFH
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CSLOpenIFH(
    CSLHandle* phCSL)
{
    return CSLOpenCommon(&g_CSLState, phCSL);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CSLCloseIFH
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CSLCloseIFH(
    CSLHandle hCSL)
{
    return CSLCloseCommon(&g_CSLState, hCSL);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CSLAddReferenceIFH
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CSLAddReferenceIFH(
    CSLHandle hCSL)
{
    return CSLAddReferenceCommon(&g_CSLState, hCSL);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CSLQueryCameraPlatformIFH
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CSLQueryCameraPlatformIFH(
    CSLCameraPlatform* pCameraPlatform)
{
    return CSLQueryCameraPlatformCommon(&g_CSLState, pCameraPlatform);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CSLImageSensorProbeIFH
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CSLImageSensorProbeIFH(
    CSLMemHandle                hPacket,
    SIZE_T                      offset,
    CSLImageSensorProbeResult*  pProbeResult)
{
    return CSLImageSensorProbeCommon(&g_CSLState, hPacket, offset, pProbeResult);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CSLSetDebugBufferIFH
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CSLSetDebugBufferIFH(
    CSLHandle               hCSL,
    CSLDebugBufferType      type,
    CSLMemHandle            hBuffer,
    SIZE_T                  offset,
    SIZE_T                  length,
    CSLDebugBufferResult*   pDebugBufferResult)
{
    return CSLSetDebugBufferCommon(&g_CSLState, hCSL, type, hBuffer, offset, length, pDebugBufferResult);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CSLEnumerateDevicesIFH
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CSLEnumerateDevicesIFH(
    CSLDeviceDescriptor* pDeviceDescriptor)
{
    return CSLEnumerateDevicesCommon(&g_CSLState, pDeviceDescriptor);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CSLQueryDeviceCapabilitiesIFH
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CSLQueryDeviceCapabilitiesIFH(
    INT32   deviceIndex,
    VOID*   pDeviceData,
    SIZE_T  deviceDataSize)
{
    return CSLQueryDeviceCapabilitiesCommon(&g_CSLState, deviceIndex, pDeviceData, deviceDataSize);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CSLAcquireDeviceIFH
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CSLAcquireDeviceIFH(
    CSLHandle           hCSL,
    CSLDeviceHandle*    phDevice,
    INT32               deviceIndex,
    CSLDeviceResource*  pDeviceResourceRequest,
    SIZE_T              numDeviceResources,
    CSLDeviceAttribute* pDeviceAttribute,
    SIZE_T              numDeviceAttribtues,
    const CHAR*         pDeviceName)
{
    CamxResult result = CamxResultSuccess;

    CAMX_UNREFERENCED_PARAM(pDeviceAttribute);
    CAMX_UNREFERENCED_PARAM(numDeviceAttribtues);
    CAMX_UNREFERENCED_PARAM(pDeviceName);

    result = CSLAcquireDeviceCommon(&g_CSLState,
                                    hCSL,
                                    phDevice,
                                    deviceIndex,
                                    pDeviceResourceRequest,
                                    numDeviceResources,
                                    NULL,
                                    0);

    CSLDeviceState* pDevice = g_CSLState.pDevicesState[*phDevice - 1];

    if (pDevice->type == CSLDeviceTypeICP)
    {
        CSLICPAcquireDeviceInfo* pDevInfo =
            static_cast<CSLICPAcquireDeviceInfo*>(pDeviceResourceRequest[0].pDeviceResourceParam);

        pDevInfo->scratchMemSize = 32; // Number doesnt really matter - just need to return a non-zero value
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CSLReleaseDeviceIFH
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CSLReleaseDeviceIFH(
    CSLHandle       hCSL,
    CSLDeviceHandle hDevice)
{
    return CSLReleaseDeviceCommon(&g_CSLState, hCSL, hDevice);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CSLAcquireHardwareIFH
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CSLAcquireHardwareIFH(
    CSLHandle           hCSL,
    CSLDeviceHandle     hDevice,
    CSLDeviceResource*  pDeviceResourceRequest,
    UINT32              version)
{
    CAMX_UNREFERENCED_PARAM(version);
    return CSLAcquireHardwareCommon(&g_CSLState,
                                    hCSL,
                                    hDevice,
                                    pDeviceResourceRequest);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CSLReleaseHardwareIFH
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CSLReleaseHardwareIFH(
    CSLHandle       hCSL,
    CSLDeviceHandle hDevice)
{
    return CSLReleaseHardwareCommon(&g_CSLState, hCSL, hDevice);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CSLLinkIFH
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CSLLinkIFH(
    CSLHandle           hCSL,
    CSLDeviceHandle*    phDevices,
    UINT                handleCount,
    CSLLinkHandle*      phLink)
{
    CAMX_UNREFERENCED_PARAM(hCSL);
    CAMX_UNREFERENCED_PARAM(phDevices);
    CAMX_UNREFERENCED_PARAM(handleCount);
    CAMX_UNREFERENCED_PARAM(phLink);

    CamxResult result = CamxResultSuccess;
    CAMX_NOT_IMPLEMENTED();
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CSLUnlinkIFH
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CSLUnlinkIFH(
    CSLHandle          hCSL,
    CSLLinkHandle*     phLink)
{
    CAMX_UNREFERENCED_PARAM(hCSL);
    CAMX_UNREFERENCED_PARAM(phLink);

    CamxResult result = CamxResultSuccess;
    CAMX_NOT_IMPLEMENTED();
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CSLSyncLinksIFH
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CSLSyncLinksIFH(
    CSLHandle           hCSL,
    CSLLinkHandle*      phLink,
    UINT                handleCount,
    CSLLinkHandle       hMasterhLink,
    CSLSyncLinkMode     syncMode)
{
    CAMX_UNREFERENCED_PARAM(hCSL);
    CAMX_UNREFERENCED_PARAM(phLink);
    CAMX_UNREFERENCED_PARAM(handleCount);
    CAMX_UNREFERENCED_PARAM(hMasterhLink);
    CAMX_UNREFERENCED_PARAM(syncMode);

    CamxResult result = CamxResultSuccess;
    CAMX_NOT_IMPLEMENTED();
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CSLOpenRequestIFH
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult  CSLOpenRequestIFH(
    CSLHandle          hCSL,
    CSLLinkHandle      hLink,
    UINT64             requestId,
    BOOL               bubble,
    CSLSyncLinkMode    syncMode,
    UINT32             expectedExposureTime)
{
    CAMX_UNREFERENCED_PARAM(hCSL);
    CAMX_UNREFERENCED_PARAM(hLink);
    CAMX_UNREFERENCED_PARAM(requestId);
    CAMX_UNREFERENCED_PARAM(bubble);
    CAMX_UNREFERENCED_PARAM(syncMode);
    CAMX_UNREFERENCED_PARAM(expectedExposureTime);

    CamxResult result = CamxResultSuccess;
    CAMX_NOT_IMPLEMENTED();
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CSLCancelRequestIFH
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult  CSLCancelRequestIFH(
    CSLHandle           hCSL,
    const CSLFlushInfo& rCSLFlushInfo)
{
    CAMX_UNREFERENCED_PARAM(hCSL);
    CAMX_UNREFERENCED_PARAM(rCSLFlushInfo);

    CamxResult result = CamxResultSuccess;
    CAMX_NOT_IMPLEMENTED();
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CSLStreamOnIFH
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CSLStreamOnIFH(
    CSLHandle hCSL,
    CSLLinkHandle* phLink,
    CSLDeviceHandle* phDevices)
{
    CAMX_UNREFERENCED_PARAM(phDevices);
    CAMX_UNREFERENCED_PARAM(phLink);
    return CSLStreamOnCommon(&g_CSLState, hCSL);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CSLStreamOffIFH
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CSLStreamOffIFH(
    CSLHandle hCSL,
    CSLLinkHandle* phLink,
    CSLDeviceHandle* phDevices,
    CSLDeactivateMode mode)
{
    CAMX_UNREFERENCED_PARAM(phDevices);
    CAMX_UNREFERENCED_PARAM(phLink);
    CAMX_UNREFERENCED_PARAM(mode);
    return CSLStreamOffCommon(&g_CSLState, hCSL);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CSLSingleDeviceStreamOnIFH
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CSLSingleDeviceStreamOnIFH(
    CSLHandle           hCSL,
    INT32               deviceIndex,
    CSLDeviceHandle*    phDevice)
{
    CAMX_UNREFERENCED_PARAM(hCSL);
    CAMX_UNREFERENCED_PARAM(deviceIndex);
    CAMX_UNREFERENCED_PARAM(phDevice);
    CamxResult result = CamxResultSuccess;
    CAMX_NOT_IMPLEMENTED();
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CSLSingleDeviceStreamOffIFH
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CSLSingleDeviceStreamOffIFH(
    CSLHandle           hCSL,
    INT32               deviceIndex,
    CSLDeviceHandle*    phDevice)
{
    CAMX_UNREFERENCED_PARAM(hCSL);
    CAMX_UNREFERENCED_PARAM(deviceIndex);
    CAMX_UNREFERENCED_PARAM(phDevice);
    CamxResult result = CamxResultSuccess;
    CAMX_NOT_IMPLEMENTED();
    return result;
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ProcessPacket
///
/// @brief This is a helper function used to process a packet.
///
/// @param pSession         Pointer to current session
/// @param pDevice          Pointer to the target device
/// @param pPacketHeader    Pointer to a packet header
///
/// @return CamxResult
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static CamxResult ProcessPacket(
    CSLState*           pCSLState,
    CSLSession*         pSession,
    CSLDeviceState*     pDevice,
    CSLPacketHeader*    pPacketHeader)
{
    CamxResult          result  = CamxResultSuccess;
    CSLPacket*          pPacket = NULL;
    CSLBufferIOConfig*  pIOConfig = NULL;

    // In this implementation, packet processing is done synchronously and the data is in the same memory space so no need
    // to copy and queue packet data. The packet content is read right from the provided pointer.
    // For the time being, just wait on the input buffers and signal the output buffers.

    CAMX_LOG_VERBOSE(CamxLogGroupCSL,
                     "Processing packet: deviceType: %d, opcode: %d, size: %d, requestId: %llu, flags: %d",
                     GetPacketDeviceType(pPacketHeader->opcode),
                     GetPacketOpcode(pPacketHeader->opcode),
                     pPacketHeader->size,
                     pPacketHeader->requestId,
                     pPacketHeader->flags);

    CAMX_ASSERT(GetPacketDeviceType(pPacketHeader->opcode) == static_cast<UCHAR>(pDevice->type));

    switch (pDevice->type)
    {
        // Packets targeting below devices are expected to have in or/and out buffers
        case CSLDeviceTypeVFE:
        case CSLDeviceTypeICP:
        case CSLDeviceTypeIFE:
        case CSLDeviceTypeCPP:
        case CSLDeviceTypeFD:
        case CSLDeviceTypeJPEGD:
        case CSLDeviceTypeJPEGE:
        case CSLDeviceTypeLRME:
            // Regardless of packet opcode, wait on the input buffers and then signal the output buffers.
            pPacket = reinterpret_cast<CSLPacket*>(pPacketHeader);
            pIOConfig = reinterpret_cast<CSLBufferIOConfig*>(reinterpret_cast<BYTE*>(pPacket->data) + pPacket->ioConfigsOffset);
            for (UINT i = 0; i < pPacket->numBufferIOConfigs; i++)
            {
                // Is this an input config
                if (CSLIODirectionInput == pIOConfig[i].direction)
                {
                    // Only wait if a valid fence is provided
                    if (CSLInvalidHandle != pIOConfig[i].hSync)
                    {
                        /// @todo (CAMX-1797) Fix this
                        // CSLFenceWait(pIOConfig[i].hSync, static_cast<UINT64>(-1));
                        CSLFenceWait(pIOConfig[i].hSync, 1);
                    }
                }
            }

            // Now assume the HW is done and output buffers are ready.
            for (UINT i = 0; i < pPacket->numBufferIOConfigs; i++)
            {
                // Is this an output config
                if (CSLIODirectionOutput == pIOConfig[i].direction)
                {
                    // Only signal if a valid fence is provided
                    if (CSLInvalidHandle != pIOConfig[i].hSync)
                    {
                        // Signal fence
                        CSLFenceSignal(pIOConfig[i].hSync, CSLFenceResultSuccess);
                    }
                }
            }

            // Call messageHandler callback if it's set.
            if (NULL != pSession->messageHandler)
            {
                CSLMessage message;
                message.type = CSLMessageTypeFrame;

                // We don't have real frame count in here so use requestId instead.
                message.message.frameMessage.frameCount = pPacket->header.requestId;
                message.message.frameMessage.requestID = pPacket->header.requestId;
                DOUBLE timestamp = static_cast<DOUBLE>((clock() - pCSLState->initializationTime)) / CLOCKS_PER_SEC;

                // convert seconds to nanoseconds
                message.message.frameMessage.timestamp = static_cast<UINT64>(timestamp * 1000000000);
                pSession->messageHandler(pSession->pMessageHandlerUserData, &message);
            }
            break;
        // Packets targeting below devices do not include in/out buffers
        case CSLDeviceTypeFlash:
        case CSLDeviceTypeLensActuator:
            break;

        default:
            CAMX_NOT_IMPLEMENTED();
            break;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CSLSubmitIFH
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CSLSubmitIFH(
    CSLHandle       hCSL,
    CSLDeviceHandle hDevice,
    CSLMemHandle    hPacket,
    SIZE_T          offset)
{
    CamxResult      result      = CamxResultSuccess;
    CSLBufferInfo*  pBufferInfo = NULL;

    CAMX_ASSERT((CSLInvalidHandle != hCSL) && (CSLInvalidHandle != hDevice));

    if (GetCSLInitializedCommon() != TRUE)
    {
        result = CamxResultEInvalidState;
    }
    else if (CSLInvalidHandle == hPacket)
    {
        result = CamxResultEInvalidArg;
        CAMX_LOG_ERROR(CamxLogGroupCSL, "Invalid arguments.");
    }
    else
    {
        g_CSLState.CSLLock->Lock();
        CSLSession*     pSession = GetSession(&g_CSLState, hCSL);
        CSLDeviceState* pDevice  = GetDeviceState(&g_CSLState, hDevice);

        CAMX_ASSERT((NULL != pSession) && (NULL != pDevice));

        if ((NULL == pSession) || (NULL == pDevice))
        {
            result = CamxResultEInvalidPointer;
            CAMX_LOG_ERROR(CamxLogGroupCSL, "hCSL: %d, hDevice: %d, pSession: %p, pDevice: %p",
                            hCSL, hDevice, pSession, pDevice);
        }
        else if (CSLSession::State::Opened != pSession->state ||
            FALSE == pSession->acquired[pDevice->index])
        {
            result = CamxResultEInvalidState;
            CAMX_LOG_ERROR(CamxLogGroupCSL, "Session not open or device not acquired this device (type: %d, index: %d).",
                pDevice->type,
                pDevice->index);
        }
        else
        {
            pBufferInfo = GetBufferInfo(&g_CSLState, hPacket);
            if (NULL == pBufferInfo)
            {
                result = CamxResultEInvalidState;
                CAMX_LOG_ERROR(CamxLogGroupCSL, "Invalid state: bad memory handle");
            }
        }
        g_CSLState.CSLLock->Unlock();

        if (CamxResultSuccess == result)
        {
            result = ProcessPacket(&g_CSLState,
                pSession,
                pDevice,
                static_cast<CSLPacketHeader*>(CamX::Utils::VoidPtrInc(pBufferInfo->pVirtualAddr, offset)));
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CSLFlushLockIFH
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CSLFlushLockIFH(
    CSLHandle           hCSL,
    const CSLFlushInfo& rCSLFlushInfo)
{
    CAMX_UNREFERENCED_PARAM(hCSL);
    CAMX_UNREFERENCED_PARAM(rCSLFlushInfo);

    CamxResult result = CamxResultSuccess;
    CAMX_NOT_IMPLEMENTED();
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CSLFlushUnlockIFH
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CSLFlushUnlockIFH(
    CSLHandle hCSL)
{
    CAMX_UNREFERENCED_PARAM(hCSL);
    CamxResult result = CamxResultSuccess;
    CAMX_NOT_IMPLEMENTED();
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CSLRegisterMessageHandlerIFH
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CSLRegisterMessageHandlerIFH(
    CSLHandle           hCSL,
    CSLLinkHandle       hCSLLinkHandle,
    CSLMessageHandler   messageHandler,
    VOID*               pUserData)
{
    CAMX_UNREFERENCED_PARAM(hCSLLinkHandle);
    return CSLRegisterMessageHandlerCommon(&g_CSLState, hCSL, messageHandler, pUserData);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CSLAllocIFH
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CSLAllocIFH(
    const CHAR*     pStr,
    CSLBufferInfo*  pBufferInfo,
    SIZE_T          bufferSize,
    SIZE_T          alignment,
    UINT32          flags,
    const INT32*    pDeviceIndices,
    UINT            deviceCount)
{
    CAMX_UNREFERENCED_PARAM(pStr);
    CAMX_UNREFERENCED_PARAM(alignment);
    CAMX_UNREFERENCED_PARAM(pDeviceIndices);
    CAMX_UNREFERENCED_PARAM(deviceCount);

    CamxResult result = CamxResultSuccess;

    if (GetCSLInitializedCommon() != TRUE)
    {
        result = CamxResultEInvalidState;
    }
    else
    {
        g_CSLState.CSLLock->Lock();

        if (g_CSLState.memCount + 1 > CSLMaxNumAllocs)
        {
            result = CamxResultENoMore;
            CAMX_LOG_ERROR(CamxLogGroupCSL, "No more memory allocations available.");
        }
        else
        {
            CSLBufferInfo*  pBuffer = NULL;
            CSLMemHandle    hBuffer = ConstructMemAlloc(&g_CSLState, &pBuffer, bufferSize, flags);
            if (CSLInvalidHandle != hBuffer &&
                NULL != pBuffer)
            {
                // Copy into user-provided struct
                *pBufferInfo = *pBuffer;
            }
            else
            {
                result = CamxResultEFailed;
                pBufferInfo->hHandle = CSLInvalidHandle;
            }
        }
        g_CSLState.CSLLock->Unlock();
    }
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CSLMapBufferIFH
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CSLMapBufferIFH(
    CSLBufferInfo*  pBufferInfo,
    INT             bufferFD,
    SIZE_T          offset,
    SIZE_T          bufferLength,
    UINT32          flags,
    const INT32*    pDeviceIndices,
    UINT            deviceCount)
{
    CAMX_UNREFERENCED_PARAM(flags);
    CAMX_UNREFERENCED_PARAM(pDeviceIndices);
    CAMX_UNREFERENCED_PARAM(deviceCount);

    CamxResult result = CamxResultSuccess;

    if (GetCSLInitializedCommon() != TRUE)
    {
        result = CamxResultEInvalidState;
    }
    else if ((NULL == pBufferInfo) || (bufferFD < 0))
    {
        result = CamxResultEInvalidArg;
    }
    else
    {
        g_CSLState.CSLLock->Lock();

        if (g_CSLState.memCount + 1 > CSLMaxNumAllocs)
        {
            result = CamxResultENoMore;
            CAMX_LOG_ERROR(CamxLogGroupCSL, "No more memory allocations available.");
        }
        else
        {
            CSLBufferInfo*  pBuffer = NULL;
            CSLMemHandle    hBuffer = ImportMemAlloc(&g_CSLState, &pBuffer, bufferFD, bufferLength, offset);

            if ((CSLInvalidHandle != hBuffer) && (NULL != pBuffer))
            {
                // Copy into user-provided struct
                *pBufferInfo = *pBuffer;
            }
            else
            {
                result = CamxResultEFailed;
                pBufferInfo->hHandle = CSLInvalidHandle;
            }
        }
        g_CSLState.CSLLock->Unlock();
    }
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CSLMapNativeBufferIFH
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CSLMapNativeBufferIFH(
    CSLBufferInfo*          pBufferInfo,
    const CSLNativeHandle*  phNativeBuffer,
    SIZE_T                  offset,
    SIZE_T                  bufferSize,
    UINT32                  flags,
    const INT32*            pDeviceIndices,
    UINT                    deviceCount)
{
    CAMX_UNREFERENCED_PARAM(pBufferInfo);
    CAMX_UNREFERENCED_PARAM(phNativeBuffer);
    CAMX_UNREFERENCED_PARAM(offset);
    CAMX_UNREFERENCED_PARAM(bufferSize);
    CAMX_UNREFERENCED_PARAM(flags);
    CAMX_UNREFERENCED_PARAM(pDeviceIndices);
    CAMX_UNREFERENCED_PARAM(deviceCount);

    CamxResult result = CamxResultSuccess;

    pBufferInfo->pVirtualAddr = reinterpret_cast<VOID*>(pBufferInfo);
    pBufferInfo->size         = 0xfeedbeef;
    pBufferInfo->hHandle      = 0x1;

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CSLGetBufferInfoIFH
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CSLGetBufferInfoIFH(
    CSLMemHandle    hBuffer,
    CSLBufferInfo*  pBufferInfo)
{
    CamxResult result = CamxResultSuccess;

    if (GetCSLInitializedCommon() != TRUE)
    {
        result = CamxResultEInvalidState;
    }
    else
    {
        *pBufferInfo = *GetBufferInfo(&g_CSLState, hBuffer);
    }
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CSLBufferCacheOpIFH
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CSLBufferCacheOpIFH(
    CSLMemHandle    hBuffer,
    BOOL            invalidate,
    BOOL            clean)
{
    CAMX_UNREFERENCED_PARAM(hBuffer);
    CAMX_UNREFERENCED_PARAM(invalidate);
    CAMX_UNREFERENCED_PARAM(clean);

    // In this fake CSL implementation, all allocations are normal malloc'd memory and not intended to be accessed by camera HW
    // hence nop here.
    return CamxResultSuccess;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CSLReleaseBufferIFH
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CSLReleaseBufferIFH(
    CSLMemHandle hBuffer)
{
    g_CSLState.CSLLock->Lock();
    DestructMemAlloc(&g_CSLState, hBuffer);
    g_CSLState.CSLLock->Unlock();

    return CamxResultSuccess;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CSLReleaseBufferForcedIFH
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

CamxResult CSLReleaseBufferForcedIFH(
    CSLMemHandle hBuffer)
{
    g_CSLState.CSLLock->Lock();
    DestructMemAlloc(&g_CSLState, hBuffer);
    g_CSLState.CSLLock->Unlock();

    return CamxResultSuccess;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CSLCreatePrivateFenceIFH
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CSLCreatePrivateFenceIFH(
    const CHAR* pName,
    CSLFence*   phFenceOut)
{
    return CSLCreatePrivateFenceCommon(&g_CSLState, pName, phFenceOut);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CSLCreateNativeFenceIFH
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CSLCreateNativeFenceIFH(
    CSLNativeFenceCreateDataPtr pCreateData,
    CSLFence*                   phFenceOut)
{
    // We can't really use native fences as they need to be signaled by Kernel. So, for our purposes we just use a private one.
    CAMX_UNREFERENCED_PARAM(pCreateData);
    return CSLCreatePrivateFenceIFH(NULL, phFenceOut);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CSLMergeFencesIFH
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CSLMergeFencesIFH(
    CSLFence*   phFences,
    SIZE_T      fenceCount,
    CSLFence*   phFenceOut)
{
    return CSLMergeFencesCommon(&g_CSLState, phFences, fenceCount, phFenceOut);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CSLGetFenceAttributeIFH
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CSLGetFenceAttributeIFH(
    CSLFence    hFence,
    UINT32      attribute,
    VOID*       pAttributeVal,
    UINT32      valSize)
{
    // IFH does not support any fence attributes.
    CAMX_UNREFERENCED_PARAM(hFence);
    CAMX_UNREFERENCED_PARAM(attribute);
    CAMX_UNREFERENCED_PARAM(pAttributeVal);
    CAMX_UNREFERENCED_PARAM(valSize);

    CamxResult result = CamxResultENoSuch;

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CSLFenceWaitIFH
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CSLFenceWaitIFH(
    CSLFence    hFence,
    UINT64      timeout)
{
    return CSLFenceWaitCommon(&g_CSLState, hFence, timeout);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CSLFenceWaitMultipleIFH
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CSLFenceWaitMultipleIFH(
    CSLFence*   phFences,
    BOOL*       pFenceSignaled,
    SIZE_T      fenceCount,
    UINT64      timeout)
{
    CAMX_UNREFERENCED_PARAM(phFences);
    CAMX_UNREFERENCED_PARAM(pFenceSignaled);
    CAMX_UNREFERENCED_PARAM(fenceCount);
    CAMX_UNREFERENCED_PARAM(timeout);

    CamxResult result = CamxResultEFailed;
    CAMX_NOT_IMPLEMENTED();
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CSLFenceAsyncWaitIFH
/// @note Currently IFH only supports one handler/data per fence
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CSLFenceAsyncWaitIFH(
    CSLFence        hFence,
    CSLFenceHandler handler,
    VOID*           pUserData)
{
    return CSLFenceAsyncWaitCommon(&g_CSLState, hFence, handler, pUserData);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CSLFenceAsyncCancelIFH
/// @note Currently IFH only supports one handler/data per fence
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CSLFenceAsyncCancelIFH(
    CSLFence        hFence,
    CSLFenceHandler handler,
    VOID*           pUserData)
{
    return CSLFenceAsyncCancelCommon(&g_CSLState, hFence, handler, pUserData);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CSLFenceSignalIFH
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CSLFenceSignalIFH(
    CSLFence        hFence,
    CSLFenceResult  fenceResult)
{
    return CSLFenceSignalCommon(&g_CSLState, hFence, fenceResult);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CSLReleaseFenceIFH
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CSLReleaseFenceIFH(
    CSLFence hFence)
{
    return CSLReleaseFenceCommon(&g_CSLState, hFence);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Jump table initialization section
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/// @brief  Define the jump table for CSL IFH mode
CSLJumpTable g_CSLjumpTableIFH =
{
    CSLInitializeIFH,
    CSLUninitializeIFH,
    CSLOpenIFH,
    CSLCloseIFH,
    CSLAddReferenceIFH,
    CSLQueryCameraPlatformIFH,
    CSLImageSensorProbeIFH,
    CSLSetDebugBufferIFH,
    CSLEnumerateDevicesIFH,
    CSLQueryDeviceCapabilitiesIFH,
    CSLAcquireDeviceIFH,
    CSLReleaseDeviceIFH,
    CSLLinkIFH,
    CSLUnlinkIFH,
    CSLSyncLinksIFH,
    CSLOpenRequestIFH,
    CSLCancelRequestIFH,
    CSLStreamOnIFH,
    CSLStreamOffIFH,
    CSLSingleDeviceStreamOnIFH,
    CSLSingleDeviceStreamOffIFH,
    CSLSubmitIFH,
    CSLFlushLockIFH,
    CSLFlushUnlockIFH,
    CSLRegisterMessageHandlerIFH,
    NULL,
    CSLAllocIFH,
    CSLMapBufferIFH,
    CSLMapNativeBufferIFH,
    CSLGetBufferInfoIFH,
    CSLBufferCacheOpIFH,
    CSLReleaseBufferIFH,
    CSLReleaseBufferForcedIFH,
    CSLCreatePrivateFenceIFH,
    CSLCreateNativeFenceIFH,
    CSLMergeFencesIFH,
    CSLGetFenceAttributeIFH,
    CSLFenceWaitIFH,
    CSLFenceWaitMultipleIFH,
    CSLFenceAsyncWaitIFH,
    CSLFenceAsyncCancelIFH,
    CSLFenceSignalIFH,
    CSLReleaseFenceIFH,
    CSLAcquireHardwareIFH,
    CSLReleaseHardwareIFH
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// GetCSLJumpTableIFH
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CSLJumpTable* GetCSLJumpTableIFH(void)
{
    return &g_CSLjumpTableIFH;
}
