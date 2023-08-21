////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2017, 2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  camxpresilmem.h
/// @brief Camxpresilmem class declaration
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef CAMXPRESILMEM_H
#define CAMXPRESILMEM_H
#include <cstdint>

struct _CamxMemHandle
{
    void* pData; ///< Presil memory data
};

typedef _CamxMemHandle* CamxMemHandle;

/// @brief Presil Memory allocation result code definitions
enum CamxMemResult {
    CamxMemSuccess = 0,
    CamxMemFailed
};

typedef int(*CamxMemAllocFunc)(
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

#endif // CAMXPRESILMEM_H
