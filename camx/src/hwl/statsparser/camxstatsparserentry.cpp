////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2017 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  camxstatsparserentry.cpp
/// @brief Entry to CAMX stats parser.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#include "camxcstatsparser.h"
#include "camxtitan17xsparser.h"

using namespace CamX;
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CreateStatsParser
///
/// @brief  This method creates the derived instance of the interface.
///
/// @param  cameraFamily   Allows parser to know for which camera family it needs to parse.
/// @param  ppStatsParser  Pointer to CHIStatsParser instance.
///
/// @return CDKResultSuccess upon success.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult CreateStatsParser(
    CHICameraFamily     cameraFamily,
    CHIStatsParser**    ppStatsParser)
{
    CDKResult           result         = CDKResultSuccess;
    CHIStatsParser*     pStatsParser   = NULL;
    void*               pParserContext = NULL;

    pStatsParser = CAMX_NEW CHIStatsParser;

    if (NULL == pStatsParser)
    {
        result = CDKResultENoMemory;
    }

    if (CDKResultSuccess == result)
    {
        if (CameraFamilyTitan == cameraFamily)
        {
            pParserContext = static_cast<void*>(Titan17xSParser::GetInstance());
        }
    }

    if (NULL == pStatsParser)
    {
        result = CDKResultENoMemory;
    }
    else
    {
        pStatsParser->StatsParserDestroy    = StatsParserDestroy;
        pStatsParser->GetMaxStatsBufferSize = GetMaxStatsBufferSize;
        pStatsParser->ParseBFStats          = ParseBFStats;
        pStatsParser->ParseHDRBEStats       = ParseHDRBEStats;
        pStatsParser->ParseAWBBGStats       = ParseAWBBGStats;
        pStatsParser->ParseCSStats          = ParseCSStats;
        pStatsParser->ParseRSStats          = ParseRSStats;
        pStatsParser->ParseIHistStats       = ParseIHistStats;
        pStatsParser->ParseTintlessBGStats  = ParseTintlessBGStats;
        pStatsParser->pContext              = pParserContext;
        *ppStatsParser                      = static_cast<CHIStatsParser*>(pStatsParser);
        result                              = CDKResultSuccess;
    }
    return result;
}