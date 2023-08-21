/*
* Copyright (c) 2018-2020 Qualcomm Technologies, Inc.
* All Rights Reserved.
* Confidential and Proprietary - Qualcomm Technologies, Inc.
*/

#include "AddressTranslator.h"
#include "DebugLogger.h"

#include <ostream>
#include <cstdint>
#include <vector>

namespace
{
    // Defines a Linker-AHB region mapping. this struct mimics the data
    // structure used by the Linux driver to map addresses. All translation
    // tables are copied from the driver code.
    // Linker address region: [LinkerRegionBegin, LinkerRegionEnd)
    // AHB Region: [AhbRegionBegin, AhbRegionBegin + LinkerRegionEnd - LinkerRegionBegin)
    // Notes:
    // 1. FW and uCode may have linker regions with the same addresses.
    //    These addresses cannot be accessed through linker address, only AHB address is supported.
    // 2. Other regions may be accessed by either linker or AHB address.

    struct fw_map
    {
        uint32_t LinkerRegionBegin;   // Defines linker region begin point
        uint32_t LinkerRegionEnd;     // Defines linker region exclusive end point
        uint32_t AhbRegionBegin;      // Defines corresponding AHB region begin point
        const char* RegionName;       // Region name
        bool IsFwRegion;              // FW/uCode region
    };

    std::ostream& operator<<(std::ostream& os, const fw_map& region)
    {
        return os << "{"
                  << Address(region.LinkerRegionBegin) << ", "
                  << Address(region.LinkerRegionEnd)  << ", "
                  << Address(region.AhbRegionBegin) << ", "
                  << region.RegionName << ", "
                  << BoolStr(region.IsFwRegion)
                  << "}";
    }

    // memory remapping table for Sparrow B0
    const std::vector<struct fw_map> sparrow_b0_fw_mapping =
    {
        {0x000000, 0x040000, 0x8c0000, "fw_code", true},        /* FW code RAM 256k */
        {0x800000, 0x808000, 0x900000, "fw_data", true},        /* FW data RAM 32k  */
        {0x840000, 0x860000, 0x908000, "fw_peri", true},        /* periph data 128k */
        {0x880000, 0x88a000, 0x880000, "rgf",     true},        /* various RGF 40k  */
        {0x88a000, 0x88b000, 0x88a000, "AGC_tbl", true},        /* AGC table   4k   */
        {0x88b000, 0x88c000, 0x88b000, "rgf_ext", true},        /* Pcie_ext_rgf 4k  */
        {0x88c000, 0x88c200, 0x88c000, "mac_rgf_ext", true},    /* mac_ext_rgf 512b */
        {0x8c0000, 0x949000, 0x8c0000, "upper", true},          /* upper area 548k  */
        /* UCODE areas - accessible by debugfs blobs but not by wmi_addr_remap. UCODE areas MUST be added AFTER FW areas! */
        {0x000000, 0x020000, 0x920000, "uc_code", false},       /* ucode code RAM 128k */
        {0x800000, 0x804000, 0x940000, "uc_data", false},       /* ucode data RAM 16k  */
    };

    // memory remapping table for Sparrow D0
    const std::vector<struct fw_map> sparrow_d0_fw_mapping =
    {
        {0x000000, 0x040000, 0x8c0000, "fw_code", true},        /* FW code RAM 256k */
        {0x800000, 0x808000, 0x900000, "fw_data", true},        /* FW data RAM 32k  */
        {0x840000, 0x860000, 0x908000, "fw_peri", true},        /* periph data 128k */
        {0x880000, 0x88a000, 0x880000, "rgf",     true},        /* various RGF 40k  */
        {0x88a000, 0x88b000, 0x88a000, "AGC_tbl", true},        /* AGC table   4k   */
        {0x88b000, 0x88c000, 0x88b000, "rgf_ext", true},        /* Pcie_ext_rgf 4k  */
        {0x88c000, 0x88c500, 0x88c000, "mac_rgf_ext", true},    /* mac_ext_rgf 1280b */
        {0x8c0000, 0x949000, 0x8c0000, "upper", true},          /* upper area 548k  */
        /* UCODE areas - accessible by debugfs blobs but not by wmi_addr_remap. UCODE areas MUST be added AFTER FW areas! */
        {0x000000, 0x020000, 0x920000, "uc_code", false},       /* ucode code RAM 128k */
        {0x800000, 0x804000, 0x940000, "uc_data", false},       /* ucode data RAM 16k  */
    };

    // memory remapping table for Talyn MA
    const std::vector<struct fw_map> talyn_ma_fw_mapping =
    {
        {0x000000, 0x100000, 0x900000, "fw_code", true},        /* FW code RAM 1M */
        {0x800000, 0x820000, 0xa00000, "fw_data", true},        /* FW data RAM 128k */
        {0x840000, 0x858000, 0xa20000, "fw_peri", true},        /* periph. data RAM 96k */
        {0x880000, 0x88a000, 0x880000, "rgf",     true},        /* various RGF 40k */
        {0x88a000, 0x88b000, 0x88a000, "AGC_tbl", true},        /* AGC table 4k */
        {0x88b000, 0x88c000, 0x88b000, "rgf_ext", true},        /* Pcie_ext_rgf 4k */
        {0x88c000, 0x88c200, 0x88c000, "mac_rgf_ext", true},    /* mac_ext_rgf 512b */
        {0x8c0000, 0x949000, 0x8c0000, "upper", true},          /* upper area 548k */
        /* UCODE areas - accessible by debugfs blobs but not by wmi_addr_remap. UCODE areas MUST be added AFTER FW areas! */
        {0x000000, 0x040000, 0xa38000, "uc_code", false},       /* ucode code RAM 256k */
        {0x800000, 0x808000, 0xa78000, "uc_data", false},       /* ucode data RAM 32k */
    };

    // memory remapping table for Talyn MB and Talyn_MC
    const std::vector<struct fw_map> talyn_mb_fw_mapping =
    {
        {0x000000, 0x100000, 0x900000, "fw_code",      true},   /* FW code RAM 1M */
        {0x800000, 0x820000, 0xa00000, "fw_data",      true},   /* FW data RAM 128k */
        {0x840000, 0x858000, 0xa20000, "fw_peri",      true},   /* periph. data RAM 96k */
        {0x880000, 0x88a000, 0x880000, "rgf",          true},   /* various RGF 40k */
        {0x88a000, 0x88b000, 0x88a000, "AGC_tbl",      true},   /* AGC table 4k */
        {0x88b000, 0x88c000, 0x88b000, "rgf_ext",      true},   /* Pcie_ext_rgf 4k */
        {0x88c000, 0x88c8d0, 0x88c000, "mac_rgf_ext",  true},   /* mac_ext_rgf 2256b */
        {0x88d000, 0x88e000, 0x88d000, "ext_user_rgf", true},   /* ext USER RGF 4k */
        {0x890000, 0x894000, 0x890000, "sec_pka",      true},   /* SEC PKA 16k */
        {0x898000, 0x898c18, 0x898000, "sec_kdf_rgf",  true},   /* SEC KDF RGF 3096b */
        {0x89a000, 0x89a84c, 0x89a000, "sec_main",     true},   /* SEC MAIN 2124b */
        {0x8a0000, 0x8a1000, 0x8a0000, "otp",          true},   /* OTP 4k */
        {0x8b0000, 0x8c0000, 0x8b0000, "dma_ext_rgf",  true},   /* DMA EXT RGF 64k */
        {0x8c0000, 0x8c0210, 0x8c0000, "dum_user_rgf", true},   /* DUM USER RGF 528b */
        {0x8c2000, 0x8c2128, 0x8c2000, "dma_ofu",      true},   /* DMA OFU 296b */
        {0x8c3000, 0x8c3100, 0x8c3000, "ucode_debug",  true},   /* ucode debug 256b */
        {0x900000, 0xa80000, 0x900000, "upper",        true},   /* upper area 1536k */
        /* UCODE areas - accessible by debugfs blobs but not by wmi_addr_remap. UCODE areas MUST be added AFTER FW areas! */
        {0x000000, 0x040000, 0xa38000, "uc_code", false},       /* ucode code RAM 256k */
        {0x800000, 0x808000, 0xa78000, "uc_data", false},       /* ucode data RAM 32k */
    };

    // memory remapping table for Borrelly P1
    const std::vector<struct fw_map> borrelly_p1_fw_mapping =
    {
        {0x000000, 0x0A0000, 0xA00000, "user_code", true},  /* USER code RAM 640KB */
        {0x800000, 0x820000, 0x960000, "user_data", true},  /* USER data RAM 128KB */
        {0x840000, 0x880000, 0x980000, "user_peri", true},  /* periph. data RAM 256KB */
        // RSXD areas, MUST be added AFTER FW areas!
        {0x000000, 0x020000, 0x9c0000, "rsxd_code", false}, /* RSXD code RAM 128KB */
        {0x800000, 0x810000, 0x9e0000, "rsxd_data", false}, /* RSXD data RAM 64KB */
    };

    bool Remap(uint32_t srcAddr, uint32_t& dstAddr, const std::vector<struct fw_map>& mapping, BasebandRevision deviceRevision)
    {
        for (const fw_map& regionMap : mapping)
        {
            LOG_VERBOSE << "Checking " << Hex<8>(srcAddr) << " against " << regionMap << std::endl;

            if (regionMap.LinkerRegionBegin <= srcAddr && srcAddr < regionMap.LinkerRegionEnd)
            {
                dstAddr = srcAddr + regionMap.AhbRegionBegin - regionMap.LinkerRegionBegin;

                LOG_VERBOSE << "Successful address translation (" << deviceRevision << "): "
                    << Address(srcAddr) << " --> " << Address(dstAddr) << std::endl;
                return true;
            }
        }

        return false;
    }
}

bool AddressTranslator::ToAhbAddress(uint32_t srcAddr, uint32_t& dstAddr, BasebandRevision deviceRevision)
{
    dstAddr = srcAddr;

    switch (deviceRevision)
    {
    case SPR_B0:
        return Remap(srcAddr, dstAddr, sparrow_b0_fw_mapping, deviceRevision);
    case SPR_D0:
        return Remap(srcAddr, dstAddr, sparrow_d0_fw_mapping, deviceRevision);
    case TLN_M_A0:
        return Remap(srcAddr, dstAddr, talyn_ma_fw_mapping, deviceRevision);
    case TLN_M_B0:
    case TLN_M_C0:
        return Remap(srcAddr, dstAddr, talyn_mb_fw_mapping, deviceRevision);
    case BRL_P1:
        return Remap(srcAddr, dstAddr, borrelly_p1_fw_mapping, deviceRevision);
    default:
        LOG_ERROR << "Unsupported device revision: " << deviceRevision;
        return false;
    }
}
