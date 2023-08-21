////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2016, 2018 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  camxhal3stream.cpp
/// @brief Definitions for HAL3Stream class.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "camxhal3stream.h"
#include "camxutils.h"

CAMX_NAMESPACE_BEGIN

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// HAL3Stream::HAL3Stream
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
HAL3Stream::HAL3Stream(
    Camera3Stream* pStream,
    const UINT32   streamIndex,
    Format         bufferFormat)
    : m_pStream(pStream)
    , m_streamIndex(streamIndex)
    , m_streamReused(FALSE)
    , m_selectedFormat(bufferFormat)
    , m_HDRMode(HDRModeMax)
{
    Utils::Memset(&m_hidlStream, 0, sizeof(m_hidlStream));
    pStream->pHalStream = &m_hidlStream;
    pStream->pHalStream->overrideFormat = pStream->format;
    if (pStream->streamType == StreamTypeInput)
    {
        pStream->pHalStream->consumerUsage = pStream->grallocUsage;
        pStream->pHalStream->producerUsage = 0;
    }
    else
    {
        pStream->pHalStream->producerUsage = pStream->grallocUsage;
        pStream->pHalStream->consumerUsage = 0;
    }

}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// HAL3Stream::~HAL3Stream
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
HAL3Stream::~HAL3Stream()
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// HAL3Stream::IsStreamConfigMatch
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL HAL3Stream::IsStreamConfigMatch(
    const Camera3Stream* pStream
    ) const
{
    BOOL isMatch = TRUE;

    if (pStream != m_pStream)
    {
        isMatch = FALSE;
    }
    else if (pStream->streamType != m_pStream->streamType)
    {
        isMatch = FALSE;
    }
    else if (pStream->width != m_pStream->width)
    {
        isMatch = FALSE;
    }
    else if (pStream->height != m_pStream->height)
    {
        isMatch = FALSE;
    }
    else if (pStream->format != m_pStream->format)
    {
        isMatch = FALSE;
    }
    else if (pStream->dataspace != m_pStream->dataspace)
    {
        isMatch = FALSE;
    }
    else if (pStream->rotation != m_pStream->rotation)
    {
        isMatch = FALSE;
    }

    return isMatch;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// HAL3Stream::GetGrallocUsage
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
GrallocUsage64 HAL3Stream::GetGrallocUsage()
{
    GrallocUsage64 grallocUsage = 0;
    if (m_pStream->pHalStream != NULL)
    {
        if ((StreamTypeOutput == m_pStream->streamType) ||
                (StreamTypeBidirectional == m_pStream->streamType))
        {
            grallocUsage = m_pStream->pHalStream->producerUsage;
        }
        else if (StreamTypeInput == m_pStream->streamType)
        {
            grallocUsage = m_pStream->pHalStream->consumerUsage;
        }
        else
        {
            grallocUsage = m_pStream->grallocUsage;
        }
    }
    else
    {
        grallocUsage = m_pStream->grallocUsage;
        CAMX_LOG_VERBOSE(CamxLogGroupChi,
                "Gralloc Usage is 32 bit here: 0x%llx", grallocUsage);
    }
    return grallocUsage;
}

CAMX_NAMESPACE_END
