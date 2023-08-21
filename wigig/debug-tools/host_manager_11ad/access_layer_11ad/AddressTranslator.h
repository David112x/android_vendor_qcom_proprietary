/*
* Copyright (c) 2018-2020 Qualcomm Technologies, Inc.
* All Rights Reserved.
* Confidential and Proprietary - Qualcomm Technologies, Inc.
*/

#pragma once

#include "HostManagerDefinitions.h"
#include <cstdint>

class AddressTranslator
{
public:
    static bool ToAhbAddress(uint32_t srcAddr, uint32_t& dstAddr, BasebandRevision deviceRevision);
};
