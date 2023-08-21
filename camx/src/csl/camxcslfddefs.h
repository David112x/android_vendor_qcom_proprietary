////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2017 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  camxcslfddefs.h
/// @brief FD Hardware Interface Definiation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef CAMXCSLFDDEFS_H
#define CAMXCSLFDDEFS_H

// NOWHINE FILE GR016: Converting CSL header to C, we need typedef

#include "camxcsl.h"
#include "camxcslresourcedefs.h"
#include "camxdefs.h"

CAMX_BEGIN_PACKED

/// @brief Maximum number of faces that can be detected by HW
static const UINT32 CSLFDMaxFaces           = 35;

/// @brief Number of entries for Raw results data
static const UINT32 CSLFDRawResultEntries   = 512;

/// @brief This enumerates FD Packet Op codes.
typedef enum
{
    CSLPacketOpCodesFDFrameUpdate   = 0x0,  ///< Packet that contains FD Frame Update configuration
} CSLPacketOpcodesFD;

/// @brief This enumerates FD Packet Command Buffer IDs
typedef enum
{
    CSLFDCmdBufferIdGeneric = 0x0,  ///< FD Generic command buffer consumed by KMD
    CSLFDCmdBufferIdCDM,            ///< FD CDM command buffer to be submitted to HW
    CSLFDCmdBufferIdMax,            ///< Max command buffer id
} CSLFDCmdBufferId;

/// @brief This enumerates FD Packet Blob types
typedef enum
{
    CSLFDBlobTypeSOCClockBwRequest  = 0x0,  ///< Blob type for SOC Clock and Bandwidth request
    CSLFDBlobTypeRawResultsRequired,        ///< Blob type to indicate whether Raw Results are required
} CSLFDBlobType;

/// @brief This enumerates FD Packet Input port IDs
typedef enum
{
    CSLFDInputPortIdImage   = 0x0,  ///< Indicates FD Input Image resource
    CSLFDInputPortIdMax,            ///< Indicates max FD Input Port Ids
} CSLFDInputPortId;

/// @brief This enumerates FD Packet Output port IDs
typedef enum
{
    CSLFDOutputPortIdResults    = 0x0,  ///< Indicates FD Output Results resouces
    CSLFDOutputPortIdRawResults,        ///< Indicates FD Output Raw Results resouces
    CSLFDOutputPortIdWorkBuffer,        ///< Indicates FD Output Work Buffer resouces
    CSLFDOutputPortIdMax,               ///< Indicates Max FD Output Port Ids
} CSLFDOutputPortId;

/// @brief This enumerates FD HW modes
typedef enum
{
    CSLFDHWModeFaceDetection    = 0x1,  ///< Indicates HW Face Detection mode
    CSLFDHWModePyramid          = 0x2,  ///< Indicates HW Pyramid mode
} CSLFDHWModeMask;

/// @brief This enumerates FD session priorities
typedef enum
{
    CSLFDPriorityHigh   = 0x0,  ///< Indicates High priority request
    CSLFDPriorityNormal,        ///< Indicates Normal priority request
} CSLFDPriority;

/// @brief FD Clock rate, bandwidth requirement information
typedef struct
{
    UINT64  clockRate;      ///< Clock rate required for this acquire/request
    UINT64  bandwidth;      ///< Bandwidth value required for this acquire/request
    UINT64  reserved[4];    ///< Reserved field
} CAMX_PACKED CSLFDSOCClockBwRequest;

/// @brief FD Face properties
typedef struct
{
    UINT32  prop1;  ///< Property 1 of face
    UINT32  prop2;  ///< Property 2 of face
    UINT32  prop3;  ///< Property 3 of face
    UINT32  prop4;  ///< Property 4 of face
} CAMX_PACKED CSLFDFace;

/// @brief FD Face results information, this is inline with the memory layout requirement of FD HW
typedef struct
{
    CSLFDFace   faces[CSLFDMaxFaces];   ///< Array of faces information
    UINT32      faceCount;              ///< Number of faces detected
    UINT32      reserved[3];            ///< Reserved field
} CAMX_PACKED CSLFDResults;

/// @brief FD HW capabilities
typedef struct
{
    CSLVersion  coreVesion;             ///< FD HW core version
    CSLVersion  wrapperVersion;         ///< FD HW wrapper version
    UINT32      rawResultsAvailable;    ///< Whether Raw results available on this HW
    UINT32      supportedModes;         ///< Modes supported by this HW. enum CSLFDHWModeMask
    UINT64      reserved;               ///< Reserved field
} CAMX_PACKED CSLFDHWCaps;

/// @brief FD HW Acquire information
typedef struct
{
    CSLFDSOCClockBwRequest  SOCRequest;     ///< SOC clock, bandwidth request information
    UINT32                  priority;       ///< Priority for this acquire. enum CSLFDPriority
    UINT32                  mode;           ///< Mode in which to run FD HW for this acquire. enum CSLFDHWModeMask
    UINT32                  getRawResults;  ///< Whether this acquire needs face raw results
    UINT32                  reserved[13];   ///< Reserved field
} CAMX_PACKED CSLFDAcquireDeviceInfo;

CAMX_END_PACKED

#endif // CAMXCSLFDDEFS_H
