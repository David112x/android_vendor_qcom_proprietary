////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2016 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  camxcslandroid.h
/// @brief The definition of Android-dependent CSL entities
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef CAMXCSLANDROID_H
#define CAMXCSLANDROID_H

// NOWHINE FILE GR016: Converting CSL header to C, we need typedef

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

#include "camxdefs.h"
#include "camxtypes.h"

/// @brief Defines in-out data for creating/importing CSL fences from native fences.
typedef struct
{
    UINT32 mode;                ///< tSpecifies the mode of this API and how the input should be interpreted.
    union
    {
        INT32 fd;               ///< In import mode, this specifies the native fence FD to be imported.
        struct
        {
            UINT32 timelineId;  ///< A 32bit data field that may be used in other modes of operation.
            UINT64 syncPoint;   ///< A 64bit data field that may be used in other modes of operation.
        };
        BYTE data[12];          ///< Byte access to the data.
    };
} CSLAndroidNativeFenceCreateData;

/// @brief Define various supported attributes that can be queried. Platform-specific attributes may be defined starting from
///        CSLFenceAttribLast + 1.
typedef enum
{
    CSLAndroidFenceAttribNativeFD = 0, ///< Indicates the FD of a native fence
    CSLAndroidFenceAttribLast          ///< The number of defined attributes
} CSLAndroidFenceAttrib;


#ifdef __cplusplus
}
#endif // __cplusplus

#endif // CAMXCSLANDROID_H
