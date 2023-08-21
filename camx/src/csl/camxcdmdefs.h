////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2016 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  camxcdmdefs.h
/// @brief The definitions needed for composing CDM commands.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef CAMXCDMDEFS_H
#define CAMXCDMDEFS_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

// This must be consistent with the register spec
static const unsigned int CDMCmdDMI                 = 0x1;                          ///< DMI
static const unsigned int CDMCmdRegContinuous       = 0x3;                          ///< Continuous register write
static const unsigned int CDMCmdRegRandom           = 0x4;                          ///< Noncontiguous register-value pairs write
static const unsigned int CDMCmdBufferIndirect      = 0x5;                          ///< Indirect buffer execution
static const unsigned int CDMCmdGenerateIRQ         = 0x6;                          ///< Generate IRQ
static const unsigned int CDMCmdWaitForEvent        = 0x7;                          ///< Wait for event
static const unsigned int CDMCmdChangeBase          = 0x8;                          ///< Change base address
static const unsigned int CDMCmdPerformanceCounter  = 0x9;                          ///< Performance counter command
static const unsigned int CDMCmdDMI32               = 0xa;                          ///< Legacy DMI for 32bit LUT
static const unsigned int CDMCmdDMI64               = 0xb;                          ///< Legacy DMI for 64bit LUT
// Make sure the following is adjusted when new commands are added (in new HW versions)
static const unsigned int CDMCmdCDMEndCommandIds    = 0xc;                          ///< End of CDM command Ids

/// @brief  Define extended commands for supporting SW command processor
static const unsigned int SwCmdDMI32                = CDMCmdCDMEndCommandIds + 64; ///< Legacy SW DMI for 32bit LUT
static const unsigned int SwCmdDMI64                = CDMCmdCDMEndCommandIds + 65; ///< Legacy SW DMI for 64bit LUT

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// cdm_get_cmd_header_size
///
/// @brief  Returns the size of the given command header.
///
/// @param  command Command ID
///
/// @return Size of the command in DWORDs
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
unsigned int cdm_get_cmd_header_size(
    unsigned int command);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// cdm_required_size_reg_continuous
///
/// @brief  Calculates the size of a reg-continuous command in dwords.
///
/// @param  numVals Number of continuous values
///
/// @return Size in dwords
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
uint32_t cdm_required_size_reg_continuous(
    uint32_t  numVals);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief  Calculates the size of a reg-random command in dwords.
///
/// @param  numRegVals  Number of register/value pairs
///
/// @return Size in dwords
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
uint32_t cdm_required_size_reg_random(
    uint32_t numRegVals);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// cdm_required_size_dmi
///
/// @brief  Calculates the size of a DMI command in dwords.
///
/// @return Size in dwords
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
uint32_t cdm_required_size_dmi(void);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// cdm_required_size_indirect
///
/// @brief  Calculates the size of an indirect command in dwords.
///
/// @return Size in dwords
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
uint32_t cdm_required_size_indirect(void);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// cdm_required_size_changebase
///
/// @brief  Calculates the size of a change-base command in dwords.
///
/// @return Size in dwords
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
uint32_t cdm_required_size_changebase(void);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// cdm_offsetof_dmi_addr
///
/// @brief  Returns the offset of address field in the DMI command header.
///
/// @return Offset of addr field
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
uint32_t cdm_offsetof_dmi_addr(void);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// cdm_offsetof_indirect_addr
///
/// @brief  Returns the offset of address field in the indirect command header.
///
/// @return Offset of addr field
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
uint32_t cdm_offsetof_indirect_addr(void);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// cdm_write_regcontinuous
///
/// @brief  Writes a command into the command buffer that, when executed, has the effect of writing a number of values at
///         the register address beginning with "reg".
///
/// @param  pCmdBuffer  Pointer to command buffer
/// @param  reg         Beginning of the register address range where values will be written
/// @param  numVals     Number of values (registers) that will be written
/// @param  pVals       An array of values that will be written
///
/// @return Pointer in command buffer pointing past the written commands
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
uint32_t* cdm_write_regcontinuous(
    uint32_t*   pCmdBuffer,
    uint32_t    reg,
    uint32_t    numVals,
    uint32_t*   pVals);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// cdm_write_regrandom
///
/// @brief  Writes a command into the command buffer that, when executed, has the effect of writing a number of
///         register/value pairs.
///
/// @param  pCmdBuffer  Pointer to command buffer
/// @param  numRegVals  Number of register/value pairs that will be written
/// @param  pRegVals    An array of register/value pairs that will be written. The even indices are registers and
///                     the odd indices arevalues, e.g., {reg1, val1, reg2, val2, ...}.
///
/// @return Pointer in command buffer pointing past the written commands
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
uint32_t* cdm_write_regrandom(
    uint32_t*   pCmdBuffer,
    uint32_t    numRegVals,
    uint32_t*   pRegVals);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// cdm_write_dmi
///
/// @brief  Writes a command into the command buffer that, when executed, has the effect of writing a LUT into a DMI RAM.
///
/// @param  pCmdBuffer      Pointer to command buffer
/// @param  dmiCmd          DMI command
/// @param  DMIAddr         Address of the DMI
/// @param  DMISel          Selected bank that the DMI will write to
/// @param  dmiBufferAddr   Device address of the DMI data
/// @param  length          Size of data in bytes
///
/// @return Pointer in command buffer pointing past the written commands
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
uint32_t* cdm_write_dmi(
    uint32_t*   pCmdBuffer,
    uint8_t     dmiCmd,
    uint32_t    DMIAddr,
    uint8_t     DMISel,
    uint32_t    dmiBufferAddr,
    uint32_t    length);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// cdm_write_indirect
///
/// @brief  Writes a command into the command buffer that, when executed, has the effect of executing an indirect command
///         buffer.
///
/// @param  pCmdBuffer          Pointer to command buffer
/// @param  indirectBufferAddr  Device addres of the indirect command buffer.
/// @param  length              Size of data in bytes
///
/// @return Pointer in command buffer pointing past the written commands
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
uint32_t* cdm_write_indirect(
    uint32_t*   pCmdBuffer,
    uint32_t    indirectBufferAddr,
    uint32_t    length);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// cdm_write_changebase
///
/// @brief  Writes a command into the command buffer that, when executed, has the effect of changing CDM (address) base.
///
/// @param  pCmdBuffer  Pointer to command buffer
/// @param  base        New base (device) address
///
/// @return Pointer in command buffer pointing past the written commands
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
uint32_t* cdm_write_changebase(
    uint32_t*   pCmdBuffer,
    uint32_t    base);

#ifdef __cplusplus
}
#endif // __cplusplus

#endif // CAMXCDMDEFS_H
