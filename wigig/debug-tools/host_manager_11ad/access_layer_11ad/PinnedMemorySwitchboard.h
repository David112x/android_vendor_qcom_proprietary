/*
* Copyright (c) 2020 Qualcomm Technologies, Inc.
* All Rights Reserved.
* Confidential and Proprietary - Qualcomm Technologies, Inc.
*/

#pragma once

#include "HostManagerDefinitions.h"
#include <cstdint>
#include <sstream>

enum class PINNED_MEMORY
{
    FW_ERROR,
    UCODE_ERROR,
    FW_VERSION_MAJOR,
    FW_VERSION_MINOR,
    FW_VERSION_SUB_MINOR,
    FW_VERSION_BUILD,
    POINTER_TABLE_PATTERN,
    POINTER_TABLE_ADR,
    REG_FW_USAGE_1,
    REG_FW_USAGE_2
};

std::ostream& operator<<(std::ostream &os, PINNED_MEMORY pin);

class PinnedMemorySwitchboard
{
public:
    static uint32_t GetAhbAddress(PINNED_MEMORY pin, BasebandType deviBasebandType);
};
