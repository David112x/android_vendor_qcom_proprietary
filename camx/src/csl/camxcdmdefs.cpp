////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2016-2018 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  camxcdmdefs.cpp
/// @brief The implementation composing CDM commands.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// NOWHINE FILE GR016 : Using kernel coding guidelines
// NOWHINE FILE GR017 : Using kernel coding guidelines
// NOWHINE FILE CF003 : Using kernel coding guidelines
// NOWHINE FILE DC011 : Using kernel coding guidelines
// NOWHINE FILE CF032 : Using kernel coding guidelines

#include <stdint.h>
#include <stddef.h>
#include "camxcdmdefs.h"

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

/// @brief Data structures that are directly accessed by HW must be packed to have the expected layout expected. Use a
///        platform-independent macro for this purpose.
#if defined(__GNUC__)

#define CAMX_PACKED         __attribute__((__packed__))
#define CAMX_BEGIN_PACKED
#define CAMX_END_PACKED
#else

#define CAMX_PACKED
#define CAMX_BEGIN_PACKED \
    __pragma(pack(push)) \
    __pragma(pack(1))

#define CAMX_END_PACKED \
    __pragma(pack(pop))
#endif // _WINDOWS

// CDM command size in dword.
static const unsigned int CDMCmdHeaderSizes[CDMCmdCDMEndCommandIds] =
{
    0,  // UNUSED
    3,  // DMI
    0,  // UNUSED
    2,  // RegContinuous
    1,  // RegRandom
    2,  // BUFFER_INDIREC
    2,  // GenerateIRQ
    3,  // WaitForEvent
    1,  // ChangeBase
    1,  // PERF_CONTINUOUSROL
    3,  // DMI32
    3,  // DMI64
};

// Must pack to have the layout expected by HW
CAMX_BEGIN_PACKED

/// @brief  Data layout definition for a CDM random register command.
typedef struct
{
    unsigned int count      : 16;   ///< Number of register writes
    unsigned int reserved   : 8;
    unsigned int cmd        : 8;    ///< Command ID (CDMCmd)
} CAMX_PACKED cdm_regrandom_cmd_t;

/// @brief  Data layout definition for a CDM register range command.
typedef struct
{
    unsigned int count      : 16;   ///< Number of register writes
    unsigned int reserved0  : 8;
    unsigned int cmd        : 8;    ///< Command ID (CDMCmd)
    unsigned int offset     : 24;   ///< Start address of the range of registers
    unsigned int reserved1  : 8;
} CAMX_PACKED cdm_regcontinuous_cmd_t;

/// @brief  Data layout definition for a CDM DMI command.
typedef struct
{
    unsigned int length     : 16;   ///< Number of bytes in LUT - 1
    unsigned int reserved   : 8;
    unsigned int cmd        : 8;    ///< Command ID (CDMCmd)
    unsigned int addr;              ///< Address of the LUT in memory
    unsigned int DMIAddr    : 24;   ///< Address of the target DMI config register
    unsigned int DMISel     : 8;    ///< DMI identifier
} CAMX_PACKED cdm_dmi_cmd_t;

/// @brief  Data layout definition for a CDM indirect buffer execution command.
typedef struct
{
    unsigned int length     : 16;   ///< Number of bytes in buffer - 1
    unsigned int reserved   : 8;
    unsigned int cmd        : 8;    ///< Command ID (CDMCmd)
    unsigned int addr;              ///< Device address of the indirect buffer
} CAMX_PACKED cdm_indirect_cmd_t;

/// @brief  Data layout definition for CDM base address change command.
typedef struct
{
    unsigned int base   : 24;   ///< Base address to be changed to
    unsigned int cmd    : 8;    ///< Command ID (CDMCmd)
} CAMX_PACKED cdm_changebase_cmd_t;

// End of packed definitions
CAMX_END_PACKED

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// cdm_get_cmd_header_size
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
unsigned int cdm_get_cmd_header_size(
    unsigned int command)
{
    return CDMCmdHeaderSizes[command];
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// cdm_required_size_reg_continuous
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
uint32_t cdm_required_size_reg_continuous(
    uint32_t  numVals)
{
    return cdm_get_cmd_header_size(CDMCmdRegContinuous) + numVals;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// cdm_required_size_reg_random
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
uint32_t cdm_required_size_reg_random(
    uint32_t numRegVals)
{
    // Each reg/val pair takes 2 DWORDs, hence * 2
    return cdm_get_cmd_header_size(CDMCmdRegRandom) + (2 * numRegVals);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// cdm_required_size_dmi
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
uint32_t cdm_required_size_dmi(void)
{
    return cdm_get_cmd_header_size(CDMCmdDMI);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// cdm_required_size_indirect
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
uint32_t cdm_required_size_indirect(void)
{
    return cdm_get_cmd_header_size(CDMCmdBufferIndirect);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// cdm_required_size_changebase
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
uint32_t cdm_required_size_changebase(void)
{
    return cdm_get_cmd_header_size(CDMCmdChangeBase);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// cdm_offsetof_dmi_addr
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
uint32_t cdm_offsetof_dmi_addr(void)
{
    return offsetof(cdm_dmi_cmd_t, addr);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// cdm_offsetof_indirect_addr
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
uint32_t cdm_offsetof_indirect_addr(void)
{
    return offsetof(cdm_indirect_cmd_t, addr);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// cdm_write_regcontinuous
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
uint32_t* cdm_write_regcontinuous(
    uint32_t*   pCmdBuffer,
    uint32_t    reg,
    uint32_t    numVals,
    uint32_t*   pVals)
{
    cdm_regcontinuous_cmd_t* pHeader = reinterpret_cast<cdm_regcontinuous_cmd_t*>(pCmdBuffer);
    pHeader->count = numVals;
    pHeader->cmd = CDMCmdRegContinuous;
    pHeader->reserved0 = 0;
    pHeader->reserved1 = 0;
    pHeader->offset = reg;

    pCmdBuffer += cdm_get_cmd_header_size(CDMCmdRegContinuous);

    for (uint32_t i = 0; i < numVals; i++)
    {
        pCmdBuffer[i] = pVals[i];
    }

    pCmdBuffer += numVals;

    return pCmdBuffer;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// cdm_write_regrandom
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
uint32_t* cdm_write_regrandom(
    uint32_t*   pCmdBuffer,
    uint32_t    numRegVals,
    uint32_t*   pRegVals)
{
    cdm_regrandom_cmd_t* pHeader = reinterpret_cast<cdm_regrandom_cmd_t*>(pCmdBuffer);
    pHeader->count = numRegVals;
    pHeader->cmd = CDMCmdRegRandom;
    pHeader->reserved = 0;

    pCmdBuffer += cdm_get_cmd_header_size(CDMCmdRegRandom);

    for (uint32_t i = 0; i < numRegVals; i++)
    {
        // Write register address followed by corresponding value.
        // pRegVals's content are like {reg0, val0, reg1, val1, ...}
        *pCmdBuffer++ = *pRegVals++;
        *pCmdBuffer++ = *pRegVals++;
    }

    return pCmdBuffer;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// cdm_write_dmi
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
uint32_t* cdm_write_dmi(
    uint32_t*   pCmdBuffer,
    uint8_t     dmiCmd,
    uint32_t    DMIAddr,
    uint8_t     DMISel,
    uint32_t    dmiBufferAddr,
    uint32_t    length)
{
    cdm_dmi_cmd_t* pHeader = reinterpret_cast<cdm_dmi_cmd_t*>(pCmdBuffer);

    pHeader->cmd        = dmiCmd;
    pHeader->addr = dmiBufferAddr;
    pHeader->length = length - 1;
    pHeader->DMIAddr = DMIAddr;
    pHeader->DMISel = DMISel;

    pCmdBuffer += cdm_get_cmd_header_size(CDMCmdDMI);

    return pCmdBuffer;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// cdm_write_indirect
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
uint32_t* cdm_write_indirect(
    uint32_t*   pCmdBuffer,
    uint32_t    indirectBufAddr,
    uint32_t    length)
{
    cdm_indirect_cmd_t* pHeader = reinterpret_cast<cdm_indirect_cmd_t*>(pCmdBuffer);
    pHeader->cmd = CDMCmdBufferIndirect;
    pHeader->addr = indirectBufAddr;
    pHeader->length = length - 1;

    pCmdBuffer += cdm_get_cmd_header_size(CDMCmdBufferIndirect);

    return pCmdBuffer;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// cdm_write_changebase
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
uint32_t* cdm_write_changebase(
    uint32_t*   pCmdBuffer,
    uint32_t    base)
{
    cdm_changebase_cmd_t* pHeader = reinterpret_cast<cdm_changebase_cmd_t*>(pCmdBuffer);
    pHeader->cmd = CDMCmdChangeBase;
    pHeader->base = base;

    pCmdBuffer += cdm_get_cmd_header_size(CDMCmdChangeBase);

    return pCmdBuffer;
}

#ifdef __cplusplus
}
#endif // __cplusplus
