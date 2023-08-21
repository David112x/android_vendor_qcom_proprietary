////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2017 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  camxtitan17xsparser.cpp
///
/// @brief Titan17xstatsparser implementation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#include "camxmem.h"
#include "camxtitan17xsparser.h"
#include "chiispstatsdefs.h"

CAMX_NAMESPACE_BEGIN

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Titan17xSParser::GetInstance
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
Titan17xSParser* Titan17xSParser::GetInstance()
{
    static Titan17xSParser s_pInstance;
    return &s_pInstance;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Titan17xSParser::GetMaxStatsBufferSize
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
INT32 Titan17xSParser::GetMaxStatsBufferSize(ISPStatsType    statsType)
{
    INT32 size = 0;

    switch (statsType)
    {
        case ISPStatsTypeBF:
            size = sizeof(BFStatsBuffer);
            break;
        case ISPStatsTypeHDRBE:
            size = sizeof(HDRBEStatsBuffer);
            break;
        case ISPStatsTypeAWBBG:
            size = sizeof(AWBBGStatsBuffer);
            break;
        case ISPStatsTypeRS:
            size = sizeof(RSStatsBuffer);
            break;
        case ISPStatsTypeCS:
            size = sizeof(CSStatsBuffer);
            break;
        case ISPStatsTypeIHist:
            size = sizeof(IHistStatsBuffer);
            break;
        case ISPStatsTypeTintlessBG:
            size = sizeof(TintlessBGStatsBuffer);
            break;
        default:
            CAMX_LOG_ERROR(CamxLogGroupUtils, "Not supported stats type %d", statsType);
            break;
    }

    return size;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Titan17xSParser::ParseBFStats
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult Titan17xSParser::ParseBFStats(
    VOID*                          pBuffer,
    const BFStatsROIConfigType*    pBFConfig,
    ParsedBFStatsOutput**          ppStats)
{
    CDKResult            result = CDKResultSuccess;
    BFStatsBuffer*       pBFStatsBuffer = NULL;

    if ((NULL == pBuffer) || (NULL == pBFConfig) || (NULL == ppStats))
    {
        result = CDKResultEInvalidArg;
    }

    if (CDKResultSuccess == result)
    {
        pBFStatsBuffer = static_cast<BFStatsBuffer*>(pBuffer);

        if (TRUE != pBFStatsBuffer->isParsed)
        {
            result = InternalParseBFStats(pBuffer,
                                          pBFConfig,
                                          pBFConfig,
                                          &pBFStatsBuffer->parsedBuffer);
        }

        if (CDKResultSuccess == result)
        {
            pBFStatsBuffer->isParsed = TRUE;
            *ppStats                 = &pBFStatsBuffer->parsedBuffer;
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Titan17xSParser::ParseAWBBGStats
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult Titan17xSParser::ParseAWBBGStats(
    VOID*                       pBuffer,
    const BGBEConfig*           pAWBBGConfig,
    ParsedAWBBGStatsOutput**    ppStats)
{
    CDKResult         result            = CDKResultSuccess;
    AWBBGStatsBuffer* pAWBBGStatsBuffer = NULL;

    if ((NULL == pBuffer) || (NULL == pAWBBGConfig) || (NULL == ppStats))
    {
        result = CDKResultEInvalidArg;
    }

    if (CDKResultSuccess == result)
    {
        pAWBBGStatsBuffer = static_cast<AWBBGStatsBuffer*>(pBuffer);

        if (TRUE != pAWBBGStatsBuffer->isParsed)
        {
            result = InternalParseAWBBGStats(pBuffer, pAWBBGConfig, pAWBBGConfig, &pAWBBGStatsBuffer->parsedBuffer);
        }

        if (CDKResultSuccess == result)
        {
            *ppStats                    = &pAWBBGStatsBuffer->parsedBuffer;
            pAWBBGStatsBuffer->isParsed = TRUE;
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Titan17xSParser::ParseTintlessBGStats
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult Titan17xSParser::ParseTintlessBGStats(
    VOID*                            pBuffer,
    const BGBEConfig*                pTintlessBGConfig,
    ParsedTintlessBGStatsOutput**    ppStats)
{
    CDKResult              result                = CDKResultSuccess;
    TintlessBGStatsBuffer* pTinlessBGStatsBuffer = NULL;

    if ((NULL == pBuffer) || (NULL == pTintlessBGConfig) || (NULL == ppStats))
    {
        result = CDKResultEInvalidArg;
    }

    if (CDKResultSuccess == result)
    {
        pTinlessBGStatsBuffer = static_cast<TintlessBGStatsBuffer*>(pBuffer);

        if (TRUE != pTinlessBGStatsBuffer->isParsed)
        {
            result = InternalParseTintlessBGStats(pBuffer,
                                                  pTintlessBGConfig,
                                                  pTintlessBGConfig,
                                                  &pTinlessBGStatsBuffer->parsedBuffer);
        }

        if (CDKResultSuccess == result)
        {
            *ppStats = &pTinlessBGStatsBuffer->parsedBuffer;
            pTinlessBGStatsBuffer->isParsed = TRUE;
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Titan17xSParser::ParseHDRBEStats
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult Titan17xSParser::ParseHDRBEStats(
    VOID*                       pBuffer,
    const BGBEConfig*           pHDRBEConfig,
    ParsedHDRBEStatsOutput**    ppStats)
{
    CDKResult         result            = CDKResultSuccess;
    HDRBEStatsBuffer* pHDRBEStatsBuffer = NULL;

    if ((NULL == pBuffer) || (NULL == pHDRBEConfig) || (NULL == ppStats))
    {
        result = CDKResultEInvalidArg;
    }

    if (CDKResultSuccess == result)
    {
        pHDRBEStatsBuffer = static_cast<HDRBEStatsBuffer*>(pBuffer);

        if (TRUE != pHDRBEStatsBuffer->isParsed)
        {
            result = InternalParseHDRBEStats(pBuffer, pHDRBEConfig, pHDRBEConfig, &pHDRBEStatsBuffer->parsedBuffer);
        }

        if (CDKResultSuccess == result)
        {
            *ppStats                    = &pHDRBEStatsBuffer->parsedBuffer;
            pHDRBEStatsBuffer->isParsed = TRUE;
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Titan17xSParser::ParseCSStats
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult Titan17xSParser::ParseCSStats(
    VOID*                      pBuffer,
    const ISPCSStatsConfig*    pCSConfig,
    ParsedCSStatsOutput**      ppStats)
{
    CDKResult         result         = CDKResultSuccess;
    CSStatsBuffer*    pCSStatsBuffer = NULL;

    if ((NULL == pBuffer) || (NULL == pCSConfig) || (NULL == ppStats))
    {
        result = CDKResultEInvalidArg;
    }

    if (CDKResultSuccess == result)
    {
        pCSStatsBuffer = static_cast<CSStatsBuffer*>(pBuffer);

        if (TRUE != pCSStatsBuffer->isParsed)
        {
            result = InternalParseCSStats(pBuffer, pCSConfig, pCSConfig, &pCSStatsBuffer->parsedBuffer);
        }

        if (CDKResultSuccess == result)
        {
            *ppStats = &pCSStatsBuffer->parsedBuffer;
            pCSStatsBuffer->isParsed = TRUE;
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Titan17xSParser::ParseRSStats
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult Titan17xSParser::ParseRSStats(
    VOID*                      pBuffer,
    const ISPRSStatsConfig*    pRSConfig,
    ParsedRSStatsOutput**      ppStats)
{
    CDKResult         result = CDKResultSuccess;
    RSStatsBuffer*    pRSStatsBuffer = NULL;

    if ((NULL == pBuffer) || (NULL == pRSConfig) || (NULL == ppStats))
    {
        result = CDKResultEInvalidArg;
    }

    if (CDKResultSuccess == result)
    {
        pRSStatsBuffer = static_cast<RSStatsBuffer*>(pBuffer);

        if (TRUE != pRSStatsBuffer->isParsed)
        {
            result = InternalParseRSStats(pBuffer, pRSConfig, pRSConfig, &pRSStatsBuffer->parsedBuffer);
        }

        if (CDKResultSuccess == result)
        {
            *ppStats = &pRSStatsBuffer->parsedBuffer;
            pRSStatsBuffer->isParsed = TRUE;
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Titan17xSParser::ParseIHistStats
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult Titan17xSParser::ParseIHistStats(
    VOID*                         pBuffer,
    const ISPIHistStatsConfig*    pIHistConfig,
    ParsedIHistStatsOutput**      ppStats)
{
    CDKResult         result = CDKResultSuccess;
    IHistStatsBuffer* pIHistStatsBuffer = NULL;

    if ((NULL == pBuffer) || (NULL == pIHistConfig) || (NULL == ppStats))
    {
        result = CDKResultEInvalidArg;
    }

    if (CDKResultSuccess == result)
    {
        pIHistStatsBuffer = static_cast<IHistStatsBuffer*>(pBuffer);

        if (TRUE != pIHistStatsBuffer->isParsed)
        {
            result = InternalParseIHistStats(pBuffer, pIHistConfig, pIHistConfig, &pIHistStatsBuffer->parsedBuffer);
        }

        if (CDKResultSuccess == result)
        {
            *ppStats = &pIHistStatsBuffer->parsedBuffer;
            pIHistStatsBuffer->isParsed = TRUE;
        }
    }

    return result;
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Titan17xSParser::InternalParseBFStats
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult Titan17xSParser::InternalParseBFStats(
    const VOID*                    pUnparsedBuffer,
    const BFStatsROIConfigType*    pAppliedBFConfig,
    const BFStatsROIConfigType*    pReferenceBFConfig,
    ParsedBFStatsOutput*           pBFStatsOutput)
{
    CAMX_UNREFERENCED_PARAM(pReferenceBFConfig);

    CDKResult                   result        = CDKResultSuccess;
    const BFStats23HwOutput*    pBFStatsHwOut = NULL;
    const UINT64*               pWord64Bit    = NULL;

    if (pAppliedBFConfig->numBFStatsROIDimension >= BFMaxROIRegions)
    {
        CAMX_LOG_ERROR(CamxLogGroupUtils, "BF Stats num of ROI regions > MAX_ROI_REGIONS");
        result = CamxResultEFailed;
    }

    if (CDKResultSuccess == result)
    {
        pWord64Bit = static_cast<const UINT64*>(pUnparsedBuffer);

        // The first 64-bit word is the frame-tag for this region stats
        pBFStatsOutput->frameTag = pWord64Bit[0];

        // Repetitive Bayer Focus stats are followed after the first 64-bit word
        pBFStatsHwOut = reinterpret_cast<const BFStats23HwOutput*>(&pWord64Bit[1]);

        /// @note It is assumed that if any output port is enabled, all the input ports are active i.e. all inputs are required
        ///       to generate any (and all) outputs: Note that BF stats output order is based on "End Pixel Order"
        ///       (i.e. the botton right corner of ROI). To recover back to the original
        ///       3A stats algorithm requested order, we retrieve back region ID for each ROI that
        ///       should follow the same ROI configuration array index.
        const UINT32 totalROIs = pAppliedBFConfig->numBFStatsROIDimension;
        for (UINT32 index = 0; index < totalROIs; index++)
        {
            pBFStatsOutput->regionID[index] = pBFStatsHwOut[index].regionID;

            // regionIDAsIndex
            UINT32 regionIDAsIndex          = pBFStatsOutput->regionID[index];

            CAMX_ASSERT(regionIDAsIndex < totalROIs);

            // Use requestedROIIndex to recover the requested order of ROI configuration.
            pBFStatsOutput->horizontal1Num[regionIDAsIndex]       = pBFStatsHwOut[index].H1Count;
            pBFStatsOutput->horizontal1Sum[regionIDAsIndex]       = pBFStatsHwOut[index].H1Sum;
            pBFStatsOutput->horizontal1Sharpness[regionIDAsIndex] = pBFStatsHwOut[index].H1Sharpness;
            pBFStatsOutput->region[regionIDAsIndex]               = pBFStatsHwOut[index].selector;
            pBFStatsOutput->verticalNum[regionIDAsIndex]          = pBFStatsHwOut[index].VCount;
            pBFStatsOutput->verticalSum[regionIDAsIndex]          = pBFStatsHwOut[index].VSum;
            pBFStatsOutput->verticalSharpness[regionIDAsIndex]    = pBFStatsHwOut[index].VSharpness;
        }

        pBFStatsOutput->numOfROIRegions = totalROIs;
    }
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Titan17xSParser::InternalParseCSStats
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult Titan17xSParser::InternalParseCSStats(
    const VOID*                pUnparsedBuffer,
    const ISPCSStatsConfig*    pAppliedCSConfig,
    const ISPCSStatsConfig*    pReferenceCSConfig,
    ParsedCSStatsOutput*       pCSStatsOutput)
{
    CDKResult    result     = CDKResultSuccess;
    UINT32       regionHNum = 0;
    UINT32       regionVNum = 0;

    CAMX_UNREFERENCED_PARAM(pReferenceCSConfig);

    // Get horizontal and vertical regions
    regionHNum = pAppliedCSConfig->CSConfig.statsHNum;
    regionVNum = pAppliedCSConfig->CSConfig.statsVNum;

    // CS sum buffer holds array of 16 bit data
    const UINT16* pCSData = static_cast<const UINT16 *>(pUnparsedBuffer);

    for (UINT32 j = 0; j < regionVNum; j++)
    {
        for (UINT32 i = 0; i < regionHNum; i++)
        {
            pCSStatsOutput->columnSum[i][j] = static_cast<UINT32>(*pCSData++);
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Titan17xSParser::InternalParseRSStats
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult Titan17xSParser::InternalParseRSStats(
    const VOID*                pUnparsedBuffer,
    const ISPRSStatsConfig*    pAppliedRSConfig,
    const ISPRSStatsConfig*    pReferenceRSConfig,
    ParsedRSStatsOutput*       pParsedRSStatsOutput)
{
    CDKResult    result = CamxResultSuccess;
    UINT32       regionHNum = 0;
    UINT32       regionVNum = 0;

    CAMX_UNREFERENCED_PARAM(pReferenceRSConfig);

    // Get horizontal and vertical regions
    regionHNum = pAppliedRSConfig->RSConfig.statsHNum;
    regionVNum = pAppliedRSConfig->RSConfig.statsVNum;

    // RS sum buffer holds row sum - each of 16 bits
    const UINT16* pRSData = static_cast<const UINT16 *>(pUnparsedBuffer);

    for (UINT32 j = 0; j < regionVNum; j++)
    {
        for (UINT32 i = 0; i < regionHNum; i++)
        {
            pParsedRSStatsOutput->rowSum[i][j] = static_cast<UINT32>(*pRSData++);
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// InternalParseIHistStats
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult Titan17xSParser::InternalParseIHistStats(
    const VOID*                   pUnparsedBuffer,
    const ISPIHistStatsConfig*    pAppliedIHistConfig,
    const ISPIHistStatsConfig*    pReferenceIHistConfig,
    ParsedIHistStatsOutput*       pParsedIHistOutput)
{
    CDKResult                   result       = CDKResultSuccess;
    const IHistStats12HwOutput* pISPIHist    = static_cast<const IHistStats12HwOutput*>(pUnparsedBuffer);

    CAMX_UNREFERENCED_PARAM(pAppliedIHistConfig);
    CAMX_UNREFERENCED_PARAM(pReferenceIHistConfig);

    /// @todo (CAMX-1866) IHist: handle dual IFE mode parsing.
    Utils::Memcpy(&pParsedIHistOutput->imageHistogram, pISPIHist, sizeof(IHistStatsOutput));

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Titan17xSParser::InternalParseBEStats
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult Titan17xSParser::InternalParseHDRBEStats(
    const VOID*                pUnparsedBuffer,
    const BGBEConfig*          pAppliedHDRBEConfig,
    const BGBEConfig*          pReferenceHDRBEConfig,
    ParsedHDRBEStatsOutput*    pHDRBEStatsOutput)
{
    CDKResult                   result = CDKResultSuccess;

    CAMX_UNREFERENCED_PARAM(pReferenceHDRBEConfig);

    if ((0 == pAppliedHDRBEConfig->horizontalNum) || (0 == pAppliedHDRBEConfig->verticalNum))
    {
        CAMX_LOG_ERROR(CamxLogGroupUtils, "Invalid total number of ROIs");
        result = CDKResultEInvalidArg;
    }

    if (CDKResultSuccess == result)
    {
        const UINT32 totalNumOfROIs = pAppliedHDRBEConfig->horizontalNum * pAppliedHDRBEConfig->verticalNum;

        switch (pAppliedHDRBEConfig->outputMode)
        {
            case BGBESaturationEnabled:
                InternalParseHDRBEStatsSaturationEnabled(pUnparsedBuffer, totalNumOfROIs, pHDRBEStatsOutput);
                break;
            case BGBEYStatsEnabled:
                InternalParseHDRBEStatsYStatsEnabled(pUnparsedBuffer, totalNumOfROIs, pHDRBEStatsOutput);
                break;
            case BGBERegular:
            default:
                InternalParseHDRBEStatsRegular(pUnparsedBuffer, totalNumOfROIs, pHDRBEStatsOutput);
                break;
        }

    }
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Titan17xSParser::InternalParseHDRBEStatsRegular
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID Titan17xSParser::InternalParseHDRBEStatsRegular(
    const VOID*             pUnparsedBuffer,
    const UINT32            totalNumOfROIs,
    ParsedHDRBEStatsOutput* pHDRBEStatsOutput)
{
    const HDRBE15StatsHwOutput* pHDRBEStatsHwOut = NULL;

    pHDRBEStatsHwOut = static_cast<const HDRBE15StatsHwOutput*>(pUnparsedBuffer);

    pHDRBEStatsOutput->numROIs = totalNumOfROIs;
    pHDRBEStatsOutput->flags.hasSatInfo = 0;
    pHDRBEStatsOutput->flags.usesY = 0;

    Utils::Memcpy(pHDRBEStatsOutput->GetChannelDataArray(), pHDRBEStatsHwOut,
                  totalNumOfROIs * sizeof(HDRBE15StatsHwOutput));
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Titan17xSParser::InternalParseHDRBEStatsSaturationEnabled
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID Titan17xSParser::InternalParseHDRBEStatsSaturationEnabled(
    const VOID*             pUnparsedBuffer,
    const UINT32            totalNumOfROIs,
    ParsedHDRBEStatsOutput* pHDRBEStatsOutput)
{
    const HDRBE15StatsWithSaturationHwOutput* pHDRBEStatsHwOut = NULL;

    pHDRBEStatsHwOut = static_cast<const HDRBE15StatsWithSaturationHwOutput*>(pUnparsedBuffer);

    pHDRBEStatsOutput->numROIs          = totalNumOfROIs;
    pHDRBEStatsOutput->flags.hasSatInfo = 1;
    pHDRBEStatsOutput->flags.usesY      = 0;

    Utils::Memcpy(pHDRBEStatsOutput->GetChannelDataArray(), pHDRBEStatsHwOut,
                  totalNumOfROIs * sizeof(HDRBE15StatsWithSaturationHwOutput));
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Titan17xSParser::InternalParseHDRBEStatsYStatsEnabled
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID Titan17xSParser::InternalParseHDRBEStatsYStatsEnabled(
    const VOID*             pUnparsedBuffer,
    const UINT32            totalNumOfROIs,
    ParsedHDRBEStatsOutput* pHDRBEStatsOutput)
{
    const HDRBE15StatsYStatsEnableHwOutput* pHDRBEStatsHwOut = NULL;

    pHDRBEStatsHwOut = static_cast<const HDRBE15StatsYStatsEnableHwOutput*>(pUnparsedBuffer);

    pHDRBEStatsOutput->numROIs          = totalNumOfROIs;
    pHDRBEStatsOutput->flags.hasSatInfo = 0;
    pHDRBEStatsOutput->flags.usesY      = 1;

    Utils::Memcpy(pHDRBEStatsOutput->GetChannelDataArray(), pHDRBEStatsHwOut,
                  totalNumOfROIs * sizeof(HDRBE15StatsYStatsEnableHwOutput));
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Titan17xSParser::InternalParseAWBBGStats
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult Titan17xSParser::InternalParseAWBBGStats(
    const VOID*                pUnparsedBuffer,
    const BGBEConfig*          pAppliedAWBBGConfig,
    const BGBEConfig*          pReferenceAWBBGConfig,
    ParsedAWBBGStatsOutput*    pAWBBGStatsOutput)
{
    CDKResult    result = CDKResultSuccess;

    CAMX_UNREFERENCED_PARAM(pReferenceAWBBGConfig);

    if ((0 == pAppliedAWBBGConfig->horizontalNum) || (0 == pAppliedAWBBGConfig->verticalNum))
    {
        CAMX_LOG_ERROR(CamxLogGroupUtils, "Invalid total number of ROIs");
        result = CDKResultEInvalidArg;
    }

    if (CDKResultSuccess == result)
    {
        const UINT32 totalNumOfROIs = pAppliedAWBBGConfig->horizontalNum * pAppliedAWBBGConfig->verticalNum;

        switch (pAppliedAWBBGConfig->outputMode)
        {
            case BGBESaturationEnabled:
                InternalParseAWBBGStatsSaturationEnabled(pUnparsedBuffer, totalNumOfROIs, pAWBBGStatsOutput);
                break;
            case BGBEYStatsEnabled:
                InternalParseAWBBGStatsYStatsEnabled(pUnparsedBuffer, totalNumOfROIs, pAWBBGStatsOutput);
                break;
            case BGBERegular:
            default:
                InternalParseAWBBGStatsRegular(pUnparsedBuffer, totalNumOfROIs, pAWBBGStatsOutput);
                break;
        }

    }
    return result;
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Titan17xSParser::InternalParseAWBBGStatsRegular
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID Titan17xSParser::InternalParseAWBBGStatsRegular(
    const VOID*             pUnparsedBuffer,
    const UINT32            totalNumOfROIs,
    ParsedAWBBGStatsOutput* pAWBBGStatsOutput)
{
    const AWBBG15StatsHwOutput* pAWBBGStatsHwOut = NULL;

    pAWBBGStatsHwOut = static_cast<const AWBBG15StatsHwOutput*>(pUnparsedBuffer);

    pAWBBGStatsOutput->numROIs = totalNumOfROIs;
    pAWBBGStatsOutput->flags.hasSatInfo = 0;
    pAWBBGStatsOutput->flags.usesY = 0;

    Utils::Memcpy(pAWBBGStatsOutput->GetChannelDataArray(), pAWBBGStatsHwOut,
                  totalNumOfROIs * sizeof(AWBBG15StatsHwOutput));
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Titan17xSParser::InternalParseAWBBGStatsSaturationEnabled
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID Titan17xSParser::InternalParseAWBBGStatsSaturationEnabled(
    const VOID*             pUnparsedBuffer,
    const UINT32            totalNumOfROIs,
    ParsedAWBBGStatsOutput* pAWBBGStatsOutput)
{
    const AWBBG15StatsWithSaturationHwOutput* pAWBBGStatsHwOut = NULL;

    pAWBBGStatsHwOut = static_cast<const AWBBG15StatsWithSaturationHwOutput*>(pUnparsedBuffer);

    pAWBBGStatsOutput->numROIs          = totalNumOfROIs;
    pAWBBGStatsOutput->flags.hasSatInfo = 1;
    pAWBBGStatsOutput->flags.usesY      = 0;

    Utils::Memcpy(pAWBBGStatsOutput->GetChannelDataArray(), pAWBBGStatsHwOut,
                  totalNumOfROIs * sizeof(AWBBG15StatsWithSaturationHwOutput));
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Titan17xSParser::InternalParseAWBBGStatsYStatsEnabled
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID Titan17xSParser::InternalParseAWBBGStatsYStatsEnabled(
    const VOID*             pUnparsedBuffer,
    const UINT32            totalNumOfROIs,
    ParsedAWBBGStatsOutput* pAWBBGStatsOutput)
{
    const AWBBG15StatsYStatsEnableHwOutput* pAWBBGStatsHwOut = NULL;

    pAWBBGStatsHwOut = static_cast<const AWBBG15StatsYStatsEnableHwOutput*>(pUnparsedBuffer);

    pAWBBGStatsOutput->numROIs          = totalNumOfROIs;
    pAWBBGStatsOutput->flags.hasSatInfo = 0;
    pAWBBGStatsOutput->flags.usesY      = 1;

    Utils::Memcpy(pAWBBGStatsOutput->GetChannelDataArray(), pAWBBGStatsHwOut,
                  totalNumOfROIs * sizeof(AWBBG15StatsYStatsEnableHwOutput));
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Titan17xSParser::InternalParseTintlessBGStats
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult Titan17xSParser::InternalParseTintlessBGStats(
    const VOID*                  pUnparsedBuffer,
    const BGBEConfig*            pAppliedTintlessBGConfig,
    const BGBEConfig*            pReferenceTintlessBGConfig,
    ParsedTintlessBGStatsOutput* pTintlessBGStatsOutput)
{
    CDKResult    result = CDKResultSuccess;

    CAMX_UNREFERENCED_PARAM(pReferenceTintlessBGConfig);

    if ((0 == pAppliedTintlessBGConfig->horizontalNum) || (0 == pAppliedTintlessBGConfig->verticalNum))
    {
        CAMX_LOG_ERROR(CamxLogGroupUtils, "Invalid total number of ROIs");
        result = CDKResultEInvalidArg;
    }

    if (CDKResultSuccess == result)
    {
        const UINT32 totalNumOfROIs = pAppliedTintlessBGConfig->horizontalNum * pAppliedTintlessBGConfig->verticalNum;

        switch (pAppliedTintlessBGConfig->outputMode)
        {
        case BGBESaturationEnabled:
            InternalParseTintlessBGStatsSaturationEnabled(pUnparsedBuffer, totalNumOfROIs, pTintlessBGStatsOutput);
            break;
        case BGBEYStatsEnabled:
            InternalParseTintlessBGStatsYStatsEnabled(pUnparsedBuffer, totalNumOfROIs, pTintlessBGStatsOutput);
            break;
        case BGBERegular:
        default:
            InternalParseTintlessBGStatsRegular(pUnparsedBuffer, totalNumOfROIs, pTintlessBGStatsOutput);
            break;
        }

    }
    return result;
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Titan17xSParser::InternalParseTintlessBGStatsRegular
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID Titan17xSParser::InternalParseTintlessBGStatsRegular(
    const VOID*                  pUnparsedBuffer,
    const UINT32                 totalNumOfROIs,
    ParsedTintlessBGStatsOutput* pTintlessBGStatsOutput)
{
    const TintlessBG15StatsHwOutput* pTintlessBGStatsHwOut = NULL;

    pTintlessBGStatsHwOut = static_cast<const TintlessBG15StatsHwOutput*>(pUnparsedBuffer);

    pTintlessBGStatsOutput->m_flags.hasYStats           = 0;
    pTintlessBGStatsOutput->m_flags.hasSaturationPixels = 0;
    pTintlessBGStatsOutput->m_numOfRegions              = totalNumOfROIs;

    CAMX_STATIC_ASSERT(sizeof(TintlessBGStatsInfo) == sizeof(TintlessBG15StatsHwOutput));
    Utils::Memcpy(pTintlessBGStatsOutput->GetTintlessBGStatsInfoArray(),
                  pTintlessBGStatsHwOut,
                  sizeof(TintlessBG15StatsHwOutput) * totalNumOfROIs);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Titan17xSParser::InternalParseTintlessBGStatsSaturationEnabled
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID Titan17xSParser::InternalParseTintlessBGStatsSaturationEnabled(
    const VOID*                  pUnparsedBuffer,
    const UINT32                 totalNumOfROIs,
    ParsedTintlessBGStatsOutput* pTintlessBGStatsOutput)
{
    const TintlessBG15StatsWithSaturationHwOutput* pTintlessBGStatsHwOut = NULL;

    pTintlessBGStatsHwOut = static_cast<const TintlessBG15StatsWithSaturationHwOutput*>(pUnparsedBuffer);

    pTintlessBGStatsOutput->m_flags.hasYStats           = 0;
    pTintlessBGStatsOutput->m_flags.hasSaturationPixels = 1;
    pTintlessBGStatsOutput->m_numOfRegions              = totalNumOfROIs;

    CAMX_STATIC_ASSERT(sizeof(TintlessBGStatsInfoWithSat) == sizeof(TintlessBG15StatsWithSaturationHwOutput));
    Utils::Memcpy(pTintlessBGStatsOutput->GetTintlessBGStatsInfoArray(),
                  pTintlessBGStatsHwOut,
                  sizeof(TintlessBG15StatsWithSaturationHwOutput) * totalNumOfROIs);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Titan17xSParser::InternalParseTintlessBGStatsYStatsEnabled
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID Titan17xSParser::InternalParseTintlessBGStatsYStatsEnabled(
    const VOID*                  pUnparsedBuffer,
    const UINT32                 totalNumOfROIs,
    ParsedTintlessBGStatsOutput* pTintlessBGStatsOutput)
{
    const TintlessBG15StatsYStatsEnableHwOutput* pTintlessBGStatsHwOut = NULL;

    pTintlessBGStatsHwOut = static_cast<const TintlessBG15StatsYStatsEnableHwOutput*>(pUnparsedBuffer);

    pTintlessBGStatsOutput->m_flags.hasYStats           = 1;
    pTintlessBGStatsOutput->m_flags.hasSaturationPixels = 0;
    pTintlessBGStatsOutput->m_numOfRegions              = totalNumOfROIs;

    CAMX_STATIC_ASSERT(sizeof(TintlessBGStatsInfo) == sizeof(TintlessBG15StatsYStatsEnableHwOutput));
    Utils::Memcpy(pTintlessBGStatsOutput->GetTintlessBGStatsInfoArray(),
                  pTintlessBGStatsHwOut,
                  sizeof(TintlessBG15StatsYStatsEnableHwOutput) * totalNumOfROIs);
}

CAMX_NAMESPACE_END
