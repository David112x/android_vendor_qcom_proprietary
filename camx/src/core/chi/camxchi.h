////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2017-2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  camxchi.h
/// @brief Landing methods for CamX implementation of CHI
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef CAMXCHI_H
#define CAMXCHI_H

#include "camxdefs.h"
#include "camxcsl.h"
#include "chi.h"
#include "chicommon.h"
#include "camxdebugprint.h"
#include "camximagebuffer.h"
#include "chinode.h"

CAMX_NAMESPACE_BEGIN

struct HALCallbacks;

enum ChiFenceState
{
    ChiFenceInit    = 0,                ///< Fence initialized
    ChiFenceSuccess,                    ///< Fence signaled with success
    ChiFenceFailed,                     ///< Fence signaled with failure
    ChiFenceInvalid                     ///< Fence invalid state
};

struct ChiFence
{
    CHIFENCEHANDLE      hChiFence;      ///< Handle to this Chi fence instance
    CHIFENCETYPE        type;           ///< Chi fence type
    INT                 aRefCount;      ///< Reference count
    CSLFence            hFence;         ///< CSL fence representing this Chi fence
    ChiFenceState       resultState;    ///< Fence signal result state
    union
    {
        UINT64          eglSync;        ///< EGL sync object (need to cast to EGLSyncKHR)
        INT             nativeFenceFD;  ///< Native fence file descriptor
    };
};

extern ChiContextOps g_jumpTableCHI;       ///< Global driver CHI entry point jump table

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ChiOverrideBypass
///
/// @brief  Entry point used by the HAL layer to bypass the override, if necessary.  Not for use by the override.  Will be
///         deprecated.
///
/// @param  pBypassCallbacks  Function pointers for bypassing the override
///
/// @return None
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID ChiOverrideBypass(
    HALCallbacks* pBypassCallbacks);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// IsValidChiBuffer
///
/// @brief  Check whether incoming CHI Buffer is of valid type or not
///
/// @param  pChiBufferInfo  Pointer to Chi Buffer info
///
/// @return TRUE if valid, FALSE otherwise
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CAMX_INLINE BOOL IsValidChiBuffer(
    const CHIBUFFERINFO* pChiBufferInfo)
{
    BOOL bIsValid = FALSE;

    if ((NULL != pChiBufferInfo) && (NULL != pChiBufferInfo->phBuffer))
    {
        // For CHI buffers, currently we support only one of these :
        //     HALGralloc : Gralloc native handle that is coming from HAL
        //     ChiGralloc : Gralloc native handle that is created by CHI for CHI allocated buffers
        //     ChiNative  : Native buffer allocated and owned by CHI through bufferManagerOps APIs
        if ((HALGralloc == pChiBufferInfo->bufferType) ||
            (ChiGralloc == pChiBufferInfo->bufferType) ||
            (ChiNative  == pChiBufferInfo->bufferType))
        {
            bIsValid = TRUE;
        }
        else
        {
            CAMX_LOG_ERROR(CamxLogGroupCore, "Buffer handle [%p] : unsupported buffer type %d",
                           pChiBufferInfo->phBuffer, pChiBufferInfo->bufferType);
        }
    }

    return bIsValid;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// IsGrallocBuffer
///
/// @brief  Check whether incoming CHI Buffer is gralloc buffer
///
/// @param  bufferType  Pointer to Chi Buffer info
///
/// @return TRUE if it is grallloc buffer, FALSE otherwise
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CAMX_INLINE BOOL IsGrallocBuffer(
    const CHIBUFFERINFO* pChiBufferInfo)
{
    BOOL          bIsNativeHandleType = FALSE;
    ImageBuffer*  pImageBuffer = NULL;
    ChiBufferType bufferType;

    if (TRUE == IsValidChiBuffer(pChiBufferInfo))
    {
        bufferType = pChiBufferInfo->bufferType;

        if ((HALGralloc == bufferType) ||
            (ChiGralloc == bufferType))
        {
            bIsNativeHandleType = TRUE;
        }
        else if (ChiNative == bufferType)
        {
            pImageBuffer        = static_cast<ImageBuffer*>(pChiBufferInfo->phBuffer);
            bIsNativeHandleType = pImageBuffer->IsGrallocBuffer();
        }
    }

    return bIsNativeHandleType;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// IsCSLBuffer
///
/// @brief  Check whether incoming CHI Buffer is csl buffer
///
/// @param  bufferType  Pointer to Chi Buffer info
///
/// @return TRUE if it is csl buffer, FALSE otherwise
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CAMX_INLINE BOOL IsCSLBuffer(
    const CHIBUFFERINFO* pChiBufferInfo)
{
    BOOL          bIsCSLBuffer = FALSE;
    ImageBuffer*  pImageBuffer = NULL;

    if (TRUE == IsValidChiBuffer(pChiBufferInfo))
    {
        if (ChiNative == pChiBufferInfo->bufferType)
        {
            pImageBuffer = static_cast<ImageBuffer*>(pChiBufferInfo->phBuffer);
            bIsCSLBuffer = pImageBuffer->IsCSLBuffer();
        }
    }

    return bIsCSLBuffer;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// IsChiNativeBufferType
///
/// @brief  Check whether incoming CHI Buffer is of ChiNative type
///
/// @param  pChiBufferInfo      Pointer to ChiBufferInfo structure
///
/// @return TRUE if ChiNative type, FALSE othewise
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CAMX_INLINE BOOL IsChiNativeBufferType(
    const CHIBUFFERINFO* pChiBufferInfo)
{
    BOOL bIsChiNative = FALSE;

    if ((NULL != pChiBufferInfo) && (ChiNative == pChiBufferInfo->bufferType))
    {
        bIsChiNative = TRUE;
    }

    return bIsChiNative;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// GetBufferHandleFromBufferInfo
///
/// @brief  Get native buffer handle from BufferInfo struct depending on the bufferType
///
/// @param  pChiBufferInfo  Pointer to Chi Buffer info
///
/// @return BufferHandle pointer
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CAMX_INLINE BufferHandle* GetBufferHandleFromBufferInfo(
    const CHIBUFFERINFO* pChiBufferInfo)
{
    BufferHandle* phNativeHandle = NULL;
    CHIBUFFERTYPE bufferType;

    if (TRUE == IsGrallocBuffer(pChiBufferInfo))
    {
        bufferType = pChiBufferInfo->bufferType;

        if ((HALGralloc == bufferType) ||
            (ChiGralloc == bufferType))
        {
            phNativeHandle = reinterpret_cast<BufferHandle*>(pChiBufferInfo->phBuffer);
        }
        else if (ChiNative == bufferType)
        {
            phNativeHandle = reinterpret_cast<ImageBuffer*>(pChiBufferInfo->phBuffer)->GetGrallocBufferHandle();
        }
    }
    else
    {
        CAMX_LOG_ERROR(CamxLogGroupCore, "Non gralloc buffer not supported yet!");
    }

    return phNativeHandle;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// IsValidCSLFence
///
/// @brief  Check if the CSL fence is valid or not
///
/// @param  hCSLFence  CLS fence handle
///
/// @return TRUE if the fence value is valid, otherwise FALSE
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CAMX_INLINE BOOL IsValidCSLFence(
    CSLFence hCSLFence)
{
    return ((CSLInvalidHandle != hCSLFence) && (-1 != hCSLFence));
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// IsValidCHIFence
///
/// @brief  Check if the CHI fence is valid or not
///
/// @param  phChiFence  Pointer to CHI fence handle
///
/// @return TRUE if the CHI fence and CSL fence it has is also valid, otherwise FALSE
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CAMX_INLINE BOOL IsValidCHIFence(
    CHIFENCEHANDLE* phChiFence)
{
    return ((NULL != phChiFence) && (NULL != *phChiFence) && IsValidCSLFence(reinterpret_cast<ChiFence*>(*phChiFence)->hFence));
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ChiBufferManagerCreate
///
/// @brief  Register callbacks to a ChiNode Interface
///
/// @param  pBufferManagerName  Pointer to CHI node interface
/// @param  pCreateData         Pointer to CHI node interface
///
/// @return CamxResultSuccess if successful
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CHIBUFFERMANAGERHANDLE ChiBufferManagerCreate(
    const CHAR*                 pBufferManagerName,
    CHIBufferManagerCreateData* pCreateData);


CAMX_NAMESPACE_END

#endif // CAMXCHI_H
