////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2016-2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  camxpacketdefs.h
/// @brief The definition of core structures for packets that will be used both by CSL and HWL
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef CAMXPACKETDEFS_H
#define CAMXPACKETDEFS_H

// NOWHINE FILE GR016: Converting CSL header to C, we need typedef

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

#include "camxdefs.h"
#include "camxcsl.h"

/// @todo (CAMX-331) Add static assert to make sure the CSL format structs are consistent with Camx side.
/// @brief This enumerates pixel formats.
typedef enum
{
    CSLFormatJpeg = 0,          ///< JPEG format.
    CSLFormatY8,                ///< Luma only, 8 bits per pixel.
    CSLFormatY16,               ///< Luma only, 16 bits per pixel.
    CSLFormatYUV420NV12,        ///< YUV 420 format as described by the NV12 fourcc.
    CSLFormatYUV420NV21,        ///< YUV 420 format as described by the NV21 fourcc.
    CSLFormatYUV422NV16,        ///< YUV 422 format as described by the NV16 fourcc
    CSLFormatBlob,              ///< Any non image data
    CSLFormatRawYUV8BIT,        ///< Packed YUV/YVU raw format. 16 bpp: 8 bits Y and 8 bits UV. U and V are
                                ///  interleaved as YUYV or YVYV.
    CSLFormatRawPrivate,        ///< Private RAW formats where data is packed into 64bit word.
                                ///  8BPP:  1 64-bit word contains 8 pixels p0 - p7, p0 is stored at LSB.
                                ///  10BPP: 1 64-bit word contains 6 pixels p0 - p5, most significant 4 bits are set to 0.
                                ///         P0 is stored at LSB.
                                ///  12BPP: 1 64-bit word contains 5 pixels p0 - p4, most significant 4 bits are set to 0.
                                ///         P0 is stored at LSB.
    CSLFormatRawMIPI,           ///< MIPI RAW formats based on MIPI CSI-2 specification.
                                ///  MIPI CSI-2 specification : https://www.mipi.org/specifications/csi-2
                                ///  8BPP: Each pixel occupies one bytes, starting at LSB. Output width of image
                                ///  has no restrictions.
                                ///  10BPP: Four pixels are held in every 5 bytes. The output width of image must
                                ///  be a multiple of 4 pixels.
                                ///  12BPP: Two pixels are held in every 3 bytes. The output width of image must
                                ///  be a multiple of 2 pixels.
    CSLFormatRawPlain16,        ///< Plain16 RAW format. Single pixel is packed into two bytes, little endian format.
                                ///  Not all bits may be used as RAW data is generally 8, 10, or 12 bits per pixel.
                                ///  Lower order bits are filled first.
    CSLFormatRawMeta8BIT,       ///< Generic 8-bit raw meta data for internal camera usage.
    CSLFormatUBWCTP10,          ///< UBWC TP10 format
    CSLFormatUBWCNV12,          ///< UBWC NV12 format
    CSLFormatUBWCNV124R,        ///< UBWC NV12-4R format
    CSLFormatYUV420NV12TP10,    ///< YUV 420 (YCbCr) format 10bits per comp tight packed format.
    CSLFormatYUV420NV21TP10,    ///< YUV 420 (YCrCb) format 10bits per comp tight packed format.
    CSLFormatYUV422NV16TP10,    ///< YUV 422 format 10bits per comp tight packed format.
    CSLFormatPD10,              ///< PD10 format
    CSLFormatPrivate,           ///< Indicates a private format whose specifics are determined by the usage context
    CSLFormatP010,              ///< P010 format
    CSLFormatRawPlain64,        ///< Raw plain 64
    CSLFormatUBWCP010,          ///< UBWC P010 format
} CSLFormat;

/// @brief This enumerates color space
typedef enum
{
    CSLColorSpaceUnknown = 0,    ///< Default-assumption data space, when not explicitly specified.
    CSLColorSpaceBT601Full,      ///< ITU-R Recommendation 601 (BT.601) - Full range.
    CSLColorSpaceBT601625,       ///< ITU-R Recommendation 601 (BT.601) - 625-line  SDTV, 625 Lines (PAL).
    CSLColorSpaceBT601525,       ///< ITU-R Recommendation 601 (BT.601) - 525-line SDTV, 525 Lines (NTSC).
    CSLColorSpaceBT709,          ///< ITU-R Recommendation 709 (BT.709) HDTV.
    CSLColorSpaceDepth           ///< The buffer contains depth ranging measurements from a depth camera per HAL definition.
} CSLColorSpace;

/// @brief This enumerates degrees of rotation in a clockwise direction. The specific variable or struct member must declare the
/// semantics of the rotation (e.g. the image HAS BEEN rotated or MUST BE rotated).
typedef enum
{
    CSLRotationCW0Degrees  = 0,    ///< Zero degree rotation.
    CSLRotationCW90Degrees,        ///< 90 degree clockwise rotation.
    CSLRotationCW180Degrees,       ///< 180 degree clockwise rotation.
    CSLRotationCW270Degrees        ///< 270 degree clockwise rotation.
} CSLRotation;

/// Enumeration of the color filter pattern for RAW outputs
typedef enum
{
    CSLColorFilterPatternY = 0,  ///< Monochrome pixel pattern.
    CSLColorFilterPatternYUYV,   ///< YUYV pixel pattern.
    CSLColorFilterPatternYVYU,   ///< YVYU pixel pattern.
    CSLColorFilterPatternUYVY,   ///< UYVY pixel pattern.
    CSLColorFilterPatternVYUY,   ///< VYUY pixel pattern.
    CSLColorFilterPatternRGGB,   ///< RGGB pixel pattern.
    CSLColorFilterPatternGRBG,   ///< GRBG pixel pattern.
    CSLColorFilterPatternGBRG,   ///< GBRG pixel pattern.
    CSLColorFilterPatternBGGR,   ///< BGGR pixel pattern.
    CSLColorFilterPatternRGB     ///< RGB pixel pattern.
} CSLColorFilterPattern;

typedef enum
{
    CSLCmdTypeInvalid = 0,  ///< Invalid command type
    CSLCmdTypeCDMDMI,       ///< DMI buffer (new format used in Titan)
    CSLCmdTypeCDMDMI16,     ///< DMI buffer for 16-bit elements
    CSLCmdTypeCDMDMI32,     ///< DMI buffer for 32-bit elements
    CSLCmdTypeCDMDMI64,     ///< DMI buffer for 64-bit elements
    CSLCmdTypeCDMDirect,    ///< Direct command buffer
    CSLCmdTypeCDMIndirect,  ///< Indirect command buffer
    CSLCmdTypeI2C,          ///< I2C command buffer
    CSLCmdTypeFW,           ///< Firmware command buffer
    CSLCmdTypeGeneric,      ///< Generic command buffer
    CSLCmdTypeLegacy        ///< Legacy blob
} CSLCmdType;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief  Below packet opcodes are defined for each device type. There may be 0 to (2^24-1) different packets per device.
///         The opcode forms the lower 24 bit of the packet code. The higher 8 bits define the device type. So, the opcode
///         are local per device; hence can overlap. Device type 0 is invalid and can be used to communicate SW packets that
///         do not necessarily target a particular device.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @todo (CAMX-550)    Move the opcodes to a new header file (or their corresponding CSL device header).
/// @note               Add new opcodes only to the end.
// Begin global opcodes
/// @todo (CAMX-616)    Make NOP opcode back to 0, and start all device opcodes from 1
static const UINT32 CSLPacketOpcodesNop     = 127;    ///< Opcode for a Nop packet
// End global opcodes

/// @brief  Define sensor-specific packet opcodes
typedef enum
{
    CSLPacketOpcodesSensorStreamOn          = 0,    ///< Packet that contains stream-on and stream-off commands
    CSLPacketOpcodesSensorUpdate,                   ///< Per-frame sensor update packet
    CSLPacketOpcodesSensorInitialConfig,            ///< Packet that contains Initial configuration commands
    CSLPacketOpcodesSensorProbe,                    ///< Packet that contains Probe configuration commands
    CSLPacketOpcodesSensorConfig,                   ///< Packet that contains sensor configuration commands ex Res,SPC etc.
    CSLPacketOpcodesSensorStreamOff,                ///< Packet that contains Streamoff configuration commands
    CSLPacketOpcodesSensorRead                      ///< Packet that contains sensor read command
} CSLPacketOpcodesSensor;

/// @brief  Define CSIPHY-specific packet opcodes
typedef enum
{
    CSLPacketOpcodesCSIPHYInitialConfig     = 0,    ///< Packet that contains CSIPHY configuration
} CSLPacketOpcodesCSIPHY;

/// @brief  Define actuator-specific packet opcodes
typedef enum
{
    CSLPacketOpcodesActuatorInitialConfig  = 0, ///< Packet that contains Initial configuration commands
    CSLPacketOpcodesActuatorAutoMove,           ///< Packet that contains move commands and applied immediately
                                                /// (i.e not synchronized with request ID)
    CSLPacketOpcodesActuatorManualMove,         ///< Packet that contains move commands and applied real time
    CSLPacketOpcodesActuatorRead                ///< Packet that contains actuator read command
} CSLPacketOpcodesActuator;

typedef enum
{
    CSLPacketOpcodesOisInitialConfig  = 0,     ///< Packet that contains Initial configuration commands
    CSLPacketOpcodesOisMode,                   ///< Packet that contains ois mode commands
    CSLPacketOpcodesOisRead                    ///< Packet that contains ois read command
} CSLPacketOpcodesOis;

/// @brief  Define flash-specific packet opcodes
typedef enum
{
    CSLPacketOpcodesFlashInitialConfig = 0,    ///< Packet that contains Initial configuration commands
    CSLPacketOpcodesFlashSet,                  ///< Packet that contains firing flash commands
    CSLPacketOpcodesFlashSetNonRealTime,       ///< Packet that contains firing flash commands non-real-time without requireId
                                               ///< Commands will process right away, without waiting in command manager queue
} CSLPacketOpcodesFlash;

/// @brief  Define EEPROM-specific packet opcodes
typedef enum
{
    CSLPacketOpcodesEEPROMInitialConfig     = 0,    ///< Packet that contains EEPROM configuration
    CSLPacketOpcodesEEPROMWriteData,                ///< Packet that contains Write data information
} CSLPacketOpcodesEEPROM;

/// @brief Define VFE-specific packet opcodes
typedef enum
{
    CSLPacketOpcodesVFEInitialConfig        = 0,    ///< Update packet
    CSLPacketOpcodesVFEUpdate                       ///< Update packet
} CSLPacketOpcodesVFE;

/// @brief Define IFE-specific packet opcodes
typedef enum
{
    CSLPacketOpcodesIFEInitialConfig        = 0,    ///< Initial config packet
    CSLPacketOpcodesIFEUpdate,                      ///< Update packet
    CSLPacketOpcodesDualIFEInitialConfig    = 0,    ///< Dual IFE initial config packet
    CSLPacketOpcodesDualIFEUpdate                   ///< Dual IFE update packet
} CSLPacketOpcodesIFE;

/// @brief Command buffer identifier for (dual) IFE
typedef enum
{
    CSLIFECmdBufferIdLeft             = 1,  ///< Left IFE's command buffer
    CSLIFECmdBufferIdRight            = 2,  ///< Right IFE's command buffer
    CSLIFECmdBufferIdCommon           = 3,  ///< Common (main) command buffer
    CSLIFECmdBufferIdDualConfig       = 9,  ///< Dual IFE configuration info
    CSLIFECmdBufferIdGenericBlobLeft  = 10, ///< Generic Blob Config for Left IFE
    CSLIFECmdBufferIdGenericBlobRight = 11, ///< Genreic Blob Config for Right IFE
    CSLIFECmdBufferIdGenericBlob      = 12, ///< Generic Blob configuration info
    CSLIFECmdBufferIdGenericDump      = 13, ///< Register Dump used per frame
    CSLIFECmdBufferIdFlushDump        = 14, ///< Register Dump at flush time
    CSLIFECmdBufferIdHangDump         = 15, ///< Register Dump at IFE Hang or Error time
} CSLIFECmdBufferId;

/// @brief Define IPE-specific packet opcodes
typedef enum
{
    CSLPacketOpcodesIPEUpdate          = 0,  ///< IPE update packet
    CSLPacketOpcodesBPSUpdate          = 1,  ///< BPS update packet
    CSLPacketOpcodesIPEMemoryMapUpdate = 2,  ///< IPE memory map update
    CSLPacketOpcodesBPSMemoryMapUpdate = 3,  ///< BPS memory map update
} CSLPacketOpcodesIPE;

/// @brief Command buffer identifier for IPE
typedef enum
{
    CSLICPCmdBufferIdGenericBlob = 1,    ///< 1: Generic Blob configuration info
    CSLIPECmdBufferMaxIds                ///< 2: Max number of command buffers in CSLIPE node
} CSLIPECmdBufferId;

/// @brief Define LRME-specific packet opcodes
typedef enum
{
    CSLPacketOpcodesLRMEUpdate = 0,  ///< LRME update packet
} CSLPacketOpcodesLRME;

static const UINT CSLMaxNumPlanes       = 3;   ///< Max number of IO ports that an IO config may refer to

/// @brief Indicates direction of IO
typedef enum
{
    CSLIODirectionInput = 1,    ///< Indicates an input config
    CSLIODirectionOutput        ///< Indicates an output config
} CSLIODirection;

/// @brief  Mask and shift constants for GenericBlob CmdBuffer header
static const UINT32 CSLGenericBlobHeaderSizeInDwords    = 1;            ///< Blob header size in dwords
static const UINT32 CSLGenericBlobCmdSizeMask           = 0xffffff00;   ///< Mask value for Blob Size in Blob Header
static const UINT32 CSLGenericBlobCmdSizeShift          = 8;            ///< Shift value for Blob Size in Blob Header
static const UINT32 CSLGenericBlobCmdTypeMask           = 0xff;         ///< Mask value for Blob Type in Blob Header
static const UINT32 CSLGenericBlobCmdTypeShift          = 0;            ///< Shift value for Blob Type in Blob Header

/// @brief Acquired devices Version
static const UINT32 CSLAcquiredDeviceVersion1 = 1;
static const UINT32 CSLAcquiredDeviceVersion2 = 2;

/// @brief  Mask and shift constants for packet opcode
static const UINT32 CSLPacketOpcodeMask         = 0xffffff;     ///< Opcode mask
static const UINT32 CSLPacketOpcodeShift        = 0;            ///< Opcode shift
static const UINT32 CSLPacketOpcodeDeviceMask   = 0xff000000;   ///< Device tpye mask
static const UINT32 CSLPacketOpcodeDeviceShift  = 24;           ///< Device type shift

/// @brief AXI BW Voting Version
static const UINT32 CSLAXIBWVotingV2 = 2;

/// @brief AXI BW Voting Transaction Type
static const UINT32 CSLAXITransactionRead            = 0;
static const UINT32 CSLAXITransactionWrite           = 1;

/// @brief AXI BW Voting Path Data Type
static const UINT32 CSLAXIPathDataIFEStartOffset      = 0;
static const UINT32 CSLAXIPathDataIFELinear           = CSLAXIPathDataIFEStartOffset + 0;
static const UINT32 CSLAXIPathDataIFEVideo            = CSLAXIPathDataIFEStartOffset + 1;
static const UINT32 CSLAXIPathDataIFEDisplay          = CSLAXIPathDataIFEStartOffset + 2;
static const UINT32 CSLAXIPathDataIFEStats            = CSLAXIPathDataIFEStartOffset + 3;
static const UINT32 CSLAXIPathDataIFERDI0             = CSLAXIPathDataIFEStartOffset + 4;
static const UINT32 CSLAXIPathDataIFERDI1             = CSLAXIPathDataIFEStartOffset + 5;
static const UINT32 CSLAXIPathDataIFERDI2             = CSLAXIPathDataIFEStartOffset + 6;
static const UINT32 CSLAXIPathDataIFERDI3             = CSLAXIPathDataIFEStartOffset + 7;
static const UINT32 CSLAXIPathDataIFEPDAF             = CSLAXIPathDataIFEStartOffset + 8;
static const UINT32 CSLAXIPathDataIFEPixelRaw         = CSLAXIPathDataIFEStartOffset + 9;
static const UINT32 CSLAXIPathDataIFEMaxOffset        = CSLAXIPathDataIFEStartOffset + 31;

static const UINT32 CSLAXIPathDataIPEMaxNum           = 31;
static const UINT32 CSLAXIPathDataIPEStartOffset      = 32;
static const UINT32 CSLAXIPathDataIPEReadInput        = CSLAXIPathDataIPEStartOffset + 0;
static const UINT32 CSLAXIPathDataIPEReadRef          = CSLAXIPathDataIPEStartOffset + 1;
static const UINT32 CSLAXIPathDataIPEWriteVideo       = CSLAXIPathDataIPEStartOffset + 2;
static const UINT32 CSLAXIPathDataIPEWriteDisplay     = CSLAXIPathDataIPEStartOffset + 3;
static const UINT32 CSLAXIPathDataIPEWriteReference   = CSLAXIPathDataIPEStartOffset + 4;
static const UINT32 CSLAXIPathDataIPEMaxOffset        = CSLAXIPathDataIPEStartOffset + CSLAXIPathDataIPEMaxNum;

static const UINT32 CSLAXIPathDataALL                 = 256;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// GetPacketOpcode
///
/// @brief  Get the operation code from a packet opcode
///
/// @param  packetCode  Packet opcode
///
/// @return operation code
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CAMX_INLINE UINT32 GetPacketOpcode(
    UINT32 packetCode)
{
    return ((packetCode & CSLPacketOpcodeMask) >> CSLPacketOpcodeShift);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// GetPacketDeviceType
///
/// @brief  Get the device type from a packet opcode
///
/// @param  packetCode  Packet opcode
///
/// @return device type
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CAMX_INLINE UINT32 GetPacketDeviceType(
    UINT32 packetCode)
{
    return ((packetCode & CSLPacketOpcodeDeviceMask) >> CSLPacketOpcodeDeviceShift);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// MakePacketOpcode
///
/// @brief  Make a packet code from a device type and an opcode
///
/// @param  deviceType  device type
/// @param  opcode      operation code
///
/// @return packet code
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CAMX_INLINE UINT32 MakePacketOpcode(
    UINT32  deviceType,
    UINT32  opcode)
{
    UINT32 packetCode = (opcode << CSLPacketOpcodeShift) & CSLPacketOpcodeMask;
    packetCode |= (deviceType << CSLPacketOpcodeDeviceShift) & CSLPacketOpcodeDeviceMask;

    return packetCode;
}

// Must pack to have the layout expected by HW
CAMX_BEGIN_PACKED

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief  A command buffer descriptor that contains information about memory used as well as metadata and a fence that will
///         signal when the command buffer is consumed.
///         Must be 64-bit aligned.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
typedef struct
{
    CSLMemHandle    hMem;       ///< Memory handle
    UINT32          offset;     ///< The offset from the start of the memory described by mem
    UINT32          size;       ///< The max size of this buffer
    UINT32          length;     ///< The (valid) length of the commands
    UINT32          type;       ///< Indicates the type of the command buffer
    UINT32          metadata;   ///< Packet-specific metadata between nodes and KMD
} CAMX_PACKED CSLCmdMemDesc;

/// @brief UBWC configuration for a Port
typedef struct
{
    UINT32  portResourceId;   ///< Identifies port resource
    UINT32  metadataStride;   ///< UBWC metadata stride
    UINT32  metadataSize;     ///< UBWC metadata plane size
    UINT32  metadataOffset;   ///< UBWC metadata offset
    UINT32  packerConfig;     ///< UBWC packer config
    UINT32  modeConfig;       ///< UBWC mode config
    UINT32  modeConfig1;      ///< UBWC mode config1
    UINT32  tileConfig;       ///< UBWC tile config
    UINT32  hInitialVal;      ///< UBWC H_INIT
    UINT32  vInitialVal;      ///< UBWC V_INIT
}CAMX_PACKED CSLPortUBWCConfig;

/// @brief UBWC configuration for a Port version 2
typedef struct
{
    UINT32  portResourceId;         ///< Identifies port resource
    UINT32  metadataStride;         ///< UBWC metadata stride
    UINT32  metadataSize;           ///< UBWC metadata plane size
    UINT32  metadataOffset;         ///< UBWC metadata offset
    UINT32  packerConfig;           ///< UBWC packer config
    UINT32  modeConfig;             ///< UBWC mode config
    UINT32  modeConfig1;            ///< UBWC mode config 1
    UINT32  tileConfig;             ///< UBWC tile config
    UINT32  hInitialVal;            ///< UBWC H_INIT
    UINT32  vInitialVal;            ///< UBWC V_INIT
    UINT32  staticControl;          ///< UBWC static control
    UINT32  control2;               ///< UBWC control 2
    UINT32  statsControl2;          ///< Stats control 2
    UINT32  lossyThreshold0;        ///< UBWC lossy threshold 0
    UINT32  lossyThreshold1;        ///< UBWC lossy threshold 1
    UINT32  offsetVarianceLossy;    ///< OffsetVarianceLossy
    UINT32  bandwidthLimit;         ///< bandwidth limit
    UINT32  reserved[3];            ///< reserved data
}CAMX_PACKED CSLPortUBWCConfigV2;

/// @brief UBWC Resource Configuration
typedef struct
{
    UINT32              UBWCAPIVersion;                            ///< Supported UBWC API Version
    UINT32              numPorts;                                  ///< Number of ports to configure for UBWC
    CSLPortUBWCConfig   portUBWCConfig[1][CSLMaxNumPlanes - 1];    ///< Starting point for UBWC Config parameters
} CAMX_PACKED CSLResourceUBWCConfig;

/// @brief UBWC Resource Configuration V2
typedef struct
{
    UINT32                UBWCAPIVersion;                            ///< Supported UBWC API Version
    UINT32                numPorts;                                  ///< Number of ports to configure for UBWC
    CSLPortUBWCConfigV2   portUBWCConfig[1][CSLMaxNumPlanes - 1];    ///< Starting point for UBWC Config parameters
} CAMX_PACKED CSLResourceUBWCConfigV2;

/// @brief Define a single plane geometry
typedef struct
{
    UINT32 width;          ///< Width of the YUV plane in bytes., tile aligned width in bytes for UBWC
    UINT32 height;         ///< Height of the YUV plane in bytes.
    UINT32 planeStride;    ///< The number of bytes between the first byte of two sequential lines on plane 1. It may be
                           ///  greater than nWidth * nDepth / 8 if the line includes padding.
    UINT32 sliceHeight;    ///< The number of lines in the plane which can be equal to or larger than actual frame height.

    UINT32 metadataStride; ///< UBWC metadata stride
    UINT32 metadataSize;   ///< UBWC metadata plane size
    UINT32 metadataOffset; ///< UBWC metadata offset
    UINT32 packerConfig;   ///< UBWC packer config
    UINT32 modeConfig;     ///< UBWC mode config
    UINT32 tileConfig;     ///< UBWC tile config
    UINT32 hInit;          ///< UBWC H_INIT
    UINT32 vInit;          ///< UBWC V_INIT
} CAMX_PACKED CSLPlane;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief  IO configuration info. A packet may contain up to CAMX_MAX_NUM_IOCONFIG_IN_PACKET number of configs. It describes
///         what buffers need to go to what HW port and how those ports need to be configured.
///         Must be 64-bit aligned.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
typedef struct
{
    CSLMemHandle            hMems[CSLMaxNumPlanes];             ///< Memory handles per plane
    UINT32                  offsets[CSLMaxNumPlanes];           ///< Memory offset from the corresponding handle
    CSLPlane                planes[CSLMaxNumPlanes];            ///< Plane descriptors
    UINT32                  format;                             ///< Format of the image
    UINT32                  colorSpace;                         ///< The color space of the image
    UINT32                  colorFilterPattern;                 ///< Color filter pattern of the RAW format
    UINT32                  bitsPerPixel;                       ///< Bits per pixel for raw formats
    UINT32                  rotation;                           ///< Rotation applied to the image
    UINT32                  portResourceId;                     ///< Identifies port resource from
    CSLFence                hSync;                              ///< If output buffer: bufdone; if input buffer: buf ready
    CSLFence                hEarlySync;                         ///< "Early done" sync
    CSLCmdMemDesc           auxCmdBuffer;                       ///< An auxiliary command buffer that may be used for
                                                                ///  programming the IO (node/opcode-specific)
    UINT32                  direction;                          ///< Direction of the config
    UINT32                  batchSize;                          ///< Batch size in HFR mode
    UINT32                  subsamplePattern;                   ///< Subsample pattern. Used in HFR mode. It should be
                                                                ///  consistent with batchSize and CAMIF programming.
    UINT32                  subsamplePeriod;                    ///< Subsample period. Used in HFR mode. It should be consistent
                                                                ///  with batchSize and CAMIF programming.
    UINT32                  framedropPattern;                   ///< Framedrop pattern.
    UINT32                  framedropPeriod;                    ///< Framedrop period. Must be multiple of subsamplePeriod if
                                                                ///  in HFR mode.
    UINT32                  flags;                              ///< For providing extra information
    UINT32                  padding;                            ///< Padding
} CAMX_PACKED CSLBufferIOConfig;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief  Base structure used to submit a packet to CSL.
///         New packet definition MUST only "extend" this structure.
///         Must be 64-bit aligned.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
typedef struct
{
    UINT32  opcode;     ///< Code that identifies the operation (device type + opcode)
    UINT32  size;       ///< The size of the packet (total dynamic size of the packet not just the struct)
    UINT64  requestId;  ///< Request ID
    UINT32  flags;      ///< Flags for this packet
    UINT32  padding;    ///< Padding
} CAMX_PACKED CSLPacketHeader;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief  To support some requirements, UMD may not be able to populate command buffers with physical (buffer) addresses.
///         For example, on platforms that need to parse command buffers in SW, indirect command buffer addresses need to be
///         64bit virtual addresses, but such addresses are not available to UMD and, even if they were, command format only
///         supports 32bit addresses; in such cases, patching information should be provided along with packets so that KMD
///         can lookup and use the right addresses.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
typedef struct
{
    CSLMemHandle    hDstBuffer; ///< Memory handle of the destination buffer that will be patched
    UINT32          dstOffset;  ///< Byte offset of the destination where to-be-patched address resides
    CSLMemHandle    hSrcBuffer; ///< Memory handle of the source buffer whose address needs to be used
    UINT32          srcOffset;  ///< Byte offset that needs to be added to the source address
} CAMX_PACKED CSLAddrPatch;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief  Most packets can be represented using just the common packet fields.
///         The reason the CSLPacketHeader is still nested within Packet is to allow all the subpackets
///         to have a uniform access path to the base fields.
///         Must be 64-bit aligned.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
typedef struct
{
    CSLPacketHeader header;             ///< Common portion of packets that will be the same for all packet types
    UINT32          cmdBuffersOffset;   ///< Byte offset within packet's data section where command buffer descs start
    UINT32          numCmdBuffers;      ///< Number of command buffers
    UINT32          ioConfigsOffset;    ///< Byte offset within packet's data section where IO configs start
    UINT32          numBufferIOConfigs; ///< Number of IO configs
    UINT32          patchsetOffset;     ///< Byte offset within packet's data section where patchset data start
    UINT32          numPatches;         ///< Number of patches in the patchset
    UINT32          kmdCmdBufferIndex;  ///< Index to the command buffer that KMD may use for commanding CDM
    UINT32          kmdCmdBufferOffset; ///< Offset in the command buffer where KMD commands begins
    UINT64          data[1];            ///< CSLCmdMemDesc instances followed by
} CAMX_PACKED CSLPacket;


/// @brief Define Per path bandwidth vote information
typedef struct
{
    UINT32 usageData;          ///< client usage data (left/right/rdi)
    UINT32 transactionType;    ///< Transaction type on the path (read/write)
    UINT32 pathDataType;       ///< Path for which vote is given (video, display, rdi)
    UINT32 reserved;           ///< Reserved for alignment
    UINT64 camnocBW;           ///< CAMNOC bw for this path
    UINT64 mnocABBW;           ///< MNOC AB bw for this path
    UINT64 mnocIBBW;           ///< MNOC IB bw for this path
    UINT64 ddrABBW;            ///< DDR AB bw for this path
    UINT64 ddrIBBW;            ///< DDR IB bw for this path
} CAMX_PACKED CSLAXIperPathBWVote;

// End of packed definitions
CAMX_END_PACKED

#ifdef __cplusplus
}
#endif // __cplusplus

#endif // CAMXPACKETDEFS_H
