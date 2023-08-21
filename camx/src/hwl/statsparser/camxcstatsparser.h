////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2017 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  camxcstatsparser.h
/// @brief Implements C style Parser interface.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#ifndef CAMXCSTATSPARSER_H
#define CAMXCSTATSPARSER_H

#include "chistatsparser.h"

#include "camxmem.h"

CAMX_NAMESPACE_BEGIN

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ParseBFStats
///
/// @brief  To get the BF Stats from the buffer.
///
/// @param  pStatsParser Pointer to CHIStatsParser instance.
/// @param  pBuffer   Pointer to the buffer.
/// @param  pBFConfig Pointer to the configuration of the AF Stats.
/// @param  pStats    Pointer to the stats buffer.
///
/// @return CDKResultSuccess if successful.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ParseBFStats(
    const CHIStatsParser*          pStatsParser,
    VOID*                          pBuffer,
    const BFStatsROIConfigType*    pBFConfig,
    ParsedBFStatsOutput**          ppStats);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ParseHDRBEStats
///
/// @brief  To get the HDRBE Stats from the buffer.
///
/// @param  pStatsParser Pointer to CHIStatsParser instance.
/// @param  pBuffer      Pointer to the buffer.
/// @param  pHDRBEConfig Pointer to the configuration of the BE Stats.
/// @param  ppStats      Pointer to the stats buffer.
///
/// @return CDKResultSuccess if successful.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ParseHDRBEStats(
    const CHIStatsParser*       pStatsParser,
    VOID*                       pBuffer,
    const BGBEConfig*           pHDRBEConfig,
    ParsedHDRBEStatsOutput**    ppStats);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ParseAWBBGStats
///
/// @brief  To get the AWBBG Stats from the buffer.
///
/// @param  pStatsParser    Pointer to CHIStatsParser instance.
/// @param  pBuffer         Pointer to the buffer.
/// @param  pAWBBGConfig    Pointer to the configuration of the AWBBG Stats.
/// @param  ppStats         Pointer to the stats buffer.
///
/// @return CDKResultSuccess if successful.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ParseAWBBGStats(
    const CHIStatsParser*       pStatsParser,
    VOID*                       pBuffer,
    const BGBEConfig*           pAWBBGConfig,
    ParsedAWBBGStatsOutput**    ppStats);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ParseTintlessBGStats
///
/// @brief  To get the TintlessBG Stats from the buffer.
///
/// @param  pStatsParser         Pointer to CHIStatsParser instance.
/// @param  pBuffer              Pointer to the buffer.
/// @param  pTintlessBGConfig    Pointer to the configuration of the TintlessBG Stats.
/// @param  ppStats              Pointer to the stats buffer.
///
/// @return CDKResultSuccess if successful.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ParseTintlessBGStats(
    const CHIStatsParser*            pStatsParser,
    VOID*                            pBuffer,
    const BGBEConfig*                pTintlessBGConfig,
    ParsedTintlessBGStatsOutput**    ppStats);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ParseCSStats
///
/// @brief  To get the CS Stats from the buffer.
///
/// @param  pStatsParser    Pointer to CHIStatsParser instance.
/// @param  pBuffer         Pointer to the buffer.
/// @param  pCSConfig       Pointer to the configuration of the CS Stats.
/// @param  ppStats         Pointer to the stats buffer.
///
/// @return CDKResultSuccess if successful.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ParseCSStats(
    const CHIStatsParser*      pStatsParser,
    VOID*                      pBuffer,
    const ISPCSStatsConfig*    pCSConfig,
    ParsedCSStatsOutput**      ppStats);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ParseRSStats
///
/// @brief  To get the RS Stats from the buffer.
///
/// @param  pStatsParser    Pointer to CHIStatsParser instance.
/// @param  pBuffer         Pointer to the buffer.
/// @param  pRSConfig       Pointer to the configuration of the RS Stats.
/// @param  ppStats         Pointer to the stats buffer.
///
/// @return CDKResultSuccess if successful.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ParseRSStats(
    const CHIStatsParser*      pStatsParser,
    VOID*                      pBuffer,
    const ISPRSStatsConfig*    pRSConfig,
    ParsedRSStatsOutput**      ppStats);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ParseIHistStats
///
/// @brief  To get the IHist Stats from the buffer.
///
/// @param  pStatsParser    Pointer to CHIStatsParser instance.
/// @param  pBuffer         Pointer to the buffer.
/// @param  pIHistConfig    Pointer to the configuration of the IHist Stats.
/// @param  ppStats         Pointer to the stats buffer.
///
/// @return CDKResultSuccess if successful.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ParseIHistStats(
    const CHIStatsParser*         pStatsParser,
    VOID*                         pBuffer,
    const ISPIHistStatsConfig*    pIHistConfig,
    ParsedIHistStatsOutput**      ppStats);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// GetMaxStatsBufferSize
///
/// @brief  This gets the Max size for the Parsed and Unparsed stats.
///
/// @param  pStatsParser Pointer to CHIStatsParser instance.
/// @param  statsType   Type of stats that is needed.
///
/// @return size
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
INT32 GetMaxStatsBufferSize(
    const CHIStatsParser* pStatsParser,
    ISPStatsType          statsType);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// StatsParserDestroy
///
/// @brief  This method destroys the derived instance of the interface.
///
/// @param  pStatsParser Pointer to CHIStatsParser instance.
///
/// @return None
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID StatsParserDestroy(
    const CHIStatsParser* pStatsParser);

CAMX_NAMESPACE_END

#endif // CAMXCSTATSPARSER_H