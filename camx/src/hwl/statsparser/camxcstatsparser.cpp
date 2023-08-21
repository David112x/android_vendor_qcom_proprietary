////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2017 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  camxcstatsparser.cpp
/// @brief Entry to CAMX stats parser. Implements C style Parser interface.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#include "chistatsparser.h"

#include "camxmem.h"
#include "camxtitan17xsparser.h"

CAMX_NAMESPACE_BEGIN

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ParseBFStats
///
/// @brief  To get the BF Stats from the buffer.
///
/// @param  pStatsParser Pointer to CHIStatsParser instance.
/// @param  pBuffer      Pointer to the buffer.
/// @param  pBFConfig    Pointer to the configuration of the AF Stats.
/// @param  pStats       Pointer to the stats buffer.
///
/// @return CDKResultSuccess if successful.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ParseBFStats(
    const CHIStatsParser*          pStatsParser,
    VOID*                          pBuffer,
    const BFStatsROIConfigType*    pBFConfig,
    ParsedBFStatsOutput**          ppStats)
{
    CDKResult        result        = CDKResultEInvalidArg;
    IStatsParser*    pIStatsParser = NULL;

    if ((NULL != pStatsParser) &&
        (NULL != pBuffer)      &&
        (NULL != pBFConfig)    &&
        (NULL != ppStats))
    {
        pIStatsParser = reinterpret_cast<IStatsParser*>(pStatsParser->pContext);
        result        = pIStatsParser->ParseBFStats(pBuffer, pBFConfig, ppStats);
    }

    return result;
}

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
    ParsedHDRBEStatsOutput**    ppStats)
{
    CDKResult        result        = CDKResultEInvalidArg;
    IStatsParser*    pIStatsParser = NULL;

    if ((NULL != pStatsParser) &&
        (NULL != pBuffer)      &&
        (NULL != pHDRBEConfig) &&
        (NULL != ppStats))
    {
        pIStatsParser = reinterpret_cast<IStatsParser*>(pStatsParser->pContext);
        result        = pIStatsParser->ParseHDRBEStats(pBuffer, pHDRBEConfig, ppStats);
    }

    return result;
}

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
    ParsedAWBBGStatsOutput**    ppStats)
{
    CDKResult        result        = CDKResultEInvalidArg;
    IStatsParser*    pIStatsParser = NULL;

    if ((NULL != pStatsParser) &&
        (NULL != pBuffer)      &&
        (NULL != pAWBBGConfig) &&
        (NULL != ppStats))
    {
        pIStatsParser = reinterpret_cast<IStatsParser*>(pStatsParser->pContext);
        result        = pIStatsParser->ParseAWBBGStats(pBuffer, pAWBBGConfig, ppStats);
    }

    return result;
}

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
    ParsedTintlessBGStatsOutput**    ppStats)
{
    CDKResult     result        = CDKResultEInvalidArg;
    IStatsParser* pIStatsParser = NULL;

    if ((NULL != pStatsParser) &&
        (NULL != pBuffer) &&
        (NULL != pTintlessBGConfig) &&
        (NULL != ppStats))
    {
        pIStatsParser = reinterpret_cast<IStatsParser*>(pStatsParser->pContext);
        result = pIStatsParser->ParseTintlessBGStats(pBuffer, pTintlessBGConfig, ppStats);
    }

    return result;
}

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
    ParsedCSStatsOutput**      ppStats)
{
    CDKResult        result        = CDKResultEInvalidArg;
    IStatsParser*    pIStatsParser = NULL;

    if ((NULL != pStatsParser) &&
        (NULL != pBuffer) &&
        (NULL != pCSConfig) &&
        (NULL != ppStats))
    {
        pIStatsParser = reinterpret_cast<IStatsParser*>(pStatsParser->pContext);
        result = pIStatsParser->ParseCSStats(pBuffer, pCSConfig, ppStats);
    }

    return result;
}

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
    ParsedRSStatsOutput**      ppStats)
{
    CDKResult        result        = CDKResultEInvalidArg;
    IStatsParser*    pIStatsParser = NULL;

    if ((NULL != pStatsParser) &&
        (NULL != pBuffer) &&
        (NULL != pRSConfig) &&
        (NULL != ppStats))
    {
        pIStatsParser = reinterpret_cast<IStatsParser*>(pStatsParser->pContext);
        result = pIStatsParser->ParseRSStats(pBuffer, pRSConfig, ppStats);
    }

    return result;
}

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
    ParsedIHistStatsOutput**      ppStats)
{
    CDKResult        result        = CDKResultEInvalidArg;
    IStatsParser*    pIStatsParser = NULL;

    if ((NULL != pStatsParser) &&
        (NULL != pBuffer) &&
        (NULL != pIHistConfig) &&
        (NULL != ppStats))
    {
        pIStatsParser = reinterpret_cast<IStatsParser*>(pStatsParser->pContext);
        result = pIStatsParser->ParseIHistStats(pBuffer, pIHistConfig, ppStats);
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// GetMaxStatsBufferSize
///
/// @brief  This gets the Max size for the Parsed and Unparsed stats.
///
/// @param  pStatsParser Pointer to CHIStatsParser instance.
/// @param  statsType    Type of stats that is needed.
///
/// @return size
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
INT32 GetMaxStatsBufferSize(
    const CHIStatsParser*    pStatsParser,
    ISPStatsType             statsType)
{
    INT32            maxSize       = 0;
    IStatsParser*    pIStatsParser = NULL;

    if (NULL != pStatsParser)
    {
        pIStatsParser = reinterpret_cast<IStatsParser*>(pStatsParser->pContext);
        maxSize       = pIStatsParser->GetMaxStatsBufferSize(statsType);
    }

    return maxSize;
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// StatsParserDestroy
///
/// @brief  This method destroys the derived instance of the interface.
///
/// @param  pStatsParser Pointer to CHIStatsParser instance.
///
/// @return None
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID StatsParserDestroy (
    const CHIStatsParser* pStatsParser)
{
    if (NULL != pStatsParser)
    {
        CAMX_DELETE pStatsParser;
        pStatsParser = NULL;
    }
}

CAMX_NAMESPACE_END