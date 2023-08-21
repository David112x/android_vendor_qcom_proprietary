// NOWHINE ENTIRE FILE
//----------------------------------------------------------------------------
// Copyright (c) 2018-2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
//---------------------------------------------------------------------------

#pragma once

#include "CommonDefs.h"

/// @brief Holds the necessary HW/chip version.
///        Needs to be set at start and then can be used by other SW components.
class NcLibChipInfo
{
public:

    enum TitanVersion
    {
        TITAN_180 = 0x00010800,
        TITAN_175 = 0x00010705,
        TITAN_170 = 0x00010700,
        TITAN_160 = 0x00010600,
        TITAN_150 = 0x00010500,
        TITAN_140 = 0x00010400,
        TITAN_120 = 0x00010200,
        TITAN_480 = 0x00040800,
    };

    /* Derived information combining Titan Tiering version and HW version */
    enum ChipVersion
    {
        TITAN_VERSION_FIRST = 0,

        TITAN_170_V1 = 1,        // SDM845 (Napali)
        TITAN_160_V1 = 2,        // SDM840 (NapaliQ)
        TITAN_175_V1 = 3,        // SDM855 (Hana)
        TITAN_170_V2 = 4,        // SDM845 (Napali) v2
        TITAN_150_V1 = 5,        // SDM640 (Talos)
        TITAN_170_V2_ONEIPE = 6, // SDM670 (Warlock 1 core)
        TITAN_480_V1 = 7,        // SDM865 (Kona)
        TITAN_VERSION_MAX,
    } ;

    /// @brief  Required initializer
    /// @param  titanVersion The version of the camera platform
    /// @param  hwVersion    The version of the camera CPAS
    /// @return None
    static void Set(uint32_t titanVersion, uint32_t hwVersion);

    /// @brief  Camera platform version accessor
    /// @return The version of the camera platform
    static inline TitanVersion GetTitanVersion()
    {
        NCLIB_ASSERT_DEBUG(s_titanVersion != INVALID_TITAN_VERSION);
        return s_titanVersion;
    }

    /// @brief  Camera CPAS version accessor
    /// @return The version of the camera CPAS
    static inline uint32_t GetHwVersion()
    {
        NCLIB_ASSERT_DEBUG(s_hwVersion != INVALID_CHIP_VERSION);
        return s_hwVersion;
    }

    /// @brief  Tests whether running on Napali
    /// @return True if running on Napali
    static inline bool IsNapali() { return GetTitanVersion() == TITAN_170; }

    /// @brief  Tests whether running on Napali or Talos
    /// @return True if running on Napali or Talos
    static inline bool IsNapaliFamily() { return (GetTitanVersion() == TITAN_170) || (GetTitanVersion() == TITAN_150); }

    /// @brief  Tests whether running on Hana
    /// @return True if running on Hana
    static inline bool IsHana()   { return GetTitanVersion() == TITAN_175; }

    /// @brief  Tests whether running on Kona
    /// @return True if running on Kona
    static inline bool IsKona()   { return GetTitanVersion() == TITAN_480; }

    /// @brief  Tests whether running on Talos
    /// @return True if running on Talos
    static inline bool IsTalos()  { return GetTitanVersion() == TITAN_150; }

private:
    NcLibChipInfo() = delete;

private:
    static const TitanVersion INVALID_TITAN_VERSION = (TitanVersion)0;
    static const ChipVersion  INVALID_CHIP_VERSION = TITAN_VERSION_MAX;

    static TitanVersion s_titanVersion;
    static uint32_t s_hwVersion;
};
