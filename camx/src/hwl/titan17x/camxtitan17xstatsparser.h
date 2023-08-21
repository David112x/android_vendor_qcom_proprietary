////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2017-2020 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  camxtitan17xstatsparser.h
///
/// @brief Titan17xStatsParser definition
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef CAMXTITAN17XSTATSPARSER_H
#define CAMXTITAN17XSTATSPARSER_H

#include "camxstatsparser.h"
#include "camxstatscommon.h"

#include "chisoftwarestatsinterface.h"

CAMX_NAMESPACE_BEGIN

class Node;

static const UINT32 IHist12Bins         = 256;

static const UINT32 HDRBHist13Bins      = 256;

static const UINT32 NumberOfIFEStripes  = 2;

CAMX_BEGIN_PACKED

/// @brief Memory map of raw hardware BF v2.3 stats region
struct BFStats23HwOutput
{
    UINT64  H1Sum       : 37;
    UINT64  reserved1   : 2;
    UINT32  H1Cnt       : 23;
    UINT64  reserved2   : 1;
    UINT32  sel         : 1;

    UINT64  H1Sharpness : 40;
    UINT64  reserved3   : 14;
    UINT32  regionID    : 8;
    UINT64  reserved4   : 2;

    UINT64  VSum        : 37;
    UINT64  reserved5   : 2;
    UINT32  VCnt        : 23;
    UINT64  reserved6   : 2;

    UINT64  VSharpness  : 40;
    UINT64  reserved7   : 24;
} CAMX_PACKED;

/// @brief Memory map of raw hardware BF v2.5 stats region
struct BFStats25HwOutput
{
    UINT64  H1Sum       : 37;
    UINT64  reserved1   : 2;
    UINT32  H1Cnt       : 23;
    UINT64  reserved2   : 1;
    UINT32  sel         : 1;

    UINT64  H1Sharpness : 40;
    UINT64  reserved3   : 14;
    UINT32  regionID    : 8;
    UINT64  reserved4   : 2;

    UINT64  VSum        : 37;
    UINT64  reserved5   : 2;
    UINT32  VCnt        : 23;
    UINT64  reserved6   : 2;

    UINT64  VSharpness  : 40;
    UINT64  reserved7   : 14;
    UINT32  outputID    : 8;
    UINT32  merge       : 1;
    UINT32  endOfBuffer : 1;
} CAMX_PACKED;


/// @brief Memory map of raw hardware HDR BE v1.5 stats region when satuation output is disabled.
struct HDRBE15StatsHwOutput
{
    UINT32    RSum      : 30;
    UINT32    reserved1 : 2;
    UINT32    BSum      : 30;
    UINT32    reserved2 : 2;
    UINT32    GrSum     : 30;
    UINT32    reserved3 : 2;
    UINT32    GbSum     : 30;
    UINT32    reserved4 : 2;

    UINT32    RCnt      : 16;
    UINT32    BCnt      : 16;
    UINT32    GrCnt     : 16;
    UINT32    GbCnt     : 16;
} CAMX_PACKED;

/// @brief Memory map of raw hardware HDR BE v1.5 stats region when satuation output is enabled.
struct HDRBE15StatsWithSaturationHwOutput
{
    UINT32    RSum      : 30;
    UINT32    reserved1 : 2;
    UINT32    BSum      : 30;
    UINT32    reserved2 : 2;
    UINT32    GrSum     : 30;
    UINT32    reserved3 : 2;
    UINT32    GbSum     : 30;
    UINT32    reserved4 : 2;

    UINT32    RCnt      : 16;
    UINT32    BCnt      : 16;
    UINT32    GrCnt     : 16;
    UINT32    GbCnt     : 16;

    UINT32    SatRSum   : 30;
    UINT32    reserved5 : 2;
    UINT32    SatBSum   : 30;
    UINT32    reserved6 : 2;
    UINT32    SatGrSum  : 30;
    UINT32    reserved7 : 2;
    UINT32    SatGbSum  : 30;
    UINT32    reserved8 : 2;

    UINT32    SatRCnt   : 16;
    UINT32    SatBCnt   : 16;
    UINT32    SatGrCnt  : 16;
    UINT32    SatGbCnt  : 16;
} CAMX_PACKED;

/// @brief Memory map of raw hardware HDR BE v1.5 stats region when Y-stats enabled.
struct HDRBE15StatsYStatsEnableHwOutput
{
    UINT32    RSum      : 30;
    UINT32    reserved1 : 2;
    UINT32    BSum      : 30;
    UINT32    reserved2 : 2;
    UINT32    GSum      : 30;
    UINT32    reserved3 : 2;
    UINT32    YSum      : 31;
    UINT32    reserved4 : 1;

    UINT32    RCnt      : 16;
    UINT32    BCnt      : 16;
    UINT32    GCnt      : 16;
    UINT32    YCnt      : 16;
} CAMX_PACKED;

/// @brief Memory map of raw hardware AWB BG v1.5 stats region when satuation output is disabled.
struct AWBBG15StatsHwOutput
{
    UINT32    RSum      : 30;
    UINT32    reserved1 : 2;
    UINT32    BSum      : 30;
    UINT32    reserved2 : 2;
    UINT32    GrSum     : 30;
    UINT32    reserved3 : 2;
    UINT32    GbSum     : 30;
    UINT32    reserved4 : 2;

    UINT32    RCnt      : 16;
    UINT32    BCnt      : 16;
    UINT32    GrCnt     : 16;
    UINT32    GbCnt     : 16;
} CAMX_PACKED;

/// @brief Memory map of raw hardware AWB BG v1.5 stats region when satuation output is enabled.
struct AWBBG15StatsWithSaturationHwOutput
{
    UINT32    RSum      : 30;
    UINT32    reserved1 : 2;
    UINT32    BSum      : 30;
    UINT32    reserved2 : 2;
    UINT32    GrSum     : 30;
    UINT32    reserved3 : 2;
    UINT32    GbSum     : 30;
    UINT32    reserved4 : 2;

    UINT32    RCnt      : 16;
    UINT32    BCnt      : 16;
    UINT32    GrCnt     : 16;
    UINT32    GbCnt     : 16;

    UINT32    SatRSum   : 30;
    UINT32    reserved5 : 2;
    UINT32    SatBSum   : 30;
    UINT32    reserved6 : 2;
    UINT32    SatGrSum  : 30;
    UINT32    reserved7 : 2;
    UINT32    SatGbSum  : 30;
    UINT32    reserved8 : 2;

    UINT32    SatRCnt   : 16;
    UINT32    SatBCnt   : 16;
    UINT32    SatGrCnt  : 16;
    UINT32    SatGbCnt  : 16;
} CAMX_PACKED;

/// @brief Memory map of raw hardware AWB BG v1.5 stats region when Y-stats enabled.
struct AWBBG15StatsYStatsEnableHwOutput
{
    UINT32    RSum      : 30;
    UINT32    reserved1 : 2;
    UINT32    BSum      : 30;
    UINT32    reserved2 : 2;
    UINT32    GSum      : 30;
    UINT32    reserved3 : 2;
    UINT32    YSum      : 31;
    UINT32    reserved4 : 1;

    UINT32    RCnt      : 16;
    UINT32    BCnt      : 16;
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
        UINT32 SaturationGRCount : 16; ///< GRCount of Tintless BG stats for saturated Pixels
        UINT32 SaturationGBCount : 16; ///< GBCount of Tintless BG stats for saturated Pixels
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

/// @brief Memory map of R/G/B bins in HDRBHist v1.3 stats
struct HDRBHistStatsHwBins
{
    UINT32    redBin    : 25;
    UINT32    reserved1 : 7;
    UINT32    greenBin  : 25;
    UINT32    reserved2 : 7;
    UINT32    blueBin   : 25;
    UINT32    reserved3 : 7;
} CAMX_PACKED;

/// @brief Memory map of raw hardware HDRBHist v1.3 stats
struct HDRBHistStatsHwOutput
{
    HDRBHistStatsHwBins channelsHDRBHist[HDRBHist13Bins]; ///< Channel data for each of the bins
} CAMX_PACKED;
CAMX_END_PACKED

/// @brief Memory map of raw hardware BHist v1.4 stats
struct BHistStatsHwOutput
{
    UINT32  histogramBin    : 25;
    UINT32  reserved        : 7;
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief Representation of a Titan17x stats parser
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class Titan17xStatsParser : public StatsParser
{
public:
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetInstance
    ///
    /// @brief  Static method to allocate the Titan17xStatsParser Object.
    ///
    /// @return Pointer to the concrete HW Stats Parser object
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static Titan17xStatsParser* GetInstance();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// Parse
    ///
    /// @brief  Pure virtual function to parse the unparsed stats buffer for a particular type of stat.
    ///
    /// @param  statsType Type of stats that is needed.
    /// @param  pInput    Pointer to parameter structure with additional parsing data
    ///
    /// @return CamxResultSuccess if successful.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual CamxResult Parse(
        ISPStatsType  statsType,
        ParseData*    pInput);

protected:
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ~Titan17xStatsParser
    ///
    /// @brief  Destructor
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual ~Titan17xStatsParser() = default;

private:
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// Titan17xStatsParser
    ///
    /// @brief  Constructor
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    Titan17xStatsParser() = default;

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ParseBFStatsDualIFE
    ///
    /// @brief  Parse raw BF Stats buffer for Dual IFE mode
    ///
    /// @param  pUnparsedBuffer The buffer that needs to be parsed.
    /// @param  pStatsConfig    Stats configuration
    /// @param  pBFStatsOutput  Parsed BF stats to fill into
    /// @param  titanVersion    Titan version
    ///
    /// @return CamxResultSuccess if successful.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult ParseBFStatsDualIFE(
        const VOID*             pUnparsedBuffer,
        ISPBFStatsConfig*       pStatsConfig,
        ParsedBFStatsOutput*    pBFStatsOutput,
        UINT32                  titanVersion);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ParseBFStatsIndexModeDualIFE
    ///
    /// @brief  Parse raw BF Stats buffer for Index-based BF Dual IFE mode
    ///
    /// @param  pUnparsedBuffer The buffer that needs to be parsed.
    /// @param  pStatsConfig    Stats configuration
    /// @param  pBFStatsOutput  Parsed BF stats to fill into
    /// @param  titanVersion    Titan version
    ///
    /// @return CamxResultSuccess if successful.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult ParseBFStatsIndexModeDualIFE(
        const VOID*             pUnparsedBuffer,
        ISPBFStatsConfig*       pStatsConfig,
        ParsedBFStatsOutput*    pBFStatsOutput,
        UINT32                  titanVersion);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ParseBFStatsBuffer
    ///
    /// @brief  Parse raw BF Stats buffer
    ///
    /// @param  pUnparsedBuffer The buffer that needs to be parsed.
    /// @param  pAppliedROI     BF Stats ROI details
    /// @param  pBFStatsOutput  Parsed BF stats to fill into
    /// @param  titanVersion    Titan version
    ///
    /// @return CamxResultSuccess if successful.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult ParseBFStatsBuffer(
        const VOID*             pUnparsedBuffer,
        BFStatsROIConfigType*   pAppliedROI,
        ParsedBFStatsOutput*    pBFStatsOutput,
        UINT32                  titanVersion);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ParseBFStatsIndexModeBufferForDualIFE
    ///
    /// @brief  Parse raw BF Stats buffer of index-based output
    ///
    /// @param  pUnparsedBuffer The buffer that needs to be parsed.
    /// @param  pAppliedROIs    The array of pointer to the BF Stats ROI details
    /// @param  pBFStatsOutput  Parsed BF stats to fill into
    ///
    /// @return CamxResultSuccess if successful.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult ParseBFStatsIndexModeBufferForDualIFE(
        const VOID*                 pUnparsedBuffer,
        const BFStatsROIConfigType* pAppliedROIs[NumberOfIFEStripes],
        ParsedBFStatsOutput*        pBFStatsOutput);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ParseBFStats
    ///
    /// @brief  Parse the unparsed stats buffer for BF stats.
    ///
    /// @param  pInput    Pointer to parameter structure with additional parsing data
    ///
    /// @return CamxResultSuccess if successful.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult ParseBFStats(
        ParseData*    pInput);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ParseHDRBEStatsDualIFE
    ///
    /// @brief  Parse raw HDR BE Stats buffer for Dual IFE mode
    ///
    /// @param  pUnparsedBuffer     The buffer that needs to be parsed.
    /// @param  pStatsConfig        Stats configuration
    /// @param  pHDRBEStatsOutput   Parsed stats to fill into
    ///
    /// @return CamxResultSuccess if successful.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult ParseHDRBEStatsDualIFE(
        const VOID*             pUnparsedBuffer,
        ISPHDRBEStatsConfig*    pStatsConfig,
        ParsedHDRBEStatsOutput* pHDRBEStatsOutput);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ParseHDRBEStatsBuffer
    ///
    /// @brief  Parse raw HDR BE Stats buffer
    ///
    /// @param  pUnparsedBuffer     The buffer that needs to be parsed.
    /// @param  pAppliedROI         BF Stats ROI details
    /// @param  pHDRBEStatsOutput   Parsed stats to fill into
    ///
    /// @return CamxResultSuccess if successful.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult ParseHDRBEStatsBuffer(
        const VOID*             pUnparsedBuffer,
        BGBEConfig*             pAppliedROI,
        ParsedHDRBEStatsOutput* pHDRBEStatsOutput);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ParseHDRBEStats
    ///
    /// @brief  Parse the unparsed stats buffer for HDR BE stats.
    ///
    /// @param  pInput    Pointer to parameter structure with additional parsing data
    ///
    /// @return CamxResultSuccess if successful.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult ParseHDRBEStats(
        ParseData*    pInput);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// PublishHDRBEStatsVendorTags
    ///
    /// @brief  Publish the parsed BG stats buffer to the appropriate vendor tagsl
    ///
    /// @param  pNode           The node for which parsing is being done
    /// @param  pStatsOutput    The pointer to parsed stats buffer
    /// @param  pISPConfig      The pointer to the ISP config applied.
    ///
    /// @return CamxResultSuccess if successful.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult PublishHDRBEStatsVendorTags(
        Node*                   pNode,
        ParsedHDRBEStatsOutput* pStatsOutput,
        PropertyISPHDRBEStats*  pISPConfig);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// PublishBEStats
    ///
    /// @brief  Publish the parsed BG stats buffer to metadata pool
    ///
    /// @param  pNode           The node for which parsing is being done
    /// @param  pStatsOutput    The pointer to parsed stats buffer
    /// @param  pISPConfig      The pointer to the ISP config applied.
    ///
    /// @return CamxResultSuccess if successful.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult PublishBEStats(
        Node*                   pNode,
        ParsedHDRBEStatsOutput* pStatsOutput,
        PropertyISPHDRBEStats*  pISPConfig);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ParseHDRBHistStats
    ///
    /// @brief  Parse the unparsed stats buffer for HDR Bayer Histogram
    ///
    /// @param  pInput    Pointer to parameter structure with additional parsing data
    ///
    /// @return CamxResultSuccess if successful.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult ParseHDRBHistStats(
        ParseData*    pInput);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// PublishHDRHistStatsVendorTags
    ///
    /// @brief  Publish the parsed stats buffer to the appropriate vendor tagsl
    ///
    /// @param  pNode           The node for which parsing is being done
    /// @param  pStatsOutput    The pointer to parsed stats buffer
    /// @param  pISPConfig      The pointer to the ISP config applied.
    ///
    /// @return CamxResultSuccess if successful.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult PublishHDRHistStatsVendorTags(
        Node*                      pNode,
        ParsedHDRBHistStatsOutput* pStatsOutput,
        PropertyISPHDRBHistStats*  pISPConfig);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// PublishHistStatsVendorTags
    ///
    /// @brief  Publish the parsed stats buffer to the appropriate vendor tagsl
    ///
    /// @param  pNode           The node for which parsing is being done
    /// @param  pStatsOutput    The pointer to parsed stats buffer
    /// @param  pISPConfig      The pointer to the ISP config applied.
    ///
    /// @return CamxResultSuccess if successful.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult PublishHistStatsVendorTags(
        Node*                   pNode,
        ParsedBHistStatsOutput* pStatsOutput,
        PropertyISPBHistStats*  pISPConfig);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// PublishHDRBHistStats
    ///
    /// @brief  Publish the parsed HDRBhist stats buffer to metadata pool
    ///
    /// @param  pNode           The node for which parsing is being done
    /// @param  pStatsOutput    The pointer to parsed stats buffer
    /// @param  pISPConfig      The pointer to the ISP config applied.
    ///
    /// @return CamxResultSuccess if successful.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult PublishHDRBHistStats(
        Node*                      pNode,
        ParsedHDRBHistStatsOutput* pStatsOutput,
        PropertyISPHDRBHistStats*  pISPConfig);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// PublishBHistStats
    ///
    /// @brief  Publish the parsed BHist stats buffer to metadata pool
    ///
    /// @param  pNode           The node for which parsing is being done
    /// @param  pStatsOutput    The pointer to parsed stats buffer
    /// @param  pISPConfig      The pointer to the ISP config applied.
    ///
    /// @return CamxResultSuccess if successful.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult PublishBHistStats(
        Node*                   pNode,
        ParsedBHistStatsOutput* pStatsOutput,
        PropertyISPBHistStats*  pISPConfig);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ParseHDRBEOutputModeSaturationEnabled
    ///
    /// @brief  Parse the unparsed stats buffer for HDR BE stats.
    ///
    /// @param  pHDRBEStatsOutput   The buffer that needs to be parsed.
    /// @param  pUnparsedBuffer     The metadata slot where the parsed stats needs to be published.
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID ParseHDRBEOutputModeSaturationEnabled(
        ParsedHDRBEStatsOutput* pHDRBEStatsOutput,
        const VOID*             pUnparsedBuffer);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ParseHDRBEOutputModeRegular
    ///
    /// @brief  Parse the unparsed stats buffer for HDRBE stats.
    ///
    /// @param  pHDRBEStatsOutput   The buffer that needs to be parsed.
    /// @param  pUnparsedBuffer     The metadata slot where the parsed stats needs to be published.
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID ParseHDRBEOutputModeRegular(
        ParsedHDRBEStatsOutput* pHDRBEStatsOutput,
        const VOID*             pUnparsedBuffer);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ParseHDRBEOutputModeYStatsEnabled
    ///
    /// @brief  Parse the unparsed stats buffer for HDR BE stats.
    ///
    /// @param  pHDRBEStatsOutput   The buffer that needs to be parsed.
    /// @param  pUnparsedBuffer     The metadata slot where the parsed stats needs to be published.
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID ParseHDRBEOutputModeYStatsEnabled(
        ParsedHDRBEStatsOutput* pHDRBEStatsOutput,
        const VOID*             pUnparsedBuffer);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ParseHDRBEConfig
    ///
    /// @brief  Populates HDR BE stats with configuration details.
    ///
    /// @param  pAppliedROI         BF Stats ROI details
    /// @param  pHDRBEStatsOutput   Parsed stats to fill into
    ///
    /// @return CamxResultSuccess if successful.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult ParseHDRBEConfig(
        const BGBEConfig* const pAppliedROI,
        ParsedHDRBEStatsOutput* const pHDRBEStatsOutput);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ParseAWBBGStatsDualIFE
    ///
    /// @brief  Parse raw AWB BG Stats buffer for Dual IFE mode
    ///
    /// @param  pUnparsedBuffer     The buffer that needs to be parsed.
    /// @param  pStatsConfig        Stats configuration
    /// @param  pAWBBGStatsOutput   Parsed stats to fill into
    ///
    /// @return CamxResultSuccess if successful.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult ParseAWBBGStatsDualIFE(
        const VOID*             pUnparsedBuffer,
        ISPAWBBGStatsConfig*    pStatsConfig,
        ParsedAWBBGStatsOutput* pAWBBGStatsOutput);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ParseAWBBGStatsBuffer
    ///
    /// @brief  Parse raw AWB BG Stats buffer
    ///
    /// @param  pUnparsedBuffer     The buffer that needs to be parsed.
    /// @param  pAppliedROI         BG Stats ROI details
    /// @param  pAWBBGStatsOutput   Parsed stats to fill into
    ///
    /// @return CamxResultSuccess if successful.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult ParseAWBBGStatsBuffer(
        const VOID*             pUnparsedBuffer,
        BGBEConfig*             pAppliedROI,
        ParsedAWBBGStatsOutput* pAWBBGStatsOutput);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ParseBPSAWBBGStatsBuffer
    ///
    /// @brief  Parse raw BPS AWB BG Stats buffer
    ///
    /// @param  pUnparsedBuffer     The buffer that needs to be parsed.
    /// @param  pAppliedROI         BPS BG Stats ROI details
    /// @param  pAWBBGStatsOutput   Parsed stats to fill into
    ///
    /// @return CamxResultSuccess if successful.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult ParseBPSAWBBGStatsBuffer(
        const VOID*             pUnparsedBuffer,
        BGBEConfig*             pAppliedROI,
        ParsedAWBBGStatsOutput* pAWBBGStatsOutput);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// StitchDualIFEStripeBuffers
    ///
    /// @brief  Stitch two unparsed buffer stripes together into an output buffer
    ///
    /// @param  pLeftBuffer      The starting address of the left buffer
    /// @param  pRightBuffer     The starting address of the right buffer
    /// @param  pOutputBuffer    The output buffer to place the parsed stats
    /// @param  elementSize      The size of the elements of all buffers
    /// @param  leftHorizNum     The number of columns the left buffer will occupy in the output
    /// @param  rightHorizNum    The number of columns the right buffer will occupy in the output
    /// @param  numberOfElements The total number of elements to copy into the output buffer
    ///
    /// @return CamxResultSuccess if successful.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult StitchDualIFEStripeBuffers(
        const VOID* const   pLeftBuffer,
        const VOID* const   pRightBuffer,
        VOID*               pOutputBuffer,
        const SIZE_T        elementSize,
        const UINT32        leftHorizNum,
        const UINT32        rightHorizNum,
        const UINT32        numberOfElements);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ParseAWBBGConfig
    ///
    /// @brief  Populates AWB BG stats with configuration details.
    ///
    /// @param  pAppliedROI         BF Stats ROI details
    /// @param  pAWBBGStatsOutput   Parsed stats to fill into
    ///
    /// @return CamxResultSuccess if successful.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult ParseAWBBGConfig(
        const BGBEConfig* const       pAppliedROI,
        ParsedAWBBGStatsOutput* const pAWBBGStatsOutput);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ParseAWBBGStats
    ///
    /// @brief  Parse the unparsed stats buffer for AWB BG stats.
    ///
    /// @param  pInput    Pointer to parameter structure with additional parsing data
    ///
    /// @return CamxResultSuccess if successful.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult ParseAWBBGStats(
        ParseData*    pInput);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// PublishAWBBGStatsVendorTags
    ///
    /// @brief  Publish the parsed BG stats buffer to the appropriate vendor tagsl
    ///
    /// @param  pNode           The node for which parsing is being done
    /// @param  pStatsOutput    The pointer to parsed stats buffer
    /// @param  pISPConfig      The pointer to the ISP config applied.
    ///
    /// @return CamxResultSuccess if successful.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult PublishAWBBGStatsVendorTags(
        Node*                   pNode,
        ParsedAWBBGStatsOutput* pStatsOutput,
        PropertyISPAWBBGStats*  pISPConfig);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// PublishParsedTintlessBGStatsVendorTags
    ///
    /// @brief  Publish the parsed Tintless BG stats buffer to the appropriate vendor tags
    ///
    /// @param  pNode           The node for which parsing is being done
    /// @param  pStatsOutput    The pointer to parsed stats buffer
    /// @param  propID          The property ID for the stats.
    ///
    /// @return CamxResultSuccess if successful.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult PublishParsedTintlessBGStatsVendorTags(
        Node*                        pNode,
        ParsedTintlessBGStatsOutput* pStatsOutput,
        PropertyID                   propID);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// PublishParsedBHistStatsVendorTags
    ///
    /// @brief  Publish the parsed BHist stats buffer to the appropriate vendor tags
    ///
    /// @param  pNode           The node for which parsing is being done
    /// @param  pStatsOutput    The pointer to parsed stats buffer
    /// @param  propID          The property ID for the stats.
    ///
    /// @return CamxResultSuccess if successful.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult PublishParsedBHistStatsVendorTags(
        Node*                        pNode,
        ParsedBHistStatsOutput*      pStatsOutput,
        PropertyID                   propID);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// PublishParsedAWBBGStatsVendorTags
    ///
    /// @brief  Publish the parsed AWBBG stats buffer to the appropriate vendor tags
    ///
    /// @param  pNode           The node for which parsing is being done
    /// @param  pStatsOutput    The pointer to parsed stats buffer
    /// @param  propID          The property ID for the stats.
    ///
    /// @return CamxResultSuccess if successful.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult PublishParsedAWBBGStatsVendorTags(
        Node*                        pNode,
        ParsedAWBBGStatsOutput*      pStatsOutput,
        PropertyID                   propID);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// PublishBGStats
    ///
    /// @brief  Publish the parsed BG stats buffer to metadata pool
    ///
    /// @param  pNode           The node for which parsing is being done
    /// @param  pStatsOutput    The pointer to parsed stats buffer
    /// @param  pISPConfig      The pointer to the ISP config applied.
    ///
    /// @return CamxResultSuccess if successful.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult PublishBGStats(
        Node*                   pNode,
        ParsedAWBBGStatsOutput* pStatsOutput,
        PropertyISPAWBBGStats*  pISPConfig);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ParseBPSAWBBGStats
    ///
    /// @brief  Parse the unparsed stats buffer for BPS AWB BG stats.
    ///
    /// @param  pInput    Pointer to parameter structure with additional parsing data
    ///
    /// @return CamxResultSuccess if successful.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult ParseBPSAWBBGStats(
        ParseData*    pInput);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ParseTintlessBGStatsDualIFE
    ///
    /// @brief  Parse raw Tintless BG Stats buffer for Dual IFE mode
    ///
    /// @param  pUnparsedBuffer         The buffer that needs to be parsed.
    /// @param  pStatsConfig            Stats configuration
    /// @param  pTintlessBGStatsOutput  Parsed stats to fill into
    ///
    /// @return CamxResultSuccess if successful.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult ParseTintlessBGStatsDualIFE(
        const VOID*                     pUnparsedBuffer,
        ISPTintlessBGStatsConfig*       pStatsConfig,
        ParsedTintlessBGStatsOutput*    pTintlessBGStatsOutput);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ParseTintlessBGStats
    ///
    /// @brief  Parse the unparsed stats buffer for Tintless BG stats.
    ///
    /// @param  pInput    Pointer to parameter structure with additional parsing data
    ///
    /// @return CamxResultSuccess if successful.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult ParseTintlessBGStats(
        ParseData*    pInput);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ParseTintlessBGStatsBuffer
    ///
    /// @brief  Parse raw Tintless BG Stats buffer
    ///
    /// @param  pUnparsedBuffer         The buffer that needs to be parsed.
    /// @param  pAppliedROI             Tintless BG Stats ROI details
    /// @param  pTintlessBGStatsOutput  Parsed stats to fill into
    ///
    /// @return CamxResultSuccess if successful.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult ParseTintlessBGStatsBuffer(
        const VOID*                     pUnparsedBuffer,
        BGBEConfig*                     pAppliedROI,
        ParsedTintlessBGStatsOutput*    pTintlessBGStatsOutput);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ParseTintlessBGStatsConfig
    ///
    /// @brief  Populates TintlessBGStats stats with configuration details.
    ///
    /// @param  pAppliedROI             Tintless BG Stats ROI details
    /// @param  pTintlessBGStatsOutput  Parsed stats to fill into
    ///
    /// @return CamxResultSuccess if successful.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult ParseTintlessBGStatsConfig(
        const BGBEConfig* const         pAppliedROI,
        ParsedTintlessBGStatsOutput*    pTintlessBGStatsOutput);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ParseIHistStats
    ///
    /// @brief  Parse the unparsed stats buffer for Image Histogram
    ///
    /// @param  pInput    Pointer to parameter structure with additional parsing data
    ///
    /// @return CamxResultSuccess if successful.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult ParseIHistStats(
        ParseData*    pInput);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ParseAWBBGOutputModeSaturationEnabled
    ///
    /// @brief  Parse the unparsed stats buffer for AWB BG saturated stats.
    ///
    /// @param  pAWBBGStatsOutput   The buffer that needs to be parsed.
    /// @param  pUnparsedBuffer     The metadata slot where the parsed stats needs to be published.
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID ParseAWBBGOutputModeSaturationEnabled(
        ParsedAWBBGStatsOutput* pAWBBGStatsOutput,
        const VOID*             pUnparsedBuffer);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ParseBPSAWBBGOutputModeSaturationEnabled
    ///
    /// @brief  Parse the unparsed stats buffer for BPS AWB BG saturated stats.
    ///
    /// @param  pAWBBGStatsOutput   The buffer that needs to be parsed.
    /// @param  pUnparsedBuffer     The metadata slot where the parsed stats needs to be published.
    /// @param  pAppliedROI         BPS BG Stats ROI details
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID ParseBPSAWBBGOutputModeSaturationEnabled(
        ParsedAWBBGStatsOutput* pAWBBGStatsOutput,
        const VOID*             pUnparsedBuffer,
        BGBEConfig*             pAppliedROI);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ParseAWBBGOutputModeRegular
    ///
    /// @brief  Parse the unparsed stats buffer for regular AWB BG stats.
    ///
    /// @param  pAWBBGStatsOutput   The buffer that needs to be parsed.
    /// @param  pUnparsedBuffer     The metadata slot where the parsed stats needs to be published.
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID ParseAWBBGOutputModeRegular(
        ParsedAWBBGStatsOutput* pAWBBGStatsOutput,
        const VOID*             pUnparsedBuffer);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ParseBPSAWBBGOutputModeRegular
    ///
    /// @brief  Parse the unparsed stats buffer for regular BPS AWB BG stats.
    ///
    /// @param  pAWBBGStatsOutput   The buffer that needs to be parsed.
    /// @param  pUnparsedBuffer     The metadata slot where the parsed stats needs to be published.
    /// @param  pAppliedROI         BPS AWB BG Stats ROI details
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID ParseBPSAWBBGOutputModeRegular(
        ParsedAWBBGStatsOutput* pAWBBGStatsOutput,
        const VOID*             pUnparsedBuffer,
        BGBEConfig*             pAppliedROI);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ParseAWBBGOutputModeYStatsEnabled
    ///
    /// @brief  Parse the unparsed stats buffer for AWB BG Y stats.
    ///
    /// @param  pAWBBGStatsOutput   The buffer that needs to be parsed.
    /// @param  pUnparsedBuffer     The metadata slot where the parsed stats needs to be published.
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID ParseAWBBGOutputModeYStatsEnabled(
        ParsedAWBBGStatsOutput* pAWBBGStatsOutput,
        const VOID*             pUnparsedBuffer);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ParseBPSAWBBGOutputModeYStatsEnabled
    ///
    /// @brief  Parse the unparsed stats buffer for BPS AWB BG Y stats.
    ///
    /// @param  pAWBBGStatsOutput   The buffer that needs to be parsed.
    /// @param  pUnparsedBuffer     The metadata slot where the parsed stats needs to be published.
    /// @param  pAppliedROI         BPS BG Stats ROI details
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID ParseBPSAWBBGOutputModeYStatsEnabled(
        ParsedAWBBGStatsOutput* pAWBBGStatsOutput,
        const VOID*             pUnparsedBuffer,
        BGBEConfig*             pAppliedROI);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ParseTintlessBGOutputModeSaturationEnabled
    ///
    /// @brief  Parse the unparsed stats buffer for Tintless BG stats.
    ///
    /// @param  pTintlesssBGStatsOutput   Pointer to the buffer that needs to be parsed.
    /// @param  pUnparsedBuffer           Pointer to the metadata slot where the parsed stats needs to be published.
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID ParseTintlessBGOutputModeSaturationEnabled(
        ParsedTintlessBGStatsOutput* pTintlesssBGStatsOutput,
        const VOID*                  pUnparsedBuffer);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ParseTintlessBGOutputModeRegular
    ///
    /// @brief  Parse the unparsed stats buffer for TintlessBG stats.
    ///
    /// @param  pTintlessBGStatsOutput   Pointer to the buffer that needs to be parsed.
    /// @param  pUnparsedBuffer          Pointer to the metadata slot where the parsed stats needs to be published.
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID ParseTintlessBGOutputModeRegular(
        ParsedTintlessBGStatsOutput* pTintlessBGStatsOutput,
        const VOID*                  pUnparsedBuffer);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ParseTintlessBGOutputModeYStatsEnabled
    ///
    /// @brief  Parse the unparsed stats buffer for Tintless BG stats.
    ///
    /// @param  pTintlessBGStatsOutput   Pointer to the buffer that needs to be parsed.
    /// @param  pUnparsedBuffer          Pointer to the metadata slot where the parsed stats needs to be published.
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID ParseTintlessBGOutputModeYStatsEnabled(
        ParsedTintlessBGStatsOutput* pTintlessBGStatsOutput,
        const VOID*                  pUnparsedBuffer);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ParseCSOutput
    ///
    /// @brief  Parse the unparsed stats buffer for CS stats. In case of dual-IFE, two output stats generated
    ///         will be merged and be outputted as a single IFE output.
    ///
    /// @param  pColumnSum        Parsed CS stats columnSum.
    /// @param  pUnparsedBuffer   The buffer that needs to be parsed.
    /// @param  pCSConfig         CS stats configuration data
    ///
    /// @return CamxResultSuccess if successful.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult ParseCSOutput(
        UINT32                  pColumnSum[][MaxCSVertRegions],
        const VOID*             pUnparsedBuffer,
        ISPCSStatsConfig*       pCSConfig);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ParseCSStatsDualIFE
    ///
    /// @brief  Parse raw CS Stats buffer for Dual IFE mode
    ///
    /// @param  pUnparsedBuffer The buffer that needs to be parsed.
    /// @param  pStatsConfig    Stats configuration
    /// @param  pCSStatsOutput  Parsed CS stats to fill into
    ///
    /// @return CamxResultSuccess if successful.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult ParseCSStatsDualIFE(
        const VOID*             pUnparsedBuffer,
        ISPCSStatsConfig*       pStatsConfig,
        ParsedCSStatsOutput*    pCSStatsOutput);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ParseCSStats
    ///
    /// @brief  Parse the unparsed stats buffer for CS stats. In case of dual-IFE, two output stats generated
    ///         will be merged and be outputted as a single IFE output.
    ///
    /// @param  pInput    Pointer to parameter structure with additional parsing data
    ///
    /// @return CamxResultSuccess if successful.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult ParseCSStats(
        ParseData*    pInput);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ParseRSOutput
    ///
    /// @brief  Parse the unparsed stats buffer for RS stats.
    ///
    /// @param  pRSStatsOutput    Output of RS stats parsing
    /// @param  pUnparsedBuffer   The buffer that needs to be parsed.
    /// @param  pRSConfig         RS stats configuration data
    ///
    /// @return CamxResultSuccess if successful.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult ParseRSOutput(
        UINT32                  pRowSum[][MaxRSVertRegions],
        const VOID*             pUnparsedBuffer,
        ISPRSStatsConfig*       pRSConfig);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ParseRSStatsDualIFE
    ///
    /// @brief  Parse raw RS Stats buffer for Dual IFE mode
    ///
    /// @param  pUnparsedBuffer The buffer that needs to be parsed.
    /// @param  pStatsConfig    Frame level stats configuration
    /// @param  pStripeConfig   Stripe configuration
    /// @param  pRSStatsOutput  Parsed RS stats to fill into
    ///
    /// @return CamxResultSuccess if successful.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult ParseRSStatsDualIFE(
        const VOID*             pUnparsedBuffer,
        ISPRSStatsConfig*       pStatsConfig,
        ISPRSStatsConfig*       pStripeConfig,
        ParsedRSStatsOutput*    pRSStatsOutput);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ParseRSStats
    ///
    /// @brief  Parse the unparsed stats buffer for RS stats.
    ///
    /// @param  pInput    Pointer to parameter structure with additional parsing data
    ///
    /// @return CamxResultSuccess if successful.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult ParseRSStats(
        ParseData*    pInput);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// LoadSoftwareRSStats
    ///
    /// @brief  Load generate software RS stats function from library
    ///
    /// @return CamxResultSuccess if successful.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult LoadSoftwareRSStats();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ParseRSSWStats
    ///
    /// @brief  Parse the software generated stats for RS stats.
    ///
    /// @param  pInput    Pointer to parameter structure with additional parsing data
    ///
    /// @return CamxResultSuccess if successful.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult ParseRSSWStats(
        ParseData*    pInput);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ParseIHistStatsDualIFE
    ///
    /// @brief  Parse raw IHist Stats buffer for Dual IFE mode
    ///
    /// @param  pUnparsedBuffer     The buffer that needs to be parsed.
    /// @param  pStatsConfig        Stats configuration
    /// @param  pIHistStatsOutput   Parsed IHist stats to fill into
    ///
    /// @return CamxResultSuccess if successful.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult ParseIHistStatsDualIFE(
        const VOID*                pUnparsedBuffer,
        ISPIHistStatsConfig*       pStatsConfig,
        ParsedIHistStatsOutput*    pIHistStatsOutput);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ParseHDRBHistStatsDualIFE
    ///
    /// @brief  Parse raw HDR BHist Stats buffer for Dual IFE mode
    ///
    /// @param  pUnparsedBuffer         The buffer that needs to be parsed.
    /// @param  pStatsConfig            Stats configuration
    /// @param  pHDRBHistStatsOutput    Parsed HDR BHist stats to fill into
    ///
    /// @return CamxResultSuccess if successful.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult ParseHDRBHistStatsDualIFE(
        const VOID*                 pUnparsedBuffer,
        ISPHDRBHistStatsConfig*     pStatsConfig,
        ParsedHDRBHistStatsOutput*  pHDRBHistStatsOutput);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ParseBHistStatsDualIFE
    ///
    /// @brief  Parse raw BHist Stats buffer for Dual IFE mode
    ///
    /// @param  pUnparsedBuffer     The buffer that needs to be parsed.
    /// @param  pStatsConfig        Stats configuration
    /// @param  pBHistStatsOutput   Parsed IHist stats to fill into
    ///
    /// @return CamxResultSuccess if successful.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult ParseBHistStatsDualIFE(
        const VOID*             pUnparsedBuffer,
        ISPBHistStatsConfig*    pStatsConfig,
        ParsedBHistStatsOutput* pBHistStatsOutput);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ParseBHistStats
    ///
    /// @brief  Parse the unparsed stats buffer for BHistogram stats.
    ///
    /// @param  pInput    Pointer to parameter structure with additional parsing data
    ///
    /// @return CamxResultSuccess if successful.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult ParseBHistStats(
        ParseData*    pInput);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// IsSkipStatsParse
    ///
    /// @brief  Returns true false flag to skip or not to skip stats parser
    ///
    /// @param  pInput    pointer to data read from property
    ///
    /// @return TRUE or FALSE
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    BOOL IsSkipStatsParse(
        VOID*    pInput);

    Titan17xStatsParser(const Titan17xStatsParser&) = delete;             ///< Disallow the copy constructor.
    Titan17xStatsParser& operator=(const Titan17xStatsParser&) = delete;  ///< Disallow assignment operator.

    CamX::OSLIBRARYHANDLE      m_hRSStatsHandle;                  ///< handle for RS stats
    CREATEGENERATERSSTATS      m_pRSStats;                        ///< Pointer to RS stats generate function
};

CAMX_NAMESPACE_END

#endif // CAMXTITAN17XSTATSPARSER_H
