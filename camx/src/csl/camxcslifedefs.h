////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2017-2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  camxcslifedefs.h
/// @brief IFE Hardware Interface Definition
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef CAMXCSLIFEDEFS_H
#define CAMXCSLIFEDEFS_H
#include "camxutils.h"

CAMX_NAMESPACE_BEGIN

static const UINT32 IFEOutputFull          = 0x3000;
static const UINT32 IFEOutputDS4           = 0x3001;
static const UINT32 IFEOutputDS16          = 0x3002;
static const UINT32 IFEOutputRaw           = 0x3003;
static const UINT32 IFEOutputFD            = 0x3004;
static const UINT32 IFEOutputPDAF          = 0x3005;
static const UINT32 IFEOutputRDI0          = 0x3006;
static const UINT32 IFEOutputRDI1          = 0x3007;
static const UINT32 IFEOutputRDI2          = 0x3008;
static const UINT32 IFEOutputRDI3          = 0x3009;
static const UINT32 IFEOutputStatsHDRBE    = 0x300A;
static const UINT32 IFEOutputStatsHDRBHIST = 0x300B;
static const UINT32 IFEOutputStatsTLBG     = 0x300C;
static const UINT32 IFEOutputStatsBF       = 0x300D;
static const UINT32 IFEOutputStatsAWBBG    = 0x300E;
static const UINT32 IFEOutputStatsBHIST    = 0x300F;
static const UINT32 IFEOutputStatsRS       = 0x3010;
static const UINT32 IFEOutputStatsCS       = 0x3011;
static const UINT32 IFEOutputStatsIHIST    = 0x3012;
static const UINT32 IFEOutputDisplayFull   = 0x3013;
static const UINT32 IFEOutputDisplayDS4    = 0x3014;
static const UINT32 IFEOutputDisplayDS16   = 0x3015;
static const UINT32 IFEOutputDualPD        = 0x3016;
static const UINT32 IFEOutputRDIRD         = 0x3017;
static const UINT32 IFEOutputLCR           = 0x3018;

// IFE input resource type
static const UINT32 IFEInputTestGen        = 0x4000;
static const UINT32 IFEInputPHY0           = 0x4001;
static const UINT32 IFEInputPHY1           = 0x4002;
static const UINT32 IFEInputPHY2           = 0x4003;
static const UINT32 IFEInputPHY3           = 0x4004;
static const UINT32 IFEInputPHY4           = 0x4005;
static const UINT32 IFEInputPHY5           = 0x4006;

// IFE fetch engine resource type
static const UINT32 IFEInputBusRead        = 0x4007;

// IFE input resource Lane Type
static const UINT32 IFELaneTypeDPHY        = 0;
static const UINT32 IFELaneTypeCPHY        = 1;

// IFE usage Types
static const UINT32 IFEUsageInvalid         = 0;
static const UINT32 IFEUsageLeftPixelPath   = 1;
static const UINT32 IFEUsageRighftPixelPath = 2;
static const UINT32 IFEUsageRDIPath         = 3;

// IFE Generic Blob types
static const UINT32 IFEGenericBlobTypeHFRConfig             = 0;
static const UINT32 IFEGenericBlobTypeResourceClockConfig   = 1;
static const UINT32 IFEGenericBlobTypeResourceBWConfig      = 2;
static const UINT32 IFEGenericBlobTypeUBWCConfig            = 3;
static const UINT32 IFEGenericBlobTypeCSIDClockConfig       = 4;
static const UINT32 IFEGenericBlobTypeBusReadConfig         = 5;
static const UINT32 IFEGenericBlobTypeUBWCConfigV2          = 6;
static const UINT32 IFEGenericBlobTypeIFETopConfig          = 7;
static const UINT32 IFEGenericBlobTypeIFEOutConfig          = 8;
static const UINT32 IFEGenericBlobTypeResourceBWConfigV2    = 9;

static const UINT32 IFEMaxNumRegReadInfoRanges = 256;
static const UINT32 IFEMaxDMIConfigWrites      = 5;
static const UINT32 MaxRegDumpOffsets          = 10;  ///< Max Register Dump Offsets, for exmaple IFE Left,
                                                      /// IFE Right and CAMNOC etc

CAMX_BEGIN_PACKED

///@ brief IFERegWriteDescriptor
struct IFERegWriteDescriptor
{
    UINT32 offset;  ///< Offset of the register that needs to be updated
    UINT32 value;   ///< Value
} CAMX_PACKED;

///@ brief Register read range descriptor
struct IFERegRangeReadDescriptor
{
    UINT32 offset;            ///< register offset
    UINT32 numberOfRegisters; ///< number of registers to be read
} CAMX_PACKED;

///@ brief IFE DMI read descriptor
struct IFEDMIReadDesciptor
{
    UINT32                    numberOfRegWrites;                      ///< Number of registers to be Wriiten
    UINT32                    numberOfPostWrites;                     ///< NUmber of Post read reg Writes
    IFERegWriteDescriptor     regWriteCmd[IFEMaxDMIConfigWrites];     ///< Array of register Write commands
    IFERegRangeReadDescriptor regRangeCmd;                            ///< register read range command
    IFERegWriteDescriptor     postregWriteCmd[IFEMaxDMIConfigWrites]; ///< Array of register Write commands
} CAMX_PACKED;

///@ brief IFE register read Info
struct IFERegReadInfo
{
    UINT32 readType;                          ///< Read type
    UINT32 reserved;                          ///< Reserved field for future use
    union
    {
        IFERegRangeReadDescriptor regReadCmd; ///< Register read Command
        IFEDMIReadDesciptor       dmiReadCmd; ///< DMI Read Command
    } regDescriptor;
} CAMX_PACKED;

///@ brief IFE Reg Dump Output
struct IFERegDumpOutput
{
    UINT64 requestID;     ///< Request ID
    UINT32 numberOfBytes; ///< NUmber of bytes Written
    UINT32 data[1];       ///< Data
} CAMX_PACKED;

///@ brief IFE Reg Dump Descriptor
struct IFERegDumpDescriptor
{
    UINT32         type;                                    ///< Reg Dump Type
    UINT32         bufferOffset;                            ///< Register Dump buffer offset
    UINT32         bufferSize;                              ///< Register Dump buffer Size
    UINT32         numberOfRegReadInfo;                     ///< Number of reg range read
    IFERegReadInfo regReadInfo[IFEMaxNumRegReadInfoRanges]; ///< Register read Info
} CAMX_PACKED;

/// @brief IFE Reg dump Imput Info Whihc specifies the Registor Dump descriptor offsets
struct IFERegDumpInfo
{
    UINT32 numberOfRegDumps;                  ///< Number of Reg Dumps
    UINT32 regDumpOffsets[MaxRegDumpOffsets]; ///< Register Dump
} CAMX_PACKED;

CAMX_END_PACKED

static const UINT32 IFERegDumpTypeIFELeft  = 1;
static const UINT32 IFERegDumpTypeIFERight = 2;
static const UINT32 IFERegDumpTypeCAMNOC   = 3;

static const UINT32 IFERegDumpReadTypeReg = 1;
static const UINT32 IFERegDumpReadTypeDMI = 2;
CAMX_NAMESPACE_END
#endif // CAMXCSLIFEDEFS_H
