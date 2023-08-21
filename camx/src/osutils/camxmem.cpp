////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2016-2017 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  camxmem.cpp
/// @brief Wrappers for memory operations
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// NOWHINE FILE CP040: This file defines memory utils.
// NOWHINE FILE CF028: Because of preprocessor switches, parens can't go on same line in many cases

#include "camxincs.h"
#include "camxmem.h"
#include "camxmemspy.h"

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Memory allocation for C and C++ code
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CamxCalloc
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID* CamxCalloc(
    SIZE_T          numBytes,
    UINT32          alignment,
    CamxMemFlags    flags,
    CamxMemType     type,
    const CHAR*     pFileName,
    UINT            lineNum)
{
    // The 'type' parameter is added here because the VS compiler requires the exact same signature for placement new[] and
    // placement delete[]. For now, we can't use it.
    CAMX_UNREFERENCED_PARAM(type);

    VOID* pMem = NULL;

    if ((numBytes > 0) &&
        ((0 == alignment) || (TRUE == CamX::Utils::IsPowerOfTwo(alignment))))
    {
        UINT32 allocAlignment   = alignment;
        SIZE_T numBytesAdjusted = numBytes;

#if CAMX_USE_MEMSPY
        // Only adjust alignment and size if we are tracking this allocation.
        if (0 == (flags & CamxMemFlagsDoNotTrack))
        {
            // Need to align allocations to multiple of page sizes so that we can protect them later
            INT pageSize = 0;
#if CAMX_DETECT_WRITE_TO_FREED_MEM
            pageSize = CamX::OsUtils::PageSize();
            if (pageSize > 0)
            {
                if (static_cast<UINT32>(pageSize) > allocAlignment)
                {
                    // Align allocation to page for later page protection
                    allocAlignment = pageSize;
                }
            }
            else
            {
                CAMX_ASSERT_ALWAYS_MESSAGE("Invalid page size returned: %d", pageSize);
            }
#endif // CAMX_DETECT_WRITE_TO_FREED_MEM

            // Add padding so that allocation has space for the tracking info, alignment, and overrun protection.
            numBytesAdjusted = CamX::MemSpy::AdjustAllocSize(numBytesAdjusted, alignment, static_cast<UINT>(pageSize));
        }
#endif // CAMX_USE_MEMSPY

        // Use the OS-appropriate function for aligned allocation
        pMem = CamX::OsUtils::MallocAligned(numBytesAdjusted, allocAlignment);

        if (NULL != pMem)
        {
            // Initialize memory to 0 if requested
            if (0 != (flags & CamxMemFlagsAllocZeroMemory))
            {
                CamX::Utils::Memset(pMem, 0x0, numBytesAdjusted);
            }

#if CAMX_USE_MEMSPY
            // Track memory with MemSpy if requested
            if (0 == (flags & CamxMemFlagsDoNotTrack))
            {
                pMem = CamX::MemSpy::TrackAlloc(pMem,
                                                numBytesAdjusted,
                                                numBytes,
                                                alignment,
                                                flags,
                                                type,
                                                pFileName,
                                                lineNum);
            }
#else // CAMX_USE_MEMSPY
            CAMX_UNREFERENCED_PARAM(pFileName);
            CAMX_UNREFERENCED_PARAM(lineNum);
#endif // CAMX_USE_MEMSPY
        }
    }

    return pMem;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CamxFree
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID CamxFree(
    VOID*           pMem,
    CamxMemFlags    flags,
    const CHAR*     pFileName,
    UINT            lineNum)
{
    // C and C++ require delete/free NULL pointers to be silently ignored
    if (NULL != pMem)
    {
        VOID* pMemAdjusted = pMem;

#if CAMX_USE_MEMSPY
        if (0 == (flags & CamxMemFlagsDoNotTrack))
        {
            pMemAdjusted = CamX::MemSpy::TrackFree(pMem,
                                                   flags,
                                                   pFileName,
                                                   lineNum);
        }
#else // CAMX_USE_MEMSPY
        CAMX_UNREFERENCED_PARAM(flags);
        CAMX_UNREFERENCED_PARAM(pFileName);
        CAMX_UNREFERENCED_PARAM(lineNum);
#endif // CAMX_USE_MEMSPY

        CamX::OsUtils::FreeAligned(pMemAdjusted);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Memory allocation for C++ code only
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// new/delete Overrides for Clang
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#if __clang__
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Placement operator new
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID* NEWDECL operator new(
    size_t          numBytes,
    CamxMemFlags    flags,
    CamxMemType     type,
    const CHAR*     pFilename,
    UINT            lineNum)
{
    return CamxCalloc(
        numBytes,
        0,
        flags | CamxMemFlagsAllocZeroMemory | CamxMemFlagsNew,
        type,
        pFilename,
        lineNum
        );
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// operator new
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID* NEWDECL operator new(
    size_t numBytes)
{
    return CamxCalloc(
        numBytes,
        0,
        CamxMemFlagsAllocZeroMemory | CamxMemFlagsDoNotTrack | CamxMemFlagsNew,
        CamxMemTypeAny,
        NULL,
        0);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Placement operator new[]
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID* NEWDECL operator new[](
    size_t          numBytes,
    CamxMemFlags    flags,
    CamxMemType     type,
    const CHAR*     pFilename,
    UINT            lineNum
    )
{
    return CamxCalloc(
        numBytes,
        0,
        flags | CamxMemFlagsAllocZeroMemory | CamxMemFlagsArray | CamxMemFlagsNew,
        type,
        pFilename,
        lineNum
        );
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// operator new[]
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID* NEWDECL operator new[](
    size_t numBytes)
{
    return CamxCalloc(
        numBytes,
        0,
        CamxMemFlagsAllocZeroMemory | CamxMemFlagsDoNotTrack | CamxMemFlagsArray | CamxMemFlagsNew,
        CamxMemTypeAny,
        NULL,
        0);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Placement operator delete
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID NEWDECL operator delete(
    VOID*           pObject,
    CamxMemFlags    flags,
    CamxMemType     type,
    const CHAR*     pFilename,
    UINT            lineNum
    )
{
    // The 'type' parameter is added here because the VS compiler requires the exact same signature for placement new and
    // placement delete. For now, we can't use it.
    CAMX_UNREFERENCED_PARAM(type);

    CamxFree(
        pObject,
        flags | CamxMemFlagsDelete,
        pFilename,
        lineNum
        );
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// operator delete
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID NEWDECL operator delete(
    VOID* pObject)
{
    CamxFree(
        pObject,
        CamxMemFlagsDelete,
        NULL,
        0);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// operator delete[]
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID NEWDECL operator delete[](
    VOID*           pObject,
    CamxMemFlags    flags,
    CamxMemType     type,
    const CHAR*     pFilename,
    UINT            lineNum
    )
{
    // The 'type' parameter is added here because the VS compiler requires the exact same signature for placement new[] and
    // placement delete[]. For now, we can't use it.
    CAMX_UNREFERENCED_PARAM(type);

    CamxFree(
        pObject,
        flags | CamxMemFlagsArray | CamxMemFlagsDelete,
        pFilename,
        lineNum
        );
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// operator delete[]
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID NEWDECL operator delete[](
    VOID* pObject)
{
    return CamxFree(
        pObject,
        CamxMemFlagsArray | CamxMemFlagsDelete,
        NULL,
        0);
}

#endif // __clang__
