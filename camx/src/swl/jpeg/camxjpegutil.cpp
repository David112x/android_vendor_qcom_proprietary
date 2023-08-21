////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2017 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  camxjpegutil.cpp
/// @brief JPEG util class implementation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "camxjpegutil.h"
#include "camxjpegexifdefs.h"
#include "camxjpegexifparams.h"

CAMX_NAMESPACE_BEGIN

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// JPEGUtil::SetDefaultQuantizationTables
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult JPEGUtil::SetDefaultQuantizationTables(
    JPEGQuantTable* const pQuantizationTables)
{
    UINT32      minTableIndex = static_cast<UINT32>(QuantTableType::QuantTableMin);
    UINT32      maxTableIndex = static_cast<UINT32>(QuantTableType::QuantTableMax);
    CamxResult  result      = CamxResultSuccess;

    if (NULL != pQuantizationTables)
    {
        for (UINT32 i = minTableIndex; i < maxTableIndex; i++)
        {
            pQuantizationTables[i].SetType(static_cast<QuantTableType>(i));
            pQuantizationTables[i].SetDefaultTable(DefaultQuality);
        }
    }
    else
    {
        CAMX_LOG_ERROR(CamxLogGroupJPEG, "Pointer to Quantization table %p", pQuantizationTables);
        result = CamxResultEInvalidPointer;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// JPEGUtil::UpdateQuantizationTableQuality
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult JPEGUtil::UpdateQuantizationTableQuality(
    JPEGQuantTable* pQuantizationTables,
    UINT32          quality)
{
    UINT32      minTableIdx = static_cast<UINT32>(QuantTableType::QuantTableMin);
    UINT32      maxTableIdx = static_cast<UINT32>(QuantTableType::QuantTableMax);
    CamxResult  result      = CamxResultSuccess;

    if (NULL != pQuantizationTables)
    {
        for (UINT32 i = minTableIdx; i < maxTableIdx; i++)
        {
            pQuantizationTables[i].UpdateTable(quality);
        }
    }
    else
    {
        CAMX_LOG_ERROR(CamxLogGroupJPEG, "Pointer to Quantization table %p", pQuantizationTables);
        result = CamxResultEInvalidPointer;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// JPEGUtil::SetDefaultHuffmanTables
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult JPEGUtil::SetDefaultHuffmanTables(
    JPEGHuffTable* const pHuffmanTables)
{
    UINT32      minTableIndex = static_cast<UINT32>(HuffTableType::HuffTableMin);
    UINT32      maxTableIndex = static_cast<UINT32>(HuffTableType::HuffTableMax);
    CamxResult  result      = CamxResultSuccess;

    if (NULL != pHuffmanTables)
    {
        for (UINT32 i = minTableIndex; i < maxTableIndex; i++)
        {
            pHuffmanTables[i].SetType(static_cast<HuffTableType>(i));
            pHuffmanTables[i].SetDefaultTable();
        }
    }
    else
    {
        CAMX_LOG_ERROR(CamxLogGroupJPEG, "Pointer to Huffman table %p", pHuffmanTables);
        result = CamxResultEInvalidPointer;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// JPEGUtil::GetUnsignedRational
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult JPEGUtil::GetUnsignedRational(
    URAT32* pRational,
    INT32   numerator,
    INT32   denominator)
{
    CamxResult result = CamxResultSuccess;

    if ((NULL != pRational) && (0 <= numerator) && (0 < denominator))
    {
        pRational->numerator   = static_cast<UINT32>(numerator);
        pRational->denominator = static_cast<UINT32>(denominator);
    }
    else
    {
        CAMX_LOG_ERROR(CamxLogGroupJPEG,
                       "Invalid input pointer %p numerator %d denominator %d",
                       pRational,
                       numerator,
                       denominator);
        result = CamxResultEInvalidArg;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// JPEGUtil::GetSignedRational
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult JPEGUtil::GetSignedRational(
    RAT32* pRational,
    INT32  numerator,
    INT32  denominator)
{
    CamxResult result = CamxResultSuccess;

    if ((NULL != pRational) && (0 <= numerator) && (0 < denominator))
    {
        pRational->numerator    = numerator;
        pRational->denominator  = denominator;
    }
    else
    {
        CAMX_LOG_ERROR(CamxLogGroupJPEG,
            "Invalid input pointer %p numerator %d denominator %d",
            pRational,
            numerator,
            denominator);
        result = CamxResultEInvalidArg;
    }

    return result;
}

CAMX_NAMESPACE_END
