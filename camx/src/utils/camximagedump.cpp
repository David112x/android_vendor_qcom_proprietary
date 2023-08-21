////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2017-2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file   camximagedump.cpp
/// @brief  Utility functions for dumping images to files.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "camximagebuffer.h"
#include "camximagedump.h"
#include "camximageformatutils.h"
#include "camxincs.h"

CAMX_NAMESPACE_BEGIN

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ImageDump::Dump
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID ImageDump::Dump(
    const ImageDumpInfo* pDumpInfo)
{
    CHAR dumpFilename[256] = { 0 };
    CHAR suffix[15]        = { 0 };

#if defined (CAMX_ANDROID_API) && (CAMX_ANDROID_API >= 28) // NOWHINE PR002 <- Win32 definition
    CHAR dataPath[]        = "/data/vendor/camera";
#else
    CHAR dataPath[]        = "/data/misc/camera";
#endif // Android-P or later

    CamxDateTime systemDateTime;
    OsUtils::GetDateTime(&systemDateTime);

    UINT numPlanes = ImageFormatUtils::GetNumberOfPlanes(pDumpInfo->pFormat);

    switch (pDumpInfo->pFormat->format)
    {
        case Format::Jpeg:
            OsUtils::SNPrintF(suffix, sizeof(suffix), "JPEG");
            break;
        case Format::Y8:
            OsUtils::SNPrintF(suffix, sizeof(suffix), "Y8");
            break;
        case Format::Y16:
            OsUtils::SNPrintF(suffix, sizeof(suffix), "Y16");
            break;
        case Format::YUV420NV12:
            OsUtils::SNPrintF(suffix, sizeof(suffix), "YUV420NV12");
            break;
        case Format::YUV420NV21:
            OsUtils::SNPrintF(suffix, sizeof(suffix), "YUV420NV21");
            break;
        case Format::YUV422NV16:
            OsUtils::SNPrintF(suffix, sizeof(suffix), "YUV422NV16");
            break;
        case Format::Blob:
            OsUtils::SNPrintF(suffix, sizeof(suffix), "BLOB");
            break;
        case Format::RawYUV8BIT:
            OsUtils::SNPrintF(suffix, sizeof(suffix), "RAWYUV8BIT");
            break;
        case Format::RawPrivate:
            OsUtils::SNPrintF(suffix, sizeof(suffix), "RAWPRIVATE");
            break;
        case Format::RawMIPI:
            OsUtils::SNPrintF(suffix, sizeof(suffix), "RAWMIPI");
            break;
        case Format::RawPlain16:
            OsUtils::SNPrintF(suffix, sizeof(suffix), "RAWPLAIN16");
            break;
        case Format::RawPlain64:
            OsUtils::SNPrintF(suffix, sizeof(suffix), "RAWPLAIN64");
            break;
        case Format::RawMeta8BIT:
            OsUtils::SNPrintF(suffix, sizeof(suffix), "RAWMETA8BIT");
            break;
        case Format::UBWCTP10:
            OsUtils::SNPrintF(suffix, sizeof(suffix), "UBWCTP10");
            break;
        case Format::UBWCP010:
            OsUtils::SNPrintF(suffix, sizeof(suffix), "UBWCP010");
            break;
        case Format::UBWCNV12:
            OsUtils::SNPrintF(suffix, sizeof(suffix), "UBWCNV12");
            break;
        case Format::UBWCNV124R:
            OsUtils::SNPrintF(suffix, sizeof(suffix), "UBWCNV124R");
            break;
        case Format::YUV420NV12TP10:
            OsUtils::SNPrintF(suffix, sizeof(suffix), "YUV420NV12TP10");
            break;
        case Format::YUV420NV21TP10:
            OsUtils::SNPrintF(suffix, sizeof(suffix), "YUV420NV21TP10");
            break;
        case Format::YUV422NV16TP10:
            OsUtils::SNPrintF(suffix, sizeof(suffix), "YUV422NV16TP10");
            break;
        case Format::PD10:
            OsUtils::SNPrintF(suffix, sizeof(suffix), "PD10");
            break;
        case Format::P010:
            OsUtils::SNPrintF(suffix, sizeof(suffix), "P010");
            break;
        default:
            OsUtils::SNPrintF(suffix, sizeof(suffix), "ERROR");
            break;
    }

    OsUtils::SNPrintF(dumpFilename, sizeof(dumpFilename),
        "%s/p[%s]_req[%d]_batch[%d]_%s[%d]_port[%d]_w[%d]_h[%d]_%04d%02d%02d_%02d%02d%02d.%s",
        dataPath,
        pDumpInfo->pPipelineName,
        pDumpInfo->requestId,
        pDumpInfo->batchId,
        pDumpInfo->pNodeName,
        pDumpInfo->nodeInstance,
        pDumpInfo->portId,
        pDumpInfo->width,
        pDumpInfo->height,
        systemDateTime.year + 1900,
        systemDateTime.month + 1,
        systemDateTime.dayOfMonth,
        systemDateTime.hours,
        systemDateTime.minutes,
        systemDateTime.seconds,
        suffix);

    CAMX_LOG_INFO(CamxLogGroupUtils, "*** Image being dumped : %s ***", dumpFilename);

    FILE* pFile = OsUtils::FOpen(dumpFilename, "wb");
    UINT  batchId = pDumpInfo->batchId;

    if (pDumpInfo->numFramesInBatch == 1)
    {
        batchId = 0;
    }
    if (NULL != pFile)
    {
        for (UINT j = 0; j < numPlanes; j++)
        {
            const BYTE* pData = pDumpInfo->pBaseAddr +
                                ImageFormatUtils::CalcPlaneOffset(pDumpInfo->pFormat,
                                                                  pDumpInfo->numFramesInBatch,
                                                                  batchId,
                                                                  j);
            SIZE_T size = ImageFormatUtils::GetPlaneSize(pDumpInfo->pFormat, j);

            OsUtils::FWrite(pData, size, 1, pFile);
        }

        OsUtils::FClose(pFile);
    }
    else
    {
        CAMX_LOG_WARN(CamxLogGroupUtils, "Image Dumping failed to open for writing: %s", dumpFilename);
    }
}

static const UINT32 pow10[10] =
{
    1, 10, 100, 1000, 10000,
    100000, 1000000, 10000000, 100000000, 1000000000,
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ImageDump::InitializeWatermarkPattern
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult ImageDump::InitializeWatermarkPattern(
    WatermarkPattern* pPattern)
{
    CAMX_ASSERT(NULL != pPattern);
    CamxResult   result          = CamxResultSuccess;
    const UINT32 patternStride   = PatternGridLength * PatternGridsPerSegment;

    BYTE  pattern111[PatternSegmentSize] = { 0 };
    BYTE  pattern101[PatternSegmentSize] = { 0 };
    BYTE  pattern001[PatternSegmentSize] = { 0 };
    BYTE  pattern100[PatternSegmentSize] = { 0 };
    BYTE  pattern011[PatternSegmentSize] = { 0 };
    BYTE* pAllDigits[PatternCount]       = { 0 };

    const BYTE* pAllDigitPatterns[PatternCount][PatternSegmentCount] =
    {
        {
            pattern111,    // 0 0 0
            pattern101,    // 0   0
            pattern101,    // 0   0
            pattern101,    // 0   0
            pattern111     // 0 0 0
        },
        {
            pattern001,    //     1
            pattern001,    //     1
            pattern001,    //     1
            pattern001,    //     1
            pattern001     //     1
        },
        {
            pattern111,    // 2 2 2
            pattern001,    //     2
            pattern111,    // 2 2 2
            pattern100,    // 2
            pattern111     // 2 2 2
        },
        {
            pattern111,    // 3 3 3
            pattern001,    //     3
            pattern011,    //   3 3
            pattern001,    //     3
            pattern111     // 3 3 3
        },
        {
            pattern101,    // 4   4
            pattern101,    // 4   4
            pattern111,    // 4 4 4
            pattern001,    //     4
            pattern001     //     4
        },
        {
            pattern111,    // 5 5 5
            pattern100,    // 5
            pattern111,    // 5 5 5
            pattern001,    //     5
            pattern111     // 5 5 5
        },
        {
            pattern111,    // 6 6 6
            pattern100,    // 6
            pattern111,    // 6 6 6
            pattern101,    // 6   6
            pattern111     // 6 6 6
        },
        {
            pattern111,    // 7 7 7
            pattern001,    //     7
            pattern001,    //     7
            pattern001,    //     7
            pattern001     //     7
        },
        {
            pattern111,    // 8 8 8
            pattern101,    // 8   8
            pattern111,    // 8 8 8
            pattern101,    // 8   8
            pattern111     // 8 8 8
        },
        {
            pattern111,    // 9 9 9
            pattern101,    // 9   9
            pattern111,    // 9 9 9
            pattern001,    //     9
            pattern111     // 9 9 9
        }
    };

    for (UINT32 i = 0; i < PatternCount; i++)
    {
        pAllDigits[i] = pPattern->pattern[i];
    }

    for (UINT32 i = 0; i < PatternGridLength; i++)
    {
        memset(pattern101 + (i * patternStride) + (PatternGridLength * 1), 0xFF, PatternGridLength);
    }

    for (UINT32 i = 0; i < PatternGridLength; i++)
    {
        memset(pattern001 + (i * patternStride) + (PatternGridLength * 0), 0xFF, PatternGridLength);
        memset(pattern001 + (i * patternStride) + (PatternGridLength * 1), 0xFF, PatternGridLength);
    }

    for (UINT32 i = 0; i < PatternGridLength; i++)
    {
        memset(pattern100 + (i * patternStride) + (PatternGridLength * 1), 0xFF, PatternGridLength);
        memset(pattern100 + (i * patternStride) + (PatternGridLength * 2), 0xFF, PatternGridLength);
    }

    for (UINT32 i = 0; i < PatternGridLength; i++)
    {
        memset(pattern011 + (i * patternStride) + (PatternGridLength * 0), 0xFF, PatternGridLength);
    }

    for (UINT32 i = 0; i < PatternCount; i++)
    {
        for (UINT32 j = 0; j < PatternSegmentCount; j++)
        {
            Utils::Memcpy(pAllDigits[i] + (PatternSegmentSize * j), pAllDigitPatterns[i][j], PatternSegmentSize);
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ImageDump::Watermark
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID ImageDump::Watermark(
    const ImageDumpInfo* pDumpInfo)
{
    CAMX_ASSERT(Format::YUV420NV12 == pDumpInfo->pFormat->format);

    if ((NULL != pDumpInfo) && (NULL != pDumpInfo->pWatermarkPattern))
    {

        const BYTE* pData = pDumpInfo->pBaseAddr +
            ImageFormatUtils::CalcPlaneOffset(pDumpInfo->pFormat,
                                              pDumpInfo->numFramesInBatch,
                                              pDumpInfo->batchId,
                                              0);
        BYTE*  pBlackOut   = NULL;
        UINT32 stride      = pDumpInfo->width;
        UINT32 xOffset     = pDumpInfo->pWatermarkPattern->watermarkOffsetX;
        UINT32 yOffset     = pDumpInfo->pWatermarkPattern->watermarkOffsetY;
        UINT32 xPitch      = 80;
        UINT32 x           = xOffset;
        UINT32 y           = yOffset;
        UINT32 xZoom       = 20;
        UINT32 yZoom       = 20;
        UINT32 digitNum    = 4;
        BOOL   isDigitShow = FALSE;
        UINT32 loopH;
        UINT32 idx;
        UINT32 digitIdx;

        for (UINT32 i = 0; i < digitNum; i++)
        {
            x = xOffset + i * xPitch;
            digitIdx = (pDumpInfo->requestId / pow10[digitNum - i - 1]) % 10;
            isDigitShow |= (0 != digitIdx);
            if (TRUE == isDigitShow || ((digitNum - 1) == i))
            {
                for (loopH = y; loopH < y + 5 * yZoom; loopH++)
                {
                    // NOWHINE CP036a: debug feature
                    pBlackOut = const_cast<BYTE*>(pData) + loopH * stride + x;
                    idx = (loopH - y) * 3 * xZoom;
                    Utils::Memcpy(pBlackOut, &pDumpInfo->pWatermarkPattern->pattern[digitIdx][idx], 3 * xZoom * sizeof(BYTE));
                }
            }
        }
    }
}

CAMX_NAMESPACE_END
