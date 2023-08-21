/*
* Copyright (c) 2020 Qualcomm Technologies, Inc.
* All Rights Reserved.
* Confidential and Proprietary - Qualcomm Technologies, Inc.
*/

#include "PinnedMemorySwitchboard.h"
#include "DebugLogger.h"
#include "Utils.h"

#include <map>

std::ostream& operator<<(std::ostream &os, PINNED_MEMORY pin)
{
    switch (pin)
    {
    case PINNED_MEMORY::FW_ERROR:               return os << "FW error";
    case PINNED_MEMORY::UCODE_ERROR:            return os << "uCode error";
    case PINNED_MEMORY::FW_VERSION_MAJOR:       return os << "FW ver. major";
    case PINNED_MEMORY::FW_VERSION_MINOR:       return os << "FW ver. minor";
    case PINNED_MEMORY::FW_VERSION_SUB_MINOR:   return os << "FW ver. sub-minor";
    case PINNED_MEMORY::FW_VERSION_BUILD:       return os << "FW ver. build";
    case PINNED_MEMORY::POINTER_TABLE_PATTERN:  return os << "Pointer table pattern";
    case PINNED_MEMORY::POINTER_TABLE_ADR:      return os << "Pointer table address";
    case PINNED_MEMORY::REG_FW_USAGE_1:         return os << "REG_FW_USAGE_1";
    case PINNED_MEMORY::REG_FW_USAGE_2:         return os << "REG_FW_USAGE_2";
    default: return os << "Unrecognized memory pin " << static_cast<int>(pin);
    }
}

namespace
{
    uint32_t WrapMapAccess(PINNED_MEMORY pin, const std::map<PINNED_MEMORY, uint32_t>& mapping)
    {
        const auto iter = mapping.find(pin);
        return iter != mapping.end() ? iter->second : Utils::REGISTER_DEFAULT_VALUE;
    }

    const std::map<PINNED_MEMORY, uint32_t> sparrow_pin_mapping  =
    {
        { PINNED_MEMORY::FW_ERROR,              0x91F020 },
        { PINNED_MEMORY::UCODE_ERROR,           0x91F028 },
        { PINNED_MEMORY::FW_VERSION_MAJOR,      0x880A2C },
        { PINNED_MEMORY::FW_VERSION_MINOR,      0x880A30 },
        { PINNED_MEMORY::FW_VERSION_SUB_MINOR,  0x880A34 },
        { PINNED_MEMORY::FW_VERSION_BUILD,      0x880A38 },
        { PINNED_MEMORY::POINTER_TABLE_PATTERN, 0x880A80 },
        { PINNED_MEMORY::POINTER_TABLE_ADR,     0x880A84 },
        { PINNED_MEMORY::REG_FW_USAGE_1,        0x880004 },
        { PINNED_MEMORY::REG_FW_USAGE_2,        0x880008 },
    };

    const std::map<PINNED_MEMORY, uint32_t> talyn_pin_mapping =
    {
        { PINNED_MEMORY::FW_ERROR,              0xA37020 },
        { PINNED_MEMORY::UCODE_ERROR,           0xA37028 },
        { PINNED_MEMORY::FW_VERSION_MAJOR,      0x880A2C },
        { PINNED_MEMORY::FW_VERSION_MINOR,      0x880A30 },
        { PINNED_MEMORY::FW_VERSION_SUB_MINOR,  0x880A34 },
        { PINNED_MEMORY::FW_VERSION_BUILD,      0x880A38 },
        { PINNED_MEMORY::POINTER_TABLE_PATTERN, 0x880A80 },
        { PINNED_MEMORY::POINTER_TABLE_ADR,     0x880A84 },
        { PINNED_MEMORY::REG_FW_USAGE_1,        0x880004 },
        { PINNED_MEMORY::REG_FW_USAGE_2,        0x880008 },
    };

    const std::map<PINNED_MEMORY, uint32_t> borrelly_pin_mapping =
    {
        { PINNED_MEMORY::FW_ERROR,              0x980000 },
        { PINNED_MEMORY::UCODE_ERROR,           0x980008 },
        { PINNED_MEMORY::FW_VERSION_MAJOR,      0x97EF44 },
        { PINNED_MEMORY::FW_VERSION_MINOR,      0x97EF48 },
        { PINNED_MEMORY::FW_VERSION_SUB_MINOR,  0x97EF4C },
        { PINNED_MEMORY::FW_VERSION_BUILD,      0x97EF50 },
        { PINNED_MEMORY::POINTER_TABLE_PATTERN, 0x97EF8C },
        { PINNED_MEMORY::POINTER_TABLE_ADR,     0x97EF90 },
        { PINNED_MEMORY::REG_FW_USAGE_1,        0x88E0AC }, // DUM_DEC_HOST_FW_CONFIG_s_REG[1]
        { PINNED_MEMORY::REG_FW_USAGE_2,        0x88E0B0 }, // DUM_DEC_HOST_FW_CONFIG_s_REG[2]
    };
}


uint32_t PinnedMemorySwitchboard::GetAhbAddress(PINNED_MEMORY pin, BasebandType deviBasebandType)
{
    switch (deviBasebandType)
    {
    case BASEBAND_TYPE_SPARROW:
        return WrapMapAccess(pin, sparrow_pin_mapping);
    case BASEBAND_TYPE_TALYN:
        return WrapMapAccess(pin, talyn_pin_mapping);
    case BASEBAND_TYPE_BORRELLY:
        return WrapMapAccess(pin, borrelly_pin_mapping);
    default:
        LOG_ERROR << "Cannot retrieve address of '" << pin << "', unsupported device type: " << deviBasebandType << std::endl;
        return Utils::REGISTER_DEFAULT_VALUE;
    }
}
