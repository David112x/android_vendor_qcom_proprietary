////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2017-2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  camxcslicpdefs.h
/// @brief ICP Hardware Interface Definiation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef CAMXCSLICPDEFS_H
#define CAMXCSLICPDEFS_H

#include "camxcsl.h"
#include "camxcslresourcedefs.h"
#include "camxdefs.h"
#include "camxpacketdefs.h"

CAMX_BEGIN_PACKED

static const UINT CSLICPGenericBlobCmdBufferClk              = 0x1;
static const UINT CSLICPGenericBlobCmdBufferConfigIO         = 0x2;
static const UINT CSLICPGenericBlobCmdBufferMapFWMemRegion   = 0x3;
static const UINT CSLICPGenericBlobCmdBufferUnMapFWMemRegion = 0x4;
static const UINT CSLICPGenericBlobCmdBufferClkV2            = 0x5;

static const UINT CSLICPMaxMemoryMapRegions = 10;
static const UINT CSLICPMemoryMapVersion0   = 0;

/// @brief ICP hardware version information
struct CSLICPDeviceVersion
{
    UINT32      deviceType; ///< Device type for the cap info(CSLICPDeviceType)
    UINT32      reserved;   ///< Reserved field to force size as a multiple of 64bits
    CSLVersion  version;    ///< Device version
} CAMX_PACKED;

/// @brief ICP device capability information
///         Contains information about firmware and its API version along with number of BPS and IPE resources available
struct CSLICPQueryCapabilityCmd
{
    CSLVersion          firmwareVersion;                                ///< Firmware version info
    CSLVersion          APIVersion;                                     ///< API version info
    UINT32              numIPE;                                         ///< Number of ipes
    UINT32              numBPS;                                         ///< Number of bps
    CSLICPDeviceVersion deviceVersion[CSLICPResourceIDMaxNumResources]; ///< Returned device capability array,
                                                                        ///< numDevices = numBPS + numIPE
} CAMX_PACKED;

/// @brief ICP resource characteristics (dimension and format) information
struct CSLICPResourceInfo
{
    UINT32      format;        ///< Image format
    UINT32      width;         ///< Image width
    UINT32      height;        ///< Image height
    UINT32      FPS;           ///< Stream frame per second
} CAMX_PACKED;

/// @brief ICP acquire device information
struct CSLICPAcquireDeviceInfo
{
    INT32               scratchMemSize;     ///< Scratch Mem size (Output Param)
    UINT32              resourceType;       ///< Resource type (IPE RealTtime / IPE NonRealTime / BPS)
    UINT32              IOConfigLen;        ///< Sizeof memory pointer for CONFIG_IO payload
    CSLMemHandle        hIOConfigCmd;       ///< Handle of memory allocated for CONFIG_IO command payload
    UINT32              secureMode;         ///< Secure camera session or non secure
    INT32               chainIdentifier;    ///< Identifier for IPE BPS dependancy
    CSLICPResourceInfo  inputResource;      ///< Input resource info struct
    INT32               numOutputResource;  ///< Number of output resources
    CSLICPResourceInfo  outputResource[1];  ///< Output resource info struct
} CAMX_PACKED;

/// @brief ICP clock and bandwidth request
struct CSLICPClockBandwidthRequest
{
    UINT64               budgetNS;               ///< Maximum duration to process a frame in Nano Seconds
    UINT32               frameCycles;            ///< FrameCycles Info
    UINT32               realtimeFlag;           ///< Flag for Realtime stream
    UINT64               unCompressedBW;         ///< Uncompressed bandwidth
    UINT64               compressedBW;           ///< Compressed bandwidth
} CAMX_PACKED;

/// @brief ICP clock and bandwidth request
struct CSLICPClockBandwidthRequestV2
{
    UINT64               budgetNS;               ///< Maximum duration to process a frame in Nano Seconds
    UINT32               frameCycles;            ///< FrameCycles Info
    UINT32               realtimeFlag;           ///< Flag for Realtime stream
    UINT32               reserved;               ///< reserved
    UINT32               numPaths;               ///< number of axi paths in bw request
    CSLAXIperPathBWVote  outputPathBWInfo[1];    ///< Per path vote info for IPE/BPS
} CAMX_PACKED;

/// @brief ICP Memory Map Region Info
struct CSLICPMemoryMapRegionInfo
{
    INT32  hHandle; ///< Mem Handle of the memory region
    UINT32 offset;  ///< Offset of the memory region relative to fd
    UINT32 size;    ///< Size of the memory region
    UINT32 flags;   ///< Flags if any
} CAMX_PACKED;

///@ brief ICP Memory Map Update request
struct CSLICPMemoryMapUpdate
{
    UINT32                    version;                               ///< Version of the Structure
    UINT32                    numberOfMappings;                      ///< number of memory map regions
    CSLICPMemoryMapRegionInfo regionInfo[CSLICPMaxMemoryMapRegions]; ///< memory map regions info
} CAMX_PACKED;

CAMX_END_PACKED

#endif // CAMXCSLICPDEFS_H
