////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2017, 2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  camxcsllrmedefs.h
/// @brief LRME Hardware Interface Definiation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef CAMXCSLLRMEDEFS_H
#define CAMXCSLLRMEDEFS_H

#include "camxcsl.h"
#include "camxcslresourcedefs.h"
#include "camxdefs.h"

#define BIT_PATTERN(a) (1 << a)
#define ALIGNMENT_8     8

enum CSLLRMEFormats {
    CSLLRMEFormatPD10    = BIT_PATTERN(0),
    CSLLRMEFormatPD8     = BIT_PATTERN(1),
    CSLLRMEFormatY8      = BIT_PATTERN(2),
    CSLLRMEFormatY10     = BIT_PATTERN(3),
    CSLLRMEFormatNV12    = BIT_PATTERN(4),
};

enum CSLLRMEDataHwFormat {
    LRME_FORMAT_LINEAR_PD10 = 0,
    LRME_FORMAT_LINEAR_PD8,
    LRME_FORMAT_Y_ONLY_8BPS,
    LRME_FORMAT_Y_ONLY_10BPS,
    LRME_FORMAT_INVALID,
};

enum CSLLRMEVectorFormat {
    CSLLRMEVectorOutputShort = 0,
    CSLLRMEVectorOutputLong
};

enum CSLLRMEIO
{
    CSLLRMETARInput = 0,
    CSLLRMEREFInput,
    CSLLRMEVectorOutput,
    CSLLRMEDS2Output,
    CSLLRMEIOMax
};

CAMX_BEGIN_PACKED

struct CSLLRMEIOInfo
{
    UINT32 format;
    UINT32 width;
    UINT32 height;
    UINT32 reserved;
} CAMX_PACKED;

CAMX_STATIC_ASSERT(((sizeof(CSLLRMEIOInfo) % ALIGNMENT_8) == 0));

struct CSLLRMEAcquireDeviceInfo
{
    UINT32 fps;
    UINT32 ds2Enable;
    struct CSLLRMEIOInfo ioFormat[CSLLRMEIOMax];
} CAMX_PACKED;

CAMX_STATIC_ASSERT(((sizeof(CSLLRMEAcquireDeviceInfo) % ALIGNMENT_8) == 0));

CAMX_END_PACKED

#endif // CAMXCSLLRMEDEFS_H
