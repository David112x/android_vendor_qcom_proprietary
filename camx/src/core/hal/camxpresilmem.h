////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2017-2020 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  camxpresilmem.h
/// @brief Camxpresilmem class declaration
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef CAMXPRESILMEM_H
#define CAMXPRESILMEM_H

struct _CamxMemHandle;
typedef _CamxMemHandle* CamxMemHandle;

/**
*
* @brief Presil Memory allocation result code definitions
*
*/
enum CamxMemResult
{
    CamxMemSuccess = 0, ///< Success
    CamxMemFailed ///< Failure
};

typedef uint32_t(*CamxMemAllocFunc)(
    CamxMemHandle* pphMem,
    uint32_t       width,
    uint32_t       height,
    uint32_t       format,
    uint32_t       usageFlags);

typedef void(*CamxMemReleaseFunc)(
    CamxMemHandle pphStreamBuffer);

typedef uint32_t(*CamxMemGetImageSizeFunc)(
    uint32_t       width,
    uint32_t       height,
    uint32_t       format,
    uint32_t       usage);

typedef uint32_t(*CamxMemGetImageSizeStrideFunc)(
    uint32_t       width,
    uint32_t       height,
    uint32_t       format,
    uint32_t       usage,
    uint32_t*      pPlaneStride,
    uint32_t*      pSliceHeight);

#endif // CAMXPRESILMEM_H
