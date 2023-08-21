////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2016 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  camxpacketbuilder.h
/// @brief PacketBuilder class declaration
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef CAMXPACKETBUILDER_H
#define CAMXPACKETBUILDER_H

#include "camxdefs.h"
#include "camxtypes.h"

CAMX_NAMESPACE_BEGIN

class   CmdBuffer;
class   Packet;
struct  CSLSensorPowerSetting;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief
///     Static class that implements the CAMX packet builder. Packet builder encapsulates all that is needed to compose command
///     buffers, e.g., various register-write formats and structures. HWL blocks will be the clients of this class and
///     therefore, will not have to know such details as who eventually runs the commands (e.g. HW or SW) or the exact
///     data structures for individual commands. It is expected that one packet builder can generate one common format that
///     will become standard across multiple platforms; however, in the possible case where one particular platform requires
///     changes to this format, a HW-specific packet builder can be used instead.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class PacketBuilder
{

public:
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// WriteRegRange
    ///
    /// @brief  Writes a command into the command buffer that, when executed, has the effect of writing a number of values at
    ///         the register address beginning with "reg".
    ///
    /// @param  pCmdBuffer  Address of the target command buffer
    /// @param  reg         Beginning of the register address range where values will be written
    /// @param  numVals     Number of values (registers) that will be written
    /// @param  pVals       An array of values that will be written
    ///
    /// @return CamxResult
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static CamxResult WriteRegRange(
        CmdBuffer*  pCmdBuffer,
        UINT32      reg,
        UINT32      numVals,
        UINT32*     pVals);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// RequiredWriteRegRangeSizeInDwords
    ///
    /// @brief  Calculates the required size in dwords for a WriteRegRange command.
    ///
    /// @param  numVals Number of values (registers) that will be written
    ///
    /// @return Size in dwords
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static UINT32 RequiredWriteRegRangeSizeInDwords(
        UINT32      numVals);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// WriteInterleavedRegs
    ///
    /// @brief  Writes a command into the command buffer that, when executed, has the effect of writing a number of
    ///         register/value pairs.
    ///
    /// @param  pCmdBuffer  Address of the target command buffer
    /// @param  numRegVals  Number of register/value pairs that will be written
    /// @param  pRegVals    An array of register/value pairs that will be written. The even indices are registers and
    ///                     the odd indices are values, e.g., {reg1, val1, reg2, val2, ...}.
    ///
    /// @return CamxResult
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static CamxResult WriteInterleavedRegs(
        CmdBuffer*  pCmdBuffer,
        UINT32      numRegVals,
        UINT32*     pRegVals);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// RequiredWriteInterleavedRegsSizeInDwords
    ///
    /// @brief  Calculates the required size of a WriteInterleavedRegs command in dwords.
    ///
    /// @param  numRegVals  Number of register/value pairs that will be written
    ///
    /// @return Size in dwords
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static UINT32 RequiredWriteInterleavedRegsSizeInDwords(
        UINT32  numRegVals);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// WriteIndirect
    ///
    /// @brief  Writes a command into the command buffer that, when executed, has the effect of executing an indirect command
    ///         buffer.
    ///
    /// @param  pCmdBuffer          Address of the target command buffer
    /// @param  pIndirectCmdBuffer  Cmd buffer object representing the indirect command buffer.
    /// @param  offset              Byte offset withing the pIndirectCmdBuffer to be used as the start address. Typically,
    ///                             should be 0 unless pIndirectCmdBuffer holds multiple indirect buffers.
    /// @param  size                Size of the LUT in bytes. IF this is 0, then offset must be 0 as well, and the full size
    ///                             of the pIndirectCmdBuffer is used as the buffer size.
    ///
    /// @return CamxResult
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static CamxResult WriteIndirect(
        CmdBuffer*  pCmdBuffer,
        CmdBuffer*  pIndirectCmdBuffer,
        UINT32      offset,
        UINT32      size);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// RequiredWriteIndirectSizeInDwords
    ///
    /// @brief  Calculates the size of a WriteIndirect command in dwords.
    ///
    /// @return Size in dwords
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static UINT32 RequiredWriteIndirectSizeInDwords();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// WriteDMI
    ///
    /// @brief  Writes a command into the command buffer that, when executed, has the effect of writing a LUT into a DMI ram.
    ///
    /// @param  pCmdBuffer  Address of the target command buffer
    /// @param  DMIAddr     Address of the DMI
    /// @param  DMIBank     Selected bank that the DMI will write to
    /// @param  pDMIBuffer  Cmd buffer object that holds the LUT data
    /// @param  offset      Byte offset withing the pDMIBuffer to be used as the start address. Typically,
    ///                     should be 0 unless pDMIBuffer holds multiple LUTs.
    /// @param  size        Size of the LUT in bytes. IF this is 0, then offset must be 0 as well, and the full size
    ///                     of the pDMIBuffer is used as LUT size.
    ///
    /// @return CamxResult
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static CamxResult WriteDMI(
        CmdBuffer*  pCmdBuffer,
        UINT32      DMIAddr,
        UINT8       DMIBank,
        CmdBuffer*  pDMIBuffer,
        UINT32      offset,
        UINT32      size);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// RequiredWriteDMISizeInDwords
    ///
    /// @brief  Calculates the size of a WriteDMI command in dwords.
    ///
    /// @return Size in dwords
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static UINT32 RequiredWriteDMISizeInDwords();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// WriteChangeRegBase
    ///
    /// @brief  Writes a command into the command buffer that, when executed, has the effect of changing the base address of all
    ///         the register/DMI writes following this command. This can be used to create command buffers that are relative to
    ///         a start address.
    ///
    /// @param  pCmdBuffer  Address of the target command buffer
    /// @param  base        Base address
    ///
    /// @return CamxResult
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static CamxResult WriteChangeRegBase(
        CmdBuffer*  pCmdBuffer,
        UINT32      base);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// RequiredWriteChangeRegBaseSizeInDwords
    ///
    /// @brief  Calculates the size of a WriteChangeRegBase command in dwords
    ///
    /// @return Size in dwords
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static UINT32 RequiredWriteChangeRegBaseSizeInDwords();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// WriteI2CInterleavedRegs
    ///
    /// @brief  Writes a command into the command buffer that, when executed, has the effect of writing a number of
    ///         register/value pairs.
    ///
    /// @param  pCmdBuffer  Address of the target command buffer
    /// @param  addrType    Address type of I2C operation
    /// @param  dataType    Data type of I2C operation
    /// @param  numRegVals  Number of register/value pairs that will be written
    /// @param  pRegVals    An array of register/value pairs that will be written. The even indices are registers and
    ///                     the odd indices are values, e.g., {reg1, val1, reg2, val2, ...}.
    ///
    /// @return CamxResult
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static CamxResult WriteI2CInterleavedRegs(
        CmdBuffer*  pCmdBuffer,
        UINT8       addrType,
        UINT8       dataType,
        UINT32      numRegVals,
        UINT32*     pRegVals);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// RequiredWriteI2CInterleavedRegsSizeInDwords
    ///
    /// @brief  Calculates the required size of a WriteI2CInterleavedRegs command in dwords.
    ///
    /// @param  numRegVals  Number of register/value pairs that will be written
    ///
    /// @return Size in dwords
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static UINT32 RequiredWriteI2CInterleavedRegsSizeInDwords(
        UINT32  numRegVals);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// WriteI2CContinuousRegs
    ///
    /// @brief  Writes a command into the command buffer that, when executed, has the effect of writing a number of
    ///         values starting at the given register address.
    ///
    /// @param  pCmdBuffer  Address of the target command buffer
    /// @param  addrType    Address type of I2C operation
    /// @param  dataType    Data type of I2C operation
    /// @param  reg         Base address of the register range
    /// @param  numVals     Number of register/value pairs that will be written
    /// @param  pVals       An array of values that will be written.
    ///
    /// @return CamxResult
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static CamxResult WriteI2CContinuousRegs(
        CmdBuffer*  pCmdBuffer,
        UINT8       addrType,
        UINT8       dataType,
        UINT32      reg,
        UINT32      numVals,
        UINT32*     pVals);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// RequiredWriteI2CContinuousRegsSizeInDwords
    ///
    /// @brief  Calculates the required size of a WriteI2CContinuousRegs command in dwords.
    ///
    /// @param  numVals Number of values that will be written
    ///
    /// @return Size in dwords
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static UINT32 RequiredWriteI2CContinuousRegsSizeInDwords(
        UINT32  numVals);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ReadI2CInterleavedRegs
    ///
    /// @brief  Writes a command into the command buffer that, when executed, has the effect of reading a number of
    ///         register and writing to a known memory location.
    ///
    /// @param  pCmdBuffer  Address of the target command buffer
    /// @param  addrType    Address type of I2C operation
    /// @param  dataType    Data type of I2C operation
    /// @param  numRegs     Number of registers that will be read
    /// @param  pRegs       An array of register addresses that will be read.
    ///
    /// @return CamxResult
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static CamxResult ReadI2CInterleavedRegs(
        CmdBuffer*  pCmdBuffer,
        UINT8       addrType,
        UINT8       dataType,
        UINT32      numRegs,
        UINT32*     pRegs);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// RequiredReadI2CInterleavedRegsSizeInDwords
    ///
    /// @brief  Calculates the required size of a ReadI2CInterleavedRegs command in dwords.
    ///
    /// @param  numRegs Number of registers that will be read
    ///
    /// @return Size in dwords
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static UINT32 RequiredReadI2CInterleavedRegsSizeInDwords(
        UINT32  numRegs);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ReadI2CContinuousRegs
    ///
    /// @brief  Writes a command into the command buffer that, when executed, has the effect of reading a number of
    ///         registers starting at the given register address.
    ///
    /// @param  pCmdBuffer  Address of the target command buffer
    /// @param  addrType    Address type of I2C operation
    /// @param  dataType    Data type of I2C operation
    /// @param  reg         Base address of the register range
    /// @param  numRegs     Number of registers that will be read
    ///
    /// @return CamxResult
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static CamxResult ReadI2CContinuousRegs(
        CmdBuffer*  pCmdBuffer,
        UINT8       addrType,
        UINT8       dataType,
        UINT32      reg,
        UINT32      numRegs);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// RequiredReadI2CContinuousRegsSizeInDwords
    ///
    /// @brief  Calculates the required size of a ReadI2CContinuousRegs command in dwords.
    ///
    /// @param  numRegs Number of registers that will be read
    ///
    /// @return Size in dwords
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static UINT32 RequiredReadI2CContinuousRegsSizeInDwords(
        UINT32  numRegs);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// WriteSensorConditionalWait
    ///
    /// @brief  Writes a command into the command buffer that, when executed, will poll the given register until its value
    ///         becomes equal to the provided (masked) value.
    ///
    /// @param  pCmdBuffer  Address of the target command buffer
    /// @param  addrType    Address type of I2C operation
    /// @param  dataType    Data type of I2C operation
    /// @param  reg         Address of the register to poll
    /// @param  val         Expected value
    /// @param  mask        Mask that should be applied to the value
    /// @param  timeout     Timeout after which the wait ends
    ///
    /// @return CamxResult
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static CamxResult WriteSensorConditionalWait(
        CmdBuffer*  pCmdBuffer,
        UINT8       addrType,
        UINT8       dataType,
        UINT32      reg,
        UINT32      val,
        UINT32      mask,
        UINT32      timeout);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// RequiredWriteSensorConditionalWaitSizeInDwords
    ///
    /// @brief  Calculates the required size of a WriteSensorConditionalWait command in dwords.
    ///
    /// @return Size in dwords
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static UINT32 RequiredWriteSensorConditionalWaitSizeInDwords();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// WriteSensorWait
    ///
    /// @brief  Writes a command into the command buffer that, when executed, will wait for the given amount of time
    ///
    /// @param  pCmdBuffer  Address of the target command buffer
    /// @param  delay       Delay after which the wait ends
    /// @param  useHW       Specifies whether the wait should happen in HW
    ///
    /// @return CamxResult
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static CamxResult WriteSensorWait(
        CmdBuffer*  pCmdBuffer,
        UINT32      delay,
        BOOL        useHW);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// RequiredWriteSensorWaitSizeInDwords
    ///
    /// @brief  Calculates the required size of a WriteSensorWait command in dwords.
    ///
    /// @return Size in dwords
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static UINT32 RequiredWriteSensorWaitSizeInDwords();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// WritePowerSequence
    ///
    /// @brief  Writes a command into the command buffer that, when executed, will execute the given power sequence
    ///
    /// @param  pCmdBuffer  Address of the target command buffer
    /// @param  numSettings Number of power settings to be written
    /// @param  pSettings   Array of power settings
    ///
    /// @return CamxResult
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static CamxResult WritePowerSequence(
        CmdBuffer*              pCmdBuffer,
        UINT32                  numSettings,
        CSLSensorPowerSetting*  pSettings);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// RequiredWritePowerSequenceSizeInDwords
    ///
    /// @brief  Calculates the required size of a WritePowerSequence command in dwords.
    ///
    /// @param  numSettings Number of power settings that will be written
    ///
    /// @return Size in dwords
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static UINT32 RequiredWritePowerSequenceSizeInDwords(
        UINT32  numSettings);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// WriteSensorProbe
    ///
    /// @brief  Writes a command into the command buffer that, when executed, will probe the given sensor
    ///
    /// @param  pCmdBuffer  Address of the target command buffer
    /// @param  sensorId    Sensor Id to be probed
    /// @param  addrType    Address type of I2C operation
    /// @param  dataType    Data type of I2C operation
    /// @param  reg         Address of the register to poll
    /// @param  val         Expected value
    /// @param  mask        Mask that should be applied to the value
    ///
    /// @return CamxResult
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static CamxResult WriteSensorProbe(
        CmdBuffer*  pCmdBuffer,
        UINT16      sensorId,
        UINT8       addrType,
        UINT8       dataType,
        UINT32      reg,
        UINT32      val,
        UINT32      mask);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// RequiredWriteSensorProbeSizeInDwords
    ///
    /// @brief  Calculates the required size of a WriteSensorProbe command in dwords.
    ///
    /// @return Size in dwords
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static UINT32 RequiredWriteSensorProbeSizeInDwords();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// RequiredGenericBlobDataSizeInDwords
    ///
    /// @brief  Calculates the required size of a WriteGenericBlobData for a given blob type in dwords.
    ///
    /// @param  blobSize    Size (in bytes) of the blob to be inserted in the command buffer and also encoded in the header.
    ///
    /// @return Size in dwords
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static UINT32 RequiredGenericBlobDataSizeInDwords(
        UINT32  blobSize);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// WriteGenericBlobData
    ///
    /// @brief  Writes a Blob data of given size with blob type into the command buffer.
    ///
    /// @param  pCmdBuffer  Address of the target command buffer
    /// @param  blobType    Identifier for this blob that will be encoded in the header
    /// @param  blobSize    Size (in bytes) of the blob to be inserted in the command buffer and also encoded in the header.
    /// @param  pBlobData   Blob data that will be inserted in the command buffer.
    ///
    /// @return CamxResult
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static CamxResult WriteGenericBlobData(
        CmdBuffer*  pCmdBuffer,
        UINT32      blobType,
        UINT32      blobSize,
        BYTE*       pBlobData);

private:
    PacketBuilder()                                 = default;  // Make default constructor private
    PacketBuilder(const PacketBuilder&)             = delete;   // Disallow the copy constructor.
    PacketBuilder& operator=(const PacketBuilder&)  = delete;   // Disallow assignment operator.
};

CAMX_NAMESPACE_END

#endif // CAMXPACKETBUILDER_H