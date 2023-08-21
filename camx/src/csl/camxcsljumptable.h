////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2016-2020 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  camxcsljumptable.h
/// @brief Defines a jump table for CSL entries.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef CAMXCSLJUMPTABLE_H
#define CAMXCSLJUMPTABLE_H

#include "camxtypes.h"
#include "camxcsl.h"

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// GetCSLMode
///
/// @brief  Get the CSL mode from CSL mode manager
///
/// @return CSL mode
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CSLMode GetCSLMode(void);

/// @brief  Jump table for CSL entries
struct CSLJumpTable
{
    CamxResult (*CSLInitialize)(void);

    CamxResult (*CSLUninitialize)(void);

    CamxResult (*CSLOpen)(
        CSLHandle* phCSL);

    CamxResult (*CSLClose)(
        CSLHandle hCSL);

    CamxResult (*CSLAddReference)(
        CSLHandle hCSL);

    CamxResult (*CSLQueryCameraPlatform)(
        CSLCameraPlatform* pCameraPlatform);

    CamxResult (*CSLImageSensorProbe)(
        CSLMemHandle                hPacket,
        SIZE_T                      offset,
        CSLImageSensorProbeResult*  pProbeResult);

    CamxResult (*CSLSetDebugBuffer)(
        CSLHandle               hCSL,
        CSLDebugBufferType      type,
        CSLMemHandle            hBuffer,
        SIZE_T                  offset,
        SIZE_T                  length,
        CSLDebugBufferResult*   pDebugBufferResult);

    CamxResult (*CSLEnumerateDevices)(
        CSLDeviceDescriptor* pDeviceDescriptor);

    CamxResult (*CSLQueryDeviceCapabilities)(
        INT32   deviceIndex,
        VOID*   pDeviceData,
        SIZE_T  deviceDataSize);

    CamxResult (*CSLAcquireDevice)(
        CSLHandle           hCSL,
        CSLDeviceHandle*    phDevice,
        INT32               deviceIndex,
        CSLDeviceResource*  pDeviceResourceRequest,
        SIZE_T              numDeviceResources,
        CSLDeviceAttribute* pDeviceAttribute,
        SIZE_T              numDeviceAttributes,
        const CHAR*         pDeviceName);

    CamxResult (*CSLReleaseDevice)(
        CSLHandle       hCSL,
        CSLDeviceHandle hDevice);

    CamxResult (*CSLLink)(
        CSLHandle           hCSL,
        CSLDeviceHandle*    phDevices,
        UINT                handleCount,
        CSLLinkHandle*      phLink);

    CamxResult (*CSLUnlink)(
        CSLHandle          hCSL,
        CSLLinkHandle*     phLink);

    CamxResult (*CSLSyncLinks)(
        CSLHandle           hCSL,
        CSLLinkHandle*      phLink,
        UINT                handleCount,
        CSLLinkHandle       hMasterhLink,
        CSLSyncLinkMode     syncMode);

    CamxResult  (*CSLOpenRequest)(
        CSLHandle          hCSL,
        CSLLinkHandle      hLink,
        UINT64             requestId,
        BOOL               bubble,
        CSLSyncLinkMode    syncMode,
        UINT32             expectedExposureTimeInMs);

    CamxResult  (*CSLCancelRequest)(
        CSLHandle           hCSL,
        const CSLFlushInfo& pCSLFlushInfo);

    CamxResult (*CSLStreamOn)(
        CSLHandle hCSL,
        CSLLinkHandle* phLink,
        CSLDeviceHandle* phDevices);

    CamxResult (*CSLStreamOff)(
        CSLHandle hCSL,
        CSLLinkHandle* phLink,
        CSLDeviceHandle* phDevices,
        CSLDeactivateMode mode);

    CamxResult(*CSLSingleDeviceStreamOn)(
        CSLHandle           hCSL,
        INT32               deviceIndex,
        CSLDeviceHandle*    phDevice);

    CamxResult(*CSLSingleDeviceStreamOff)(
        CSLHandle           hCSL,
        INT32               deviceIndex,
        CSLDeviceHandle*    phDevice);

    CamxResult (*CSLSubmit)(
        CSLHandle           hCSL,
        CSLDeviceHandle     hDevice,
        CSLMemHandle        hPacket,
        SIZE_T              offset);

    CamxResult(*CSLFlushLock)(
        CSLHandle           hCSL,
        const CSLFlushInfo& rCSLFlushInfo);

    CamxResult(*CSLFlushUnlock)(
        CSLHandle           hCSL);

    CamxResult (*CSLRegisterMessageHandler)(
        CSLHandle           hCSL,
        CSLLinkHandle       hCSLLinkHandle,
        CSLMessageHandler   handler,
        VOID*               pUserData);

    CamxResult (*CSLRegisterSessionMessageHandler)(
        CSLHandle                  hCSL,
        CSLSessionMessageHandler   msgHandler,
        VOID*                      pUserData);

    CamxResult (*CSLAlloc)(
        const CHAR*     pStr,
        CSLBufferInfo*  pBufferInfo,
        SIZE_T          bufferSize,
        SIZE_T          alignment,
        UINT32          flags,
        const INT32*    pDeviceIndices,
        UINT            deviceCount);

    CamxResult (*CSLMapBuffer)(
        CSLBufferInfo*  pBufferInfo,
        INT             bufferFD,
        SIZE_T          offset,
        SIZE_T          bufferLength,
        UINT32          flags,
        const INT32*    pDeviceIndices,
        UINT            deviceCount);

    CamxResult (*CSLMapNativeBuffer)(
        CSLBufferInfo*          pBufferInfo,
        const CSLNativeHandle*  phNativeBuffer,
        SIZE_T                  offset,
        SIZE_T                  bufferLength,
        UINT32                  flags,
        const INT32*            pDeviceIndices,
        UINT                    deviceCount);

    CamxResult (*CSLGetBufferInfo)(
        CSLMemHandle    hBuffer,
        CSLBufferInfo*  pBufferInfo);

    CamxResult (*CSLBufferCacheOp)(
        CSLMemHandle    hBuffer,
        BOOL            invalidate,
        BOOL            clean);

    CamxResult (*CSLReleaseBuffer)(
        CSLMemHandle hBuffer);

    CamxResult (*CSLReleaseBufferForced)(
        CSLMemHandle hBuffer);

    CamxResult (*CSLCreatePrivateFence)(
        const CHAR* pName,
        CSLFence*   pFenceOut);

    CamxResult (*CSLCreateNativeFence)(
        CSLNativeFenceCreateDataPtr pCreateData,
        CSLFence*                   pFenceOut);

    CamxResult (*CSLMergeFences)(
        CSLFence*   phFences,
        SIZE_T      fenceCount,
        CSLFence*   pFenceOut);

    CamxResult (*CSLGetFenceAttrib)(
        CSLFence    hFence,
        UINT32      attrib,
        VOID*       pAttribVal,
        UINT32      valSize);

    CamxResult (*CSLFenceWait)(
        CSLFence    hFence,
        UINT64      timeout);

    CamxResult (*CSLFenceWaitMultiple)(
        CSLFence*   phFences,
        BOOL*       pFenceSignaled,
        SIZE_T      fenceCount,
        UINT64      timeout);

    CamxResult (*CSLFenceAsyncWait)(
        CSLFence        hFence,
        CSLFenceHandler handler,
        VOID*           pUserData);

    CamxResult (*CSLFenceAsyncCancel)(
        CSLFence        hFence,
        CSLFenceHandler handler,
        VOID*           pUserData);

    CamxResult (*CSLFenceSignal)(
        CSLFence        hFence,
        CSLFenceResult  status);

    CamxResult (*CSLReleaseFence)(
        CSLFence hFence);

    CamxResult(*CSLAcquireHardware)(
        CSLHandle           hCSL,
        CSLDeviceHandle     hDevice,
        CSLDeviceResource*  pDeviceResourceRequest,
        UINT32              version);

    CamxResult(*CSLReleaseHardware)(
        CSLHandle       hCSL,
        CSLDeviceHandle hDevice);

    CamxResult (*CSLDumpRequest) (
        CSLHandle              hCSL,
        CSLDumpRequestInfo*    pDumpRequestInfo,
        SIZE_T*                pFilledLength);
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief  Encapsulates CSL jumptable initialization and access.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class CSLModeManager
{
public:
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// CSLModeManager
    ///
    /// @brief  Constructor
    ///
    /// @param  pInitParams CSL initialization params used for creation
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    explicit CSLModeManager(
        const CSLInitializeParams* pInitParams);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ~CSLModeManager
    ///
    /// @brief  Destructor
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual ~CSLModeManager();


    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetJumpTable
    ///
    /// @brief  Returns the jump table for CSL entry points
    ///
    /// @return Pointer to the CSL jump table for this call
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    inline CSLJumpTable* GetJumpTable() const
    {
        return m_pJumpTable;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// Mode
    ///
    /// @brief  Returns the CSL mode
    ///
    /// @return CSLMode used for jumptable update
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    inline CSLMode Mode() const
    {
        return m_mode;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// SensorEmulation
    ///
    /// @brief  Returns TRUE if sensor emulation is turned on
    ///
    /// @return Sensor emulation mode
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    inline BOOL SensorEmulation() const
    {
        return m_emulatedSensorParams.enableSensorSimulation;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// DumpSensorEmulationOutput
    ///
    /// @brief  Returns TRUE if sensor emulation dumps are turned on
    ///
    /// @return TRUE if dump sensor output is on.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    inline BOOL DumpSensorEmulationOutput() const
    {
        return m_emulatedSensorParams.dumpSensorEmulationOutput;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// SensorEmulator
    ///
    /// @brief  Returns sensor emulator app name
    ///
    /// @return Sensor emulator app name
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    inline const CHAR* SensorEmulator() const
    {
        return m_emulatedSensorParams.sensorEmulator;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// SensorEmulatorPath
    ///
    /// @brief  Returns sensor emulator app path
    ///
    /// @return Sensor emulator app path
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    inline const CHAR* SensorEmulatorPath() const
    {
        return m_emulatedSensorParams.sensorEmulatorPath;
    }

private:
    CSLJumpTable*           m_pJumpTable;                       ///< Current jump table for CSL
    CSLMode                 m_mode;                             ///< Current mode
    CSLEmulatedSensorParams m_emulatedSensorParams;             ///< Current sensor emulation mode

    CSLModeManager(const CSLModeManager&) = delete;             ///< Disable copy constructor
    CSLModeManager& operator=(const CSLModeManager&) = delete;  ///< Disable assignment
};


#endif // CAMXCSLJUMPTABLE_H
