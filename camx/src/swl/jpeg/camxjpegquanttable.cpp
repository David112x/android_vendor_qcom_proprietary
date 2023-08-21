////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2016-2018 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  camxjpegquanttable.cpp
/// @brief Quantization table class implementation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/// @todo (CAMX-1988) Code clean up apply NC001 coding standard on entire project
/// @todo (CAMX-1989) Code clean up apply GR030 coding standard on entire project

#include "camxjpegquanttable.h"
#include "camxincs.h"

CAMX_NAMESPACE_BEGIN

/// default Luma Qtable
static const UINT16 DefaultQuantTableLuma[QuantTableSize] =
{
    16, 11, 10, 16, 24, 40, 51, 61,
    12, 12, 14, 19, 26, 58, 60, 55,
    14, 13, 16, 24, 40, 57, 69, 56,
    14, 17, 22, 29, 51, 87, 80, 62,
    18, 22, 37, 56, 68, 109, 103, 77,
    24, 35, 55, 64, 81, 104, 113, 92,
    49, 64, 78, 87, 103, 121, 120, 101,
    72, 92, 95, 98, 112, 100, 103, 99
};

/// default Chroma Qtable
static const UINT16 DefaultQuantTableChroma[QuantTableSize] =
{
    17, 18, 24, 47, 99, 99, 99, 99,
    18, 21, 26, 66, 99, 99, 99, 99,
    24, 26, 56, 99, 99, 99, 99, 99,
    47, 66, 99, 99, 99, 99, 99, 99,
    99, 99, 99, 99, 99, 99, 99, 99,
    99, 99, 99, 99, 99, 99, 99, 99,
    99, 99, 99, 99, 99, 99, 99, 99,
    99, 99, 99, 99, 99, 99, 99, 99
};

static const UINT32 MinJpegQuality    = 1;    ///< Min Jpeg quality
static const UINT32 MaxJpegQuality    = 98;   ///< Max Jpeg quality
static const UINT32 MinJpegQuantValue = 1;    ///< Min Jpeg quantization value
static const UINT32 MaxJpegQuantValue = 255;  ///< Max Jpeg quantization value

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// JPEGQuantTable::JPEGQuantTable
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
JPEGQuantTable::JPEGQuantTable()
{
    m_type = QuantTableType::QuantTableLuma;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// JPEGQuantTable::JPEGQuantTable
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
JPEGQuantTable::JPEGQuantTable(
    QuantTableType type)
{
    m_type = type;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// JPEGQuantTable::~JPEGQuantTable
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
JPEGQuantTable::~JPEGQuantTable()
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// JPEGQuantTable::UpdateTable
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void JPEGQuantTable::UpdateTable(
    UINT32  quality)
{
    UINT32 i;
    DOUBLE scaleFactor;

    quality = Utils::ClampUINT32(quality, MinJpegQuality, MaxJpegQuality);

    if (50 == quality)
    {
        return;
    }

    if (quality > 50)
    {
        scaleFactor = 50.0 / static_cast <DOUBLE>(100 - quality);
    }
    else
    {
        scaleFactor = static_cast <DOUBLE>(quality / 50.0);
    }

    /// Scale quant entries
    for (i = 0; i < QuantTableSize; i++)
    {
        /// Compute new value based on input percent
        /// and on the 50% table (low)
        /// Add 0.5 after the divide to round up fractional divides to be
        /// more conservative.
        m_table[i] = static_cast <UINT16>((static_cast <DOUBLE>(m_table[i] / scaleFactor)) + 0.5);

        /// Clamp
        m_table[i] = static_cast <UINT16>(Utils::ClampUINT32(m_table[i], MinJpegQuantValue, MaxJpegQuantValue));
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// JPEGQuantTable::SetDefaultTable
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void JPEGQuantTable::SetDefaultTable(
    UINT32  quality)
{
    const UINT16* pTable = (m_type == QuantTableType::QuantTableLuma) ? DefaultQuantTableLuma : DefaultQuantTableChroma;
    Utils::Memcpy(m_table, pTable, QuantTableSize * sizeof(UINT16));
    UpdateTable(quality);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// JPEGQuantTable::SetTableValue
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult JPEGQuantTable::SetTable(
    UINT16*  pTable)
{
    CamxResult result = CamxResultSuccess;

    if (NULL == pTable)
    {
        CAMX_LOG_ERROR(CamxLogGroupJPEG, "invalid table pointer");
        result = CamxResultEInvalidArg;
    }

    if (CamxResultSuccess == result)
    {
        Utils::Memcpy(m_table, pTable, QuantTableSize * sizeof(UINT16));
    }

    return result;
}

CAMX_NAMESPACE_END
