////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2016-2020 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  camxcslsensordefs.h
/// @brief The definitions needed for composing sensor packets.
///         Must match definitions in kernel\include\uapi\media\cam_sensor.h
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// NOWHINE FILE GR016: Converting CSL header to C, we need typedef

#ifndef CAMXCSLSENSORDEFS_H
#define CAMXCSLSENSORDEFS_H

#include "camxdefs.h"

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

static const UINT MaxOISNameSize = 32;

// Must pack to enforce the expected command layout
CAMX_BEGIN_PACKED

/// @brief  Sensor command type
typedef enum
{
    CSLSensorCmdTypeInvalid = 0,            ///< Invalid command
    CSLSensorCmdTypeProbe,                  ///< Sensor probe
    CSLSensorCmdTypePowerUp,                ///< Power up
    CSLSensorCmdTypePowerDown,              ///< Power down
    CSLSensorCmdTypeI2CInfo,                ///< I2C slave information command
    CSLSensorCmdTypeI2CRandomRegWrite,      ///< I2C random write
    CSLSensorCmdTypeI2CRandomRegRead,       ///< I2C random read
    CSLSensorCmdTypeI2CContinuousRegWrite,  ///< I2C continuous write
    CSLSensorCmdTypeI2CContinuousRegRead,   ///< I2C continuous read
    CSLSensorCmdTypeWait,                   ///< Wait
    CSLSensorCmdTypeFlashInit,              ///< Flash init command
    CSLSensorCmdTypeFlashFire,              ///< Flash fire command
    CSLSensorCmdTypeFlashRER,               ///< Flash red eye reduction sequence
    CSLSensorCmdTypeFlashQueryCurrent,      ///< Flash query current
    CSLSensorCmdTypeFlashWidget,            ///< Flash Widget
    CSLSensorCmdTypeReadData,               ///< Read Data
    CSLSensorCmdTypeFlashFireInit,          ///< Initial flash fire command
    CSLSensorCmdTypeMax,                    ///< Max
} CSLSensorCmdType;

/// @brief  Sensor I2C operation code
typedef enum
{
    CSLSensorI2COpcodeInvalid = 0,                      ///< Invalid opcode
    CSLSensorI2COpcodeRandomWrite,                      ///< Random write
    CSLSensorI2COpcodeRandomWriteVerify,                ///< Random write with verification
    CSLSensorI2COpcodeContinuousWriteBurst,             ///< Continuous burst write
    CSLSensorI2COpcodeContinuousWriteBurstVerify,       ///< Continuous burst write with verification
    CSLSensorI2COpcodeContinuousWriteSequence,          ///< Continuous sequence write
    CSLSensorI2COpcodeContinuousWriteSequenceVerify,    ///< Continuous sequence write with verification
    CSLSensorI2COpcodeRandomRead,                       ///< Random Read
    CSLSensorI2COpcodeContinuousRead,                   ///< Continuous Read
    CSLSensorI2COpcodeMax,                              ///< Max
} CSLSensorI2COpcode;

/// @brief  Sensor WAIT operation code
typedef enum
{
    CSLSensorWaitOpcodeInvalid = 0,     ///< Invalid
    CSLSensorWaitOpcodeConditional,     ///< Conditional wait
    CSLSensorWaitOpcodeUnconditionalHw, ///< Unconditional hardware wait
    CSLSensorWaitOpcodeUnconditionalSw, ///< Unconditional software wait
    CSLSensorWaitOpcodeMax,             ///< Max
} CSLSensorWaitOpcode;

/// @brief  Flash opcode
typedef enum
{
    CSLFlashOpcodeInvalid = 0,    ///< Invalid
    CSLFlashOpcodeOff,            ///< Off
    CSLFlashOpcodeFireLow,        ///< Fire with low current
    CSLFlashOpcodeFireHigh,       ///< Fire with high current
    CSLFlashOpcodeMax,            ///< Max
} CSLFlashOpcode;

/// @brief  Sensor I2C address type
typedef enum
{
    CSLSensorI2CTypeInvalid = 0,    ///< Invalid type
    CSLSensorI2CTypeByte,           ///< Byte
    CSLSensorI2CTypeWord,           ///< Word
    CSLSensorI2CType3Byte,          ///< 3-byte
    CSLSensorI2CTypeDword,          ///< Dword
    CSLSensorI2CTypeMax,            ///< Max
} CSLSensorI2CType;

/// @brief  Sensor I2C data type
typedef enum
{
    CSLSensorI2CDataTypeInvalid = 0,       ///< Invalid type
    CSLSensorI2CDataTypeByte,              ///< Byte
    CSLSensorI2CDataTypeWord,              ///< Word
    CSLSensorI2CDataTypeDword,             ///< Dword
    CSLSensorI2CDataTypeSetByteMask,       ///< SetByteMask
    CSLSensorI2CDataTypeUnsetByteMask,     ///< UnsetByteMask
    CSLSensorI2CDataTypeSetWordMask,       ///< SetWordMask
    CSLSensorI2CDataTypeUnsetWordMask,     ///< UnsetWordMask
    CSLSensorI2CDataTypeSetByteWriteMask,  ///< SetByteWriteMask
    CSLSensorI2CDataTypeMax,               ///< Max
} CSLSensorI2CDataType;

/// @brief  Camera roll definitions
typedef enum
{
    CSLSensorRoll0          = 0,    ///< 0 mounting angle
    CSLSensorRoll90         = 90,   ///< 90-degree mounting angle
    CSLSensorRoll180        = 180,  ///< 180-degree mounting angle
    CSLSensorRoll270        = 270,  ///< 270-degree mounting angle
    CSLSensorRollInvalid    = 360   ///< Beginning of invalid range of values
} CSLSensorRoll;

/// @brief  Camera yaw definitions
typedef enum
{
    CSLSensorYawFront   = 0,    ///< Yaw of a front-facing camera
    CSLSensorYawRear    = 180,  ///< Yaw of a rear-facing camera
    CSLSensorYawInvalid = 360   ///< Beginning of invalid range of values
} CSLSensorYaw;

/// @brief  Camera pitch definitions
typedef enum
{
    CSLSensorPitchLevel     = 0,    ///< Pitch of front-facing or rear-facing camera
    CSLSensorPitchInvalid   = 360   ///< Beginning of invalid range of values
} CSLSensorPitch;

/// @brief  Sensor Power Sequence type
typedef enum
{
    CSLSensorSequenceMCLK = 0,              ///< Clock
    CSLSensorSequenceVANA,                  ///< Analog voltage
    CSLSensorSequenceVDIG,                  ///< Digital voltage
    CSLSensorSequenceVIO,                   ///< I/O voltage
    CSLSensorSequenceVAF,                   ///< AF voltage
    CSLSensorSequenceVAFPWDN,               ///< AF power down
    CSLSensorSequenceCustomReg1,            ///< Customer Reg1
    CSLSensorSequenceCustomReg2,            ///< Customer Reg2
    CSLSensorSequenceReset,                 ///< Reset
    CSLSensorSequenceStandby,               ///< Standby
    CSLSensorSequenceCustomGPIO1,           ///< GPIO1
    CSLSensorSequenceCustomGPIO2,           ///< GPIO2
    CSLSensorSequenceTypeMax,               ///< MAX
} CSLSensorPowerSequenceType;

/// @brief  Capabilities info for sensor
typedef struct
{
    UINT32  slotInfo;       ///< Informs about the slotId or cell Index
    UINT32  secureCamera;   ///< Informs whether the camera is in secure/Non-secure mode
    UINT32  pitch;          ///< Pitch
    UINT32  roll;           ///< Informs about the how sensor is mounted
    UINT32  yaw;            ///< Yaw
    UINT32  actuatorSlotId; ///< Actuator slot id which connected to sensor
    UINT32  EEPROMSlotId;   ///< EEPROM slot id which connected to sensor
    UINT32  OISSlotId;      ///< OIS slot id which connected to sensor
    UINT32  flashSlotId;    ///< Flash slot id which connected to sensor
    UINT32  CSIPHYSlotId;   ///< CSIPHY slot id which connected to sensor
} CAMX_PACKED CSLSensorCapability;

/// @brief  Capabilities info CSIPHY
typedef struct {
    UINT32  slotInfo;   ///< Informs about the slotId or cell Index
    UINT32  version;    ///< CSIPHY version
    UINT32  clockLane;  ///< Of the 5 lanes, informs lane configured as clock lane
    UINT32  reserved;   ///< Reserved
}  CAMX_PACKED CSLCSIPHYCapability;

/// @brief  Capabilities info EEPROM
typedef struct {
    UINT32  slotInfo;      ///< Informs about the slotId or cell Index
    UINT16  kernelProbe;   ///< Is this Module probed in Kernel
    UINT16  multiModule;   ///< Multimodule support available
}  CAMX_PACKED CSLEEPROMCapability;

/// @brief  Capabilities info Actuator
typedef struct {
    UINT32  slotInfo; ///< Informs about the slotId or cell Index
    UINT32  reserved; ///< Reserved
} CAMX_PACKED CSLActuatorCapability;

/// @brief  Capabilities info OIS
typedef struct {
    UINT32  slotInfo; ///< Informs about the slotId or cell Index
    UINT16  reserved; ///< Reserved
} CAMX_PACKED CSLOISCapability;


/// @brief  Acquire Device info CSIPHY
typedef struct {
    UINT32  comboMode;  ///< Informs about the phy combo mode
    UINT32  reserved;   ///< Reserved
} CAMX_PACKED CSLCSIPHYAcquireDevice;

/// @brief  contains OIS firmware related
typedef struct {
    UINT32  prog;        ///< Informs about fw prog
    UINT32  coeff;       ///< Informs about fw coefficent
    UINT32  pheripheral; ///< Informs about fw pheripheral
    UINT32  memory;      ///< Informs about fw memory
} CAMX_PACKED CSLOISopcode;

/// @brief  contains OIS slave I2C related info
typedef struct
{
    UINT32       slaveAddr;                 ///< Slave address
    UINT8        I2CFrequencyMode;          ///< Lower 4-bits used for I2C freq mode
    UINT8        cmdType;                   ///< Command type
    UINT8        OISFwFlag;                 ///< is true if firmware is supported
    UINT8        isOISCalib;                ///< is ture if calibration data available
    CHAR         OISName[MaxOISNameSize];   ///< OIS Name
    CSLOISopcode opcode;                    ///< opcode
} CAMX_PACKED CSLOISI2CInfo;

/// @brief  contains slave I2C related info
typedef struct
{
    UINT32  slaveAddr;          ///< Slave address
    UINT8   I2CFrequencyMode;   ///< Lower 4-bits used for I2C freq mode
    UINT8   cmdType;            ///< Command type
    UINT16  reserved;           ///< Reserved
} CAMX_PACKED CSLSensorI2CInfo;

/// @brief  Sensor slave info for probing
typedef struct
{
    UINT8   dataType;       ///< Slave data type
    UINT8   addrType;       ///< Slave address type
    UINT8   opcode;         ///< Opcode
    UINT8   cmdType;        ///< Command type
    UINT32  regAddr;        ///< Sensor ID register address
    UINT32  expectedData;   ///< Data expected at slave address
    UINT32  dataMask;       ///< Data mask if only few bits are valid
    UINT16  cameraId;       ///< Informs the slot to which camera needs to be probed
    UINT16  pipelineDelay;  ///< pipeline delay
} CAMX_PACKED CSLSensorProbeCmd;

/// @brief  Power setting info
typedef struct
{
    UINT16  powerSequenceType;  ///< Type of power sequence
    UINT16  reserved;           ///< Reserved
    UINT32  configValLow;       ///< Lower 32 bit value configuration value
    UINT32  configValHigh;      ///< Higher 32 bit value configuration value
} CAMX_PACKED CSLSensorPowerSetting;

/// @brief  Power settings command
typedef struct
{
    UINT32                  count;              ///< Number of power settings in powerSettings
    UINT8                   reserved;           ///< Reserved
    UINT8                   cmdType;            ///< Command type
    UINT16                  reserved1;          ///< Reserved
    CSLSensorPowerSetting   powerSettings[1];   ///< Extended data area containing power settings
} CAMX_PACKED CSLSensorPowerCmd;

/// @brief  Header of READ/WRITE I2C command
typedef struct
{
    UINT32  count;      ///< Number of registers, data, or reg-data pairs
    UINT8   opcode;     ///< Opcode
    UINT8   cmdType;    ///< Command type
    UINT8   dataType;   ///< I2C address type
    UINT8   addrType;   ///< I2C data type
} CAMX_PACKED CSLSensorI2CHeader;

/// @brief  Payload for I2C random write
typedef struct
{
    UINT32  reg;    ///< Register address
    UINT32  val;    ///< Register data
} CAMX_PACKED CSLSensorI2CRegValPair;

/// @brief  I2C random write command
typedef struct
{
    CSLSensorI2CHeader      header;         ///< I2C header
    CSLSensorI2CRegValPair  regValPairs[1]; ///< Array of address/data pairs
} CAMX_PACKED CSLSensorI2CRandomWriteCmd;

/// @brief  Payload for I2C burst/sequential write
typedef struct
{
    UINT32  val;      ///< Register data
    UINT32  reserved; ///< Reserved field
} CAMX_PACKED CSLSensorI2CVal;

/// @brief  I2C burst/sequential write command
typedef struct
{
    CSLSensorI2CHeader  header;     ///< I2C header
    UINT32              reg;        ///< Register base address
    CSLSensorI2CVal     data[1];    ///< Array of values to be written
} CAMX_PACKED CSLSensorI2CBurstWriteCmd;

/// @brief  I2C random read command
typedef struct
{
    CSLSensorI2CHeader  header;     ///< I2C header
    UINT32              regs[1];    ///< Array of register addresses to be read
} CAMX_PACKED CSLSensorI2CRandomReadCmd;

/// @brief  I2C continuous continuous read command
typedef struct
{
    CSLSensorI2CHeader  header; ///< I2C header
    UINT32              reg;    ///< Register base address to be read
} CAMX_PACKED CSLSensorI2CContinuousReadCmd;

/// @brief  Conditional wait command
typedef struct
{
    UINT8   dataType;               ///< Data type
    UINT8   addrType;               ///< Address type
    UINT16  reserved;               ///< Reserved
    UINT8   opcode;                 ///< Opcode
    UINT8   cmdType;                ///< Command type
    UINT16  timeoutMilliseconds;    ///< Timeout in milli-seconds
    UINT32  reg;                    ///< Register address
    UINT32  val;                    ///< Value
    UINT32  mask;                   ///< Mask
} CAMX_PACKED CSLSensorWaitConditionalCmd;

/// @brief  Unconditional wait command
typedef struct
{
    UINT16  delayMicroseconds;  ///< Delay in micro-seconds
    UINT16  reserved;           ///< reserved
    UINT8   opcode;             ///< Opcode
    UINT8   cmdType;            ///< Command type
    UINT16  reserved1;          ///< reserved
} CAMX_PACKED CSLSensorWaitCmd;

/// @brief CSIPHY configuration info
typedef struct
{
    UINT16  laneMask;               ///< Lane mask details
    UINT16  laneAssign;             ///< Lane sensor will be using
    UINT8   CSIPHY3Phase;           ///< Details on 3Phase / 2Phase operation
    UINT8   comboMode;              ///< Info regarding combo_mode is enable / disable
    UINT8   laneCount;              ///< Total number of lanes
    UINT8   secureMode;             ///< secure / non-secure mode
    UINT64  settleTimeMilliseconds; ///< Settling time in ms
    UINT64  dataRate;               ///< Data rate
    UINT32  mipiFlags;              ///< Mipi flags mask
    UINT32  reserved;               ///< reserved
} CAMX_PACKED CSLSensorCSIPHYInfo;

/// @brief Maximum sensor flash trigger
static const UINT8 CSLFlashMaxLEDTrigger = 3;

/// @brief Flash init command info
typedef struct {
    UINT32 flashType;               ///< Flash type
    UINT8  reserved;                ///< Reserved
    UINT8  cmdType;                 ///< Cmd type
    UINT16 reserved1;               ///< reserved
} CAMX_PACKED CSLFlashInfoCmd;

/// @brief Flash fire cmd
typedef struct {
    UINT32 count;                                       ///< Number of flash module
    UINT8  opcode;                                      ///< Opcode
    UINT8  cmdType;                                     ///< Cmd type
    UINT16 reserved;                                    ///< reserved
    UINT32 currentMilliampere[CSLFlashMaxLEDTrigger];   ///< Array of current for each flash module
    INT64  irLedData;                                   ///< IR LED data
} CAMX_PACKED CSLFlashFireCmd;

/// @brief Flash RER cmd
typedef struct {
    UINT32 count;                    ///< Number of flash module
    UINT8  opcode;                   ///< Opcode
    UINT8  cmdType;                  ///< Cmd type
    UINT16 numberOfIteration;        ///< Number of iteration
    UINT32 flashOnDelayMillisecond;  ///< Flash on delay
    UINT32 flashOffDelayMillisecond; ///< Flash off delay
    UINT32 currentMilliampere[CSLFlashMaxLEDTrigger];    ///< Array of current for each flash module
}CAMX_PACKED CSLFlashRERCmd;

// @brief Flash query current cmd
typedef struct {
    UINT32 reserved;                 ///< Reserved
    UINT8  opcode;                   ///< Opcode
    UINT8  cmdType;                  ///< Cmd type
    UINT32 currentMilliampere;       ///< Current
    UINT16 reserved1;                ///< reserved
} CAMX_PACKED CSLFlashQueryCurrentCmd;

/// @brief Flash query capability info cmd
typedef struct {
    UINT32 slotInfo;                                ///< Slot info
    UINT32 maxFlashCurrent[CSLFlashMaxLEDTrigger];  ///< Maximum supported current for flash
    UINT32 maxFlashDuration[CSLFlashMaxLEDTrigger]; ///< Maximum flash turn on duration
    UINT32 maxTorchCurrent[CSLFlashMaxLEDTrigger];  ///< Maximum supported current for torch
} CAMX_PACKED CSLFlashQueryCapability;

// End of packed definitions
CAMX_END_PACKED

#ifdef __cplusplus
}
#endif // __cplusplus

#endif // CAMXCSLSENSORDEFS_H
