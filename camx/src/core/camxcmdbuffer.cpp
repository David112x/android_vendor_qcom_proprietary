////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2016-2017 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  camxcmdbuffer.cpp
/// @brief CmdBuffer class implementation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "camxcmdbuffer.h"
#include "camxincs.h"
#include "camxmem.h"


CAMX_NAMESPACE_BEGIN

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CmdBuffer::Create
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CmdBuffer::Create(
    const CmdParams*        pParams,
    const CSLBufferInfo*    pBufferInfo,
    SIZE_T                  offset,
    SIZE_T                  size,
    CmdBuffer**             ppCmdBufferOut)
{
    CamxResult  result      = CamxResultSuccess;
    CmdBuffer*  pCmdBuffer  = NULL;

    if (NULL == ppCmdBufferOut)
    {
        CAMX_LOG_ERROR(CamxLogGroupUtils, "ppCmdBufferOut is NULL.");
        result = CamxResultEInvalidArg;
    }
    else
    {
        pCmdBuffer = CAMX_NEW CmdBuffer();
        if (NULL == pCmdBuffer)
        {
            CAMX_LOG_ERROR(CamxLogGroupUtils, "Out of memory; cannot create CmdBuffer");
            result = CamxResultENoMemory;
        }
        else
        {
            result = pCmdBuffer->Initialize(pParams, pBufferInfo, offset, size);
        }
    }

    if (CamxResultSuccess == result)
    {
        *ppCmdBufferOut = pCmdBuffer;
    }
    else
    {
        if (NULL != pCmdBuffer)
        {
            CAMX_DELETE pCmdBuffer;
            pCmdBuffer = NULL;
        }
    }
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CmdBuffer::Destroy
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID CmdBuffer::Destroy()
{
    CAMX_DELETE this;
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CmdBuffer::CmdBuffer
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CmdBuffer::CmdBuffer()
{
    m_pendingDwords         = 0;
    m_resourceUsedDwords    = 0;
    m_numNestedBuffers      = 0;
    m_pNestedBuffersInfo    = NULL;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CmdBuffer::~CmdBuffer
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CmdBuffer::~CmdBuffer()
{
    if (NULL != m_pNestedBuffersInfo)
    {
        CAMX_FREE(m_pNestedBuffersInfo);
        m_pNestedBuffersInfo = NULL;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CmdBuffer::Initialize
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CmdBuffer::Initialize(
    const CmdParams*        pParams,
    const CSLBufferInfo*    pBufferInfo,
    SIZE_T                  offset,
    SIZE_T                  size)
{
    CamxResult result = PacketResource::Initialize(pBufferInfo, offset, size);
    if (CamxResultSuccess == result)
    {
        CAMX_ASSERT(NULL != pParams);

        if ((TRUE == pParams->enableAddrPatching) && (TRUE == pParams->mustInlineIndirectBuffers))
        {
            CAMX_LOG_ERROR(CamxLogGroupUtils, "Can't have both inlining and patching at the same time. Ignoring patching.");
            result = CamxResultEInvalidArg;
        }
        else if ((TRUE == pParams->enableAddrPatching) && (0 == pParams->maxNumNestedAddrs))
        {
            CAMX_LOG_ERROR(CamxLogGroupUtils, "enableAddrPatching is true but maxNumNestedAddrs is 0");
            result = CamxResultEInvalidArg;
        }
        else
        {
            m_params    = *pParams;
            m_type      = pParams->type;

            if (pParams->maxNumNestedAddrs > 0)
            {
                m_pNestedBuffersInfo =
                    static_cast<NestedAddrInfo*>(CAMX_CALLOC(pParams->maxNumNestedAddrs * sizeof(NestedAddrInfo)));
                if (NULL == m_pNestedBuffersInfo)
                {
                    CAMX_LOG_ERROR(CamxLogGroupUtils, "Out of memory");
                    result = CamxResultENoMemory;
                }
            }
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CmdBuffer::Reset
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID CmdBuffer::Reset()
{
    m_resourceUsedDwords    = 0;
    m_pendingDwords         = 0;
    m_numNestedBuffers      = 0;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CmdBuffer::BeginCommands
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID* CmdBuffer::BeginCommands(
    UINT32 numDwordsToReserve)
{
    VOID* pDwords = NULL;

    if ((GetNumDwordsAvailable() >= numDwordsToReserve) && (0 != numDwordsToReserve))
    {
        pDwords = GetCurrentWriteAddr();
        m_pendingDwords = numDwordsToReserve;
        // Put a canary to check in commit for overrun
        // This requires that we allocate one extra dword for command buffers (in command buffer manager)
        *static_cast<UINT*>(Utils::VoidPtrInc(pDwords, numDwordsToReserve * sizeof(UINT32))) = CamxCanary;
    }
    else
    {
        CAMX_LOG_ERROR(CamxLogGroupUtils, "Failed to reserve words: %d room on command buffer", numDwordsToReserve);
    }

    return pDwords;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CmdBuffer::CommitCommands
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CmdBuffer::CommitCommands()
{
    CamxResult result = CamxResultSuccess;

    CAMX_ASSERT_MESSAGE(GetNumDwordsAvailable() >= m_pendingDwords,
                        "Pending dwords: %d to commit is greater than available dwords: %d",
                        m_pendingDwords, GetNumDwordsAvailable());

    if (0 >= m_pendingDwords)
    {
        CAMX_LOG_ERROR(CamxLogGroupUtils, "Invalid Command buffer state pending dwords: %d",
                       m_pendingDwords);
        result = CamxResultEInvalidState;
    }
    else
    {
        // check for overrun
        CAMX_ASSERT(CamxCanary == *static_cast<UINT*>(Utils::VoidPtrInc(GetCurrentWriteAddr(),
                                                                        m_pendingDwords * sizeof(UINT32))));

        m_resourceUsedDwords += m_pendingDwords;
        m_pendingDwords = 0;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CmdBuffer::CancelCommands
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CmdBuffer::CancelCommands()
{
    CamxResult result = CamxResultSuccess;

    m_pendingDwords = 0;

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CmdBuffer::GetCmdBufferDesc
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CmdBuffer::GetCmdBufferDesc(
    CSLCmdMemDesc* pCmdDescOut)
{
    CamxResult result = CamxResultSuccess;

    if (NULL == pCmdDescOut)
    {
        CAMX_LOG_ERROR(CamxLogGroupUtils, "pCmdDescOut is NULL.");
        result = CamxResultEInvalidArg;
    }
    else
    {
        pCmdDescOut->hMem       = GetMemHandle();
        pCmdDescOut->offset     = static_cast<UINT32>(GetOffset());
        pCmdDescOut->size       = static_cast<UINT32>(GetMaxLength());
        pCmdDescOut->length     = static_cast<UINT32>(m_resourceUsedDwords * sizeof(UINT32));
        pCmdDescOut->type       = static_cast<UINT32>(GetType());
        pCmdDescOut->metadata   = m_metadata;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CmdBuffer::AddNestedCmdBufferInfo
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CmdBuffer::AddNestedCmdBufferInfo(
    UINT32      dstOffset,
    CmdBuffer*  pCmdBuffer,
    UINT32      srcOffset)
{
    CamxResult result = CamxResultSuccess;

    if (0 == m_params.enableAddrPatching)
    {
        CAMX_LOG_ERROR(CamxLogGroupUtils, "Command buffer was created without patching support.");
        result = CamxResultEUnsupported;
    }
    else if (m_numNestedBuffers >= m_params.maxNumNestedAddrs)
    {
        CAMX_LOG_ERROR(CamxLogGroupUtils, "Number of immediately-nested command buffers: %d is greater than MAX: %d",
                       m_numNestedBuffers, m_params.maxNumNestedAddrs);
        result = CamxResultEOutOfBounds;
    }
    else if (NULL == pCmdBuffer)
    {
        CAMX_LOG_ERROR(CamxLogGroupUtils, "Command buffer is NULL.");
        result = CamxResultEInvalidArg;
    }
    else
    {
        CAMX_ASSERT(m_pNestedBuffersInfo != NULL);

        m_pNestedBuffersInfo[m_numNestedBuffers].isCmdBuffer  = TRUE;
        m_pNestedBuffersInfo[m_numNestedBuffers].dstOffset    = dstOffset;
        m_pNestedBuffersInfo[m_numNestedBuffers].pCmdBuffer   = pCmdBuffer;
        m_pNestedBuffersInfo[m_numNestedBuffers].srcOffset    = srcOffset;

        m_numNestedBuffers++;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CmdBuffer::AddNestedBufferInfo
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CmdBuffer::AddNestedBufferInfo(
    UINT32          dstOffset,
    CSLMemHandle    hMem,
    UINT32          srcOffset)
{
    CamxResult result = CamxResultSuccess;

    if (0 == m_params.enableAddrPatching)
    {
        CAMX_LOG_ERROR(CamxLogGroupUtils, "Command buffer was created without patching support.");
        result = CamxResultEUnsupported;
    }
    else if (m_numNestedBuffers >= m_params.maxNumNestedAddrs)
    {
        CAMX_LOG_ERROR(CamxLogGroupUtils, "Number of immediately-nested command buffers: %d is greater than MAX: %d",
                       m_numNestedBuffers, m_params.maxNumNestedAddrs);
        result = CamxResultEOutOfBounds;
    }
    else if (CSLInvalidHandle == hMem)
    {
        CAMX_LOG_ERROR(CamxLogGroupUtils, "Invalid arg");
        result = CamxResultEInvalidArg;
    }
    else
    {
        CAMX_ASSERT(m_pNestedBuffersInfo != NULL);

        m_pNestedBuffersInfo[m_numNestedBuffers].isCmdBuffer  = FALSE;
        m_pNestedBuffersInfo[m_numNestedBuffers].dstOffset    = dstOffset;
        m_pNestedBuffersInfo[m_numNestedBuffers].hSrcBuffer   = hMem;
        m_pNestedBuffersInfo[m_numNestedBuffers].srcOffset    = srcOffset;

        m_numNestedBuffers++;
    }

    return result;
}

CAMX_NAMESPACE_END
