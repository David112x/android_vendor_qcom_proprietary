////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2017 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  camxtitan17xsparser.h
///
/// @brief Titan17x StatsParser header file
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef CAMXTITAN17XSPARSER_H
#define CAMXTITAN17XSPARSER_H

#include "chiispstatsdefs.h"

#include "camxcsl.h"
#include "camxistatsparser.h"
#include "camxtitan17xstatsdef.h"
#include "camxtitan17xdefs.h"


CAMX_NAMESPACE_BEGIN

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief Representation of a BF buffer that will be composed of rawbuffer and parsed buffer
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
struct BFStatsBuffer
{
    BFStats23HwOutput   rawBuffer[BFMaxROIRegions];       ///< BF Raw Buffer
    BOOL                isParsed;                         ///< To indicate if its already parsed
    ParsedBFStatsOutput parsedBuffer;                     ///< Parsed BF Buffer
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief Representation of a BE buffer that will be composed of rawbuffer and parsed buffer
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
struct HDRBEStatsBuffer
{
    HDRBE15StatsWithSaturationHwOutput rawBuffer[HDRBEStatsMaxHorizontalRegions][HDRBEStatsMaxVerticalRegions];
                                                        ///< BE Raw Buffer
                                                        ///  Max size of packed HDRBE is
                                                        ///  HDRBE15StatsWithSaturationHwOutput
                                                        ///  compared against HDRBE15StatsHwOutput
                                                        ///  and HDRBE15StatsYStatsEnableHwOutput.
    BOOL                               isParsed;        ///< To indicate if its already parsed
    ParsedHDRBEStatsOutput             parsedBuffer;    ///< Parsed HDRBE Buffer
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief Representation of a AWBBG buffer that will be composed of rawbuffer and parsed buffer
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
struct AWBBGStatsBuffer
{
    AWBBG15StatsWithSaturationHwOutput rawBuffer[AWBBGStatsMaxHorizontalRegions][AWBBGStatsMaxVerticalRegions];
                                                        ///< AWBBG Raw Buffer
                                                        ///  Max size of packed AWB is
                                                        ///  AWBBG15StatsWithSaturationHwOutput
                                                        ///  compared against AWBBG15StatsHwOutput
                                                        ///  and AWBBG15StatsYStatsEnableHwOutput.
    BOOL                               isParsed;        ///< To indicate if its already parsed
    ParsedAWBBGStatsOutput             parsedBuffer;    ///< Parsed AWBBG Buffer
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief Representation of a CS buffer that will be composed of rawbuffer and parsed buffer
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
struct CSStatsBuffer
{
    UINT16              rawBuffer[CSStatsMaxHorizontalRegions][CSStatsMaxVerticalRegions];       ///< CS Raw Buffer
    BOOL                isParsed;                                                                ///< To indicate if
                                                                                                 ///  its already parsed
    ParsedCSStatsOutput parsedBuffer;                                                            ///< Parsed CS Buffer
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief Representation of a RS buffer that will be composed of rawbuffer and parsed buffer
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
struct RSStatsBuffer
{
    UINT16              rawBuffer[RSStatsMaxHorizontalRegions][RSStatsMaxVerticalRegions];       ///< CS Raw Buffer
    BOOL                isParsed;                                                                ///< To indicate if
                                                                                                 ///  its already parsed
    ParsedRSStatsOutput parsedBuffer;                                                            ///< Parsed RS Buffer
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief Representation of a TintlessBG buffer that will be composed of rawbuffer and parsed buffer
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
struct TintlessBGStatsBuffer
{
    TintlessBG15StatsWithSaturationHwOutput rawBuffer[TintlessBGStatsMaxHorizontalRegions][TintlessBGStatsMaxVerticalRegions];
                                                           ///< TintlessBG Raw Buffer
                                                           ///  Max size of packed Tintless is
                                                           ///  TintlessBG15StatsWithSaturationHwOutput
                                                           ///  compared against TintlessBG15StatsHwOutput
                                                           ///  and TintlessBG15StatsYStatsEnableHwOutput.
    BOOL                                    isParsed;      ///< To indicate if its already parsed
    ParsedTintlessBGStatsOutput             parsedBuffer;  ///< Parsed TintlessBG Buffer
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief Representation of a iHist buffer that will be composed of rawbuffer and parsed buffer
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
struct IHistStatsBuffer
{
    IHistStats12HwOutput      rawBuffer;       ///< IHist Raw Buffer
    BOOL                      isParsed;        ///< To indicate if its already parsed
    ParsedIHistStatsOutput    parsedBuffer;    ///< Parsed BF Buffer
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief Representation of a titan17x stats parser.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class Titan17xSParser : public IStatsParser
{

public:
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetInstance
    ///
    /// @brief  Static method to allocate the IStatsParser Object.
    ///
    /// @return Pointer to the concrete IStatsParser object.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static Titan17xSParser* GetInstance();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ParseBFStats
    ///
    /// @brief  To get the BF Stats from the buffer.
    ///
    /// @param  pBuffer      Pointer to the buffer.
    /// @param  pBFConfig    Pointer to the configuration of the AF Stats.
    /// @param  pStats       Pointer to the stats buffer.
    ///
    /// @return CDKResultSuccess if successful.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CDKResult ParseBFStats(
        VOID*                          pBuffer,
        const BFStatsROIConfigType*    pBFConfig,
        ParsedBFStatsOutput**          ppStats);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ParseHDRBEStats
    ///
    /// @brief  To get the HDRBE Stats from the buffer.
    ///
    /// @param  pBuffer         Pointer to the buffer.
    /// @param  pHDRBEConfig    Pointer to the configuration of the BE Stats.
    /// @param  ppStats         Pointer to the stats buffer.
    ///
    /// @return CDKResultSuccess if successful.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CDKResult ParseHDRBEStats(
        VOID*                       pBuffer,
        const BGBEConfig*           pHDRBEConfig,
        ParsedHDRBEStatsOutput**    ppStats);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ParseAWBBGStats
    ///
    /// @brief  To get the AWBBG Stats from the buffer.
    ///
    /// @param  pBuffer         Pointer to the buffer.
    /// @param  pAWBBGConfig    Pointer to the configuration of the AWBBG Stats.
    /// @param  ppStats         Pointer to the stats buffer.
    ///
    /// @return CDKResultSuccess if successful.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CDKResult ParseAWBBGStats(
        VOID*                       pBuffer,
        const BGBEConfig*           pAWBBGConfig,
        ParsedAWBBGStatsOutput**    ppStats);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ParseTintlessBGStats
    ///
    /// @brief  To get the TintlessBG Stats from the buffer.
    ///
    /// @param  pBuffer           Pointer to the buffer.
    /// @param  pTintlessBGConfig Pointer to the configuration of the TintlessBG Stats.
    /// @param  ppStats           Pointer to the stats buffer.
    ///
    /// @return CDKResultSuccess if successful.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CDKResult ParseTintlessBGStats(
        VOID*                         pBuffer,
        const BGBEConfig*             pTintlessBGConfig,
        ParsedTintlessBGStatsOutput** ppStats);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ParseCSStats
    ///
    /// @brief  To get the CS Stats from the buffer.
    ///
    /// @param  pBuffer      Pointer to the buffer.
    /// @param  pCSConfig    Pointer to the configuration of the CS Stats.
    /// @param  ppStats      Pointer to the stats buffer.
    ///
    /// @return CDKResultSuccess if successful.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CDKResult ParseCSStats(
        VOID*                      pBuffer,
        const ISPCSStatsConfig*    pCSConfig,
        ParsedCSStatsOutput**      ppStats);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ParseRSStats
    ///
    /// @brief  To get the RS Stats from the buffer.
    ///
    /// @param  pBuffer      Pointer to the buffer.
    /// @param  pRSConfig    Pointer to the configuration of the RS Stats.
    /// @param  ppStats      Pointer to the stats buffer.
    ///
    /// @return CDKResultSuccess if successful.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CDKResult ParseRSStats(
        VOID*                      pBuffer,
        const ISPRSStatsConfig*    pRSConfig,
        ParsedRSStatsOutput**      ppStats);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ParseIHistStats
    ///
    /// @brief  To get the IHist Stats from the buffer.
    ///
    /// @param  pBuffer         Pointer to the buffer.
    /// @param  pIHistConfig    Pointer to the configuration of the IHist Stats.
    /// @param  ppStats         Pointer to the stats buffer.
    ///
    /// @return CDKResultSuccess if successful.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CDKResult ParseIHistStats(
        VOID*                         pBuffer,
        const ISPIHistStatsConfig*    pIHistConfig,
        ParsedIHistStatsOutput**      ppStats);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetMaxStatsBufferSize
    ///
    /// @brief  This gets the Max size for the Parsed and Unparsed stats.
    ///
    /// @param  statsType   Type of stats that is needed.
    ///
    /// @return size
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    INT32 GetMaxStatsBufferSize(
        ISPStatsType    statsType);

protected:
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ~Titan17xSParser
    ///
    /// @brief  Destructor
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual ~Titan17xSParser() = default;

private:
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// Titan17xSParser
    ///
    /// @brief  Constructor
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    Titan17xSParser() = default;

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// InternalParseHDRBEStats
    ///
    /// @brief  This is the internal function that parses the BE Stats
    ///
    /// @param  pUnparsedBuffer     Pointer to the unparsed buffer.
    /// @param  pAppliedHDRBEConfig Configuration that was used for stats.
    /// @param  pAppliedHDRBEConfig Configuration that was requested for stats.
    /// @param  pHDRBEStatsOutput   Pointer to the Parsed output stats.
    ///
    /// @return CDKResultSuccess if successful.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CDKResult InternalParseHDRBEStats(
        const VOID*                pUnparsedBuffer,
        const BGBEConfig*          pAppliedHDRBEConfig,
        const BGBEConfig*          pReferenceHDRBEConfig,
        ParsedHDRBEStatsOutput*    pHDRBEStatsOutput);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// InternalParseHDRBEStatsRegular
    ///
    /// @brief  This is the internal function that parses the regular BE stats
    ///
    /// @param  pUnparsedBuffer   Pointer to the unparsed buffer.
    /// @param  totalNumOfROIs    Total number of ROIs configured.
    /// @param  pHDRBEStatsOutput Pointer to the Parsed output stats.
    ///
    /// @return void
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID InternalParseHDRBEStatsRegular(
        const VOID*             pUnparsedBuffer,
        const UINT32            totalNumOfROIs,
        ParsedHDRBEStatsOutput* pHDRBEStatsOutput);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// InternalParseHDRBEStatsSaturationEnabled
    ///
    /// @brief  This is the internal function that parses the BE Stats with Saturation Stats from the buffer.
    ///
    /// @param  pUnparsedBuffer   Pointer to the unparsed buffer.
    /// @param  totalNumOfROIs    Total number of ROIs configured.
    /// @param  pHDRBEStatsOutput Pointer to the Parsed output stats.
    ///
    /// @return void
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID InternalParseHDRBEStatsSaturationEnabled(
        const VOID*             pUnparsedBuffer,
        const UINT32            totalNumOfROIs,
        ParsedHDRBEStatsOutput* pHDRBEStatsOutput);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// InternalParseHDRBEStatsYStatsEnabled
    ///
    /// @brief  This is the internal function that parses the BE Stats with Y Stats from the buffer.
    ///
    /// @param  pUnparsedBuffer   Pointer to the unparsed buffer.
    /// @param  totalNumOfROIs    Total number of ROIs configured.
    /// @param  pHDRBEStatsOutput Pointer to the Parsed output stats.
    ///
    /// @return void
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID InternalParseHDRBEStatsYStatsEnabled(
        const VOID*             pUnparsedBuffer,
        const UINT32            totalNumOfROIs,
        ParsedHDRBEStatsOutput* pHDRBEStatsOutput);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// InternalParseAWBBGStats
    ///
    /// @brief  This is the internal function that parses the AWBBG Stats.
    ///
    /// @param  pUnparsedBuffer       Pointer to the unparsed buffer.
    /// @param  pAppliedAWBBGConfig   Applied Stats config.
    /// @param  pReferenceAWBBGConfig Reference Stats config.
    /// @param  pAWBBGStatsOutput     Pointer to the Parsed output stats.
    ///
    /// @return void
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CDKResult InternalParseAWBBGStats(
        const VOID*                pUnparsedBuffer,
        const BGBEConfig*          pAppliedAWBBGConfig,
        const BGBEConfig*          pReferenceAWBBGConfig,
        ParsedAWBBGStatsOutput*    pAWBBGStatsOutput);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// InternalParseAWBBGStatsRegular
    ///
    /// @brief  This is the internal function that parses the regular AWBBG Stats.
    ///
    /// @param  pUnparsedBuffer   Pointer to the unparsed buffer.
    /// @param  totalNumOfROIs    Total number of ROIs configured.
    /// @param  pAWBBGStatsOutput Pointer to the Parsed output stats.
    ///
    /// @return void
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID InternalParseAWBBGStatsRegular(
        const VOID*             pUnparsedBuffer,
        const UINT32            totalNumOfROIs,
        ParsedAWBBGStatsOutput* pAWBBGStatsOutput);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// InternalParseAWBBGStatsSaturationEnabled
    ///
    /// @brief  This is the internal function that parses the AWBBG Stats with Saturation Stats from the buffer.
    ///
    /// @param  pUnparsedBuffer   Pointer to the unparsed buffer.
    /// @param  totalNumOfROIs    Total number of ROIs configured.
    /// @param  pAWBBGStatsOutput Pointer to the Parsed output stats.
    ///
    /// @return void
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID InternalParseAWBBGStatsSaturationEnabled(
        const VOID*             pUnparsedBuffer,
        const UINT32            totalNumOfROIs,
        ParsedAWBBGStatsOutput* pAWBBGStatsOutput);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// InternalParseAWBBGStatsYStatsEnabled
    ///
    /// @brief  This is the internal function that parses the AWBBG Stats with YStats from the raw buffer.
    ///
    /// @param  pUnparsedBuffer   Pointer to the unparsed buffer.
    /// @param  totalNumOfROIs    Total number of ROIs configured.
    /// @param  pAWBBGStatsOutput Pointer to the Parsed output stats.
    ///
    /// @return void
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID InternalParseAWBBGStatsYStatsEnabled(
        const VOID*             pUnparsedBuffer,
        const UINT32            totalNumOfROIs,
        ParsedAWBBGStatsOutput* pAWBBGStatsOutput);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// InternalParseTintlessBGStats
    ///
    /// @brief  This is the internal function that parses the TintlessBG Stats.
    ///
    /// @param  pUnparsedBuffer            Pointer to the unparsed buffer.
    /// @param  pAppliedTintlessBGConfig   Pointer to applied Stats config.
    /// @param  pReferenceTintlessBGConfig Pointer to reference Stats config.
    /// @param  pTintlessBGStatsOutput     Pointer to the Parsed output stats.
    ///
    /// @return void
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CDKResult InternalParseTintlessBGStats(
        const VOID*                  pUnparsedBuffer,
        const BGBEConfig*            pAppliedTintlessBGConfig,
        const BGBEConfig*            pReferenceTintlessBGConfig,
        ParsedTintlessBGStatsOutput* pTintlessBGStatsOutput);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// InternalParseTintlessBGStatsRegular
    ///
    /// @brief  This is the internal function that parses the regular TintlessBG Stats.
    ///
    /// @param  pUnparsedBuffer        Pointer to the unparsed buffer.
    /// @param  totalNumOfROIs         Total number of ROIs configured.
    /// @param  pTintlessBGStatsOutput Pointer to the Parsed output stats.
    ///
    /// @return void
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID InternalParseTintlessBGStatsRegular(
        const VOID*                  pUnparsedBuffer,
        const UINT32                 totalNumOfROIs,
        ParsedTintlessBGStatsOutput* pTintlessBGStatsOutput);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// InternalParseTintlessBGStatsSaturationEnabled
    ///
    /// @brief  This is the internal function that parses the TintlessBG Stats with Saturation Stats from the buffer.
    ///
    /// @param  pUnparsedBuffer        Pointer to the unparsed buffer.
    /// @param  totalNumOfROIs         Total number of ROIs configured.
    /// @param  pTintlessBGStatsOutput Pointer to the Parsed output stats.
    ///
    /// @return void
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID InternalParseTintlessBGStatsSaturationEnabled(
        const VOID*                  pUnparsedBuffer,
        const UINT32                 totalNumOfROIs,
        ParsedTintlessBGStatsOutput* pTintlessBGStatsOutput);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// InternalParseTintlessBGStatsYStatsEnabled
    ///
    /// @brief  This is the internal function that parses the TintlessBG Stats with YStats from the raw buffer.
    ///
    /// @param  pUnparsedBuffer        Pointer to the unparsed buffer.
    /// @param  totalNumOfROIs         Total number of ROIs configured.
    /// @param  pTintlessBGStatsOutput Pointer to the Parsed output stats.
    ///
    /// @return void
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID InternalParseTintlessBGStatsYStatsEnabled(
        const VOID*                  pUnparsedBuffer,
        const UINT32                 totalNumOfROIs,
        ParsedTintlessBGStatsOutput* pTintlessBGStatsOutput);


    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// InternalParseCSStats
    ///
    /// @brief  This is the internal function that parses the CS Stats from the raw buffer.
    ///
    /// @param  pUnparsedBuffer         Pointer to the unparsed buffer.
    /// @param  pAppliedCSConfig        Pointer to the applied stats configutaion.
    /// @param  pReferenceCSConfig      Pointer to the reference stats configutaion.
    /// @param  pParsedCSStatsOutput    Pointer to the Parsed output stats.
    ///
    /// @return CDKResultSuccess if successful.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CDKResult InternalParseCSStats(
        const VOID*                pUnparsedBuffer,
        const ISPCSStatsConfig*    pAppliedCSConfig,
        const ISPCSStatsConfig*    pReferenceCSConfig,
        ParsedCSStatsOutput*       pParsedCSStatsOutput);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// InternalParseRSStats
    ///
    /// @brief  This is the internal function that parses the RS Stats from the raw buffer.
    ///
    /// @param  pUnparsedBuffer         Pointer to the unparsed buffer.
    /// @param  pAppliedRSConfig        Pointer to the applied stats configutaion.
    /// @param  pReferenceRSConfig      Pointer to the reference stats configutaion.
    /// @param  pParsedRSStatsOutput    Pointer to the Parsed output stats.
    ///
    /// @return CDKResultSuccess if successful.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CDKResult InternalParseRSStats(
        const VOID*                pUnparsedBuffer,
        const ISPRSStatsConfig*    pAppliedRSConfig,
        const ISPRSStatsConfig*    pReferenceRSConfig,
        ParsedRSStatsOutput*       pParsedRSStatsOutput);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// InternalParseIHistStats
    ///
    /// @brief  This is the internal function that parses the IHist Stats from the raw buffer.
    ///
    /// @param  pUnparsedBuffer          Pointer to the unparsed buffer.
    /// @param  pAppliedIHistConfig      Pointer to the applied stats configutaion.
    /// @param  pReferenceIHistConfig    Pointer to the reference stats configutaion.
    /// @param  pParsedIHistOutput       Pointer to the Parsed output stats.
    ///
    /// @return CDKResultSuccess if successful.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CDKResult InternalParseIHistStats(
        const VOID*                   pUnparsedBuffer,
        const ISPIHistStatsConfig*    pAppliedIHistConfig,
        const ISPIHistStatsConfig*    pReferenceIHistConfig,
        ParsedIHistStatsOutput*       pParsedIHistOutput);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// InternalParseBFStats
    ///
    /// @brief  This is the internal function that parses the AF Stats from the raw buffer.
    ///
    /// @param  pUnparsedBuffer       Pointer to the unparsed buffer.
    /// @param  pAppliedBFConfig      Pointer to the applied stats configutaion.
    /// @param  pReferenceBFConfig    Pointer to the reference stats configutaion.
    /// @param  pBFStatsOutput        Pointer to the Parsed output stats.
    ///
    /// @return CDKResultSuccess if successful.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CDKResult InternalParseBFStats(
        const VOID*                    pUnparsedBuffer,
        const BFStatsROIConfigType*    pAppliedBFConfig,
        const BFStatsROIConfigType*    pReferenceBFConfig,
        ParsedBFStatsOutput*           pBFStatsOutput);

    Titan17xSParser(const Titan17xSParser&) = delete;             ///< Disallow the copy constructor.

    Titan17xSParser& operator=(const Titan17xSParser&) = delete;  ///< Disallow assignment operator.
};

CAMX_NAMESPACE_END

#endif // CAMXTITAN17XSPARSER_H
