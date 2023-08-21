////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2017-2020 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  camxtitan17xstatsparser.cpp
///
/// @brief Titan17xStatsParser implementation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "chiispstatsdefs.h"

#include "camxmem.h"
#include "camxnode.h"
#include "camxpropertyblob.h"
#include "camxsettingsmanager.h"
#include "camxtitan17xdefs.h"
#include "camxtitan17xstatsparser.h"
#include "camxutils.h"
#include "camxvendortags.h"

CAMX_NAMESPACE_BEGIN

static const CHAR* pRSGenerateFunctionName             = "GenerateSoftwareRSStats";
static const CHAR* pDefaultSoftwareStatsLibraryName    = "com.qti.stats.statsgenerator";

static const UINT RSStatsConfigBitDepth = 8;
static const INT  MinAfPipelineDelay  = 3;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Titan17xStatsParser::GetInstance
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
Titan17xStatsParser* Titan17xStatsParser::GetInstance()
{
    static Titan17xStatsParser s_titan170xStatsParserInstance;

    return &s_titan170xStatsParserInstance;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Titan17xStatsParser::ParseBFStatsDualIFE
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult Titan17xStatsParser::ParseBFStatsDualIFE(
    const VOID*             pUnparsedBuffer,
    ISPBFStatsConfig*       pStatsConfig,
    ParsedBFStatsOutput*    pBFStatsOutput,
    UINT32                  titanVersion)
{
    CamxResult              result      = CamxResultSuccess;
    BFStatsROIConfigType*   pAppliedROI = NULL;

    // Since we're going to stitch left & right outputs into this buffer for dual-IFE case, let's clear it first
    Utils::Memset(pBFStatsOutput, 0, sizeof(ParsedBFStatsOutput));

    // parse left stripe
    pAppliedROI = &pStatsConfig[0].BFConfig.BFStats.BFStatsROIConfig;
    result = ParseBFStatsBuffer(pUnparsedBuffer, pAppliedROI, pBFStatsOutput, titanVersion);

    if (CamxResultSuccess == result)
    {
        // parse right stripe
        UINT32 offset = BFStatsMaxWidth;

        pAppliedROI = &pStatsConfig[1].BFConfig.BFStats.BFStatsROIConfig;

        result = ParseBFStatsBuffer(static_cast<const UINT8 *>(pUnparsedBuffer) + offset,
                                    pAppliedROI,
                                    pBFStatsOutput,
                                    titanVersion);
    }
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Titan17xStatsParser::ParseBFStatsIndexModeDualIFE
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult Titan17xStatsParser::ParseBFStatsIndexModeDualIFE(
    const VOID*             pUnparsedBuffer,
    ISPBFStatsConfig*       pStatsConfig,
    ParsedBFStatsOutput*    pBFStatsOutput,
    UINT32                  titanVersion)
{
    CAMX_UNREFERENCED_PARAM(titanVersion);

    CamxResult result = CamxResultSuccess;

    const BFStatsROIConfigType* pAppliedROIs[NumberOfIFEStripes] =
    {
        &pStatsConfig[0].BFConfig.BFStats.BFStatsROIConfig, // Left stripe
        &pStatsConfig[1].BFConfig.BFStats.BFStatsROIConfig  // Right stripe
    };

    // Clear only the data that impact parsing. That is, do not call memset zero to clear entire structure.
    pBFStatsOutput->numOfROIRegions = 0;
    pBFStatsOutput->frameTag        = 0;

    result = ParseBFStatsIndexModeBufferForDualIFE(pUnparsedBuffer, pAppliedROIs, pBFStatsOutput);

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Titan17xStatsParser::StitchDualIFEStripeBuffers
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult Titan17xStatsParser::StitchDualIFEStripeBuffers(
    const VOID* const   pLeftBuffer,
    const VOID* const   pRightBuffer,
    VOID*               pOutputBuffer,
    const SIZE_T        elementSize,
    const UINT32        leftHorizNum,
    const UINT32        rightHorizNum,
    const UINT32        numberOfElements)
{
    CamxResult   result           = CamxResultSuccess;
    const UINT32 totalHorizNum    = leftHorizNum + rightHorizNum;
    UINT32       row              = 0;
    UINT32       column           = 0;
    UINT32       stripeIdx        = 0;
    const UINT8* pSelectedStripe  = NULL;
    UINT8*       pOutputDest      = NULL;

    for (UINT32 outputIndex = 0; outputIndex < numberOfElements; outputIndex++)
    {
        if (column < leftHorizNum)
        {
            stripeIdx = outputIndex - row * rightHorizNum;
            pSelectedStripe = static_cast<const UINT8*>(pLeftBuffer);
        }
        else
        {
            stripeIdx = outputIndex - ((row + 1) * leftHorizNum);
            pSelectedStripe = static_cast<const UINT8*>(pRightBuffer);
        }
        pOutputDest = static_cast<UINT8*>(pOutputBuffer) + (elementSize * outputIndex);
        pSelectedStripe += (elementSize * stripeIdx);
        Utils::Memcpy(pOutputDest, pSelectedStripe, elementSize);

        column++;
        if (totalHorizNum <= column )
        {
            column = 0;
            row++;
        }
    }

    return result;
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Titan17xStatsParser::ParseBFStatsBuffer
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult Titan17xStatsParser::ParseBFStatsBuffer(
    const VOID*             pUnparsedBuffer,
    BFStatsROIConfigType*   pAppliedROI,
    ParsedBFStatsOutput*    pBFStatsOutput,
    UINT32                  titanVersion)
{
    const UINT64*               pWord64Bit      = NULL;
    const BFStats23HwOutput*    pBFStatsHwOut   = NULL;
    CamxResult                  result          = CamxResultSuccess;

    if (pAppliedROI->numBFStatsROIDimension >= BFMaxROIRegions)
    {
        CAMX_LOG_ERROR(CamxLogGroupHWL, "BF Stats num of ROI regions > MAX_ROI_REGIONS");
        result = CamxResultEFailed;
    }
    else
    {
        pWord64Bit = static_cast<const UINT64*>(pUnparsedBuffer);

        if (titanVersion >= CSLCameraTitanVersion::CSLTitan480)
        {
            pBFStatsHwOut = reinterpret_cast<const BFStats23HwOutput*>(pUnparsedBuffer);
        }
        else
        {
            // Repetitive Bayer Focus stats are followed after the first 64-bit word
            pBFStatsHwOut = reinterpret_cast<const BFStats23HwOutput*>(&pWord64Bit[1]);
        }

        /// @note   Note that BF stats output order is based on "End Pixel Order"
        ///         (i.e. the botton right corner of ROI). To recover back to the original
        ///         3A stats algorithm requested order, we retrieve back region ID for each ROI that
        ///         should follow the same ROI configuration array index.
        const UINT32 numROIToParse = pAppliedROI->numBFStatsROIDimension;
        UINT32       index;
        UINT32       regionIDAsIndex;
        // numOfROIRegions is initialized to 0 in ParseBFStats
        // Stitch only if we already have data from the left IFE
        if (0 == pBFStatsOutput->numOfROIRegions)
        {
            // From BF v2.5, the BF output no longer has frame tag information regardless of frame-based or index-based mode.
            if (titanVersion >= CSLCameraTitanVersion::CSLTitan480)
            {
                pBFStatsOutput->frameTag = 0;
            }
            else
            {
                // The first 64-bit word is the frame-tag for this region stats
                pBFStatsOutput->frameTag = pWord64Bit[0];
            }

            for (index = 0; index < numROIToParse; index++)
            {
                pBFStatsOutput->regionID[index] = pBFStatsHwOut[index].regionID;
                regionIDAsIndex = pBFStatsOutput->regionID[index];

                // Use requestedROIIndex to recover the requested order of ROI configuration.
                pBFStatsOutput->horizontal1Num[regionIDAsIndex]       = pBFStatsHwOut[index].H1Cnt;
                pBFStatsOutput->horizontal1Sum[regionIDAsIndex]       = pBFStatsHwOut[index].H1Sum;
                pBFStatsOutput->horizontal1Sharpness[regionIDAsIndex] = pBFStatsHwOut[index].H1Sharpness;
                pBFStatsOutput->region[regionIDAsIndex]               = pBFStatsHwOut[index].sel;
                pBFStatsOutput->verticalNum[regionIDAsIndex]          = pBFStatsHwOut[index].VCnt;
                pBFStatsOutput->verticalSum[regionIDAsIndex]          = pBFStatsHwOut[index].VSum;
                pBFStatsOutput->verticalSharpness[regionIDAsIndex]    = pBFStatsHwOut[index].VSharpness;

                CAMX_LOG_VERBOSE(CamxLogGroupISP, "Type %d, Index %d, H1[%d, %d, %d] V[%d, %d, %d]",
                                 pBFStatsHwOut[index].sel,
                                 pBFStatsOutput->regionID[index],
                                 pBFStatsHwOut[index].H1Cnt,
                                 pBFStatsHwOut[index].H1Sum,
                                 pBFStatsHwOut[index].H1Sharpness,
                                 pBFStatsHwOut[index].VCnt,
                                 pBFStatsHwOut[index].VSum,
                                 pBFStatsHwOut[index].VSharpness);
            }
            // Update total number of regions
            pBFStatsOutput->numOfROIRegions = numROIToParse;
        }
        else
        {
            UINT32 totalROICount = pBFStatsOutput->numOfROIRegions;

            for (index = 0; index < numROIToParse; index++)
            {
                regionIDAsIndex = pBFStatsHwOut[index].regionID;

                // if right regionIdx already doesn't exist in left regionId[], we need to add it
                if (0 == pBFStatsOutput->horizontal1Sum[regionIDAsIndex])
                {
                    pBFStatsOutput->regionID[totalROICount] = regionIDAsIndex;
                    totalROICount++;
                }
                pBFStatsOutput->horizontal1Num[regionIDAsIndex]       += pBFStatsHwOut[index].H1Cnt;
                pBFStatsOutput->horizontal1Sum[regionIDAsIndex]       += pBFStatsHwOut[index].H1Sum;
                pBFStatsOutput->horizontal1Sharpness[regionIDAsIndex] += pBFStatsHwOut[index].H1Sharpness;
                pBFStatsOutput->verticalNum[regionIDAsIndex]          += pBFStatsHwOut[index].VCnt;
                pBFStatsOutput->verticalSum[regionIDAsIndex]          += pBFStatsHwOut[index].VSum;
                pBFStatsOutput->verticalSharpness[regionIDAsIndex]    += pBFStatsHwOut[index].VSharpness;
                pBFStatsOutput->region[regionIDAsIndex]               = pBFStatsHwOut[index].sel;
            }
            // Update total number of regions
            pBFStatsOutput->numOfROIRegions = totalROICount;
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Titan17xStatsParser::ParseBFStatsIndexModeBufferForDualIFE
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult Titan17xStatsParser::ParseBFStatsIndexModeBufferForDualIFE(
    const VOID*                 pUnparsedBuffer,
    const BFStatsROIConfigType* pAppliedROIs[NumberOfIFEStripes],
    ParsedBFStatsOutput*        pBFStatsOutput)
{
    const BFStats25HwOutput*    pBFStatsHwOut = NULL;
    CamxResult                  result = CamxResultSuccess;

    if ((pAppliedROIs[0]->numBFStatsROIDimension >= BFMaxROIRegions) ||
        (pAppliedROIs[1]->numBFStatsROIDimension >= BFMaxROIRegions))
    {
        CAMX_LOG_ERROR(CamxLogGroupHWL, "BF Stats num of ROI regions > MAX_ROI_REGIONS");
        result = CamxResultEFailed;
    }
    else
    {
        pBFStatsHwOut = reinterpret_cast<const BFStats25HwOutput*>(pUnparsedBuffer);

        UINT32 outputIndex      = 0;
        UINT32 totalROICount    = 0;

        for (UINT32 stripeIndex = 0; stripeIndex < NumberOfIFEStripes; stripeIndex++)
        {
            /// @note   Note that BF stats in index mode is completedly determined by the output ID.
            const UINT32    numROIToParse   = pAppliedROIs[stripeIndex]->numBFStatsROIDimension;

            for (UINT32 index = 0; index < numROIToParse; index++)
            {
                /// @note   The merge-bit enabled region should only appear in *right stripe*, and the left stripe region
                ///         should have been already proccessed (i.e., while the current loop run as stripeIndex = 0.
                if (TRUE == pBFStatsHwOut[outputIndex].merge)
                {
                    const UINT32 regionIDAsIndex = pBFStatsHwOut[outputIndex].regionID;

                    CAMX_LOG_VERBOSE(CamxLogGroupISP, "Merging to regionIDAsIndex = %d for index = %d",
                                     regionIDAsIndex, outputIndex);

                    pBFStatsOutput->horizontal1Sum[regionIDAsIndex]       += pBFStatsHwOut[outputIndex].H1Sum;
                    pBFStatsOutput->horizontal1Num[regionIDAsIndex]       += pBFStatsHwOut[outputIndex].H1Cnt;
                    pBFStatsOutput->horizontal1Sharpness[regionIDAsIndex] += pBFStatsHwOut[outputIndex].H1Sharpness;
                    pBFStatsOutput->verticalSum[regionIDAsIndex]          += pBFStatsHwOut[outputIndex].VSum;
                    pBFStatsOutput->verticalNum[regionIDAsIndex]          += pBFStatsHwOut[outputIndex].VCnt;
                    pBFStatsOutput->verticalSharpness[regionIDAsIndex]    += pBFStatsHwOut[outputIndex].VSharpness;
                    pBFStatsOutput->region[regionIDAsIndex]                = pBFStatsHwOut[outputIndex].sel;
                }
                else
                {
                    pBFStatsOutput->horizontal1Sum[outputIndex]       = pBFStatsHwOut[outputIndex].H1Sum;
                    pBFStatsOutput->horizontal1Num[outputIndex]       = pBFStatsHwOut[outputIndex].H1Cnt;
                    pBFStatsOutput->horizontal1Sharpness[outputIndex] = pBFStatsHwOut[outputIndex].H1Sharpness;
                    pBFStatsOutput->regionID[outputIndex]             = pBFStatsHwOut[outputIndex].regionID;
                    pBFStatsOutput->verticalSum[outputIndex]          = pBFStatsHwOut[outputIndex].VSum;
                    pBFStatsOutput->verticalNum[outputIndex]          = pBFStatsHwOut[outputIndex].VCnt;
                    pBFStatsOutput->verticalSharpness[outputIndex]    = pBFStatsHwOut[outputIndex].VSharpness;
                    pBFStatsOutput->region[outputIndex]               = pBFStatsHwOut[outputIndex].sel;     // Region Type

                    /// The total ROIs should be only counted for region with merge-bit FALSE.
                    totalROICount++;
                }

                CAMX_LOG_VERBOSE(CamxLogGroupISP, "Type %d, Region ID=%d, Mergebit=%d, H1[%d, %d, %d] V[%d, %d, %d]",
                                 pBFStatsHwOut[outputIndex].sel,
                                 pBFStatsHwOut[outputIndex].regionID,
                                 pBFStatsHwOut[outputIndex].merge,
                                 pBFStatsHwOut[outputIndex].H1Cnt,
                                 pBFStatsHwOut[outputIndex].H1Sum,
                                 pBFStatsHwOut[outputIndex].H1Sharpness,
                                 pBFStatsHwOut[outputIndex].VCnt,
                                 pBFStatsHwOut[outputIndex].VSum,
                                 pBFStatsHwOut[outputIndex].VSharpness);

                // outputIndex needs to be increaed for every region being processed.
                outputIndex++;

            }
        }

        // Update total number of regions
        pBFStatsOutput->numOfROIRegions = totalROICount;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Titan17xStatsParser::ParseBFStats
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult Titan17xStatsParser::ParseBFStats(
    ParseData*    pInput)
{
    CAMX_ENTRYEXIT(CamxLogGroupStats);

    BFStatsROIConfigType* pAppliedROI               = NULL;
    CamxResult            result                    = CamxResultSuccess;
    static const UINT     GetProps[]                = {PropertyIDISPBFConfig, PropertyIDSkipStatsParserTypeBF };
    static const UINT     GetPropsLength            = CAMX_ARRAY_SIZE(GetProps);
    VOID*                 pData[GetPropsLength]     = { 0 };
    UINT maximumPipelineDelay = pInput->pNode->GetMaximumPipelineDelay() - 1;

    if (maximumPipelineDelay < MinAfPipelineDelay)
    {
        maximumPipelineDelay = MinAfPipelineDelay;
    }
    UINT64                offsets[GetPropsLength]   = { maximumPipelineDelay, 1 };

    if (FALSE == pInput->skipParse)
    {
        pInput->pNode->GetDataList(GetProps, pData, offsets, GetPropsLength);

        if (NULL == pData[0])
        {
            CAMX_LOG_ERROR(CamxLogGroupHWL, "BF Stats not able to be parsed!");
            result = CamxResultEFailed;
        }
    }

    // Parse the BF stats buffer and fill it in the 3A structure
    if (CamxResultSuccess == result)
    {
        if (FALSE == pInput->skipParse)
        {
            if (FALSE == IsSkipStatsParse(pData[1]))
            {
                // To reduce CPU load and BW overhead, avoid clearing the whole pInput->pStatsOutput structure
                // only clear/init the variables holding the sizes and other states necessary to prevent out-of-bounds accesses.
                ParsedBFStatsOutput* pStatsOutput = reinterpret_cast<ParsedBFStatsOutput*>(pInput->pStatsOutput);
                pStatsOutput->numOfROIRegions = 0;

                PropertyISPBFStats* pBFStatsMetadata = reinterpret_cast<PropertyISPBFStats*>(pData[0]);

                if (pBFStatsMetadata->dualIFEMode == FALSE)
                {
                    // Parse the BF stats buffer and fill it in the 3A structure
                    pAppliedROI = &pBFStatsMetadata->statsConfig.BFConfig.BFStats.BFStatsROIConfig;
                    result = ParseBFStatsBuffer(pInput->pUnparsedBuffer, pAppliedROI, pStatsOutput,
                                                pBFStatsMetadata->titanVersion);
                }
                else
                {
                    // From BF v2.5, the index mode needs a different parsing scheme.
                    if (pBFStatsMetadata->titanVersion >= CSLCameraTitanVersion::CSLTitan480)
                    {
                        result = ParseBFStatsIndexModeDualIFE(pInput->pUnparsedBuffer,
                                                              pBFStatsMetadata->stripeConfig,
                                                              pStatsOutput,
                                                              pBFStatsMetadata->titanVersion);
                    }
                    else
                    {
                        result = ParseBFStatsDualIFE(pInput->pUnparsedBuffer,
                                                     pBFStatsMetadata->stripeConfig,
                                                     pStatsOutput,
                                                     pBFStatsMetadata->titanVersion);
                    }
                }
            }
        }
        if (CamxResultSuccess == result)
        {
            static const UINT WriteProps[]                              = { PropertyIDParsedBFStatsOutput };
            const VOID*       pOutputData[CAMX_ARRAY_SIZE(WriteProps)]  = { &pInput->pStatsOutput };
            UINT              pDataCount[CAMX_ARRAY_SIZE(WriteProps)]   = { sizeof(pInput->pStatsOutput) };

            result = pInput->pNode->WriteDataList(WriteProps, pOutputData, pDataCount, CAMX_ARRAY_SIZE(WriteProps));
        }
    }

    if (CamxResultSuccess != result)
    {
        pInput->pNode->WritePreviousData(PropertyIDParsedBFStatsOutput, sizeof(pInput->pStatsOutput));
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Titan17xStatsParser::ParseHDRBEStatsDualIFE
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult Titan17xStatsParser::ParseHDRBEStatsDualIFE(
    const VOID*             pUnparsedBuffer,
    ISPHDRBEStatsConfig*    pStatsConfig,
    ParsedHDRBEStatsOutput* pHDRBEStatsOutput)
{
    CamxResult              result               = CamxResultSuccess;
    const BGBEConfig* const pLeftStatsConfig     = &pStatsConfig[0].HDRBEConfig;
    const BGBEConfig* const pRightStatsConfig    = &pStatsConfig[1].HDRBEConfig;

    const BOOL              leftStripeValid      = pLeftStatsConfig->isStripeValid;
    const BOOL              rightStripeValid     = pRightStatsConfig->isStripeValid;

    // Revisit this to check if offset should be based on total regions
    const UINT32            rightStripeOffset    = HDRBEStatsMaxWidth;
    const UINT8* const      pRightUnparsedBuffer = static_cast<const UINT8 *>(pUnparsedBuffer) + rightStripeOffset;

    // Store the configuration details for the left stripe directly into the output
    result = ParseHDRBEConfig(pLeftStatsConfig, pHDRBEStatsOutput);

    if (CamxResultSuccess == result)
    {
        const UINT32 leftHorizNum      = (leftStripeValid) ? pLeftStatsConfig->horizontalNum : 0;
        const UINT32 rightHorizNum     = (rightStripeValid) ? pRightStatsConfig->horizontalNum : 0;
        const UINT32 totalLeftRegions  = leftHorizNum  * pLeftStatsConfig->verticalNum;
        const UINT32 totalRightRegions = rightHorizNum * pRightStatsConfig->verticalNum;

        CAMX_LOG_VERBOSE(CamxLogGroupISP,
                         "Left Stripe: isvalid:%d numROI:%d usesY:%d hasSat:%d Hnum:%d Vnum:%d Region:%dX%d ROI(%d,%d,%d,%d)",
                         leftStripeValid,
                         pHDRBEStatsOutput->numROIs,
                         pHDRBEStatsOutput->flags.usesY,
                         pHDRBEStatsOutput->flags.hasSatInfo,
                         leftHorizNum,
                         pStatsConfig[0].HDRBEConfig.verticalNum,
                         pLeftStatsConfig->regionWidth,
                         pLeftStatsConfig->regionHeight,
                         pLeftStatsConfig->ROI.left,
                         pLeftStatsConfig->ROI.top,
                         pLeftStatsConfig->ROI.width,
                         pLeftStatsConfig->ROI.height);
        CAMX_LOG_VERBOSE(CamxLogGroupISP,
                         "Right Stripe: isvalid:%d numROI:%d usesY:%d hasSat:%d Hnum:%d Vnum:%d Region:%dX%d ROI(%d,%d,%d,%d)",
                         rightStripeValid,
                         totalRightRegions,
                         pRightStatsConfig->outputMode == BGBEYStatsEnabled,
                         pRightStatsConfig->outputMode == BGBESaturationEnabled,
                         rightHorizNum,
                         pStatsConfig[1].HDRBEConfig.verticalNum,
                         pRightStatsConfig->regionWidth,
                         pRightStatsConfig->regionHeight,
                         pRightStatsConfig->ROI.left,
                         pRightStatsConfig->ROI.top,
                         pRightStatsConfig->ROI.width,
                         pRightStatsConfig->ROI.height);

        if (leftStripeValid == TRUE)
        {
            CAMX_ASSERT(totalLeftRegions == pHDRBEStatsOutput->numROIs);
        }
        if ((leftStripeValid == TRUE) && (rightStripeValid == TRUE))
        {
            CAMX_ASSERT(pLeftStatsConfig->outputMode  == pRightStatsConfig->outputMode);
            CAMX_ASSERT(pLeftStatsConfig->verticalNum == pRightStatsConfig->verticalNum);
        }

        // To reduce CPU load and BW overhead, avoid clearing the whole pHDRBEStatsOutput structure
        // only clear/init the variables holding the sizes and other states necessary to prevent out-of-bounds accesses.

        pHDRBEStatsOutput->numROIs += ((rightStripeValid) ? totalRightRegions : 0);

        result = StitchDualIFEStripeBuffers(pUnparsedBuffer,
                                            pRightUnparsedBuffer,
                                            pHDRBEStatsOutput->GetChannelDataArray(),
                                            pHDRBEStatsOutput->flags.hasSatInfo ? sizeof(HDRBEChannelDataWithSat)
                                                                                : sizeof(HDRBEChannelData),
                                            leftHorizNum,
                                            rightHorizNum,
                                            pHDRBEStatsOutput->numROIs);

    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Titan17xStatsParser::ParseHDRBEStatsBuffer
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult Titan17xStatsParser::ParseHDRBEStatsBuffer(
    const VOID*             pUnparsedBuffer,
    BGBEConfig*             pAppliedROI,
    ParsedHDRBEStatsOutput* pHDRBEStatsOutput)
{
    CamxResult result = CamxResultSuccess;
    result = ParseHDRBEConfig(pAppliedROI, pHDRBEStatsOutput);

    if (CamxResultSuccess == result)
    {
        // Parse the HDR BE stats buffer and fill it in the 3A structure
        switch (pAppliedROI->outputMode)
        {
            case BGBEYStatsEnabled:
                ParseHDRBEOutputModeYStatsEnabled(pHDRBEStatsOutput, pUnparsedBuffer);
                break;

            case BGBESaturationEnabled:
                ParseHDRBEOutputModeSaturationEnabled(pHDRBEStatsOutput, pUnparsedBuffer);
                break;

            case BGBERegular:
            default:
                ParseHDRBEOutputModeRegular(pHDRBEStatsOutput, pUnparsedBuffer);
                break;
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Titan17xStatsParser::ParseHDRBEConfig
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult Titan17xStatsParser::ParseHDRBEConfig(
    const BGBEConfig* const       pAppliedROI,
    ParsedHDRBEStatsOutput* const pHDRBEStatsOutput)
{

    CamxResult result = CamxResultSuccess;
    if (NULL == pAppliedROI)
    {
        CAMX_LOG_ERROR(CamxLogGroupHWL, "Invalid pointer. HDE BE ROI configuration pointer is NULL");
        result = CamxResultEFailed;
    }

    if (CamxResultSuccess == result)
    {
        if ((0 == pAppliedROI->horizontalNum) || (0 == pAppliedROI->verticalNum))
        {
            CAMX_LOG_ERROR(CamxLogGroupHWL,
                           "Invalid total number of ROIs. horizontalNum is %u, verticalNum is %u",
                           pAppliedROI->horizontalNum,
                           pAppliedROI->verticalNum);

            result = CamxResultEInvalidArg;
        }
        pHDRBEStatsOutput->numROIs = pAppliedROI->horizontalNum * pAppliedROI->verticalNum;
    }
    if (CamxResultSuccess == result)
    {
        switch (pAppliedROI->outputMode)
        {
            case BGBEYStatsEnabled:
                pHDRBEStatsOutput->flags.hasSatInfo = 0;
                pHDRBEStatsOutput->flags.usesY      = 1;
                break;
            case BGBESaturationEnabled:
                pHDRBEStatsOutput->flags.hasSatInfo = 1;
                pHDRBEStatsOutput->flags.usesY      = 0;
                break;
            case BGBERegular:
            default:
                pHDRBEStatsOutput->flags.hasSatInfo = 0;
                pHDRBEStatsOutput->flags.usesY      = 0;
                break;
        }
    }

    return result;
}



////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Titan17xStatsParser::ParseHDRBEStats
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult Titan17xStatsParser::ParseHDRBEStats(
    ParseData*    pInput)
{
    CAMX_ENTRYEXIT(CamxLogGroupStats);

    BGBEConfig*           pAppliedROI               = NULL;
    CamxResult            result                    = CamxResultSuccess;
    static const UINT     GetProps[]                = {PropertyIDISPHDRBEConfig, PropertyIDSkipStatsParserTypeHDRBE };
    static const UINT     GetPropsLength            = CAMX_ARRAY_SIZE(GetProps);
    VOID*                 pData[GetPropsLength]     = { 0 };
    UINT64                offsets[GetPropsLength]   = { pInput->pNode->GetMaximumPipelineDelay(), 1 };

    if (FALSE == pInput->skipParse)
    {
        pInput->pNode->GetDataList(GetProps, pData, offsets, GetPropsLength);

        if (NULL == pData[0])
        {
            result = CamxResultEFailed;
        }
    }

    /// @todo (CAMX-1005)   HDR BE bit depth/pipeline shifting implementation needs to be done.
    ///                     No legacy implementation to consult. Need a discussion for the implementation later.
    if (CamxResultSuccess == result)
    {
        ParsedHDRBEStatsOutput* pStatsOutput = reinterpret_cast<ParsedHDRBEStatsOutput*>(pInput->pStatsOutput);
        PropertyISPHDRBEStats* pHDRBEStats = reinterpret_cast<PropertyISPHDRBEStats*>(pData[0]);

        if (FALSE == pInput->skipParse)
        {
            if (FALSE == IsSkipStatsParse(pData[1]))
            {
                // To reduce CPU load and BW overhead, avoid clearing the whole pInput->pStatsOutput structure
                // only clear/init the variables holding the sizes and other states necessary to prevent out-of-bounds accesses.
                pStatsOutput->numROIs = 0;

                if (pHDRBEStats->dualIFEMode == FALSE)
                {
                    pAppliedROI = &pHDRBEStats->statsConfig.HDRBEConfig;
                    result = ParseHDRBEStatsBuffer(pInput->pUnparsedBuffer, pAppliedROI, pStatsOutput);
                }
                else
                {
                    result = ParseHDRBEStatsDualIFE(pInput->pUnparsedBuffer, pHDRBEStats->stripeConfig, pStatsOutput);
                }
            }
        }
        if (CamxResultSuccess == result)
        {
            static const UINT WriteProps[] = { PropertyIDParsedHDRBEStatsOutput };
            const VOID*       pOutputData[CAMX_ARRAY_SIZE(WriteProps)] = { &pInput->pStatsOutput };
            UINT              pDataCount[CAMX_ARRAY_SIZE(WriteProps)] = { sizeof(pInput->pStatsOutput) };

            result = pInput->pNode->WriteDataList(WriteProps, pOutputData, pDataCount, CAMX_ARRAY_SIZE(WriteProps));
        }

        if (CamxResultSuccess == result)
        {
            result = PublishHDRBEStatsVendorTags(pInput->pNode, pStatsOutput, pHDRBEStats);
            if (CamxResultSuccess != result)
            {
                CAMX_LOG_ERROR(CamxLogGroupHWL, "Failed to publish BE stats vendor tag");
            }
        }

    }

    if (CamxResultSuccess != result)
    {
        CAMX_LOG_ERROR(CamxLogGroupStats, "Failed HDRBE Parsing for request %llu", pInput->pNode->GetCurrentRequest());
        pInput->pNode->WritePreviousData(PropertyIDParsedHDRBEStatsOutput, sizeof(pInput->pStatsOutput));
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Titan17xStatsParser::PublishHDRBEStatsVendorTags
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult Titan17xStatsParser::PublishHDRBEStatsVendorTags(
    Node*                   pNode,
    ParsedHDRBEStatsOutput* pStatsOutput,
    PropertyISPHDRBEStats*  pISPConfig)
{
    CamxResult  result         = CamxResultSuccess;
    UINT32      enable_metaTag = 0;
    UINT32      type_metaTag   = 0;

    BOOL isBEStatsEnabled = StatisticsHistogramModeOn;
    result = VendorTagManager::QueryVendorTagLocation("org.codeaurora.qcamera3.bayer_exposure",
                                                      "enable",
                                                      &enable_metaTag);

    if (CamxResultSuccess == result)
    {
        result = VendorTagManager::QueryVendorTagLocation("org.codeaurora.qcamera3.bayer_exposure",
                                                          "stats_type",
                                                          &type_metaTag);
    }
    if (CamxResultSuccess == result)
    {
        ISPStatsType      statsType                                 = ISPStatsTypeHDRBE;
        static const UINT WriteProps[]                              = { enable_metaTag, type_metaTag };
        const VOID*       pOutputData[CAMX_ARRAY_SIZE(WriteProps)]  = { &isBEStatsEnabled, &statsType };
        UINT              pDataCount[CAMX_ARRAY_SIZE(WriteProps)] = { sizeof(isBEStatsEnabled)/sizeof(UINT32),
        sizeof(statsType)/sizeof(UINT32) };
        result = pNode->WriteDataList(WriteProps, pOutputData, pDataCount, CAMX_ARRAY_SIZE(WriteProps));
    }

    result = PublishBEStats(pNode, pStatsOutput, pISPConfig); // publish RGB stats

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Titan17xStatsParser::PublishBEStats
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult Titan17xStatsParser::PublishBEStats(
    Node*                   pNode,
    ParsedHDRBEStatsOutput* pStatsOutput,
    PropertyISPHDRBEStats*  pISPConfig)
{
    CamxResult  result       = CamxResultSuccess;

    if (pISPConfig == NULL)
    {
        return result;
    }

    UINT32 height_metaTag  = 0;
    UINT32 width_metaTag   = 0;
    UINT32 r_stats_metaTag = 0;
    UINT32 g_stats_metaTag = 0;
    UINT32 b_stats_metaTag = 0;

    UINT32 maxHeight       = pISPConfig->statsConfig.HDRBEConfig.ROI.height;
    UINT32 maxWidth        = pISPConfig->statsConfig.HDRBEConfig.ROI.width;
    UINT32 horizontalNum   = pISPConfig->statsConfig.HDRBEConfig.horizontalNum;
    UINT32 verticalNum     = pISPConfig->statsConfig.HDRBEConfig.verticalNum;
    UINT32 BEStatsCount    = horizontalNum*verticalNum;
    UINT32 outputBitDepth  = pISPConfig->statsConfig.HDRBEConfig.outputBitDepth;

    if (CamxResultSuccess == result)
    {
        result = VendorTagManager::QueryVendorTagLocation("org.codeaurora.qcamera3.bayer_exposure",
                                                          "height",
                                                          &height_metaTag);
    }

    if (CamxResultSuccess == result)
    {
        result = VendorTagManager::QueryVendorTagLocation("org.codeaurora.qcamera3.bayer_exposure",
                                                          "width",
                                                          &width_metaTag);
    }


    UINT32 r_value[MaxHDRBEStatsNum];
    UINT32 g_value[MaxHDRBEStatsNum];
    UINT32 b_value[MaxAWBBGStatsNum];

    for (UINT32 pix = 0; pix < BEStatsCount; pix++)
    {
        HDRBEChannelData channel_data = pStatsOutput->GetChannelData(pix);
        if (channel_data.RCount)
        {
            r_value[pix] = channel_data.RSum / channel_data.RCount;
        }
        else
        {
            r_value[pix] = 0;
        }

        if (channel_data.GCount)
        {
            g_value[pix] = channel_data.GSum / channel_data.GCount;
        }
        else
        {
            g_value[pix] = 0;
        }

        if (channel_data.BCount)
        {
            b_value[pix] = channel_data.BSum / channel_data.BCount;
        }
        else
        {
            b_value[pix] = 0;
        }
    }

    // Query r_stats tag
    if (CamxResultSuccess == result)
    {
        result = VendorTagManager::QueryVendorTagLocation("org.codeaurora.qcamera3.bayer_exposure",
                                                          "r_stats",
                                                          &r_stats_metaTag);
    }

    // Query g_stats tag
    if (CamxResultSuccess == result)
    {
        result = VendorTagManager::QueryVendorTagLocation("org.codeaurora.qcamera3.bayer_exposure",
                                                          "g_stats",
                                                          &g_stats_metaTag);
    }

    // Query b_stats tag
    if (CamxResultSuccess == result)
    {
        result = VendorTagManager::QueryVendorTagLocation("org.codeaurora.qcamera3.bayer_exposure",
                                                          "b_stats",
                                                          &b_stats_metaTag);
    }

    if (CamxResultSuccess == result)
    {
        static const UINT WriteProps[] =
        {
            height_metaTag,
            width_metaTag,
            r_stats_metaTag,
            g_stats_metaTag,
            b_stats_metaTag
        };
        const VOID* pOutputData[CAMX_ARRAY_SIZE(WriteProps)]  =
        {
            &maxHeight,
            &maxWidth,
            r_value,
            g_value,
            b_value
        };
        UINT pDataCount[CAMX_ARRAY_SIZE(WriteProps)] =
        {
            1,
            1,
            BEStatsCount,
            BEStatsCount,
            BEStatsCount
        };

        result = pNode->WriteDataList(WriteProps, pOutputData, pDataCount, CAMX_ARRAY_SIZE(WriteProps));
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Titan17xStatsParser::ParseHDRBEOutputModeRegular
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID Titan17xStatsParser::ParseHDRBEOutputModeRegular(
    ParsedHDRBEStatsOutput* pHDRBEStatsOutput,
    const VOID*             pUnparsedBuffer)
{
    CAMX_ENTRYEXIT(CamxLogGroupStats);

    const HDRBE15StatsHwOutput* pHDRBEStatsHwOut = NULL;
    const UINT32                totalNumOfROIs   = pHDRBEStatsOutput->numROIs;

    pHDRBEStatsHwOut = static_cast<const HDRBE15StatsHwOutput*>(pUnparsedBuffer);


    Utils::Memcpy(pHDRBEStatsOutput->GetChannelDataArray(), pHDRBEStatsHwOut,
                  totalNumOfROIs * sizeof(HDRBE15StatsHwOutput));
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Titan17xStatsParser::ParseHDRBEOutputModeSaturationEnabled
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID Titan17xStatsParser::ParseHDRBEOutputModeSaturationEnabled(
    ParsedHDRBEStatsOutput* pHDRBEStatsOutput,
    const VOID*             pUnparsedBuffer)
{
    const HDRBE15StatsWithSaturationHwOutput* pHDRBEStatsHwOut = NULL;
    const UINT32                              totalNumOfROIs   = pHDRBEStatsOutput->numROIs;

    pHDRBEStatsHwOut = static_cast<const HDRBE15StatsWithSaturationHwOutput*>(pUnparsedBuffer);
    Utils::Memcpy(pHDRBEStatsOutput->GetChannelDataArray(), pHDRBEStatsHwOut,
                  totalNumOfROIs * sizeof(HDRBE15StatsWithSaturationHwOutput));
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Titan17xStatsParser::ParseHDRBEOutputModeYStatsEnabled
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID Titan17xStatsParser::ParseHDRBEOutputModeYStatsEnabled(
    ParsedHDRBEStatsOutput* pHDRBEStatsOutput,
    const VOID*             pUnparsedBuffer)
{
    const HDRBE15StatsYStatsEnableHwOutput* pHDRBEStatsHwOut = NULL;
    const UINT32                            totalNumOfROIs   = pHDRBEStatsOutput->numROIs;

    pHDRBEStatsHwOut = static_cast<const HDRBE15StatsYStatsEnableHwOutput*>(pUnparsedBuffer);
    Utils::Memcpy(pHDRBEStatsOutput->GetChannelDataArray(), pHDRBEStatsHwOut,
                  totalNumOfROIs * sizeof(HDRBE15StatsYStatsEnableHwOutput));
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Titan17xStatsParser::ParseAWBBGStatsDualIFE
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult Titan17xStatsParser::ParseAWBBGStatsDualIFE(
    const VOID*             pUnparsedBuffer,
    ISPAWBBGStatsConfig*    pStatsConfig,
    ParsedAWBBGStatsOutput* pAWBBGStatsOutput)
{
    CamxResult              result               = CamxResultSuccess;
    const BGBEConfig* const pLeftConfig          = &pStatsConfig[0].AWBBGConfig;
    const BGBEConfig* const pRightConfig         = &pStatsConfig[1].AWBBGConfig;

    // Revisit this to check if offset should be based on total regions
    const UINT32            rightStripeOffset    = AWBBGStatsMaxWidth;
    const UINT8*            pRightUnparsedBuffer = static_cast<const UINT8 *>(pUnparsedBuffer) + rightStripeOffset;


    result = ParseAWBBGConfig(pLeftConfig, pAWBBGStatsOutput);
    if (CamxResultSuccess == result)
    {
        const UINT32 leftHorizNum      = pLeftConfig->horizontalNum;
        const UINT32 rightHorizNum     = pRightConfig->horizontalNum;
        const UINT32 totalLeftRegions  = leftHorizNum  * pLeftConfig->verticalNum;
        const UINT32 totalRightRegions = rightHorizNum * pRightConfig->verticalNum;

        CAMX_ASSERT(totalLeftRegions == pAWBBGStatsOutput->numROIs);
        CAMX_ASSERT(pLeftConfig->outputMode == pRightConfig->outputMode);
        CAMX_ASSERT(pLeftConfig->verticalNum == pRightConfig->verticalNum);

        // To reduce CPU load and BW overhead, avoid clearing the whole pAWBBGStatsOutput structure
        // only clear/init the variables holding the sizes and other states necessary to prevent out-of-bounds accesses.

        pAWBBGStatsOutput->numROIs += totalRightRegions;

        const UINT32  hasSatInfo  = pAWBBGStatsOutput->flags.hasSatInfo;
        const UINT32  elementSize = hasSatInfo ? sizeof(AWBBGChannelDataWithSat) : sizeof(AWBBGChannelData);

        result = StitchDualIFEStripeBuffers(pUnparsedBuffer,
                                            pRightUnparsedBuffer,
                                            pAWBBGStatsOutput->GetChannelDataArray(),
                                            elementSize,
                                            leftHorizNum,
                                            rightHorizNum,
                                            pAWBBGStatsOutput->numROIs);
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Titan17xStatsParser::ParseBPSAWBBGStatsBuffer
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult Titan17xStatsParser::ParseBPSAWBBGStatsBuffer(
    const VOID*             pUnparsedBuffer,
    BGBEConfig*             pAppliedROI,
    ParsedAWBBGStatsOutput* pAWBBGStatsOutput)
{
    CamxResult result = CamxResultSuccess;
    UINT32 totalNumOfROIs = 0;

    if (NULL == pAppliedROI)
    {
        CAMX_LOG_ERROR(CamxLogGroupHWL, "Invalid pointer. AWB BG ROI configuration pointer is NULL");
        result = CamxResultEFailed;
    }

    if (CamxResultSuccess == result)
    {
        if ((0 == pAppliedROI->horizontalNum) || (0 == pAppliedROI->verticalNum))
        {
            CAMX_LOG_ERROR(CamxLogGroupHWL,
                "Invalid total number of ROIs. horizontalNum is %u, verticalNum is %u",
                pAppliedROI->horizontalNum,
                pAppliedROI->verticalNum);

            result = CamxResultEInvalidState;
        }
        totalNumOfROIs = pAppliedROI->horizontalNum * pAppliedROI->verticalNum;
    }


    if (CamxResultSuccess == result)
    {
        // Parse the AWB BG stats buffer and fill it in the 3A structure
        CAMX_TRACE_MESSAGE_F(CamxLogGroupStats, "AWB BG numRoi %d", totalNumOfROIs);

        switch (pAppliedROI->outputMode)
        {
            case BGBEYStatsEnabled:
                ParseBPSAWBBGOutputModeYStatsEnabled(pAWBBGStatsOutput,
                    pUnparsedBuffer,
                    pAppliedROI);
                break;

            case BGBESaturationEnabled:
                ParseBPSAWBBGOutputModeSaturationEnabled(pAWBBGStatsOutput,
                    pUnparsedBuffer,
                    pAppliedROI);
                break;

            case BGBERegular:
            default:
                ParseBPSAWBBGOutputModeRegular(pAWBBGStatsOutput,
                    pUnparsedBuffer,
                    pAppliedROI);
                break;
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Titan17xStatsParser::ParseAWBBGConfig
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult Titan17xStatsParser::ParseAWBBGConfig(
    const BGBEConfig* const       pAppliedROI,
    ParsedAWBBGStatsOutput* const pAWBBGStatsOutput)
{
    CamxResult result = CamxResultSuccess;
    if (NULL == pAppliedROI)
    {
        CAMX_LOG_ERROR(CamxLogGroupHWL, "Invalid pointer. AWB BG ROI configuration pointer is NULL");
        result = CamxResultEFailed;
    }

    if (CamxResultSuccess == result)
    {
        if ((0 == pAppliedROI->horizontalNum) || (0 == pAppliedROI->verticalNum))
        {
            CAMX_LOG_ERROR(CamxLogGroupHWL,
                "Invalid total number of ROIs. horizontalNum is %u, verticalNum is %u",
                pAppliedROI->horizontalNum,
                pAppliedROI->verticalNum);

            result = CamxResultEInvalidState;
        }
        pAWBBGStatsOutput->numROIs = pAppliedROI->horizontalNum * pAppliedROI->verticalNum;

        switch (pAppliedROI->outputMode)
        {
            case BGBEYStatsEnabled:
                pAWBBGStatsOutput->flags.hasSatInfo = 0;
                pAWBBGStatsOutput->flags.usesY = 1;
                break;
            case BGBESaturationEnabled:
                pAWBBGStatsOutput->flags.hasSatInfo = 1;
                pAWBBGStatsOutput->flags.usesY = 0;
                break;
            case BGBERegular:
            default:
                pAWBBGStatsOutput->flags.hasSatInfo = 0;
                pAWBBGStatsOutput->flags.usesY = 0;
                break;
        }
    }
    return result;

}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Titan17xStatsParser::ParseAWBBGStatsBuffer
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult Titan17xStatsParser::ParseAWBBGStatsBuffer(
    const VOID*             pUnparsedBuffer,
    BGBEConfig*             pAppliedROI,
    ParsedAWBBGStatsOutput* pAWBBGStatsOutput)
{
    CamxResult result = CamxResultSuccess;

    result = ParseAWBBGConfig(pAppliedROI, pAWBBGStatsOutput);

    if (CamxResultSuccess == result)
    {
        // Parse the AWB BG stats buffer and fill it in the 3A structure
        CAMX_TRACE_MESSAGE_F(CamxLogGroupStats, "AWB BG numRoi %d", pAWBBGStatsOutput->numROIs);

        switch (pAppliedROI->outputMode)
        {
            case BGBEYStatsEnabled:
                ParseAWBBGOutputModeYStatsEnabled(pAWBBGStatsOutput, pUnparsedBuffer);
                break;
            case BGBESaturationEnabled:
                ParseAWBBGOutputModeSaturationEnabled(pAWBBGStatsOutput, pUnparsedBuffer);
                break;
            case BGBERegular:
            default:
                ParseAWBBGOutputModeRegular(pAWBBGStatsOutput, pUnparsedBuffer);
                break;
        }
    }

    return result;
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Titan17xStatsParser::ParseBPSAWBBGStats
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult Titan17xStatsParser::ParseBPSAWBBGStats(
    ParseData*    pInput)
{
    CAMX_ENTRYEXIT(CamxLogGroupStats);

    BGBEConfig*           pAppliedROI               = NULL;
    CamxResult            result                    = CamxResultSuccess;
    static const UINT     GetProps[]                = {PropertyIDISPAWBBGConfig, PropertyIDSkipStatsParserTypeAWBBG };
    static const UINT     GetPropsLength            = CAMX_ARRAY_SIZE(GetProps);
    VOID*                 pData[GetPropsLength]     = { 0 };
    UINT64                offsets[GetPropsLength]   = { pInput->pNode->GetMaximumPipelineDelay(), 1 };

    if (FALSE == pInput->skipParse)
    {
        pInput->pNode->GetDataList(GetProps, pData, offsets, GetPropsLength);

        if (NULL != pData[0])
        {
            pAppliedROI = &reinterpret_cast<ISPAWBBGStatsConfig*>(pData[0])->AWBBGConfig;

            if (NULL == pAppliedROI)
            {
                CAMX_LOG_ERROR(CamxLogGroupHWL, "Invalid pointer. AWB BG ROI configuration pointer is NULL");
                result = CamxResultEInvalidPointer;
            }
        }
        else
        {
            CAMX_LOG_ERROR(CamxLogGroupHWL, "PropertyID %08x has not been published!", PropertyIDISPAWBBGConfig);
            result = CamxResultEFailed;
        }
    }

    if (CamxResultSuccess == result)
    {
        if (FALSE == pInput->skipParse)
        {
            if (FALSE == IsSkipStatsParse(pData[1]))
            {
                PropertyISPAWBBGStats* pAWBBGStats = reinterpret_cast<PropertyISPAWBBGStats*>(pData[0]);

                // To reduce CPU load and BW overhead, avoid clearing the whole pInput->pStatsOutput structure
                // only clear/init the variables holding the sizes and other states necessary to prevent out-of-bounds accesses.
                ParsedAWBBGStatsOutput* pStatsOutput = reinterpret_cast<ParsedAWBBGStatsOutput*>(pInput->pStatsOutput);
                pStatsOutput->numROIs = 0;

                pAppliedROI = &pAWBBGStats->statsConfig.AWBBGConfig;
                result = ParseBPSAWBBGStatsBuffer(pInput->pUnparsedBuffer, pAppliedROI, pStatsOutput);
            }
        }

        // Publish both AWBBG and HDRBE stats - HDRBE should always be the last property,
        // see the WritePropSize information for reasoning.
        static const UINT WriteProps[] =
        {
            PropertyIDParsedAWBBGStatsOutput,
            PropertyIDParsedHDRBEStatsOutput
        };
        const VOID* pOutputData[CAMX_ARRAY_SIZE(WriteProps)] =
        {
            &pInput->pStatsOutput,
            &pInput->pStatsOutput
        };
        UINT  pDataCount[CAMX_ARRAY_SIZE(WriteProps)] =
        {
            sizeof(pInput->pStatsOutput),
            sizeof(pInput->pStatsOutput)
        };

        // Publish HDRBE property using the AWBBG stats only if we are in a realtime BPS scenario.
        // Otherwise, publish only the AWBBG stats.
        // Care should be taken if additional properties are added so that HDRBE is always the last
        // property published in this list.
        UINT WritePropsSize = (TRUE == pInput->pNode->IsCameraRunningOnBPS())
            ? CAMX_ARRAY_SIZE(WriteProps) : CAMX_ARRAY_SIZE(WriteProps) - 1;

        result = pInput->pNode->WriteDataList(WriteProps, pOutputData, pDataCount, WritePropsSize);
    }
    else
    {
        pInput->pNode->WritePreviousData(PropertyIDParsedAWBBGStatsOutput, sizeof(pInput->pStatsOutput));
        if (TRUE == pInput->pNode->IsCameraRunningOnBPS())
        {
            pInput->pNode->WritePreviousData(PropertyIDParsedHDRBEStatsOutput, sizeof(pInput->pStatsOutput));
        }
    }


    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Titan17xStatsParser::ParseAWBBGStats
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult Titan17xStatsParser::ParseAWBBGStats(
    ParseData*    pInput)
{
    CAMX_ENTRYEXIT(CamxLogGroupStats);

    BGBEConfig*           pAppliedROI               = NULL;
    CamxResult            result                    = CamxResultSuccess;
    static const UINT     GetProps[]                = {PropertyIDISPAWBBGConfig, PropertyIDSkipStatsParserTypeAWBBG};
    static const UINT     GetPropsLength            = CAMX_ARRAY_SIZE(GetProps);
    VOID*                 pData[GetPropsLength]     = { 0 };
    UINT64                offsets[GetPropsLength]   = { pInput->pNode->GetMaximumPipelineDelay(), 1 };

    if (FALSE == pInput->skipParse)
    {
        pInput->pNode->GetDataList(GetProps, pData, offsets, GetPropsLength);

        if (NULL != pData[0])
        {
            pAppliedROI = &reinterpret_cast<ISPAWBBGStatsConfig*>(pData[0])->AWBBGConfig;

            if (NULL == pAppliedROI)
            {
                CAMX_LOG_ERROR(CamxLogGroupHWL, "Invalid pointer. AWB BG ROI configuration pointer is NULL");
                result = CamxResultEInvalidPointer;
            }
        }
        else
        {
            CAMX_LOG_ERROR(CamxLogGroupHWL, "PropertyID %08x has not been published!", PropertyIDISPAWBBGConfig);
            result = CamxResultEFailed;
        }
    }

    if (CamxResultSuccess == result)
    {
        ParsedAWBBGStatsOutput* pStatsOutput = reinterpret_cast<ParsedAWBBGStatsOutput*>(pInput->pStatsOutput);
        PropertyISPAWBBGStats* pAWBBGStats   = reinterpret_cast<PropertyISPAWBBGStats*>(pData[0]);

        if (FALSE == pInput->skipParse)
        {
            if (FALSE == IsSkipStatsParse(pData[1]))
            {
                // To reduce CPU load and BW overhead, avoid clearing the whole pInput->pStatsOutput structure
                // only clear/init the variables holding the sizes and other states necessary to prevent out-of-bounds accesses.

                pStatsOutput->numROIs = 0;

                if (pAWBBGStats->dualIFEMode == FALSE)
                {
                    pAppliedROI = &pAWBBGStats->statsConfig.AWBBGConfig;
                    result = ParseAWBBGStatsBuffer(pInput->pUnparsedBuffer, pAppliedROI, pStatsOutput);
                }
                else
                {
                    result = ParseAWBBGStatsDualIFE(pInput->pUnparsedBuffer, pAWBBGStats->stripeConfig, pStatsOutput);
                }
            }
        }
        if (CamxResultSuccess == result)
        {
            static const UINT WriteProps[] = { PropertyIDParsedAWBBGStatsOutput };
            const VOID*       pOutputData[CAMX_ARRAY_SIZE(WriteProps)] = { &pInput->pStatsOutput };
            UINT              pDataCount[CAMX_ARRAY_SIZE(WriteProps)] = { sizeof(pInput->pStatsOutput) };

            result = pInput->pNode->WriteDataList(WriteProps, pOutputData, pDataCount, CAMX_ARRAY_SIZE(WriteProps));
        }
        if (CamxResultSuccess == result)
        {
            const StaticSettings* pSettings = HwEnvironment::GetInstance()->GetStaticSettings();
            if (TRUE == (pSettings->enableFeature2Dump))
            {
                result = PublishParsedAWBBGStatsVendorTags(pInput->pNode, pStatsOutput, PropertyIDParsedAWBBGStatsOutput);
                if (CamxResultSuccess != result)
                {
                    CAMX_LOG_ERROR(CamxLogGroupHWL, "Failed to publish parsed AWBBG stats to vendor tags.");
                }
            }
        }

        if (CamxResultSuccess == result)
        {
            // Publish BG stats to the appropriate vendor tags
            result = PublishAWBBGStatsVendorTags(pInput->pNode, pStatsOutput, pAWBBGStats);
            if (CamxResultSuccess != result)
            {
                CAMX_LOG_ERROR(CamxLogGroupHWL, "Failed to publish BG stats vendor tags.");
            }
        }
    }

    if (CamxResultSuccess != result)
    {
        pInput->pNode->WritePreviousData(PropertyIDParsedAWBBGStatsOutput, sizeof(pInput->pStatsOutput));
    }

    return result;
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Titan17xStatsParser::PublishAWBBGStatsVendorTags
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult Titan17xStatsParser::PublishAWBBGStatsVendorTags(
    Node*                   pNode,
    ParsedAWBBGStatsOutput* pStatsOutput,
    PropertyISPAWBBGStats*  pISPConfig)
{
    CamxResult  result         = CamxResultSuccess;
    UINT32      enable_metaTag = 0;
    UINT32      type_metaTag   = 0;

    BOOL isBGStatsEnabled = TRUE;
    result = VendorTagManager::QueryVendorTagLocation("org.codeaurora.qcamera3.bayer_grid",
                                                      "enable",
                                                      &enable_metaTag);

    if (CamxResultSuccess == result)
    {
        result = VendorTagManager::QueryVendorTagLocation("org.codeaurora.qcamera3.bayer_grid",
                                                          "stats_type",
                                                          &type_metaTag);
    }
    if (CamxResultSuccess == result)
    {
        ISPStatsType      statsType                                 = ISPStatsTypeAWBBG;
        static const UINT WriteProps[]                              = { enable_metaTag, type_metaTag };
        const VOID*       pOutputData[CAMX_ARRAY_SIZE(WriteProps)]  = { &isBGStatsEnabled, &statsType };
        UINT              pDataCount[CAMX_ARRAY_SIZE(WriteProps)] = { sizeof(isBGStatsEnabled)/sizeof(UINT32),
        sizeof(statsType)/sizeof(UINT32) };

        result = pNode->WriteDataList(WriteProps, pOutputData, pDataCount, CAMX_ARRAY_SIZE(WriteProps));
    }

    result = PublishBGStats(pNode, pStatsOutput, pISPConfig); // publish RGB stats

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Titan17xStatsParser::PublishParsedTintlessBGStatsVendorTags
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult Titan17xStatsParser::PublishParsedTintlessBGStatsVendorTags(
    Node*                        pNode,
    ParsedTintlessBGStatsOutput* pStatsOutput,
    PropertyID                   propID)
{
    CamxResult  result                = CamxResultSuccess;
    UINT32      stats_metaTag         = 0;
    UINT32      propID_metaTag        = 0;
    PropertyID  propertyID            = propID;

    result = VendorTagManager::QueryVendorTagLocation("org.codeaurora.qcamera3.tintless",
                                                      "stats",
                                                      &stats_metaTag);
    if (CamxResultSuccess == result)
    {
        result = VendorTagManager::QueryVendorTagLocation("org.codeaurora.qcamera3.tintless",
                                                          "propertyID",
                                                          &propID_metaTag);
    }

    if (CamxResultSuccess == result)
    {
        static const UINT WriteProps[]                             = { propID_metaTag, stats_metaTag };
        const VOID*       pOutputData[CAMX_ARRAY_SIZE(WriteProps)] = { &propertyID, pStatsOutput };
        UINT              pDataCount[CAMX_ARRAY_SIZE(WriteProps)]  = { 1, sizeof(ParsedTintlessBGStatsOutput) / sizeof(INT8) };

        result = pNode->WriteDataList(WriteProps, pOutputData, pDataCount, CAMX_ARRAY_SIZE(WriteProps));
    }

    if (CamxResultSuccess == result)
    {
        static const UINT GetProps[]              = { stats_metaTag };
        static const UINT GetPropsLength          = CAMX_ARRAY_SIZE(GetProps);
        VOID*             pData[GetPropsLength]   = { 0 };
        UINT64            offsets[GetPropsLength] = { 0 };
        pNode->GetDataList(GetProps, pData, offsets, GetPropsLength);

        if (NULL == pData[0])
        {
            CAMX_LOG_ERROR(CamxLogGroupISP, "VendorTagID %08x has not been published!", stats_metaTag);
            result = CamxResultEFailed;
        }

        if (CamxResultSuccess == result)
        {
            static const UINT WriteProps[]                             = { propertyID };
            const VOID*       pOutputData[CAMX_ARRAY_SIZE(WriteProps)] = { &pData[0] };
            UINT              pDataCount[CAMX_ARRAY_SIZE(WriteProps)]  = { sizeof(pData[0]) };

            result = pNode->WriteDataList(WriteProps, pOutputData, pDataCount, CAMX_ARRAY_SIZE(WriteProps));
        }
    }
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Titan17xStatsParser::PublishParsedBHistStatsVendorTags
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult Titan17xStatsParser::PublishParsedBHistStatsVendorTags(
    Node*                        pNode,
    ParsedBHistStatsOutput* pStatsOutput,
    PropertyID              propID)
{
    CamxResult  result              = CamxResultSuccess;
    UINT32      stats_metaTag       = 0;
    UINT32      propID_metaTag      = 0;
    PropertyID  propertyID          = propID;

    result = VendorTagManager::QueryVendorTagLocation("org.codeaurora.qcamera3.bhist",
                                                      "stats",
                                                      &stats_metaTag);
    if (CamxResultSuccess == result)
    {
        result = VendorTagManager::QueryVendorTagLocation("org.codeaurora.qcamera3.bhist",
                                                          "propertyID",
                                                          &propID_metaTag);
    }

    if (CamxResultSuccess == result)
    {
        static const UINT WriteProps[]                             = { propID_metaTag, stats_metaTag };
        const VOID*       pOutputData[CAMX_ARRAY_SIZE(WriteProps)] = { &propertyID, pStatsOutput };
        UINT              pDataCount[CAMX_ARRAY_SIZE(WriteProps)]  = { 1, sizeof(ParsedBHistStatsOutput) / sizeof(INT8) };

        result = pNode->WriteDataList(WriteProps, pOutputData, pDataCount, CAMX_ARRAY_SIZE(WriteProps));
    }

    if (CamxResultSuccess == result)
    {
        static const UINT GetProps[]              = { stats_metaTag };
        static const UINT GetPropsLength          = CAMX_ARRAY_SIZE(GetProps);
        VOID*             pData[GetPropsLength]   = { 0 };
        UINT64            offsets[GetPropsLength] = { 0 };
        pNode->GetDataList(GetProps, pData, offsets, GetPropsLength);

        if (NULL == pData[0])
        {
            CAMX_LOG_ERROR(CamxLogGroupISP, "VendorTagID %08x has not been published!", stats_metaTag);
            result = CamxResultEFailed;
        }

        if (CamxResultSuccess == result)
        {
            static const UINT WriteProps[]                             = { propertyID };
            const VOID*       pOutputData[CAMX_ARRAY_SIZE(WriteProps)] = { &pData[0] };
            UINT              pDataCount[CAMX_ARRAY_SIZE(WriteProps)]  = { sizeof(pData[0]) };

            result = pNode->WriteDataList(WriteProps, pOutputData, pDataCount, CAMX_ARRAY_SIZE(WriteProps));
        }
    }
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Titan17xStatsParser::PublishParsedAWBBGStatsVendorTags
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult Titan17xStatsParser::PublishParsedAWBBGStatsVendorTags(
    Node*                        pNode,
    ParsedAWBBGStatsOutput*      pStatsOutput,
    PropertyID                   propID)
{
    CamxResult  result         = CamxResultSuccess;
    UINT32      stats_metaTag  = 0;
    UINT32      propID_metaTag = 0;
    PropertyID  propertyID     = propID;

    result = VendorTagManager::QueryVendorTagLocation("org.codeaurora.qcamera3.awbbg",
                                                      "stats",
                                                      &stats_metaTag);
    if (CamxResultSuccess == result)
    {
        result = VendorTagManager::QueryVendorTagLocation("org.codeaurora.qcamera3.awbbg",
                                                          "propertyID",
                                                          &propID_metaTag);
    }

    if (CamxResultSuccess == result)
    {
        static const UINT WriteProps[]                             = { propID_metaTag, stats_metaTag };
        const VOID*       pOutputData[CAMX_ARRAY_SIZE(WriteProps)] = { &propertyID, pStatsOutput };
        UINT              pDataCount[CAMX_ARRAY_SIZE(WriteProps)]  = { 1, sizeof(ParsedAWBBGStatsOutput) / sizeof(INT8) };

        result = pNode->WriteDataList(WriteProps, pOutputData, pDataCount, CAMX_ARRAY_SIZE(WriteProps));
    }

    if (CamxResultSuccess == result)
    {
        static const UINT GetProps[]              = { stats_metaTag };
        static const UINT GetPropsLength          = CAMX_ARRAY_SIZE(GetProps);
        VOID*             pData[GetPropsLength]   = { 0 };
        UINT64            offsets[GetPropsLength] = { 0 };
        pNode->GetDataList(GetProps, pData, offsets, GetPropsLength);

        if (NULL == pData[0])
        {
            CAMX_LOG_ERROR(CamxLogGroupISP, "VendorTagID %08x has not been published!", stats_metaTag);
            result = CamxResultEFailed;
        }

        if (CamxResultSuccess == result)
        {
            static const UINT WriteProps[]                             = { propertyID };
            const VOID*       pOutputData[CAMX_ARRAY_SIZE(WriteProps)] = { &pData[0] };
            UINT              pDataCount[CAMX_ARRAY_SIZE(WriteProps)]  = { sizeof(pData[0]) };

            result = pNode->WriteDataList(WriteProps, pOutputData, pDataCount, CAMX_ARRAY_SIZE(WriteProps));
        }
    }
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Titan17xStatsParser::PublishBGStats
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult Titan17xStatsParser::PublishBGStats(
    Node*                   pNode,
    ParsedAWBBGStatsOutput* pStatsOutput,
    PropertyISPAWBBGStats*  pISPConfig)
{
    CamxResult  result       = CamxResultSuccess;

    if (pISPConfig == NULL)
    {
        return result;
    }

    UINT32 height_metaTag  = 0;
    UINT32 width_metaTag   = 0;
    UINT32 r_stats_metaTag = 0;
    UINT32 g_stats_metaTag = 0;
    UINT32 b_stats_metaTag = 0;

    UINT32 maxHeight       = pISPConfig->statsConfig.AWBBGConfig.ROI.height;
    UINT32 maxWidth        = pISPConfig->statsConfig.AWBBGConfig.ROI.width;
    UINT32 horizontalNum   = pISPConfig->statsConfig.AWBBGConfig.horizontalNum;
    UINT32 verticalNum     = pISPConfig->statsConfig.AWBBGConfig.verticalNum;
    UINT32 BGStatsCount    = horizontalNum*verticalNum;
    UINT32 outputBitDepth  = pISPConfig->statsConfig.AWBBGConfig.outputBitDepth;

    if (CamxResultSuccess == result)
    {
        result = VendorTagManager::QueryVendorTagLocation("org.codeaurora.qcamera3.bayer_grid",
                                                          "height",
                                                          &height_metaTag);
    }

    if (CamxResultSuccess == result)
    {
        result = VendorTagManager::QueryVendorTagLocation("org.codeaurora.qcamera3.bayer_grid",
                                                          "width",
                                                          &width_metaTag);
    }


    UINT32 r_value[MaxAWBBGStatsNum];
    UINT32 g_value[MaxAWBBGStatsNum];
    UINT32 b_value[MaxAWBBGStatsNum];

    for (UINT32 pix = 0; pix < BGStatsCount; pix++)
    {
        AWBBGChannelData* pChannel_data = pStatsOutput->GetChannelData(pix);
        if (pChannel_data->RCount)
        {
            r_value[pix] = pChannel_data->RSum / pChannel_data->RCount;
        }
        else
        {
            r_value[pix] = 0;
        }

        if (pChannel_data->GCount)
        {
            g_value[pix] = pChannel_data->GSum / pChannel_data->GCount;
        }
        else
        {
            g_value[pix] = 0;
        }

        if (pChannel_data->BCount)
        {
            b_value[pix] = pChannel_data->BSum / pChannel_data->BCount;
        }
        else
        {
            b_value[pix] = 0;
        }
    }

    // Publishing r_stats
    if (CamxResultSuccess == result)
    {
        result = VendorTagManager::QueryVendorTagLocation("org.codeaurora.qcamera3.bayer_grid",
                                                          "r_stats",
                                                          &r_stats_metaTag);
    }

    // Publishing g_stats
    if (CamxResultSuccess == result)
    {
        result = VendorTagManager::QueryVendorTagLocation("org.codeaurora.qcamera3.bayer_grid",
                                                          "g_stats",
                                                          &g_stats_metaTag);
    }

    // Publishing b_stats
    if (CamxResultSuccess == result)
    {
        result = VendorTagManager::QueryVendorTagLocation("org.codeaurora.qcamera3.bayer_grid",
                                                          "b_stats",
                                                          &b_stats_metaTag);
    }

    if (CamxResultSuccess == result)
    {
        static const UINT WriteProps[] =
        {
            height_metaTag,
            width_metaTag,
            r_stats_metaTag,
            g_stats_metaTag,
            b_stats_metaTag
        };

        const VOID* pOutputData[CAMX_ARRAY_SIZE(WriteProps)] =
        {
            &maxHeight,
            &maxWidth,
            r_value,
            g_value,
            b_value
        };

        UINT pDataCount[CAMX_ARRAY_SIZE(WriteProps)] =
        {
            1,
            1,
            BGStatsCount,
            BGStatsCount,
            BGStatsCount
        };

        result = pNode->WriteDataList(WriteProps, pOutputData, pDataCount, CAMX_ARRAY_SIZE(WriteProps));
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Titan17xStatsParser::ParseTintlessBGStatsDualIFE
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult Titan17xStatsParser::ParseTintlessBGStatsDualIFE(
    const VOID*                     pUnparsedBuffer,
    ISPTintlessBGStatsConfig*       pStatsConfig,
    ParsedTintlessBGStatsOutput*    pTintlessBGStatsOutput)
{
    CamxResult                  result               = CamxResultSuccess;
    const BGBEConfig* const     pLeftConfig          = &pStatsConfig[0].tintlessBGConfig;
    const BGBEConfig* const     pRightConfig         = &pStatsConfig[1].tintlessBGConfig;

    // Revisit this to check if offset should be based on total regions
    const UINT32                offset               = TintlessBGStatsWidth;
    const UINT8* const          pRightUnparsedBuffer = static_cast<const UINT8 *>(pUnparsedBuffer) + offset;


    result = ParseTintlessBGStatsConfig(pLeftConfig, pTintlessBGStatsOutput);
    if (CamxResultSuccess == result)
    {
        const UINT32 leftHorizNum      = pLeftConfig->horizontalNum;
        const UINT32 rightHorizNum     = pRightConfig->horizontalNum;
        const UINT32 totalLeftRegions  = leftHorizNum  * pLeftConfig->verticalNum;
        const UINT32 totalRightRegions = rightHorizNum * pLeftConfig->verticalNum;

        CAMX_ASSERT(totalLeftRegions == pTintlessBGStatsOutput->m_numOfRegions);
        CAMX_ASSERT(pLeftConfig->outputMode == pRightConfig->outputMode);
        CAMX_ASSERT(pLeftConfig->verticalNum == pRightConfig->verticalNum);

        pTintlessBGStatsOutput->m_numOfRegions += totalRightRegions;
        // To reduce CPU load and BW overhead, avoid clearing the whole pTintlessBGStatsOutput structure
        // only clear/init the variables holding the sizes and other states necessary to prevent out-of-bounds accesses.

        const SIZE_T elementSize = (BGBESaturationEnabled == pLeftConfig->outputMode) ? sizeof(TintlessBGStatsInfoWithSat)
                                                                                      : sizeof(TintlessBGStatsInfo);
        StitchDualIFEStripeBuffers(pUnparsedBuffer,
                                   pRightUnparsedBuffer,
                                   pTintlessBGStatsOutput->GetTintlessBGStatsInfoArray(),
                                   elementSize,
                                   leftHorizNum,
                                   rightHorizNum,
                                   pTintlessBGStatsOutput->m_numOfRegions);
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Titan17xStatsParser::ParseTintlessBGStatsConfig
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult Titan17xStatsParser::ParseTintlessBGStatsConfig(
    const BGBEConfig* const         pAppliedROI,
    ParsedTintlessBGStatsOutput*    pTintlessBGStatsOutput)
{
    CamxResult result = CamxResultSuccess;
    UINT32 totalNumOfROIs = 0;

    if (NULL == pAppliedROI)
    {
        CAMX_LOG_ERROR(CamxLogGroupHWL, "Invalid pointer. Tintless BG ROI configuration pointer is NULL");
        result = CamxResultEFailed;
    }

    if (CamxResultSuccess == result)
    {
        if ((0 == pAppliedROI->horizontalNum) || (0 == pAppliedROI->verticalNum))
        {
            CAMX_LOG_ERROR(CamxLogGroupHWL,
                           "Invalid total number of ROIs. horizontalNum is %u, verticalNum is %u",
                           pAppliedROI->horizontalNum,
                           pAppliedROI->verticalNum);

            result = CamxResultEInvalidState;
        }
        totalNumOfROIs = pAppliedROI->horizontalNum * pAppliedROI->verticalNum;
    }

    if (CamxResultSuccess == result)
    {
        pTintlessBGStatsOutput->SetStatsBitWidth(pAppliedROI->outputBitDepth);
        pTintlessBGStatsOutput->SetChannelGainThreshold(pAppliedROI->channelGainThreshold);
        pTintlessBGStatsOutput->m_numOfRegions = totalNumOfROIs;
        switch (pAppliedROI->outputMode)
        {
            case BGBEYStatsEnabled:
                pTintlessBGStatsOutput->m_flags.hasYStats           = 1;
                pTintlessBGStatsOutput->m_flags.hasSaturationPixels = 0;
                break;
            case BGBESaturationEnabled:
                pTintlessBGStatsOutput->m_flags.hasYStats           = 0;
                pTintlessBGStatsOutput->m_flags.hasSaturationPixels = 1;
                break;
            case BGBERegular:
            default:
                pTintlessBGStatsOutput->m_flags.hasYStats           = 0;
                pTintlessBGStatsOutput->m_flags.hasSaturationPixels = 0;
                break;
        }
    }
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Titan17xStatsParser::ParseTintlessBGStatsBuffer
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult Titan17xStatsParser::ParseTintlessBGStatsBuffer(
    const VOID*                     pUnparsedBuffer,
    BGBEConfig*                     pAppliedROI,
    ParsedTintlessBGStatsOutput*    pTintlessBGStatsOutput)
{
    CamxResult result = CamxResultSuccess;
    result = ParseTintlessBGStatsConfig(pAppliedROI, pTintlessBGStatsOutput);
    if (CamxResultSuccess == result)
    {
        result = CamxResultEFailed;
        // Parse the Tintless BG stats buffer and fill it in the 3A structure

        switch (pAppliedROI->outputMode)
        {
            case BGBEYStatsEnabled:
                if ((sizeof(TintlessBGStatsInfo) * MaxTintlessBGStatsNum) >=
                    (sizeof(TintlessBG15StatsYStatsEnableHwOutput) * pTintlessBGStatsOutput->m_numOfRegions))
                {
                    ParseTintlessBGOutputModeYStatsEnabled(pTintlessBGStatsOutput, pUnparsedBuffer);
                    result = CamxResultSuccess;
                }

                break;
            case BGBESaturationEnabled:
                if ((sizeof(TintlessBGStatsInfoWithSat) * MaxTintlessBGStatsNum) >=
                    (sizeof(TintlessBG15StatsWithSaturationHwOutput) * pTintlessBGStatsOutput->m_numOfRegions))
                {
                    ParseTintlessBGOutputModeSaturationEnabled(pTintlessBGStatsOutput, pUnparsedBuffer);
                    result = CamxResultSuccess;
                }
                break;
            case BGBERegular:
            default:
                if ((sizeof(TintlessBGStatsInfo) * MaxTintlessBGStatsNum) >=
                    (sizeof(TintlessBG15StatsHwOutput) * pTintlessBGStatsOutput->m_numOfRegions))
                {
                    ParseTintlessBGOutputModeRegular(pTintlessBGStatsOutput, pUnparsedBuffer);
                    result = CamxResultSuccess;
                }
                break;
        }

        if (CamxResultEFailed == result)
        {
            CAMX_LOG_ERROR(CamxLogGroupHWL,
                "Invalid total number of ROIs %u from horizontalNum is %u, verticalNum is %u",
                pTintlessBGStatsOutput->m_numOfRegions,
                pAppliedROI->horizontalNum,
                pAppliedROI->verticalNum);
        }
    }
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Titan17xStatsParser::ParseTintlessBGStats
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult Titan17xStatsParser::ParseTintlessBGStats(
    ParseData*    pInput)
{
    CAMX_ENTRYEXIT(CamxLogGroupStats);
    PropertyISPTintlessBG*  pPropertyTintlessBG       = NULL;
    BGBEConfig*             pAppliedROI               = NULL;
    CamxResult              result                    = CamxResultSuccess;
    static const UINT       GetProps[]                = { PropertyIDISPTintlessBGConfig,
    PropertyIDSkipStatsParserTypeTintlessBG };
    static const UINT       GetPropsLength            = CAMX_ARRAY_SIZE(GetProps);
    VOID*                   pData[GetPropsLength]     = { 0 };
    UINT64                  offsets[GetPropsLength]   = { pInput->pNode->GetMaximumPipelineDelay(), 1 };

    if (FALSE == pInput->skipParse)
    {
        pInput->pNode->GetDataList(GetProps, pData, offsets, GetPropsLength);

        if (NULL != pData[0])
        {
            pPropertyTintlessBG = reinterpret_cast<PropertyISPTintlessBG*>(pData[0]);
            pAppliedROI         = &(pPropertyTintlessBG->statsConfig.tintlessBGConfig);

            if (NULL == pAppliedROI)
            {
                CAMX_LOG_ERROR(CamxLogGroupHWL, "Invalid pointer. Tintless BG ROI configuration pointer is NULL");
                result = CamxResultEInvalidPointer;
            }

            if (CamxResultSuccess == result)
            {
                if ((0 == pAppliedROI->horizontalNum) || (0 == pAppliedROI->verticalNum))
                {
                    CAMX_LOG_ERROR(CamxLogGroupHWL,
                        "Invalid total number of ROIs. horizontalNum is %u, verticalNum is %u",
                        pAppliedROI->horizontalNum,
                        pAppliedROI->verticalNum);

                    result = CamxResultEInvalidState;
                }
            }
        }
        else
        {
            CAMX_LOG_ERROR(CamxLogGroupHWL, "PropertyID %08x has not been published!", PropertyIDISPTintlessBGConfig);
            result = CamxResultEFailed;
        }
    }

    if (CamxResultSuccess == result)
    {
        if (FALSE == pInput->skipParse)
        {
            if (FALSE == IsSkipStatsParse(pData[1]))
            {
                // To reduce CPU load and BW overhead, avoid clearing the whole
                // pInput->pStatsOutput structure
                // only clear/init the variables holding the sizes and other states necessary to prevent
                // out-of-bounds accesses.
                ParsedTintlessBGStatsOutput* pStatsOutput = reinterpret_cast<ParsedTintlessBGStatsOutput*>(
                                                            pInput->pStatsOutput);
                pStatsOutput->m_numOfRegions = 0;

                if (FALSE == pPropertyTintlessBG->dualIFEMode)
                {
                    result = ParseTintlessBGStatsBuffer(pInput->pUnparsedBuffer, pAppliedROI, pStatsOutput);
                }
                else
                {
                    result = ParseTintlessBGStatsDualIFE(pInput->pUnparsedBuffer, pPropertyTintlessBG->stripeConfig,
                                                         pStatsOutput);
                }
            }
        }

        if (CamxResultSuccess == result)
        {
            // Publish the parsed Tintless BG stats result
            static const UINT WriteProps[]                              = { PropertyIDParsedTintlessBGStatsOutput };
            const VOID*       pOutputData[CAMX_ARRAY_SIZE(WriteProps)]  = { &pInput->pStatsOutput };
            UINT              pDataCount[CAMX_ARRAY_SIZE(WriteProps)]   = { sizeof(pInput->pStatsOutput) };

            result = pInput->pNode->WriteDataList(WriteProps, pOutputData, pDataCount, CAMX_ARRAY_SIZE(WriteProps));
        }
        if (CamxResultSuccess == result)
        {
            // Publish Tintless stats to the appropriate vendor tags
            const StaticSettings* pSettings = HwEnvironment::GetInstance()->GetStaticSettings();
            if (TRUE == (pSettings->enableFeature2Dump))
            {
                ParsedTintlessBGStatsOutput* pOut = reinterpret_cast<ParsedTintlessBGStatsOutput*>(pInput->pStatsOutput);
                result = PublishParsedTintlessBGStatsVendorTags(pInput->pNode, pOut, PropertyIDParsedTintlessBGStatsOutput);
                if (CamxResultSuccess != result)
                {
                    CAMX_LOG_ERROR(CamxLogGroupHWL, "Failed to publish parsed Tintless BG stats vendor tags.");
                }
            }
        }
    }
    else
    {
        CAMX_LOG_ERROR(CamxLogGroupHWL, "Failed to parse Tintless Stats");
        result = CamxResultEFailed;
    }

    if (CamxResultSuccess != result)
    {
        pInput->pNode->WritePreviousData(PropertyIDParsedTintlessBGStatsOutput, sizeof(pInput->pStatsOutput));
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Titan17xStatsParser::ParseBPSAWBBGOutputModeRegular
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID Titan17xStatsParser::ParseBPSAWBBGOutputModeRegular(
    ParsedAWBBGStatsOutput* pAWBBGStatsOutput,
    const VOID*             pUnparsedBuffer,
    BGBEConfig*             pAppliedROI)
{
    CAMX_ENTRYEXIT(CamxLogGroupStats);
    const AWBBG15StatsHwOutput* pAWBBGStatsHwOut    = NULL;
    AWBBGChannelData*           pChannelData        = NULL;

    pAWBBGStatsOutput->numROIs = pAppliedROI->verticalNum * pAppliedROI->horizontalNum;
    pAWBBGStatsOutput->flags.hasSatInfo = 0;
    pAWBBGStatsOutput->flags.usesY = 0;

    pChannelData     = reinterpret_cast<AWBBGChannelData*>(pAWBBGStatsOutput->GetChannelDataArray());
    pAWBBGStatsHwOut = static_cast<const AWBBG15StatsHwOutput*>(pUnparsedBuffer);

    for (INT i = 0; i < pAppliedROI->verticalNum; i++)
    {
        Utils::Memcpy(pChannelData, pAWBBGStatsHwOut,
                      pAppliedROI->horizontalNum * sizeof(AWBBG15StatsHwOutput));

        pChannelData = reinterpret_cast<AWBBGChannelData*>(reinterpret_cast<UCHAR*>(pChannelData) +
                                                 (sizeof(AWBBGChannelData) * pAppliedROI->horizontalNum));

        pAWBBGStatsHwOut = reinterpret_cast<const AWBBG15StatsHwOutput*>(reinterpret_cast<const UCHAR*>(pAWBBGStatsHwOut) +
                                                     BPSAWBBGStatsMaxWidth);
    }

}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Titan17xStatsParser::ParseAWBBGOutputModeRegular
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID Titan17xStatsParser::ParseAWBBGOutputModeRegular(
    ParsedAWBBGStatsOutput* pAWBBGStatsOutput,
    const VOID*             pUnparsedBuffer)
{
    CAMX_ENTRYEXIT(CamxLogGroupStats);
    const AWBBG15StatsHwOutput* pAWBBGStatsHwOut = NULL;
    const UINT32 totalNumOfROIs = pAWBBGStatsOutput->numROIs;

    pAWBBGStatsHwOut = static_cast<const AWBBG15StatsHwOutput*>(pUnparsedBuffer);

    Utils::Memcpy(pAWBBGStatsOutput->GetChannelDataArray(), pAWBBGStatsHwOut,
                  totalNumOfROIs * sizeof(AWBBG15StatsHwOutput));


}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Titan17xStatsParser::ParseBPSAWBBGOutputModeSaturationEnabled
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID Titan17xStatsParser::ParseBPSAWBBGOutputModeSaturationEnabled(
    ParsedAWBBGStatsOutput* pAWBBGStatsOutput,
    const VOID*             pUnparsedBuffer,
    BGBEConfig*             pAppliedROI)
{
    const AWBBG15StatsWithSaturationHwOutput* pAWBBGStatsHwOut = NULL;
    AWBBGChannelData*                         pChannelData     = NULL;

    pAWBBGStatsOutput->numROIs          = pAppliedROI->horizontalNum * pAppliedROI->verticalNum;
    pAWBBGStatsOutput->flags.hasSatInfo = 1;
    pAWBBGStatsOutput->flags.usesY      = 0;

    pChannelData     = reinterpret_cast<AWBBGChannelDataWithSat*>(pAWBBGStatsOutput->GetChannelDataArray());
    pAWBBGStatsHwOut = static_cast<const AWBBG15StatsWithSaturationHwOutput*>(pUnparsedBuffer);

    for (INT i = 0; i < pAppliedROI->verticalNum; i++)
    {
        Utils::Memcpy(pChannelData, pAWBBGStatsHwOut,
                      pAppliedROI->horizontalNum * sizeof(AWBBG15StatsWithSaturationHwOutput));

        pChannelData = reinterpret_cast<AWBBGChannelDataWithSat*>(reinterpret_cast<UCHAR*>(pChannelData) +
                                                 (sizeof(AWBBGChannelDataWithSat) * pAppliedROI->horizontalNum));

        pAWBBGStatsHwOut =
            reinterpret_cast<const AWBBG15StatsWithSaturationHwOutput*>(reinterpret_cast<const UCHAR*>(pAWBBGStatsHwOut) +
                                                                        BPSAWBBGStatsMaxWidth);
    }
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Titan17xStatsParser::ParseAWBBGOutputModeSaturationEnabled
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID Titan17xStatsParser::ParseAWBBGOutputModeSaturationEnabled(
    ParsedAWBBGStatsOutput* pAWBBGStatsOutput,
    const VOID*             pUnparsedBuffer)
{
    const AWBBG15StatsWithSaturationHwOutput* pAWBBGStatsHwOut = NULL;
    const UINT32                              totalNumOfROIs   = pAWBBGStatsOutput->numROIs;

    pAWBBGStatsHwOut = static_cast<const AWBBG15StatsWithSaturationHwOutput*>(pUnparsedBuffer);

    Utils::Memcpy(pAWBBGStatsOutput->GetChannelDataArray(), pAWBBGStatsHwOut,
                  totalNumOfROIs * sizeof(AWBBG15StatsWithSaturationHwOutput));
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Titan17xStatsParser::ParseBPSAWBBGOutputModeYStatsEnabled
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID Titan17xStatsParser::ParseBPSAWBBGOutputModeYStatsEnabled(
    ParsedAWBBGStatsOutput* pAWBBGStatsOutput,
    const VOID*             pUnparsedBuffer,
    BGBEConfig*             pAppliedROI)
{
    const AWBBG15StatsYStatsEnableHwOutput* pAWBBGStatsHwOut = NULL;
    AWBBGChannelData*                       pChannelData     = NULL;

    pAWBBGStatsOutput->numROIs          = pAppliedROI->horizontalNum * pAppliedROI->verticalNum;
    pAWBBGStatsOutput->flags.hasSatInfo = 0;
    pAWBBGStatsOutput->flags.usesY      = 1;

    pChannelData     = reinterpret_cast<AWBBGChannelData*>(pAWBBGStatsOutput->GetChannelDataArray());
    pAWBBGStatsHwOut = static_cast<const AWBBG15StatsYStatsEnableHwOutput*>(pUnparsedBuffer);

    for (INT i = 0; i < pAppliedROI->verticalNum; i++)
    {
        Utils::Memcpy(pChannelData, pAWBBGStatsHwOut,
                      pAppliedROI->horizontalNum * sizeof(AWBBG15StatsYStatsEnableHwOutput));

        pChannelData = reinterpret_cast<AWBBGChannelData*>(reinterpret_cast<UCHAR*>(pChannelData) +
                                                 (sizeof(AWBBGChannelData) * pAppliedROI->horizontalNum));

        pAWBBGStatsHwOut =
            reinterpret_cast<const AWBBG15StatsYStatsEnableHwOutput*>(reinterpret_cast<const UCHAR*>(pAWBBGStatsHwOut) +
                                                                      BPSAWBBGStatsMaxWidth);
    }

}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Titan17xStatsParser::ParseAWBBGOutputModeYStatsEnabled
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID Titan17xStatsParser::ParseAWBBGOutputModeYStatsEnabled(
    ParsedAWBBGStatsOutput* pAWBBGStatsOutput,
    const VOID*             pUnparsedBuffer)
{
    const AWBBG15StatsYStatsEnableHwOutput* pAWBBGStatsHwOut = NULL;
    const UINT32                            totalNumOfROIs   = pAWBBGStatsOutput->numROIs;

    pAWBBGStatsHwOut = static_cast<const AWBBG15StatsYStatsEnableHwOutput*>(pUnparsedBuffer);

    Utils::Memcpy(pAWBBGStatsOutput->GetChannelDataArray(), pAWBBGStatsHwOut,
                  totalNumOfROIs * sizeof(AWBBG15StatsYStatsEnableHwOutput));
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Titan17xStatsParser::ParseTintlessBGOutputModeRegular
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID Titan17xStatsParser::ParseTintlessBGOutputModeRegular(
    ParsedTintlessBGStatsOutput* pTintlessBGStatsOutput,
    const VOID*                  pUnparsedBuffer)
{
    const TintlessBG15StatsHwOutput* pTintlessBGStatsHwOut = NULL;
    const UINT32                     totalNumOfROIs        = pTintlessBGStatsOutput->m_numOfRegions;

    pTintlessBGStatsHwOut = static_cast<const TintlessBG15StatsHwOutput*>(pUnparsedBuffer);

    CAMX_STATIC_ASSERT(sizeof(TintlessBGStatsInfo) == sizeof(TintlessBG15StatsHwOutput));
    Utils::Memcpy(pTintlessBGStatsOutput->GetTintlessBGStatsInfoArray(),
                  pTintlessBGStatsHwOut,
                  sizeof(TintlessBG15StatsHwOutput) * totalNumOfROIs);
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Titan17xStatsParser::ParseTintlessBGOutputModeSaturationEnabled
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID Titan17xStatsParser::ParseTintlessBGOutputModeSaturationEnabled(
    ParsedTintlessBGStatsOutput* pTintlessBGStatsOutput,
    const VOID*                  pUnparsedBuffer)
{
    const TintlessBG15StatsWithSaturationHwOutput* pTintlessBGStatsHwOut = NULL;
    const UINT32                                   totalNumOfROIs = pTintlessBGStatsOutput->m_numOfRegions;

    pTintlessBGStatsHwOut = static_cast<const TintlessBG15StatsWithSaturationHwOutput*>(pUnparsedBuffer);

    CAMX_STATIC_ASSERT(sizeof(TintlessBGStatsInfoWithSat) == sizeof(TintlessBG15StatsWithSaturationHwOutput));
    Utils::Memcpy(pTintlessBGStatsOutput->GetTintlessBGStatsInfoArray(),
                  pTintlessBGStatsHwOut,
                  sizeof(TintlessBG15StatsWithSaturationHwOutput) * totalNumOfROIs);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Titan17xStatsParser::ParseTintlessBGOutputModeYStatsEnabled
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID Titan17xStatsParser::ParseTintlessBGOutputModeYStatsEnabled(
    ParsedTintlessBGStatsOutput* pTintlessBGStatsOutput,
    const VOID*                  pUnparsedBuffer)
{
    const TintlessBG15StatsYStatsEnableHwOutput* pTintlessBGStatsHwOut = NULL;
    const UINT32 totalNumOfROIs = pTintlessBGStatsOutput->m_numOfRegions;

    pTintlessBGStatsHwOut = static_cast<const TintlessBG15StatsYStatsEnableHwOutput*>(pUnparsedBuffer);

    CAMX_STATIC_ASSERT(sizeof(TintlessBGStatsInfo) == sizeof(TintlessBG15StatsYStatsEnableHwOutput));
    Utils::Memcpy(pTintlessBGStatsOutput->GetTintlessBGStatsInfoArray(),
                  pTintlessBGStatsHwOut,
                  sizeof(TintlessBG15StatsYStatsEnableHwOutput) * totalNumOfROIs);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Titan17xStatsParser::ParseIHistStatsDualIFE
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult Titan17xStatsParser::ParseIHistStatsDualIFE(
    const VOID*                pUnparsedBuffer,
    ISPIHistStatsConfig*       pStatsConfig,
    ParsedIHistStatsOutput*    pIHistStatsOutput)
{
    CamxResult                  result      = CamxResultSuccess;
    const IHistStats12HwOutput* pISPIHist   = static_cast<const IHistStats12HwOutput*>(pUnparsedBuffer);

    Utils::Memcpy(&pIHistStatsOutput->imageHistogram, pISPIHist, sizeof(IHistStatsOutput));
    pIHistStatsOutput->numBins = pStatsConfig[0].numBins;


    UINT32                      offset = IHistStatsWidth;
    const IHistStats12HwOutput* pRightIHist =
        reinterpret_cast<const IHistStats12HwOutput*>(static_cast<const UINT8 *>(pUnparsedBuffer) + offset);

    CAMX_ASSERT(pStatsConfig[0].numBins == pStatsConfig[1].numBins);

    for (UINT32 binIdx = 0; binIdx < pIHistStatsOutput->numBins; binIdx++)
    {
        pIHistStatsOutput->imageHistogram.blueHistogram[binIdx]     += pRightIHist->blueHistogram[binIdx];
        pIHistStatsOutput->imageHistogram.greenHistogram[binIdx]    += pRightIHist->greenHistogram[binIdx];
        pIHistStatsOutput->imageHistogram.redHistogram[binIdx]      += pRightIHist->redHistogram[binIdx];
        pIHistStatsOutput->imageHistogram.YCCHistogram[binIdx]      += pRightIHist->YCCHistogram[binIdx];
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Titan17xStatsParser::ParseIHistStats
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult Titan17xStatsParser::ParseIHistStats(
    ParseData*    pInput)
{
    CAMX_ENTRYEXIT(CamxLogGroupStats);

    CamxResult        result                    = CamxResultSuccess;
    static const UINT GetProps[]                = {PropertyIDISPIHistConfig, PropertyIDSkipStatsParserTypeIHist };
    static const UINT GetPropsLength            = CAMX_ARRAY_SIZE(GetProps);
    VOID*             pData[GetPropsLength]     = { 0 };
    UINT64            offsets[GetPropsLength]   = { pInput->pNode->GetMaximumPipelineDelay(), 1 };

    if (FALSE == pInput->skipParse)
    {
        pInput->pNode->GetDataList(GetProps, pData, offsets, GetPropsLength);

        if (NULL == pData[0])
        {
            CAMX_LOG_ERROR(CamxLogGroupHWL, "PropertyID %08x has not been published!", PropertyIDISPIHistConfig);
            result = CamxResultEFailed;
        }
    }

    if (CamxResultSuccess == result)
    {
        if (FALSE == pInput->skipParse)
        {
            if (FALSE == IsSkipStatsParse(pData[1]))
            {
                // Parse the Image Histogram stats buffer and fill it in the common structure
                const IHistStats12HwOutput* pISPIHist = static_cast<const IHistStats12HwOutput*>(pInput->pUnparsedBuffer);
                PropertyISPIHistStats*      pIHistStatsMetadata = reinterpret_cast<PropertyISPIHistStats*>(pData[0]);

                // To reduce CPU load and BW overhead, avoid clearing the whole pInput->pStatsOutput structure
                // only clear/init the variables holding the sizes and other states necessary to prevent out-of-bounds accesses.
                ParsedIHistStatsOutput* pStatsOutput = reinterpret_cast<ParsedIHistStatsOutput*>(pInput->pStatsOutput);
                pStatsOutput->numBins = 0;

                if (pIHistStatsMetadata->dualIFEMode == FALSE)
                {
                    Utils::Memcpy(&pStatsOutput->imageHistogram, pISPIHist, sizeof(IHistStatsOutput));
                    pStatsOutput->numBins = pIHistStatsMetadata->statsConfig.numBins;
                }
                else
                {
                    result = ParseIHistStatsDualIFE(pInput->pUnparsedBuffer,
                                                    pIHistStatsMetadata->stripeConfig,
                                                    pStatsOutput);
                }
            }
        }

        if (CamxResultSuccess == result)
        {
            static const UINT WriteProps[]                              = { PropertyIDParsedIHistStatsOutput };
            const VOID*       pOutputData[CAMX_ARRAY_SIZE(WriteProps)]  = { &pInput->pStatsOutput };
            UINT              pDataCount[CAMX_ARRAY_SIZE(WriteProps)]   = { sizeof(pInput->pStatsOutput) };

            result = pInput->pNode->WriteDataList(WriteProps, pOutputData, pDataCount, CAMX_ARRAY_SIZE(WriteProps));
        }
    }

    if (CamxResultSuccess != result)
    {
        pInput->pNode->WritePreviousData(PropertyIDParsedIHistStatsOutput, sizeof(pInput->pStatsOutput));
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Titan17xStatsParser::ParseHDRBHistStatsDualIFE
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult Titan17xStatsParser::ParseHDRBHistStatsDualIFE(
    const VOID*                 pUnparsedBuffer,
    ISPHDRBHistStatsConfig*     pStatsConfig,
    ParsedHDRBHistStatsOutput*  pHDRBHistStatsOutput)
{
    CamxResult                      result          = CamxResultSuccess;
    const HDRBHistStatsHwOutput*    pISPHDRBHist    = static_cast<const HDRBHistStatsHwOutput*>(pUnparsedBuffer);

    UINT32                          offset          = HDRBHistStatsMaxWidth;
    const HDRBHistStatsHwOutput*    pRightHDRHist   =
            reinterpret_cast<const HDRBHistStatsHwOutput*>(static_cast<const UINT8 *>(pUnparsedBuffer) + offset);

    CAMX_ASSERT(pStatsConfig[0].numBins == pStatsConfig[1].numBins);

    pHDRBHistStatsOutput->numBins = pStatsConfig[0].numBins;

    for (UINT32 bin = 0; bin < pStatsConfig[0].numBins; bin++)
    {
        pHDRBHistStatsOutput->HDRBHistStats.redHistogram[bin]   = pISPHDRBHist->channelsHDRBHist[bin].redBin
             + pRightHDRHist->channelsHDRBHist[bin].redBin;
        pHDRBHistStatsOutput->HDRBHistStats.greenHistogram[bin] = pISPHDRBHist->channelsHDRBHist[bin].greenBin
             + pRightHDRHist->channelsHDRBHist[bin].greenBin;
        pHDRBHistStatsOutput->HDRBHistStats.blueHistogram[bin]  = pISPHDRBHist->channelsHDRBHist[bin].blueBin
             + pRightHDRHist->channelsHDRBHist[bin].blueBin;
    }
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Titan17xStatsParser::ParseHDRBHistStats
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult Titan17xStatsParser::ParseHDRBHistStats(
    ParseData*    pInput)
{
    CAMX_ENTRYEXIT(CamxLogGroupStats);

    CamxResult        result                    = CamxResultSuccess;
    static const UINT GetProps[]                = {PropertyIDISPHDRBHistConfig, PropertyIDSkipStatsParserTypeHDRBHist };
    static const UINT GetPropsLength            = CAMX_ARRAY_SIZE(GetProps);
    VOID*             pData[GetPropsLength]     = { 0 };
    UINT64            offsets[GetPropsLength]   = { pInput->pNode->GetMaximumPipelineDelay(), 1 };

    if (FALSE == pInput->skipParse)
    {
        pInput->pNode->GetDataList(GetProps, pData, offsets, GetPropsLength);

        if (NULL == pData[0])
        {
            CAMX_LOG_ERROR(CamxLogGroupHWL, "PropertyID %08x has not been published!", PropertyIDISPHDRBHistConfig);
            result = CamxResultEFailed;
        }
    }

    if (CamxResultSuccess == result)
    {
        ParsedHDRBHistStatsOutput* pOut                    = reinterpret_cast<ParsedHDRBHistStatsOutput*>(pInput->pStatsOutput);
        PropertyISPHDRBHistStats*  pHDRBHistStatsMetadata  = reinterpret_cast<PropertyISPHDRBHistStats*>(pData[0]);

        if (FALSE == pInput->skipParse)
        {
            if (FALSE == IsSkipStatsParse(pData[1]))
            {
                const HDRBHistStatsHwOutput* pISPHDRBHist = static_cast<const HDRBHistStatsHwOutput*>(pInput->pUnparsedBuffer);

                // To reduce CPU load and BW overhead, avoid clearing the whole pOut structure
                // only clear/init the variables holding the sizes and other states necessary to prevent out-of-bounds accesses.
                pOut->numBins = 0;

                if (pHDRBHistStatsMetadata->dualIFEMode == FALSE)
                {
                    for (UINT32 bin = 0; bin < pHDRBHistStatsMetadata->statsConfig.numBins; bin++)
                    {
                        pOut->HDRBHistStats.redHistogram[bin] = pISPHDRBHist->channelsHDRBHist[bin].redBin;
                        pOut->HDRBHistStats.greenHistogram[bin] = pISPHDRBHist->channelsHDRBHist[bin].greenBin;
                        pOut->HDRBHistStats.blueHistogram[bin] = pISPHDRBHist->channelsHDRBHist[bin].blueBin;
                    }
                    pOut->numBins = pHDRBHistStatsMetadata->statsConfig.numBins;
                }
                else
                {
                    result = ParseHDRBHistStatsDualIFE(pInput->pUnparsedBuffer, pHDRBHistStatsMetadata->stripeConfig, pOut);
                }
            }
        }

        if (CamxResultSuccess == result)
        {
            static const UINT WriteProps[]                              = { PropertyIDParsedHDRBHistStatsOutput };
            const VOID*       pOutputData[CAMX_ARRAY_SIZE(WriteProps)]  = { &pInput->pStatsOutput };
            UINT              pDataCount[CAMX_ARRAY_SIZE(WriteProps)]   = { sizeof(pInput->pStatsOutput) };

            result = pInput->pNode->WriteDataList(WriteProps, pOutputData, pDataCount, CAMX_ARRAY_SIZE(WriteProps));

            if (CamxResultSuccess == result)
            {
                // Don't publish HDRHIST, use vendor tag from CR 2165290 to know the mode
                if (CamxResultSuccess != result)
                {
                    CAMX_LOG_ERROR(CamxLogGroupHWL, "Failed to publish Histogram vendor tags.");
                }
            }
        }
    }

    if (CamxResultSuccess != result)
    {
        pInput->pNode->WritePreviousData(PropertyIDParsedHDRBHistStatsOutput, sizeof(pInput->pStatsOutput));
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Titan17xStatsParser::PublishHDRHistStatsVendorTags
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult Titan17xStatsParser::PublishHDRHistStatsVendorTags(
    Node*                      pNode,
    ParsedHDRBHistStatsOutput* pStatsOutput,
    PropertyISPHDRBHistStats*  pISPConfig)
{
    CamxResult  result = CamxResultSuccess;
    UINT32      metaTag = 0;

    // Histogram is default enabled and there is no configuration
    BOOL isHistogramEnabled = StatisticsHistogramModeOn;
    result = VendorTagManager::QueryVendorTagLocation("org.codeaurora.qcamera3.histogram",
                                                      "enable",
                                                      &metaTag);
    if (CamxResultSuccess == result)
    {
        static const UINT WriteProps[]                              = { metaTag };
        const VOID*       pOutputData[CAMX_ARRAY_SIZE(WriteProps)]  = { &isHistogramEnabled };
        UINT              pDataCount[CAMX_ARRAY_SIZE(WriteProps)]   = { sizeof(isHistogramEnabled) / sizeof(BOOL) };

        result = pNode->WriteDataList(WriteProps, pOutputData, pDataCount, CAMX_ARRAY_SIZE(WriteProps));
    }

    // Publishing the histogram stats type between HDRBHist and BHist
    if (CamxResultSuccess == result)
    {
        result = VendorTagManager::QueryVendorTagLocation("org.codeaurora.qcamera3.histogram",
                                                          "stats_type",
                                                          &metaTag);
    }
    if (CamxResultSuccess == result)
    {
        ISPStatsType      statsType                                 = ISPStatsTypeHDRBHist;
        static const UINT WriteProps[]                              = { metaTag };
        const VOID*       pOutputData[CAMX_ARRAY_SIZE(WriteProps)]  = { &statsType };
        UINT              pDataCount[CAMX_ARRAY_SIZE(WriteProps)]   = { sizeof(ISPStatsType) / sizeof(UINT32) };

        result = pNode->WriteDataList(WriteProps, pOutputData, pDataCount, CAMX_ARRAY_SIZE(WriteProps));
    }

    result = PublishHDRBHistStats(pNode, pStatsOutput, pISPConfig);

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Titan17xStatsParser::PublishHistStatsVendorTags
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult Titan17xStatsParser::PublishHistStatsVendorTags(
    Node*                   pNode,
    ParsedBHistStatsOutput* pStatsOutput,
    PropertyISPBHistStats*  pISPConfig)
{
    CamxResult  result = CamxResultSuccess;
    UINT32      metaTag = 0;

    // Histogram is default enabled and there is no configuration
    BOOL isHistogramEnabled = StatisticsHistogramModeOn;
    result = VendorTagManager::QueryVendorTagLocation("org.codeaurora.qcamera3.histogram",
                                                      "enable",
                                                      &metaTag);
    if (CamxResultSuccess == result)
    {
        static const UINT WriteProps[]                              = { metaTag };
        const VOID*       pOutputData[CAMX_ARRAY_SIZE(WriteProps)]  = { &isHistogramEnabled };
        UINT              pDataCount[CAMX_ARRAY_SIZE(WriteProps)]   = { sizeof(isHistogramEnabled) / sizeof(BOOL) };

        result = pNode->WriteDataList(WriteProps, pOutputData, pDataCount, CAMX_ARRAY_SIZE(WriteProps));
    }

    // Publishing the histogram stats type between HDRBHist and BHist
    if (CamxResultSuccess == result)
    {
        result = VendorTagManager::QueryVendorTagLocation("org.codeaurora.qcamera3.histogram",
                                                          "stats_type",
                                                          &metaTag);
    }
    if (CamxResultSuccess == result)
    {
        ISPStatsType      statsType                                 = ISPStatsTypeBHist;
        static const UINT WriteProps[]                              = { metaTag };
        const VOID*       pOutputData[CAMX_ARRAY_SIZE(WriteProps)]  = { &statsType };
        UINT              pDataCount[CAMX_ARRAY_SIZE(WriteProps)]   = { sizeof(ISPStatsType) / sizeof(UINT32) };

        result = pNode->WriteDataList(WriteProps, pOutputData, pDataCount, CAMX_ARRAY_SIZE(WriteProps));
    }

    result = PublishBHistStats(pNode, pStatsOutput, pISPConfig);

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Titan17xStatsParser::PublishHDRBHistStats
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult Titan17xStatsParser::PublishHDRBHistStats(
    Node*                      pNode,
    ParsedHDRBHistStatsOutput* pStatsOutput,
    PropertyISPHDRBHistStats*  pISPConfig)
{
    CamxResult  result = CamxResultSuccess;
    UINT32      metaTag = 0;

    if (CamxResultSuccess == result)
    {
        result = VendorTagManager::QueryVendorTagLocation("org.codeaurora.qcamera3.histogram",
                                                          "buckets",
                                                          &metaTag);
    }
    if (CamxResultSuccess == result)
    {
        static const UINT WriteProps[]                              = { metaTag };
        const VOID*       pOutputData[CAMX_ARRAY_SIZE(WriteProps)]  = { &pStatsOutput->numBins };
        UINT              pDataCount[CAMX_ARRAY_SIZE(WriteProps)]   = { sizeof(pStatsOutput->numBins) / sizeof(UINT32) };

        result = pNode->WriteDataList(WriteProps, pOutputData, pDataCount, CAMX_ARRAY_SIZE(WriteProps));
    }

    UINT32     maxCount = (NULL == pISPConfig) ? 0 : pISPConfig->statsConfig.HDRBHistConfig.ROI.width *
                                                     pISPConfig->statsConfig.HDRBHistConfig.ROI.height;

    if (CamxResultSuccess == result)
    {
        result = VendorTagManager::QueryVendorTagLocation("org.codeaurora.qcamera3.histogram",
                                                          "max_count",
                                                          &metaTag);
    }

    if (CamxResultSuccess == result)
    {
        static const UINT WriteProps[]                              = { metaTag };
        const VOID*       pOutputData[CAMX_ARRAY_SIZE(WriteProps)]  = { &maxCount };
        UINT              pDataCount[CAMX_ARRAY_SIZE(WriteProps)]   = { sizeof(maxCount) / sizeof(UINT32) };

        result = pNode->WriteDataList(WriteProps, pOutputData, pDataCount, CAMX_ARRAY_SIZE(WriteProps));
    }

    // Publishing the histogram stats
    if (CamxResultSuccess == result)
    {
        result = VendorTagManager::QueryVendorTagLocation("org.codeaurora.qcamera3.histogram",
                                                          "stats",
                                                          &metaTag);
    }

    if (CamxResultSuccess == result)
    {
        static const UINT WriteProps[]                              = { metaTag };
        const VOID*       pOutputData[CAMX_ARRAY_SIZE(WriteProps)]  = { &(pStatsOutput->HDRBHistStats) };
        UINT              pDataCount[CAMX_ARRAY_SIZE(WriteProps)]   = { sizeof(HDRBHistStatsOutput) / sizeof(UINT32) };

        result = pNode->WriteDataList(WriteProps, pOutputData, pDataCount, CAMX_ARRAY_SIZE(WriteProps));
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Titan17xStatsParser::PublishBHistStats
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult Titan17xStatsParser::PublishBHistStats(
    Node*                   pNode,
    ParsedBHistStatsOutput* pStatsOutput,
    PropertyISPBHistStats*  pISPConfig)
{
    CamxResult  result = CamxResultSuccess;
    UINT32      metaTag = 0;

    if (CamxResultSuccess == result)
    {
        result = VendorTagManager::QueryVendorTagLocation("org.codeaurora.qcamera3.histogram",
                                                          "buckets",
                                                          &metaTag);
    }
    if (CamxResultSuccess == result)
    {
        static const UINT WriteProps[]                              = { metaTag };
        const VOID*       pOutputData[CAMX_ARRAY_SIZE(WriteProps)]  = { &pStatsOutput->numBins };
        UINT              pDataCount[CAMX_ARRAY_SIZE(WriteProps)]   = { sizeof(pStatsOutput->numBins) / sizeof(UINT32) };

        result = pNode->WriteDataList(WriteProps, pOutputData, pDataCount, CAMX_ARRAY_SIZE(WriteProps));
    }

    UINT32     maxCount = (NULL == pISPConfig) ? 0 : pISPConfig->statsConfig.BHistConfig.ROI.width *
                                                     pISPConfig->statsConfig.BHistConfig.ROI.height;

    if (CamxResultSuccess == result)
    {
        result = VendorTagManager::QueryVendorTagLocation("org.codeaurora.qcamera3.histogram",
                                                          "max_count",
                                                          &metaTag);
    }

    if (CamxResultSuccess == result)
    {
        static const UINT WriteProps[]                              = { metaTag };
        const VOID*       pOutputData[CAMX_ARRAY_SIZE(WriteProps)]  = { &maxCount };
        UINT              pDataCount[CAMX_ARRAY_SIZE(WriteProps)]   = { sizeof(maxCount) / sizeof(UINT32) };

        result = pNode->WriteDataList(WriteProps, pOutputData, pDataCount, CAMX_ARRAY_SIZE(WriteProps));
    }

    // Publishing the histogram stats
    if (CamxResultSuccess == result)
    {
        result = VendorTagManager::QueryVendorTagLocation("org.codeaurora.qcamera3.histogram",
                                                          "stats",
                                                          &metaTag);
    }

    if (CamxResultSuccess == result)
    {
        static const UINT WriteProps[]                              = { metaTag };
        const VOID*       pOutputData[CAMX_ARRAY_SIZE(WriteProps)]  = { pStatsOutput->BHistogramStats };
        UINT              pDataCount[CAMX_ARRAY_SIZE(WriteProps)]   = { sizeof(HDRBHistStatsOutput) / sizeof(UINT32) };

        result = pNode->WriteDataList(WriteProps, pOutputData, pDataCount, CAMX_ARRAY_SIZE(WriteProps));
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Titan17xStatsParser::ParseBHistStatsDualIFE
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult Titan17xStatsParser::ParseBHistStatsDualIFE(
    const VOID*             pUnparsedBuffer,
    ISPBHistStatsConfig*    pStatsConfig,
    ParsedBHistStatsOutput* pBHistStatsOutput)
{
    CamxResult                  result          = CamxResultSuccess;
    const BHistStatsHwOutput*   pISPBHist       = static_cast<const BHistStatsHwOutput*>(pUnparsedBuffer);
    const BHistStatsHwOutput*   pRightBHist     =
        reinterpret_cast<const BHistStatsHwOutput*>(static_cast<const UINT8 *>(pUnparsedBuffer) + BHistStatsWidth);

    CAMX_ASSERT(pStatsConfig[0].numBins == pStatsConfig[1].numBins);

    pBHistStatsOutput->numBins      = pStatsConfig[0].numBins;
    pBHistStatsOutput->channelType  = pStatsConfig[0].BHistConfig.channel;
    pBHistStatsOutput->uniform      = pStatsConfig[0].BHistConfig.uniform;

    for (UINT32 index = 0; index < pBHistStatsOutput->numBins; index++)
    {
        pBHistStatsOutput->BHistogramStats[index] = pISPBHist[index].histogramBin + pRightBHist[index].histogramBin;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Titan17xStatsParser::ParseBHistStats
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult Titan17xStatsParser::ParseBHistStats(
    ParseData*    pInput)
{
    CAMX_ENTRYEXIT(CamxLogGroupStats);

    CamxResult                  result                    = CamxResultSuccess;
    static const UINT           GetProps[]                = { PropertyIDISPBHistConfig, PropertyIDSkipStatsParserTypeBHist};
    static const UINT           GetPropsLength            = CAMX_ARRAY_SIZE(GetProps);
    VOID*                       pData[GetPropsLength]     = { 0 };
    UINT64                      offsets[GetPropsLength]   = { pInput->pNode->GetMaximumPipelineDelay(), 1 };

    if (FALSE == pInput->skipParse)
    {
        pInput->pNode->GetDataList(GetProps, pData, offsets, GetPropsLength);

        if (NULL == pData[0])
        {
            CAMX_LOG_ERROR(CamxLogGroupHWL, "PropertyID %08x has not been published!", PropertyIDISPBHistConfig);
            result = CamxResultEFailed;
        }
    }

    if (CamxResultSuccess == result)
    {
        PropertyISPBHistStats*  pBHistStatsMetadata = reinterpret_cast<PropertyISPBHistStats*>(pData[0]);
        ParsedBHistStatsOutput* pOut                = reinterpret_cast<ParsedBHistStatsOutput*>(pInput->pStatsOutput);
        if (FALSE == pInput->skipParse)
        {
            if (FALSE == IsSkipStatsParse(pData[1]))
            {
                const BHistStatsHwOutput* pBHistStatsHwOut = static_cast<const BHistStatsHwOutput*>(pInput->pUnparsedBuffer);

                // To reduce CPU load and BW overhead, avoid clearing the whole pOut structure
                // only clear/init the variables holding the sizes and other states necessary to prevent out-of-bounds accesses.
                pOut->numBins = 0;
                pOut->requestID = pBHistStatsMetadata->statsConfig.requestID;

                if (pBHistStatsMetadata->dualIFEMode == FALSE)
                {
                    ISPBHistStatsConfig*    pAppliedROI = &pBHistStatsMetadata->statsConfig;

                    pOut->numBins = pAppliedROI->numBins;
                    pOut->channelType = pAppliedROI->BHistConfig.channel;
                    pOut->uniform = pAppliedROI->BHistConfig.uniform;

                    for (UINT32 index = 0; index < pOut->numBins; index++)
                    {
                        pOut->BHistogramStats[index] = pBHistStatsHwOut[index].histogramBin;
                    }
                }
                else
                {
                    result = ParseBHistStatsDualIFE(pInput->pUnparsedBuffer, pBHistStatsMetadata->stripeConfig, pOut);
                }
            }
        }

        if (CamxResultSuccess == result)
        {
            static const UINT WriteProps[]                             = { PropertyIDParsedBHistStatsOutput };
            const VOID*       pOutputData[CAMX_ARRAY_SIZE(WriteProps)] = { &pInput->pStatsOutput };
            UINT              pDataCount[CAMX_ARRAY_SIZE(WriteProps)]  = { sizeof(pInput->pStatsOutput) };

            result = pInput->pNode->WriteDataList(WriteProps, pOutputData, pDataCount, CAMX_ARRAY_SIZE(WriteProps));
            if (CamxResultSuccess != result)
            {
                CAMX_LOG_ERROR(CamxLogGroupHWL, "Failed to publish PropertyIDParsedBHistStatsOutput.");
            }
            if (CamxResultSuccess == result)
            {
                const StaticSettings* pSettings = HwEnvironment::GetInstance()->GetStaticSettings();
                if (TRUE == (pSettings->enableFeature2Dump))
                {
                    result = PublishParsedBHistStatsVendorTags(pInput->pNode, pOut, PropertyIDParsedBHistStatsOutput);
                    if (CamxResultSuccess != result)
                    {
                        CAMX_LOG_ERROR(CamxLogGroupHWL, "Failed to publish ParsedBHistStatsOutput vendor tags.");
                    }
                }
            }
        }

        if (CamxResultSuccess == result)
        {
            // Publish Bhist stats to the appropriate vendor tags
            result = PublishHistStatsVendorTags(pInput->pNode, pOut, pBHistStatsMetadata);
            if (CamxResultSuccess != result)
            {
                CAMX_LOG_ERROR(CamxLogGroupHWL, "Failed to publish Histogram vendor tags.");
            }
        }
    }

    if (CamxResultSuccess != result)
    {
        pInput->pNode->WritePreviousData(PropertyIDParsedBHistStatsOutput, sizeof(pInput->pStatsOutput));
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Titan17xStatsParser::IsSkipStatsParse
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL Titan17xStatsParser::IsSkipStatsParse(
    VOID*    pInput)
{
    BOOL isSkipParse = FALSE;
    if (NULL != pInput)
    {
        isSkipParse = *reinterpret_cast<BOOL*>(pInput);
    }
    return isSkipParse;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Titan17xStatsParser::ParseCSOutput
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult Titan17xStatsParser::ParseCSOutput(
    UINT32                  pColumnSum[][MaxCSVertRegions],
    const VOID*             pUnparsedBuffer,
    ISPCSStatsConfig*       pCSConfig)
{
    CamxResult result = CamxResultSuccess;
    if (NULL == pCSConfig)
    {
        CAMX_LOG_ERROR(CamxLogGroupHWL, "Invalid pointer. CS Stats configuration pointer is NULL");
        result = CamxResultEInvalidPointer;
    }
    if (CamxResultSuccess == result)
    {
        // Get horizontal and vertical regions
        const UINT32  regionHNum = pCSConfig->CSConfig.statsHNum;
        const UINT32  regionVNum = pCSConfig->CSConfig.statsVNum;

        // CS sum buffer holds array of 16 bit data
        const UINT16* pCSData = static_cast<const UINT16 *>(pUnparsedBuffer);

        for (UINT32 v = 0; v < regionVNum; v++)
        {
            for (UINT32 h = 0; h < regionHNum; h++)
            {
                pColumnSum[h][v] = static_cast<UINT32>(*pCSData++);
            }
        }
    }
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Titan17xStatsParser::ParseCSStatsDualIFE
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult Titan17xStatsParser::ParseCSStatsDualIFE(
    const VOID*             pUnparsedBuffer,
    ISPCSStatsConfig*       pStatsConfig,
    ParsedCSStatsOutput*    pCSStatsOutput)
{
    CamxResult    result    = CamxResultSuccess;
    const UINT32  leftHNum  = pStatsConfig[0].CSConfig.statsHNum;
    const UINT32  rightHNum = pStatsConfig[1].CSConfig.statsHNum;
    const UINT32  totalHNum = leftHNum + rightHNum;

    CAMX_ASSERT(pStatsConfig[0].CSConfig.statsVNum == pStatsConfig[1].CSConfig.statsVNum);
    // parse left stripe
    result = ParseCSOutput(pCSStatsOutput->columnSum, pUnparsedBuffer, &pStatsConfig[0]);

    if (CamxResultSuccess == result)
    {
        UINT32 offset = CSStatsWidth;
        const UINT8* pRightUnParsedBuffer = static_cast<const UINT8 *>(pUnparsedBuffer) + offset;
        result = ParseCSOutput(&(pCSStatsOutput->columnSum[leftHNum]), pRightUnParsedBuffer, &pStatsConfig[1]);
    }

    return result;
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Titan17xStatsParser::ParseCSStats
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult Titan17xStatsParser::ParseCSStats(
    ParseData*    pInput)
{
    CAMX_ENTRYEXIT(CamxLogGroupStats);

    CamxResult        result                    = CamxResultSuccess;
    static const UINT GetProps[]                = {PropertyIDISPCSConfig, PropertyIDSkipStatsParserTypeCS };
    static const UINT GetPropsLength            = CAMX_ARRAY_SIZE(GetProps);
    VOID*             pData[GetPropsLength]     = { 0 };
    UINT64            offsets[GetPropsLength]   = { pInput->pNode->GetMaximumPipelineDelay(), 1 };

    if (FALSE == pInput->skipParse)
    {
        pInput->pNode->GetDataList(GetProps, pData, offsets, GetPropsLength);

        if (NULL == pData[0])
        {
            CAMX_LOG_ERROR(CamxLogGroupISP, "PropertyID %08x has not been published!", PropertyIDISPCSConfig);
            result = CamxResultEFailed;
        }
    }

    if (CamxResultSuccess == result)
    {
        if (FALSE == pInput->skipParse)
        {
            if (FALSE == IsSkipStatsParse(pData[1]))
            {
                PropertyISPCSStats* pCSStatsMetadata = reinterpret_cast<PropertyISPCSStats*>(pData[0]);

                // To reduce CPU load and BW overhead, avoid clearing the whole pInput->pStatsOutput structure
                // only clear/init the variables holding the sizes and other states necessary to prevent out-of-bounds accesses.
                ParsedCSStatsOutput* pStatsOutput = reinterpret_cast<ParsedCSStatsOutput*>(pInput->pStatsOutput);

                if (pCSStatsMetadata->dualIFEMode == FALSE)
                {
                    result = ParseCSOutput(pStatsOutput->columnSum, pInput->pUnparsedBuffer, &pCSStatsMetadata->statsConfig);
                }
                else
                {
                    result = ParseCSStatsDualIFE(pInput->pUnparsedBuffer, pCSStatsMetadata->stripeConfig, pStatsOutput);
                }
            }
        }

        if (CamxResultSuccess == result)
        {
            static const UINT WriteProps[]                              = { PropertyIDParsedCSStatsOutput };
            const VOID*       pOutputData[CAMX_ARRAY_SIZE(WriteProps)]  = { &pInput->pStatsOutput };
            UINT              pDataCount[CAMX_ARRAY_SIZE(WriteProps)]   = { sizeof(pInput->pStatsOutput) };

            result = pInput->pNode->WriteDataList(WriteProps, pOutputData, pDataCount, CAMX_ARRAY_SIZE(WriteProps));
        }
    }

    if (CamxResultSuccess != result)
    {
        pInput->pNode->WritePreviousData(PropertyIDParsedCSStatsOutput, sizeof(pInput->pStatsOutput));
    }
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Titan17xStatsParser::ParseRSOutput
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult Titan17xStatsParser::ParseRSOutput(
    UINT32                  pRowSum[][MaxRSVertRegions],
    const VOID*             pUnparsedBuffer,
    ISPRSStatsConfig*       pRSConfig)
{
    CamxResult result = CamxResultSuccess;
    if (NULL == pRSConfig)
    {
        CAMX_LOG_ERROR(CamxLogGroupHWL, "Invalid pointer. RS Stats configuration pointer is NULL");
        result = CamxResultEFailed;
    }
    if (CamxResultSuccess == result)
    {
        // RS sum buffer holds row sum - each of 16 bits
        const UINT16* pRSData    = static_cast<const UINT16 *>(pUnparsedBuffer);

        // Get horizontal and vertical regions
        const UINT32  regionHNum = pRSConfig->RSConfig.statsHNum;
        const UINT32  regionVNum = pRSConfig->RSConfig.statsVNum;

        for (UINT32 v = 0; v < regionVNum; v++)
        {
            for (UINT32 h = 0; h < regionHNum; h++)
            {
                // Array dimension is H X V
                pRowSum[h][v] = static_cast<UINT32>(*pRSData++);
            }
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Titan17xStatsParser::ParseRSStatsDualIFE
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult Titan17xStatsParser::ParseRSStatsDualIFE(
    const VOID*             pUnparsedBuffer,
    ISPRSStatsConfig*       pStatsConfig,
    ISPRSStatsConfig*       pStripeConfig,
    ParsedRSStatsOutput*    pRSStatsOutput)
{
    CamxResult         result               = CamxResultSuccess;
    const UINT32       leftHNum             = pStripeConfig[0].RSConfig.statsHNum;
    const UINT32       totalVNum            = pStripeConfig[0].RSConfig.statsVNum;
    const UINT32       offset               = RSStatsWidth;
    const UINT8* const pRightUnparsedBuffer = static_cast<const UINT8*>(pUnparsedBuffer) + offset;

    // parse left stripe
    result = ParseRSOutput(pRSStatsOutput->rowSum, pUnparsedBuffer, &pStripeConfig[0]);
    if (CamxResultSuccess == result)
    {
        CAMX_ASSERT(pStripeConfig[0].RSConfig.statsVNum == pStripeConfig[1].RSConfig.statsVNum);
        // There can only be one total column with multiple rows. In that case, row sum of left and right stripes need to be
        // added. Otherwise left and right stripe row sum need to be arranged.
        if (pStatsConfig->RSConfig.statsHNum == 1)
        {
            const UINT16* pRightRowSum = reinterpret_cast<const UINT16 *>(pRightUnparsedBuffer);
            for (UINT32 i = 0; i < totalVNum; i++)
            {
                pRSStatsOutput->rowSum[0][i] += pRightRowSum[i];
            }
        }
        else
        {
            result = ParseRSOutput(&(pRSStatsOutput->rowSum[leftHNum]), pRightUnparsedBuffer, &pStripeConfig[1]);
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Titan17xStatsParser::ParseRSStats
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult Titan17xStatsParser::ParseRSStats(
    ParseData*    pInput)
{
    CAMX_ENTRYEXIT(CamxLogGroupStats);

    CamxResult        result                    = CamxResultSuccess;
    static const UINT GetProps[]                = {PropertyIDISPRSConfig, PropertyIDSkipStatsParserTypeRS };
    static const UINT GetPropsLength            = CAMX_ARRAY_SIZE(GetProps);
    VOID*             pData[GetPropsLength]     = { 0 };
    UINT64            offsets[GetPropsLength]   = { pInput->pNode->GetMaximumPipelineDelay(), 1 };

    if (FALSE == pInput->skipParse)
    {
        pInput->pNode->GetDataList(GetProps, pData, offsets, GetPropsLength);

        if (NULL == pData[0])
        {
            CAMX_LOG_ERROR(CamxLogGroupISP, "PropertyID %08x has not been published!", PropertyIDISPRSConfig);
            result = CamxResultEFailed;
        }
    }

    if (CamxResultSuccess == result)
    {
        if (FALSE == pInput->skipParse)
        {
            if (FALSE == IsSkipStatsParse(pData[1]))
            {
                PropertyISPRSStats* pRSStatsMetadata    = reinterpret_cast<PropertyISPRSStats*>(pData[0]);
                // To reduce CPU load and BW overhead, avoid clearing the whole pInput->pStatsOutput structure
                // only clear/init the variables holding the sizes and other states necessary to prevent out-of-bounds accesses.
                ParsedRSStatsOutput* pStatsOutput = reinterpret_cast<ParsedRSStatsOutput*>(pInput->pStatsOutput);

                if (pRSStatsMetadata->dualIFEMode == FALSE)
                {
                    result = ParseRSOutput(pStatsOutput->rowSum, pInput->pUnparsedBuffer, &pRSStatsMetadata->statsConfig);
                }
                else
                {
                    result = ParseRSStatsDualIFE(pInput->pUnparsedBuffer,
                                                 &pRSStatsMetadata->statsConfig,
                                                 pRSStatsMetadata->stripeConfig,
                                                 pStatsOutput);
                }
            }
        }

        if (CamxResultSuccess == result)
        {
            static const UINT WriteProps[]                              = { PropertyIDParsedRSStatsOutput };
            const VOID*       pOutputData[CAMX_ARRAY_SIZE(WriteProps)]  = { &pInput->pStatsOutput };
            UINT              pDataCount[CAMX_ARRAY_SIZE(WriteProps)]   = { sizeof(pInput->pStatsOutput) };

            result = pInput->pNode->WriteDataList(WriteProps, pOutputData, pDataCount, CAMX_ARRAY_SIZE(WriteProps));
        }
    }

    if (CamxResultSuccess != result)
    {
        pInput->pNode->WritePreviousData(PropertyIDParsedRSStatsOutput, sizeof(pInput->pStatsOutput));
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Titan17xStatsParser::LoadSoftwareRSStats
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult Titan17xStatsParser::LoadSoftwareRSStats()
{
    VOID* pAddr                           = NULL;
    const CHAR* pLibraryName              = NULL;
    const CHAR* pLibraryPath              = NULL;
    CamxResult  result                    = CamxResultSuccess;

    pLibraryName = pDefaultSoftwareStatsLibraryName;
    pLibraryPath = DefaultAlgorithmPath;

    pAddr = StatsUtil::LoadAlgorithmLib(&m_hRSStatsHandle, pLibraryPath, pLibraryName, pRSGenerateFunctionName);
    if (NULL == pAddr)
    {
        result = CamxResultEFailed;
        CAMX_LOG_ERROR(CamxLogGroupISP, "Failed to load library for stats generation");

        if (NULL != m_hRSStatsHandle)
        {
            OsUtils::LibUnmap(m_hRSStatsHandle);
            m_hRSStatsHandle = NULL;
        }
    }
    else
    {
        m_pRSStats = reinterpret_cast<CREATEGENERATERSSTATS>(pAddr);
        result = CamxResultSuccess;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Titan17xStatsParser::ParseRSSWStats
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult Titan17xStatsParser::ParseRSSWStats(
    ParseData*    pInput)
{
    CAMX_ENTRYEXIT(CamxLogGroupStats);

    CamxResult        result                  = CamxResultSuccess;
    static const UINT GetProps[]              = { PropertyIDAFDStatsControl };
    static const UINT GetPropsLength          = CAMX_ARRAY_SIZE(GetProps);
    VOID*             pData[GetPropsLength]   = { 0 };
    UINT64            offsets[GetPropsLength] = { pInput->pNode->GetMaximumPipelineDelay() };

    if (NULL == m_pRSStats)
    {
        result = LoadSoftwareRSStats();
    }

    // SW RS Statistics need to have the image format to retrieve
    // the image dimensions.
    if ((CamxResultSuccess == result) && (FALSE == pInput->skipParse) && (NULL == pInput->pImageFormat))
    {
        CAMX_LOG_ERROR(CamxLogGroupStats, "Input image format is unavailable!");
        result = CamxResultEFailed;
    }


    if ((CamxResultSuccess == result) && (FALSE == pInput->skipParse))
    {
        pInput->pNode->GetDataList(GetProps, pData, offsets, GetPropsLength);

        if (NULL == pData[0])
        {
            CAMX_LOG_ERROR(CamxLogGroupISP, "PropertyID %08x has not been published!", PropertyIDAFDStatsControl);
            result = CamxResultEFailed;
        }
    }

    if (CamxResultSuccess == result)
    {
        ISPRSStatsConfig   statsConfig = { { 0 } };
        StatsRowSum        statsRowSum = { 0 };

        if (FALSE == pInput->skipParse)
        {
            StatsDimension dimensions = { pInput->pImageFormat->width, pInput->pImageFormat->height };
            statsConfig.RSConfig      = (reinterpret_cast<AFDStatsControl*>(pData[0]))->statsConfig;

            // To reduce CPU load and BW overhead, avoid clearing the whole pInput->pStatsOutput structure only
            // clear/init the variables holding the sizes and other states necessary to prevent out-of-bounds accesses.

            ParsedRSStatsOutput* pStatsOutput = reinterpret_cast<ParsedRSStatsOutput*>(pInput->pStatsOutput);
            statsRowSum.rowSum = &pStatsOutput->rowSum[0][0];
            result = m_pRSStats(static_cast<UINT8*>(pInput->pUnparsedBuffer),
                                &dimensions,
                                pInput->pImageFormat->formatParams.yuvFormat[0].planeStride,
                                &statsConfig.RSConfig,
                                &statsRowSum);
        }

        if (CamxResultSuccess == result)
        {
            statsConfig.RSConfig.statsHNum = statsRowSum.horizontalRegionCount;
            statsConfig.RSConfig.statsVNum = statsRowSum.verticalRegionCount;
            statsConfig.statsRgnWidth      = statsRowSum.regionWidth;
            statsConfig.statsRgnHeight     = statsRowSum.regionHeight;
            statsConfig.bitDepth           = RSStatsConfigBitDepth;

            static const UINT WriteProps[] =
            {
                PropertyIDISPRSConfig,
                PropertyIDParsedRSStatsOutput
            };
            const VOID* pOutputData[CAMX_ARRAY_SIZE(WriteProps)] =
            {
                &statsConfig,
                &pInput->pStatsOutput
            };
            UINT pDataCount[CAMX_ARRAY_SIZE(WriteProps)] =
            {
                sizeof(statsConfig),
                sizeof(pInput->pStatsOutput)
            };

            result = pInput->pNode->WriteDataList(WriteProps, pOutputData, pDataCount, CAMX_ARRAY_SIZE(WriteProps));
        }
    }

    if (CamxResultSuccess != result)
    {
        pInput->pNode->WritePreviousData(PropertyIDISPRSConfig, sizeof(ISPRSStatsConfig));
        pInput->pNode->WritePreviousData(PropertyIDParsedRSStatsOutput, sizeof(pInput->pStatsOutput));
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Titan17xStatsParser::Parse
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult Titan17xStatsParser::Parse(
    ISPStatsType  statsType,
    ParseData*    pInput)
{
    CamxResult result = CamxResultSuccess;

    // Error Handling
    if (NULL == pInput)
    {
        CAMX_LOG_ERROR(CamxLogGroupHWL, "pInput is NULL");
        result = CamxResultEFailed;
    }
    else if ((pInput->skipParse == FALSE) && (pInput->pUnparsedBuffer == NULL))
    {
        CAMX_LOG_ERROR(CamxLogGroupHWL, "pUnparsedBuffer is NULL, and parser will not skip");
        result = CamxResultEFailed;
    }
    else
    {
        switch (statsType)
        {
            case ISPStatsTypeBF:
                result = ParseBFStats(pInput);
                break;

            case ISPStatsTypeHDRBE:
                result = ParseHDRBEStats(pInput);
                break;

            case ISPStatsTypeHDRBHist:
                result = ParseHDRBHistStats(pInput);
                break;

            case ISPStatsTypeAWBBG:
                result = ParseAWBBGStats(pInput);
                break;

            case ISPStatsTypeIHist:
                result = ParseIHistStats(pInput);
                break;

            case ISPStatsTypeRS:
                result = ParseRSStats(pInput);
                break;

            case ISPStatsTypeCS:
                result = ParseCSStats(pInput);
                break;

            case ISPStatsTypeBHist:
                result = ParseBHistStats(pInput);
                break;

            case ISPStatsTypeTintlessBG:
                result = ParseTintlessBGStats(pInput);
                break;

            case ISPStatsTypeBPSAWBBG:
                result = ParseBPSAWBBGStats(pInput);
                break;

            case ISPStatsTypeBPSRegYUV:
                result = ParseRSSWStats(pInput);
                break;

            default:
                CAMX_LOG_ERROR(CamxLogGroupHWL, "Invalid stats type input");
                result = CamxResultEInvalidArg;
        }
    }

    return result;
}

CAMX_NAMESPACE_END
