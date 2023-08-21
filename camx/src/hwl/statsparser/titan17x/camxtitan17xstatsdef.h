////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2017 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  camxtitan17xstatsdef.h
///
/// @brief Titan17x stats defintion
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef CAMXTITAN17XSTATSDEF_H
#define CAMXTITAN17XSTATSDEF_H

#include "camxmem.h"

CAMX_NAMESPACE_BEGIN

static const UINT32 IHist12Bins = 256;

CAMX_BEGIN_PACKED

/// @brief Memory map of raw hardware BF v2.3 stats region
struct BFStats23HwOutput
{
    UINT64  H1Sum       : 37;
    UINT64  reserved1   : 2;
    UINT32  H1Count     : 23;
    UINT64  reserved2   : 1;
    UINT32  selector    : 1;

    UINT64  H1Sharpness    : 40;
    UINT64  reserved3      : 14;
    UINT32  regionID       : 8;
    UINT64  reserved4      : 2;

    UINT64  VSum         : 37;
    UINT64  reserved5    : 2;
    UINT32  VCount       : 23;
    UINT64  reserved6    : 2;

    UINT64  VSharpness   : 40;
    UINT64  reserved7    : 24;
} CAMX_PACKED;

/// @brief Memory map of raw hardware HDR BE v1.5 stats region when Saturateduation output is disabled.
struct HDRBE15StatsHwOutput
{
    UINT32    RSum         : 30;
    UINT32    reserved1    : 2;
    UINT32    BSum         : 30;
    UINT32    reserved2    : 2;
    UINT32    GrSum        : 30;
    UINT32    reserved3    : 2;
    UINT32    GbSum        : 30;
    UINT32    reserved4    : 2;

    UINT32    RCount       : 16;
    UINT32    BCount       : 16;
    UINT32    GrCount      : 16;
    UINT32    GbCount      : 16;
} CAMX_PACKED;

/// @brief Memory map of raw hardware HDR BE v1.5 stats region when Saturateduation output is enabled.
struct HDRBE15StatsWithSaturationHwOutput
{
    UINT32    RSum         : 30;
    UINT32    reserved1    : 2;
    UINT32    BSum         : 30;
    UINT32    reserved2    : 2;
    UINT32    GrSum        : 30;
    UINT32    reserved3    : 2;
    UINT32    GbSum        : 30;
    UINT32    reserved4    : 2;

    UINT32    RCount    : 16;
    UINT32    BCount    : 16;
    UINT32    GrCount   : 16;
    UINT32    GbCount   : 16;

    UINT32    SaturatedRSum     : 30;
    UINT32    reserved5         : 2;
    UINT32    SaturatedBSum     : 30;
    UINT32    reserved6         : 2;
    UINT32    SaturatedGrSum    : 30;
    UINT32    reserved7         : 2;
    UINT32    SaturatedGbSum    : 30;
    UINT32    reserved8         : 2;

    UINT32    SaturatedRCount     : 16;
    UINT32    SaturatedBCount     : 16;
    UINT32    SaturatedGrCount    : 16;
    UINT32    SaturatedGbCount    : 16;
} CAMX_PACKED;

/// @brief Memory map of raw hardware HDR BE v1.5 stats region when Y-stats enabled.
struct HDRBE15StatsYStatsEnableHwOutput
{
    UINT32    RSum         : 30;
    UINT32    reserved1    : 2;
    UINT32    BSum         : 30;
    UINT32    reserved2    : 2;
    UINT32    GSum         : 30;
    UINT32    reserved3    : 2;
    UINT32    YSum         : 31;
    UINT32    reserved4    : 1;

    UINT32    RCount   : 16;
    UINT32    BCount   : 16;
    UINT32    GCnt     : 16;
    UINT32    YCnt     : 16;
} CAMX_PACKED;

/// @brief Memory map of raw hardware AWB BG v1.5 stats region when Saturateduation output is disabled.
struct AWBBG15StatsHwOutput
{
    UINT32    RSum         : 30;
    UINT32    reserved1    : 2;
    UINT32    BSum         : 30;
    UINT32    reserved2    : 2;
    UINT32    GrSum        : 30;
    UINT32    reserved3    : 2;
    UINT32    GbSum        : 30;
    UINT32    reserved4    : 2;

    UINT32    RCount    : 16;
    UINT32    BCount    : 16;
    UINT32    GrCount   : 16;
    UINT32    GbCount   : 16;
} CAMX_PACKED;

/// @brief Memory map of raw hardware AWB BG v1.5 stats region when Saturateduation output is enabled.
struct AWBBG15StatsWithSaturationHwOutput
{
    UINT32    RSum         : 30;
    UINT32    reserved1    : 2;
    UINT32    BSum         : 30;
    UINT32    reserved2    : 2;
    UINT32    GrSum        : 30;
    UINT32    reserved3    : 2;
    UINT32    GbSum        : 30;
    UINT32    reserved4    : 2;

    UINT32    RCount     : 16;
    UINT32    BCount     : 16;
    UINT32    GrCount    : 16;
    UINT32    GbCount    : 16;

    UINT32    SaturatedRSum    : 30;
    UINT32    reserved5        : 2;
    UINT32    SaturatedBSum    : 30;
    UINT32    reserved6        : 2;
    UINT32    SaturatedGrSum   : 30;
    UINT32    reserved7        : 2;
    UINT32    SaturatedGbSum   : 30;
    UINT32    reserved8        : 2;

    UINT32    SaturatedRCount     : 16;
    UINT32    SaturatedBCount     : 16;
    UINT32    SaturatedGrCount    : 16;
    UINT32    SaturatedGbCount    : 16;
} CAMX_PACKED;

/// @brief Memory map of raw hardware AWB BG v1.5 stats region when Y-stats enabled.
struct AWBBG15StatsYStatsEnableHwOutput
{
    UINT32    RSum         : 30;
    UINT32    reserved1    : 2;
    UINT32    BSum         : 30;
    UINT32    reserved2    : 2;
    UINT32    GSum         : 30;
    UINT32    reserved3    : 2;
    UINT32    YSum         : 31;
    UINT32    reserved4    : 1;

    UINT32    RCount    : 16;
    UINT32    BCount    : 16;
    UINT32    GCnt      : 16;
    UINT32    YCnt      : 16;
} CAMX_PACKED;

/// @brief Memory map of raw hardware Tintless BG v1.5 stats region when satuation output is disabled.
struct TintlessBG15StatsHwOutput
{
    struct
    {
        UINT32 RSum      : 30; ///< RSum of Tintless BG stats
        UINT32 reserved1 : 2;  ///< Reserved field of Tintless BG Stats
    };

    struct
    {
        UINT32 BSum      : 30; ///< BSum of Tinltess BG stats
        UINT32 reserved2 : 2;  ///< Reserved field of Tintless BG Stats
    };

    struct
    {
        UINT32 GRSum     : 30; ///< GRSum of Tintless BG stats
        UINT32 reserved3 : 2;  ///< Reserved field of Tintless BG Stats
    };

    struct
    {
        UINT32 GBSum     : 30; ///< GBSum of Tintless BG stats
        UINT32 reserved4 : 2;  ///< Reserved field of Tintless BG Stats
    };

    struct
    {
        UINT32 RCount : 16; ///< RCount of Tintless BG stats
        UINT32 BCount : 16; ///< BCount of Tintless BG stats
    };

    struct
    {
        UINT32 GRCount : 16; ///< GRCount of Tintless BG stats
        UINT32 GBCount : 16; ///< GBCount of Tintless BG stats
    };
} CAMX_PACKED;

/// @brief Memory map of raw hardware Tintless BG v1.5 stats region when satuation output is enabled.
struct TintlessBG15StatsWithSaturationHwOutput
{
    struct
    {
        UINT32 RSum      : 30; ///< RSum of Tintless BG stats
        UINT32 reserved1 : 2;  ///< Reserved field of Tintless BG Stats
    };

    struct
    {
        UINT32 BSum      : 30; ///< BSum of Tintless BG stats
        UINT32 reserved2 : 2;  ///< Reserved field of Tintless BG Stats
    };

    struct
    {
        UINT32 GRSum     : 30; ///< GrSum of Tintless BG stats
        UINT32 reserved3 : 2;  ///< Reserved field of Tintless BG Stats
    };

    struct
    {
        UINT32 GBSum     : 30; ///< GbSum of Tintless BG stats
        UINT32 reserved4 : 2;  ///< Reserved field of Tintless BG Stats
    };

    struct
    {
        UINT32 RCount : 16; ///< RCount of Tintless BG stats
        UINT32 BCount : 16; ///< BCount of Tintless BG stats
    };

    struct
    {
        UINT32 GRCount : 16; ///< GRCount of Tintless BG stats
        UINT32 GBCount : 16; ///< GBCount of Tintless BG stats
    };

    struct
    {
        UINT32 SaturationRSum : 30; ///< RSum of Tintless BG stats for saturated Pixels
        UINT32 reserved5      : 2;  ///< Reserved field of Tintless BG Stats
    };

    struct
    {
        UINT32 SaturationBSum : 30; ///< BSum of Tintless BG stats for saturated Pixels
        UINT32 reserved6      : 2;  ///< Reserved field of Tintless BG Stats
    };

    struct
    {
        UINT32 SaturationGRSum : 30; ///< GRSum of Tintless BG stats for saturated Pixels
        UINT32 reserved7       : 2;  ///< Reserved field of Tintless BG Stats
    };

    struct
    {
        UINT32 SaturationGBSum : 30; ///< GBSum of Tintless BG stats for saturated Pixels
        UINT32 reserved8       : 2;  ///< Reserved field of Tintless BG Stats
    };

    struct
    {
        UINT32 SaturationRCount : 16; ///< RCount of Tintless BG stats for saturated Pixels
        UINT32 SaturationBCount : 16; ///< BCount of Tintless BG stats for saturated Pixels
    };

    struct
    {
        UINT32 SaturationGRCount : 16; ///< GrCount of Tintless BG stats for saturated Pixels
        UINT32 SaturationGBCount : 16; ///< GbCount of Tintless BG stats for saturated Pixels
    };
} CAMX_PACKED;

/// @brief Memory map of raw hardware Tintless BG v1.5 stats region when Y-stats enabled.
struct TintlessBG15StatsYStatsEnableHwOutput
{
    struct
    {
        UINT32 RSum      : 30; ///< RSum of Tintless BG stats
        UINT32 reserved1 : 2;  ///< Reserved field of Tintless BG Stats
    };

    struct
    {
        UINT32 BSum      : 30; ///< BSum of Tintless BG stats
        UINT32 reserved2 : 2;  ///< Reserved field of Tintless BG Stats
    };

    struct
    {
        UINT32 GSum      : 30; ///< GSum of Tintless BG stats
        UINT32 reserved3 : 2;  ///< Reserved field of Tintless BG Stats
    };

    struct
    {
        UINT32 YSum      : 30; ///< YSum of Tintless BG stats
        UINT32 reserved4 : 2;  ///< Reserved field of Tintless BG Stats
    };

    struct
    {
        UINT32 RCount : 16; ///< RCount of Tintless BG stats
        UINT32 BCount : 16; ///< BCount of Tintless BG stats
    };

    struct
    {
        UINT32 GCount : 16; ///< GCount of Tintless BG stats
        UINT32 YCount : 16; ///< YCount of Tintless BG stats
    };
} CAMX_PACKED;

/// @brief Memory map of raw hardware IHist v1.2 stats
struct IHistStats12HwOutput
{
    UINT16 YCCHistogram[IHist12Bins];   ///< Array containing the either the Y, Cb or Cr histogram values
    UINT16 greenHistogram[IHist12Bins]; ///< Array containing the green histogram values
    UINT16 blueHistogram[IHist12Bins];  ///< Array containing the blue histogram values
    UINT16 redHistogram[IHist12Bins];   ///< Array containing the red histogram values
} CAMX_PACKED;

CAMX_END_PACKED

CAMX_NAMESPACE_END

#endif // CAMXTITAN17XSTATSDEF_H
