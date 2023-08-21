////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2016-2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file camxmemspy.cpp
///
/// @brief CamX MemSpy definitions. MemSpy is used to track memory allocations.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "camxincs.h"
#include "camxmem.h"

#include "camxmemspy.h"

// Only compile in if enabled
#if CAMX_USE_MEMSPY

CAMX_NAMESPACE_BEGIN

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Constants needed for type definitions
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// File name constants
static const UINT MemSpyMaxFilenameSize     = 31;
static const UINT MemSpyFilenameBufferSize  = MemSpyMaxFilenameSize + 1;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Type definitions
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/// A handle to an entry in the list that tracks allocations
typedef VOID* MemSpyEntryHandle;

/// @brief Encapsulates tracking information
struct MemSpyTrackingInfo
{
    UINT64 headerSignature1;    ///< Header signature value to detect invalid memory operations
};

/// @brief Memspy allocation callsite descriptor
struct MemSpyAllocCallsiteDesc
{
    CHAR        filename[MemSpyFilenameBufferSize]; ///< The last MemSpyMaxFilenameSize chars of the filename of the
                                                    ///  allocation callsite
    UINT        lineNum;                            ///< Line of allocation callsite
    CamxMemType type;                               ///< Type of allocation
    SIZE_T      numBytesInUse;                      ///< Bytes of memory currently in use from this allocation callsite
    UINT        numAllocsInUse;                     ///< Number of allocations currently in use from this allocation callsite
    UINT        numAllocsLifetime;                  ///< Number of lifetime allocations from this allocation callsite
    SIZE_T      numBytesWatermark;                  ///< Largest numBytesInUse for lifetime of allocation callsite
    UINT        numAllocsWatermark;                 ///< Number of allocations that make up numBytesWatermark
    UINT64      lastFreeTime;                       ///< Timestamp of last allocation freed by this allocation callsite
    UINT        numLastFreeAllocs;                  ///< Number of allocations freed during the last timestamp
    UINT64      totalLiveTime;                      ///< Total live time for all allocations on this allocation callsite
};

/// @brief Memspy allocation callsite descriptor key
struct MemSpyAllocCallsiteDescKey
{
    const CHAR* pFileName;                          ///< File name where allocation occurs
    UINT        lineNum;                            ///< Line number where allocation occurs
};

/// @brief Memspy allocation descriptor
struct MemSpyAllocDesc
{
    MemSpyAllocCallsiteDesc*    pAllocCallsiteDesc; ///< Descriptor for this allocation callsite
    VOID*                       pOriginalPtr;       ///< Pointer to originally allocated memory
    VOID*                       pClientPtr;         ///< Pointer that was given to the caller
    SIZE_T                      allocSize;          ///< Actual size of allocation after adding padding
    SIZE_T                      clientSize;         ///< Size of allocation requested by client
    BOOL                        expectDelete;       ///< TRUE if allocation should be freed with delete
    BOOL                        expectArray;        ///< TRUE if allocation should be freed with the array variant
    MemSpy*                     pMemSpyOwner;       ///< MemSpy instance that created this allocation
    UINT64                      allocTime;          ///< Timestamp that this allocation was created
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Constants
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Signatures used to detect invalid memory operations
static const UINT64 MemSpyFreeHeaderSignature   = 0xBADA110CBADA110C;   ///< Used after a buffer is freed to detect another free
static const UINT64 MemSpyHeaderSignature1      = 0xD0D0FACED0D0FACE;   ///< Used in a tracking structure to detect corruption
static const UINT64 MemSpyOverrunSignature      = 0xDEADBEEFDEADBEEF;   ///< Used at the end of a buffer to detect overruns

// Heuristics for reports
/// @todo (CAMX-471): Add heuristic thresholds to settings
/// @todo (CAMX-453): Empirically determine the heuristic thresholds used in reports
static const UINT   MemSpyMaxLeakStats          = 50;       ///< Controls the number of memory leak stats to report
static const FLOAT  MemSpyFreedAtEndThreshold   = 0.25f;    ///< The threshold for the ratio of the number of allocs freed at
                                                            ///  the last timestamp to the total number of allocs. If greater
                                                            ///  than the threshold, it may be an indication that excessive
                                                            ///  accumulation is occurring (since significant freeing is done at
                                                            ///  the end) and a memory accumulation report is printed.
static const FLOAT  MemSpyAvgPtrTimeThreshold   = 0.25f;    ///< The threshold for the ratio of a specific allocation callsite's
                                                            ///  average pointer lifetime to the total time. If greater than the
                                                            ///  threshold, it may be an indication that excessive accumlation
                                                            ///  is occurring (since a significant number of objects are
                                                            ///  staying around for a long time) and a memory accumulation
                                                            ///  report is printed.
static const FLOAT  MemSpyWatermarkThreshold    = 0.075f;   ///< The threshold for the percentage of the total-bytes-used-high-
                                                            ///  watermark that is taken up by a single allocation callsite. If
                                                            ///  greater than the threshold, it may be an indication that
                                                            ///  excessive accumulation is occurring (since a single callsite
                                                            ///  allocating a large portion of the high watermark may be
                                                            ///  suspicious) and a memory accumulation report is printed.
static const UINT   MemSpyNumAllocsThreshold    = 10;       ///< The threshold for number of allocations from a specific
                                                            ///  callsite at which a memory accumulation report is printed.

// This is the additional amount added to an allocation request in order to track allocations and detect memory corruption.
static const UINT MemSpySizeOfOverhead = sizeof(MemSpyTrackingInfo) + sizeof(MemSpyOverrunSignature);

#if CAMX_DETECT_WRITE_TO_FREED_MEM
// Constants for detection of writing to freed memory
/// @todo (CAMX-454): Empirically determine the maximum allocation size (and number) in the freed list. Depends on how much
///                   memory we are willing to give up.
static const SIZE_T FreedListMaxAllocSize   = 16384;    ///< The maximum size of a buffer to keep on the internal freed list
                                                        ///  (i.e. to not free back to the OS).
static const UINT   FreedListMaxAllocations = 5000;     ///< The maximum number of buffers to keep on the internal freed list
                                                        ///  (i.e. to not free back to the OS).
#endif // CAMX_DETECT_WRITE_TO_FREED_MEM

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Static Data
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// This is a flag indicating whether the MemSpy singleton is in a valid state
BOOL MemSpy::s_isValid = FALSE;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Static Methods
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CamxMemTypeToString
///
/// @brief  Returns the string representation of the memory type
///
/// @param  memType The type of memory to convert to a string
///
/// @return String representation of the memory type
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static const CHAR* CamxMemTypeToString(
    CamxMemType memType)
{
    const CHAR* pResult = CamxMemTypeStrings[CamxMemTypeAny];

    if (memType < CamxMemTypeCount)
    {
        pResult = CamxMemTypeStrings[memType];
    }

    return pResult;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CheckSignatures
///
/// @brief  Checks the validty of a buffer's signatures and prints a report if the signatures are invalid.
///
/// @param  signature1  First signature to check
/// @param  pClientPtr  The pointer returned to the client from TrackAlloc()
/// @param  rAllocDesc  The descriptor for the allocation to check
/// @param  pFileName   Name of the file in which the free was called
/// @param  lineNum     Line number at which the free was called
///
/// @return None
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static VOID CheckSignatures(
    UINT64              signature1,
    const VOID*         pClientPtr,
    MemSpyAllocDesc&    rAllocDesc,
    const CHAR*         pFileName,
    UINT                lineNum)
{
    BOOL isValidCallsiteDesc = (NULL != rAllocDesc.pAllocCallsiteDesc);

    if (MemSpyHeaderSignature1 != signature1)
    {
        // If the signatures are invalid, assert
        CAMX_ASSERT_ALWAYS();

        CAMX_LOG_ERROR(CamxLogGroupMemSpy,
                       "============== Invalid Signature Detection Report ==============");

        if (MemSpyFreeHeaderSignature == signature1)
        {
            CAMX_LOG_ERROR(CamxLogGroupMemSpy, "Detected Double Free!");
        }

        CAMX_LOG_ERROR(CamxLogGroupMemSpy,
                       "Alloc File: %s, LineNum: %d, Type: %s, Ptr: 0x%p, Size: %d",
                       isValidCallsiteDesc ? rAllocDesc.pAllocCallsiteDesc->filename : "<Unknown>",
                       isValidCallsiteDesc ? rAllocDesc.pAllocCallsiteDesc->lineNum : -1,
                       isValidCallsiteDesc ? CamxMemTypeToString(rAllocDesc.pAllocCallsiteDesc->type) : "<Unknown>",
                       rAllocDesc.pClientPtr,
                       rAllocDesc.allocSize);

        CAMX_LOG_ERROR(CamxLogGroupMemSpy,
                       "Free File: %s, LineNum: %d, Ptr: 0x%p, Sig1: 0x%x",
                       (NULL != pFileName) ? pFileName : "<Unknown>",
                       lineNum,
                       pClientPtr,
                       signature1);

        CAMX_LOG_ERROR(CamxLogGroupMemSpy,
                       "=========== End of Invalid Signature Detection Report ==========");
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CheckMemSpyOwner
///
/// @brief  Checks if a buffer is freed by the same Memspy instance that allocated it and prints a report if a mismatch is
///         detected.
///
/// @note   MemSpy instances are owned per shared library. Due to shared library lifecycles, an allocation must be freed by the
///         same shared library that allocated it or tracking may not work right (especially if allocation out lives the library
///         that allocated it we'd get a false error).
///
/// @param  pMemSpy     The MemSpy instance freeing the allocation
/// @param  pAllocDesc  The descriptor for the allocation to check
/// @param  pFileName   Name of the file in which the free was called
/// @param  lineNum     Line number at which the free was called
///
/// @return None
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static VOID CheckMemSpyOwner(
    MemSpy*          pMemSpy,
    MemSpyAllocDesc& rAllocDesc,
    const CHAR*      pFileName,
    UINT             lineNum)
{
    BOOL isValidCallsiteDesc = (NULL != rAllocDesc.pAllocCallsiteDesc);

    if (rAllocDesc.pMemSpyOwner != pMemSpy)
    {
        CAMX_ASSERT_ALWAYS();

        CAMX_LOG_WARN(CamxLogGroupMemSpy,
                      "================ Owner Mismatch Detection Report ===============");

        CAMX_LOG_WARN(CamxLogGroupMemSpy,
                      "Alloc File: %s, LineNum: %d, Type: %s, Ptr: 0x%p, Size: %d, Owner: 0x%p",
                      isValidCallsiteDesc ? rAllocDesc.pAllocCallsiteDesc->filename : "<Unknown>",
                      isValidCallsiteDesc ? rAllocDesc.pAllocCallsiteDesc->lineNum : -1,
                      isValidCallsiteDesc ? CamxMemTypeToString(rAllocDesc.pAllocCallsiteDesc->type) : "<Unknown>",
                      rAllocDesc.pClientPtr,
                      rAllocDesc.allocSize,
                      rAllocDesc.pMemSpyOwner);

        CAMX_LOG_WARN(CamxLogGroupMemSpy,
                      "Free File: %s, LineNum: %d, Owner: 0x%p",
                      (NULL != pFileName) ? pFileName : "<Unknown>",
                      lineNum,
                      pMemSpy);

        CAMX_LOG_WARN(CamxLogGroupMemSpy,
                      "============= End of Owner Mismatch Detection Report ===========");
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CheckForOverrun
///
/// @brief  Checks to see whether the buffer was over-run and prints a report if a mismatch is detected.
///
/// @param  pClientPtr  The pointer returned to the client from TrackAlloc()
/// @param  rAllocDesc  The descriptor for the allocation to check if allocate and free calls match
/// @param  pFileName   Name of the file in which the free was called
/// @param  lineNum     Line number at which the free was called
///
/// @return None
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static VOID CheckForOverrun(
    const VOID*      pClientPtr,
    MemSpyAllocDesc& rAllocDesc,
    const CHAR*      pFileName,
    UINT             lineNum)
{
    BOOL isValidCallsiteDesc = (NULL != rAllocDesc.pAllocCallsiteDesc);

    // Check for a buffer overrun
    SIZE_T sizePadded       = CamX::Utils::ByteAlign(rAllocDesc.clientSize, alignof(UINT64));
    UINT64 overrunSignature = *static_cast<const UINT64*>(Utils::ConstVoidPtrInc(pClientPtr, sizePadded));
    if (MemSpyOverrunSignature != overrunSignature)
    {
        CAMX_ASSERT_ALWAYS();

        CAMX_LOG_ERROR(CamxLogGroupMemSpy,
                      "==================== Overrun Detection Report ==================");

        CAMX_LOG_ERROR(CamxLogGroupMemSpy,
                       "Alloc File: %s, LineNum: %d, Type: %s, Ptr: 0x%p, Size: %d",
                       isValidCallsiteDesc ? rAllocDesc.pAllocCallsiteDesc->filename : "<Unknown>",
                       isValidCallsiteDesc ? rAllocDesc.pAllocCallsiteDesc->lineNum : -1,
                       isValidCallsiteDesc ? CamxMemTypeToString(rAllocDesc.pAllocCallsiteDesc->type) : "<Unknown>",
                       rAllocDesc.pClientPtr,
                       rAllocDesc.allocSize);

        CAMX_LOG_ERROR(CamxLogGroupMemSpy,
                       "Free File: %s, LineNum: %d, Ptr: 0x%p, Sig: 0x%x",
                       (NULL != pFileName) ? pFileName : "<Unknown>",
                       lineNum,
                       pClientPtr,
                       overrunSignature);

        CAMX_LOG_ERROR(CamxLogGroupMemSpy,
                       "================= End of Overrun Detection Report ==============");
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CheckAllocateAndFreeMatch
///
/// @brief  Checks to see whether allocate and free calls match and prints a report if a mismatch is detected.
///
/// @param  flags       Flags indicating the type of the allocation
/// @param  rAllocDesc  The allocation descriptor for which to check if allocate and free calls match
/// @param  pFileName   Name of the file in which the allocation was called
/// @param  lineNum     Line number at which the allocation was called
///
/// @return None
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static VOID CheckAllocateAndFreeMatch(
    CamxMemFlags     flags,
    MemSpyAllocDesc& rAllocDesc,
    const CHAR*      pFileName,
    UINT             lineNum)
{
    BOOL isDelete           = ((flags & CamxMemFlagsDelete) > 0) ? TRUE : FALSE;
    BOOL isArray            = ((flags & CamxMemFlagsArray) > 0) ? TRUE : FALSE;
    BOOL isValidCallsiteDesc   = (NULL != rAllocDesc.pAllocCallsiteDesc);

    // Check to make sure allocate and free calls match
    if ((isDelete != rAllocDesc.expectDelete) || (isArray != rAllocDesc.expectArray))
    {
        CAMX_ASSERT_ALWAYS();

        CAMX_LOG_ERROR(CamxLogGroupMemSpy,
                       "==================== Mismatched Call Report ====================");
        CAMX_LOG_ERROR(CamxLogGroupMemSpy,
                       "Alloc: File: %s, LineNum: %d, Type: %s, Ptr: 0x%p, Size: %d, Delete: %d, Array: %d",
                       isValidCallsiteDesc ? rAllocDesc.pAllocCallsiteDesc->filename : "<Unknown>",
                       isValidCallsiteDesc ? rAllocDesc.pAllocCallsiteDesc->lineNum : -1,
                       isValidCallsiteDesc ? CamxMemTypeToString(rAllocDesc.pAllocCallsiteDesc->type) : "<Unknown>",
                       rAllocDesc.pClientPtr,
                       rAllocDesc.allocSize,
                       rAllocDesc.expectDelete,
                       rAllocDesc.expectArray);

        CAMX_LOG_ERROR(CamxLogGroupMemSpy,
                       "Free: File: %s, LineNum: %d, Delete: %d, Array: %d",
                       (NULL != pFileName) ? pFileName : "<Unknown>",
                       lineNum,
                       isDelete,
                       isArray);

        CAMX_LOG_ERROR(CamxLogGroupMemSpy,
                       "================= End of Mismatched Call Report ================");
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CompareAllocSize
///
/// @brief  Compares two allocation descriptors containing the allocation size.
///
/// @param  pData1 The first allocation descriptor
/// @param  pData2 The second allocation descriptor
///
/// @return If *pData1 > *pData2 return a positive number,
///         else if *pData1 == *pData2 return 0,
///         else (if) *pData1 < *pData2 return a negative number.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static INT CompareAllocSize(
    const VOID* pData1,
    const VOID* pData2)
{
    SIZE_T difference = 0;

    if ((NULL != pData1) && (NULL != pData2))
    {
        const MemSpyAllocDesc* pAllocDesc1 = static_cast<const MemSpyAllocDesc*>(pData1);
        const MemSpyAllocDesc* pAllocDesc2 = static_cast<const MemSpyAllocDesc*>(pData2);

        difference = pAllocDesc1->allocSize - pAllocDesc2->allocSize;
    }

    return static_cast<INT>(difference);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CompareNumBytesInUse
///
/// @brief  Compares two allocation callsite descriptors containing the number of bytes in use.
///
/// @param  pData1 The first allocation callsite descriptor
/// @param  pData2 The second allocation callsite descriptor
///
/// @return If *pData1 > *pData2 return a positive number,
///         else if *pData1 == *pData2 return 0,
///         else (if) *pData1 < *pData2 return a negative number.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static INT CompareNumBytesInUse(
    const VOID* pData1,
    const VOID* pData2)
{
    SIZE_T difference = 0;

    if ((NULL != pData1) && (NULL != pData2))
    {
        const MemSpyAllocCallsiteDesc* pAllocCallsiteDesc1 = static_cast<const MemSpyAllocCallsiteDesc*>(pData1);
        const MemSpyAllocCallsiteDesc* pAllocCallsiteDesc2 = static_cast<const MemSpyAllocCallsiteDesc*>(pData2);

        difference = pAllocCallsiteDesc1->numBytesInUse - pAllocCallsiteDesc2->numBytesInUse;
    }

    return static_cast<INT>(difference);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CompareNumBytesWatermark
///
/// @brief  Compares two allocation callsite descriptors containing the number of bytes in use at the high watermark.
///
/// @param  pData1 The first allocation callsite descriptor
/// @param  pData2 The second allocation callsite descriptor
///
/// @return If *pData1 > *pData2 return a positive number,
///         else if *pData1 == *pData2 return 0,
///         else (if) *pData1 < *pData2 return a negative number.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static INT CompareNumBytesWatermark(
    const VOID* pData1,
    const VOID* pData2)
{
    SIZE_T difference = 0;

    if ((NULL != pData1) && (NULL != pData2))
    {
        const MemSpyAllocCallsiteDesc* pAllocCallsiteDesc1 = static_cast<const MemSpyAllocCallsiteDesc*>(pData1);
        const MemSpyAllocCallsiteDesc* pAllocCallsiteDesc2 = static_cast<const MemSpyAllocCallsiteDesc*>(pData2);

        difference = pAllocCallsiteDesc1->numBytesWatermark - pAllocCallsiteDesc2->numBytesWatermark;
    }

    return static_cast<INT>(difference);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CompareNumAllocsLifetime
///
/// @brief  Compares two allocation callsite descriptors containing the number of allocations in use.
///
/// @param  pData1 The first allocation callsite descriptor
/// @param  pData2 The second allocation callsite descriptor
///
/// @return If *pData1 > *pData2 return a positive number,
///         else if *pData1 == *pData2 return 0,
///         else (if) *pData1 < *pData2 return a negative number.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static INT CompareNumAllocsLifetime(
    const VOID* pData1,
    const VOID* pData2)
{
    INT difference = 0;

    if ((NULL != pData1) && (NULL != pData2))
    {
        const MemSpyAllocCallsiteDesc* pAllocCallsiteDesc1 = static_cast<const MemSpyAllocCallsiteDesc*>(pData1);
        const MemSpyAllocCallsiteDesc* pAllocCallsiteDesc2 = static_cast<const MemSpyAllocCallsiteDesc*>(pData2);

        difference = pAllocCallsiteDesc1->numAllocsLifetime - pAllocCallsiteDesc2->numAllocsLifetime;
    }

    return difference;
}

#if CAMX_DETECT_WRITE_TO_FREED_MEM
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// DeleteFreedAlloc
///
/// @brief  Permanently deletes a freed allocation.
///
/// @param  pFreedAlloc The freed allocation to permanently delete
///
/// @return None
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static VOID DeleteFreedAlloc(
    VOID* pFreedAlloc)
{
    if (NULL != pFreedAlloc)
    {
        // Get old allocation size
        SIZE_T* pSize = static_cast<SIZE_T*>(pFreedAlloc);

        // Unprotect the allocation
        OsUtils::MemoryProtect(pFreedAlloc, pSize[0], CamxReadWrite);

        // Free old allocation
        CAMX_FREE_ALIGNED_NO_SPY(pFreedAlloc);
    }
}
#endif // CAMX_DETECT_WRITE_TO_FREED_MEM

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Public Methods
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// MemSpy::AdjustAllocSize
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
SIZE_T MemSpy::AdjustAllocSize(
    SIZE_T  size,
    UINT32  alignment,
    UINT32  pageSize)
{
    // Make sure there is room for the tracking information and alignment in the beginning. We make the assumption here that
    // the allocation will always be aligned to either the pageSize (in the case of freed-write detection) or the input
    // alignment. Therefore, if the alignment requirement is large, the alignment padding itself will accomodate our tracking
    // info.
    SIZE_T padding = (alignment >= sizeof(MemSpyTrackingInfo)) ? alignment : (sizeof(MemSpyTrackingInfo) + alignment);

    // Pad the size to the alignment requirements of MemSpyOverrunSignature which will follow the buffer
    SIZE_T sizePadded = CamX::Utils::ByteAlign(size, alignof(UINT64));

    // Depending on whether we need to pad to a page size (for freed-write detection), here we make sure there is room for
    // the tracking information, the buffer alignment padding, the buffer, the alignment padding for the overrun signature,
    // and the overrun signature.
    SIZE_T sizeAdjusted = padding + sizePadded + sizeof(MemSpyOverrunSignature);

    // If freed-write detection is enabled, add padding so that allocation size is multiple of page size (so it takes up entire
    // page(s)).
    if (pageSize > 0)
    {
        sizeAdjusted = CamX::Utils::ByteAlign(sizeAdjusted, pageSize);
    }

    // Check for overflow
    if ((size == 0) || (size > sizeAdjusted))
    {
        sizeAdjusted = 0;
    }

    return sizeAdjusted;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// MemSpy::TrackAlloc
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID* MemSpy::TrackAlloc(
    VOID*           pMem,
    SIZE_T          allocSize,
    SIZE_T          clientSize,
    UINT32          alignment,
    CamxMemFlags    flags,
    CamxMemType     type,
    const CHAR*     pFileName,
    UINT            lineNum)
{
    CamxResult result = CamxResultSuccess;

    CAMX_ASSERT(NULL != pMem);
    CAMX_ASSERT(allocSize > MemSpySizeOfOverhead); // Verifies the client used the size from AdjustAllocSize()

    VOID* pTrackingInfo         = NULL;
    VOID* pClientPtr            = NULL;
    SIZE_T filenameOffset       = 0;
    SIZE_T length               = 0;

    if ((NULL != pMem) &&
        ((0 == alignment) || (TRUE == CamX::Utils::IsPowerOfTwo(alignment))) &&
        (allocSize > MemSpySizeOfOverhead))
    {
        // Calculate the pointer for the client based on the alignment
        pClientPtr = Utils::VoidPtrInc(pMem, sizeof(MemSpyTrackingInfo));

        if (alignment > 0)
        {
            pClientPtr = Utils::ByteAlignPtr(pClientPtr, static_cast<SIZE_T>(alignment));
        }

        // Place the tracking structure just before the client pointer
        pTrackingInfo = Utils::VoidPtrDec(pClientPtr, sizeof(MemSpyTrackingInfo));

        // Adjust filename if it exists...we're only using the last MemSpyMaxFilenameSize characters
        if (NULL != pFileName)
        {
            length = OsUtils::StrLen(pFileName);
            if (length >= MemSpyMaxFilenameSize)
            {
                filenameOffset = length - MemSpyMaxFilenameSize + 1;
            }
        }

        MemSpy* pMemSpy = GetInstance();
        CAMX_ASSERT(NULL != pMemSpy);
        if (NULL != pMemSpy)
        {
            if (NULL != pMemSpy->m_pMemSpyLock)
            {
                pMemSpy->m_pMemSpyLock->Lock();
            }
        }

        // Check again to make sure we have a valid instance in case the lock had been held by the destructor.
        pMemSpy = GetInstance();
        if (NULL != pMemSpy)
        {
            MemSpyAllocCallsiteDesc* pAllocCallsiteDesc = NULL;

            MemSpyAllocCallsiteDescKey allocCallsiteDescKey = {0};
            allocCallsiteDescKey.pFileName                  = pFileName;
            allocCallsiteDescKey.lineNum                    = lineNum;
            VOID* pVal                                      = NULL;

            result = pMemSpy->m_pAllocCallsiteMap->GetInPlace(&allocCallsiteDescKey, &pVal);

            // Check to see if there is an existing entry for this allocation callsite, otherwise, create a new entry
            if (CamxResultSuccess == result)
            {
                pAllocCallsiteDesc = reinterpret_cast<MemSpyAllocCallsiteDesc*>(pVal);
                if ((NULL != pAllocCallsiteDesc)                &&
                    (pAllocCallsiteDesc->lineNum == lineNum)    &&
                    (pAllocCallsiteDesc->type == type)          &&
                    ((NULL == pFileName) || (0 == OsUtils::StrCmp(pAllocCallsiteDesc->filename, (pFileName + filenameOffset)))))
                {
                    // We've found an existing callsite, update its metadata
                    pAllocCallsiteDesc->numBytesInUse += allocSize;
                    pAllocCallsiteDesc->numAllocsInUse++;
                    pAllocCallsiteDesc->numAllocsLifetime++;

                    if (pAllocCallsiteDesc->numBytesInUse > pAllocCallsiteDesc->numBytesWatermark)
                    {
                        pAllocCallsiteDesc->numBytesWatermark   = pAllocCallsiteDesc->numBytesInUse;
                        pAllocCallsiteDesc->numAllocsWatermark  = pAllocCallsiteDesc->numAllocsInUse;
                    }
                }
            }
            else if (CamxResultENoSuch == result)
            {
                MemSpyAllocCallsiteDesc allocCallsiteDesc = {{0}};

                // Populate the metadata to track the allocation callsite
                if (NULL == pFileName)
                {
                    Utils::Memset(allocCallsiteDesc.filename, 0, sizeof(allocCallsiteDesc.filename));
                }
                else
                {
                    OsUtils::StrLCpy(allocCallsiteDesc.filename, (pFileName + filenameOffset), MemSpyMaxFilenameSize);
                }
                allocCallsiteDesc.lineNum             = lineNum;
                allocCallsiteDesc.type                = type;
                allocCallsiteDesc.numBytesInUse       = allocSize;
                allocCallsiteDesc.numAllocsInUse      = 1;
                allocCallsiteDesc.numBytesWatermark   = allocSize;
                allocCallsiteDesc.numAllocsWatermark  = 1;
                allocCallsiteDesc.numAllocsLifetime   = 1;

                result = pMemSpy->m_pAllocCallsiteMap->Put(&allocCallsiteDescKey, &allocCallsiteDesc);
                if (CamxResultSuccess != result)
                {
                    CAMX_LOG_ERROR(CamxLogGroupMemSpy,
                                   "Adding client pointer to active allocation list failed with result=%d",
                                   result);
                }

                // Get a pointer to the callsite allocation structure to associate it with the active allocation data
                result              = pMemSpy->m_pAllocCallsiteMap->GetInPlace(&allocCallsiteDescKey, &pVal);
                pAllocCallsiteDesc  = reinterpret_cast<MemSpyAllocCallsiteDesc*>(pVal);
            }
            else
            {
                CAMX_LOG_ERROR(CamxLogGroupMemSpy,
                               "Retrieving client pointer from active allocation list failed with result=%d",
                               result);
            }

            // Update the totals
            pMemSpy->m_totalNumBytesInUse += allocSize;
            pMemSpy->m_totalNumAllocsInUse++;

            if (pMemSpy->m_totalNumBytesInUse > pMemSpy->m_totalNumBytesWatermark)
            {
                pMemSpy->m_totalNumBytesWatermark   = pMemSpy->m_totalNumBytesInUse;
                pMemSpy->m_totalNumAllocsWatermark  = pMemSpy->m_totalNumAllocsInUse;
            }

            // Populate the metadata to track the individual allocation
            // Save this allocation in the list
            MemSpyAllocDesc allocDesc       = {0};
            allocDesc.pAllocCallsiteDesc    = pAllocCallsiteDesc;
            allocDesc.pOriginalPtr          = pMem;
            allocDesc.pClientPtr            = pClientPtr;
            allocDesc.allocSize             = allocSize;
            allocDesc.clientSize            = clientSize;
            allocDesc.expectDelete          = ((flags & CamxMemFlagsNew) > 0) ? TRUE : FALSE;
            allocDesc.expectArray           = ((flags & CamxMemFlagsArray) > 0) ? TRUE : FALSE;
            allocDesc.pMemSpyOwner          = pMemSpy;
            allocDesc.allocTime             = pMemSpy->m_currentTime++;

            // Add the allocation descriptor to the list
            result = pMemSpy->m_pActiveAllocMap->Put(&pClientPtr, &allocDesc);
            if (CamxResultSuccess != result)
            {
                CAMX_LOG_ERROR(CamxLogGroupMemSpy, "Could not add allocation descriptor to list!");
            }
            else
            {
                // Populate the tracking info
                MemSpyTrackingInfo memSpyTrackingInfo   = {0};
                memSpyTrackingInfo.headerSignature1     = MemSpyHeaderSignature1;

                // Adjust pMem so it points to where the tracking info will be saved
                pMem = pTrackingInfo;

                // Copy the tracking info just before the client pointer
                *(static_cast<MemSpyTrackingInfo*>(pMem)) = memSpyTrackingInfo;
            }

            if (NULL != pMemSpy->m_pMemSpyLock)
            {
                pMemSpy->m_pMemSpyLock->Unlock();
            }

            // Add the buffer overrun signature
            SIZE_T sizePadded = CamX::Utils::ByteAlign(clientSize, alignof(UINT64));
            *(static_cast<UINT64*>(Utils::VoidPtrInc(allocDesc.pClientPtr, sizePadded))) = MemSpyOverrunSignature;

            // Return the pointer that the client will use
            pMem = allocDesc.pClientPtr;

        }
    }

    return pMem;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// MemSpy::TrackFree
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID* MemSpy::TrackFree(
    VOID*           pMem,
    CamxMemFlags    flags,
    const CHAR*     pFileName,
    UINT            lineNum)
{
    CamxResult           result         = CamxResultSuccess;
    MemSpyTrackingInfo*  pTrackingInfo  = NULL;
    VOID*                pFreePtr       = pMem;
    VOID*                pVal           = NULL;
    MemSpyAllocDesc*     pAllocDesc     = NULL;

    MemSpy* pMemSpy = GetInstance();
    if (NULL != pMemSpy)
    {
        if (NULL != pMemSpy->m_pMemSpyLock)
        {
            pMemSpy->m_pMemSpyLock->Lock();
        }
    }

    // Check again to make sure we have a valid instance in case the lock had been held by the destructor.
    pMemSpy = GetInstance();
    if (NULL != pMemSpy)
    {
        // Get the tracking info and allocation descriptor associated with the memory being freed
        result = pMemSpy->GetTrackingInfo(pMem, pTrackingInfo, pAllocDesc);
        if (CamxResultSuccess == result)
        {
            // Check Signature
            CheckSignatures(pTrackingInfo->headerSignature1, pMem, *pAllocDesc, pFileName, lineNum);

            // Get the pointer we're going to free
            pFreePtr = pAllocDesc->pOriginalPtr;

            // Validate the pointer
            CheckMemSpyOwner(pMemSpy, *pAllocDesc, pFileName, lineNum);
            CheckForOverrun(pMem, *pAllocDesc, pFileName, lineNum);
            CheckAllocateAndFreeMatch(flags, *pAllocDesc, pFileName, lineNum);

            SIZE_T size = pAllocDesc->allocSize;

            if (NULL != pAllocDesc->pAllocCallsiteDesc)
            {
                // Update the callsite info
                pAllocDesc->pAllocCallsiteDesc->numBytesInUse -= size;
                pAllocDesc->pAllocCallsiteDesc->numAllocsInUse--;

                // Timestamps from other MemSpy instances have no meaning, since it's an error to free an alloc in
                // another MemSpy instance there's no need to support accumulating those stats.
                if (pMemSpy == pAllocDesc->pMemSpyOwner)
                {
                    // Update the number of allocations freed at the current timestamp
                    if (pAllocDesc->pAllocCallsiteDesc->lastFreeTime != pMemSpy->m_currentTime)
                    {
                        pAllocDesc->pAllocCallsiteDesc->lastFreeTime      = pMemSpy->m_currentTime;
                        pAllocDesc->pAllocCallsiteDesc->numLastFreeAllocs = 1;
                    }
                    else
                    {
                        pAllocDesc->pAllocCallsiteDesc->numLastFreeAllocs++;
                    }

                    if (pMemSpy->m_currentTime >= pAllocDesc->allocTime)
                    {
                        UINT64 prevTotalLiveTime = pAllocDesc->pAllocCallsiteDesc->totalLiveTime;

                        pAllocDesc->pAllocCallsiteDesc->totalLiveTime += (pMemSpy->m_currentTime - pAllocDesc->allocTime);

                        if (prevTotalLiveTime > pAllocDesc->pAllocCallsiteDesc->totalLiveTime)
                        {
                            CAMX_LOG_ERROR(CamxLogGroupMemSpy, "Total live timestamp rollover detected");
                        }
                    }
                    else
                    {
                        CAMX_LOG_ERROR(CamxLogGroupMemSpy, "Timestamp rollover detected!");
                    }
                }
            }

            pMemSpy->m_totalNumBytesInUse -= size;
            pMemSpy->m_totalNumAllocsInUse--;

            // Free tracking memory
            pMemSpy->m_pActiveAllocMap->Remove(&pMem);

            // Initialize the tracking info to detect a double free
            pTrackingInfo->headerSignature1 = MemSpyFreeHeaderSignature;


#if CAMX_DETECT_WRITE_TO_FREED_MEM
            INT alignment = OsUtils::PageSize();

            // Make sure this is valid to protect, then protect it
            if ((NULL != pFreePtr) &&
                (size > 0) &&
                (Utils::ByteAlign(size, alignment) == size) &&
                (Utils::ByteAlignPtr(pFreePtr, alignment) == pFreePtr))
            {
                // Only detect writes to freed memory if allocation is not too big
                SIZE_T maxSize = (static_cast<SIZE_T>(alignment) > FreedListMaxAllocSize) ?
                    alignment : FreedListMaxAllocSize;
                if (size <= maxSize)
                {
                    VOID* pNewFreePtr = pFreePtr;

                    // Need to free allocations off the end of the list (i.e. LRU) if we are over our limit
                    for (LDLLNode* pNode = pMemSpy->m_freedAllocList.RemoveFromHead();
                         ((NULL != pNode) && (pMemSpy->m_freedAllocList.NumNodes() >= FreedListMaxAllocations));
                         pNode = pMemSpy->m_freedAllocList.RemoveFromTail())
                    {
                        DeleteFreedAlloc(pNode->pData);
                        pMemSpy->PutFreeNode(pNode);
                    }

                    // Store the size of the allocation at the start of the freed memory
                    SIZE_T* pSize = static_cast<SIZE_T*>(pNewFreePtr);
                    pSize[0] = size;

                    // Protect the freed memory as read-only
                    result = OsUtils::MemoryProtect(pNewFreePtr, size, CamxReadOnly);
                    if (CamxResultSuccess == result)
                    {
                        LDLLNode* pNode = pMemSpy->GetFreeNode();

                        if (NULL != pNode)
                        {
                            // Insert the existing allocation on the freed allocation list to keep track of it
                            pNode->pData = pNewFreePtr;
                            pMemSpy->m_freedAllocList.InsertToHead(pNode);

                            // The freed allocation list now owns this memory, so don't free it later
                            pFreePtr = NULL;
                        }
                    }
                    CAMX_ASSERT(CamxResultSuccess == result);
                }
            }
#endif // CAMX_DETECT_WRITE_TO_FREED_MEM
        }

        if (NULL != pMemSpy->m_pMemSpyLock)
        {
            pMemSpy->m_pMemSpyLock->Unlock();
        }
    }

    return pFreePtr;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// MemSpy::PrintReport
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID MemSpy::PrintReport()
{
    MemSpy* pMemSpy = GetInstance();
    if (NULL != pMemSpy)
    {
        if (NULL != pMemSpy->m_pMemSpyLock)
        {
            pMemSpy->m_pMemSpyLock->Lock();
        }
    }

    // Check again to make sure we have a valid instance in case the lock had been held by the destructor.
    pMemSpy = GetInstance();
    if (NULL != pMemSpy)
    {
        pMemSpy->GenerateReport();

        if (NULL != pMemSpy->m_pMemSpyLock)
        {
            pMemSpy->m_pMemSpyLock->Unlock();
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// MemSpy::PrintRuntimeReport
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID MemSpy::PrintRuntimeReport()
{
    MemSpy* pMemSpy = GetInstance();
    if (NULL != pMemSpy)
    {
        if (NULL != pMemSpy->m_pMemSpyLock)
        {
            pMemSpy->m_pMemSpyLock->Lock();
        }
    }

    // Check again to make sure we have a valid instance in case the lock had been held by the destructor.
    pMemSpy = GetInstance();
    if (NULL != pMemSpy)
    {
        pMemSpy->GenerateRuntimeReport();

        if (NULL != pMemSpy->m_pMemSpyLock)
        {
            pMemSpy->m_pMemSpyLock->Unlock();
        }
    }
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Private Methods
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// MemSpy::GetInstance
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
MemSpy* MemSpy::GetInstance()
{
    static MemSpy s_memSpySingleton;

    MemSpy* pMemSpySingleton = NULL;

    if (TRUE == s_isValid)
    {
        pMemSpySingleton =  &s_memSpySingleton;
    }

    return pMemSpySingleton;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// MemSpy::MemSpy
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
MemSpy::MemSpy()
    : m_pMemSpyLock(NULL)
    , m_totalNumBytesInUse(0)
    , m_totalNumAllocsInUse(0)
    , m_totalNumBytesWatermark(0)
    , m_totalNumAllocsWatermark(0)
    , m_currentTime(0)
{
    m_pMemSpyLock = Mutex::CreateNoSpy("MemSpy");
    if (NULL == m_pMemSpyLock)
    {
        CAMX_ASSERT_ALWAYS_MESSAGE("Failed to create mutex lock!");
    }
    else
    {
        s_isValid = TRUE;

        HashmapParams hashMapParams = { 0 };

        // Create the allocation callsite hashmap
        hashMapParams.keySize               = sizeof(MemSpyAllocCallsiteDescKey);
        hashMapParams.valSize               = sizeof(MemSpyAllocCallsiteDesc);
        hashMapParams.multiMap              = 0;
        hashMapParams.preallocateBuckets    = TRUE;
        m_pAllocCallsiteMap                 = Hashmap::Create(&hashMapParams);

        // Create the active allocation hashmap
        hashMapParams.keySize               = sizeof(VOID*);
        hashMapParams.valSize               = sizeof(MemSpyAllocDesc);
        m_pActiveAllocMap                   = Hashmap::Create(&hashMapParams);

        if ((NULL == m_pActiveAllocMap) || (NULL == m_pAllocCallsiteMap))
        {
            CAMX_ASSERT_ALWAYS_MESSAGE("Out of memory");
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// MemSpy::~MemSpy
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
MemSpy::~MemSpy()
{
    s_isValid = FALSE;

    if (NULL != m_pMemSpyLock)
    {
        m_pMemSpyLock->Lock();
    }

    // First generate the report
    GenerateReport();

    // Free objects in maps
    m_pActiveAllocMap->Clear();
    m_pAllocCallsiteMap->Clear();

#if CAMX_DETECT_WRITE_TO_FREED_MEM
    LDLLNode* pNode = m_freedAllocList.RemoveFromHead();

    while (NULL != pNode)
    {
        DeleteFreedAlloc(pNode->pData);
        CAMX_FREE_NO_SPY(pNode);
        pNode = m_freedAllocList.RemoveFromHead();
    }
#endif // CAMX_DETECT_WRITE_TO_FREED_MEM

    // Destroy created objects in reverse order of creation
    if (NULL != m_pMemSpyLock)
    {
        m_pMemSpyLock->Unlock();
        m_pMemSpyLock->Destroy();
        m_pMemSpyLock = NULL;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// MemSpy::GetTrackingInfo
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult MemSpy::GetTrackingInfo(
    VOID*                   pClientPtr,
    MemSpyTrackingInfo*&    rpTrackingInfo,
    MemSpyAllocDesc*&       rpAllocDesc)
{
    MemSpyTrackingInfo* pPossibleTrackingInfo   = NULL;
    CamxResult          result                  = CamxResultSuccess;

    if (NULL == pClientPtr)
    {
        CAMX_LOG_ERROR(CamxLogGroupMemSpy, "pClientPtr is NULL");
        result = CamxResultEInvalidPointer;
    }
    else
    {
        INT pageSize = OsUtils::PageSize();
        if (pageSize <= 0)
        {
            CAMX_LOG_ERROR(CamxLogGroupMemSpy, "Invalid page size: %d", pageSize);
            result = CamxResultEInvalidState;
        }
        else
        {
            // There are two high-level cases to consider:
            //   1) The buffer was allocated and tracked using MemSpy
            //   2) The buffer was allocated and not tracked using MemSpy
            // Because our delete override is used by all CamX and non-CamX libraries linked into our shared libs, we first
            // search the list of active allocations to see if this client pointer matches a known client pointer.
            //  If so, proceed as if it is tracked. If not, proceed as if it is not tracked using this MemSpy.
            MemSpyAllocDesc*    pAllocDesc  = NULL;
            VOID*               pVal        = NULL;

            result = m_pActiveAllocMap->GetInPlace(&pClientPtr, &pVal);
            if (CamxResultSuccess == result)
            {
                pAllocDesc = reinterpret_cast<MemSpyAllocDesc*>(pVal);

                if (pAllocDesc->pClientPtr == pClientPtr)
                {
                    // We know we have a tracked allocation. Decrement the client pointer to look for tracking info. Treat
                    // the possible tracking info as suspect until proven valid.
                    pPossibleTrackingInfo =
                        (static_cast<MemSpyTrackingInfo*>(Utils::VoidPtrDec(pClientPtr, sizeof(MemSpyTrackingInfo))));
                }
            }

            // If the client pointer is not in the list, we know this buffer was not allocated using this MemSpy,
            //  yet is being freed as if it was, so print an warning.
            if (NULL == pPossibleTrackingInfo)
            {
                CAMX_LOG_VERBOSE(CamxLogGroupMemSpy,
                                 "Buffer: %p not allocated using MemSpy, but attempting to free using MemSpy!",
                                 pClientPtr);
                result = CamxResultENoSuch;
            }
            else
            {
                rpTrackingInfo  = pPossibleTrackingInfo;
                rpAllocDesc     = pAllocDesc;
            }
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// MemSpy::PrintMemStats
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID MemSpy::PrintMemStats()
{
    CamxResult result = CamxResultSuccess;
    LDLLNode* pNode   = NULL;

    // Get the list of allocation callsites from the map
    LightweightDoublyLinkedList allocCallsiteList;
    result = m_pAllocCallsiteMap->GetValList(allocCallsiteList);

    CAMX_LOG_INFO(CamxLogGroupMemSpy,
                  "================== Memory Usage Statistics Report ==================");

    CAMX_LOG_INFO(CamxLogGroupMemSpy,
                  "Total high watermark (in bytes):                  %d",
                  m_totalNumBytesWatermark);

    CAMX_LOG_INFO(CamxLogGroupMemSpy,
                     "Number of allocs comprising total high watermark: %d",
                     m_totalNumAllocsWatermark);

    CAMX_LOG_INFO(CamxLogGroupMemSpy,
                  "");

    // Highest numBytesWatermark
    CAMX_LOG_INFO(CamxLogGroupMemSpy,
                  "Top %d allocation callsites with highest watermarks (HW):",
                  MemSpyMaxLeakStats);

    CAMX_LOG_INFO(CamxLogGroupMemSpy,
                  "-----------------------------------------------------------------------------------------");

    CAMX_LOG_INFO(CamxLogGroupMemSpy,
                  "| HW in Bytes| # Allocs@HW|          CamxMemType|                      File Name:LineNum|");

    CAMX_LOG_INFO(CamxLogGroupMemSpy,
                  "-----------------------------------------------------------------------------------------");

    if (CamxResultSuccess == result)
    {
        // Sort the list by numBytesInUse watermark
        allocCallsiteList.Sort(&CompareNumBytesWatermark);

        UINT numStats = 0;

        for (pNode = allocCallsiteList.Tail();
             ((NULL != pNode) && (MemSpyMaxLeakStats > numStats));
             pNode = LightweightDoublyLinkedList::PrevNode(pNode))
        {
            MemSpyAllocCallsiteDesc* pAllocCallsiteData = static_cast<MemSpyAllocCallsiteDesc*>(pNode->pData);

            if (NULL != pAllocCallsiteData)
            {
                numStats++;
                CAMX_LOG_INFO(CamxLogGroupMemSpy,
                              "| %11d| %11d| %20s| %30s:%7d|",
                              pAllocCallsiteData->numBytesWatermark,
                              pAllocCallsiteData->numAllocsWatermark,
                              CamxMemTypeToString(pAllocCallsiteData->type),
                              pAllocCallsiteData->filename,
                              pAllocCallsiteData->lineNum);
            }

        }
    }

    CAMX_LOG_INFO(CamxLogGroupMemSpy,
                     "-----------------------------------------------------------------------------------------");

    CAMX_LOG_INFO(CamxLogGroupMemSpy,
                     "");

    // Callsites with most allocations
    CAMX_LOG_INFO(CamxLogGroupMemSpy,
                  "Top %d allocation callsites with most lifetime allocations:",
                  MemSpyMaxLeakStats);

    CAMX_LOG_INFO(CamxLogGroupMemSpy,
                  "---------------------------------------------------------------------------------------------");

    CAMX_LOG_INFO(CamxLogGroupMemSpy,
                  "| Lifetime Allocs| HW in Bytes|          CamxMemType|                      File Name:LineNum|");

    CAMX_LOG_INFO(CamxLogGroupMemSpy,
                  "---------------------------------------------------------------------------------------------");

    if (CamxResultSuccess == result)
    {
        // Sort the list by number of lifetime allocations
        allocCallsiteList.Sort(&CompareNumAllocsLifetime);

        UINT numStats = 0;
        for (pNode = allocCallsiteList.Tail();
             ((NULL != pNode) && (MemSpyMaxLeakStats > numStats));
             pNode = LightweightDoublyLinkedList::PrevNode(pNode), numStats++)
        {
            MemSpyAllocCallsiteDesc* pAllocCallsiteData = static_cast<MemSpyAllocCallsiteDesc*>(pNode->pData);

            if (NULL != pAllocCallsiteData)
            {
                CAMX_LOG_INFO(CamxLogGroupMemSpy,
                              "| %15d| %11d| %20s| %30s:%7d|",
                              pAllocCallsiteData->numAllocsLifetime,
                              pAllocCallsiteData->numBytesWatermark,
                              CamxMemTypeToString(pAllocCallsiteData->type),
                              pAllocCallsiteData->filename,
                              pAllocCallsiteData->lineNum);
            }
        }
    }

    CAMX_LOG_INFO(CamxLogGroupMemSpy,
                  "---------------------------------------------------------------------------------------------");

    CAMX_LOG_INFO(CamxLogGroupMemSpy,
                  "============== End of Memory Usage Statistics Report ===============");

    allocCallsiteList.FreeAllNodesAndTheirClientData();

}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// MemSpy::CheckMemLeaks
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID MemSpy::CheckMemLeaks()
{
    CamxResult result = CamxResultSuccess;
    LightweightDoublyLinkedList activeAllocList;
    LightweightDoublyLinkedList allocCallsiteList;

    // Check if there are allocations left unfreed
    if (0 != m_pActiveAllocMap->Size())
    {
        // All allocations on list are now memory leaks
        CAMX_ASSERT_ALWAYS_MESSAGE("Memory leaks detected!");

        // Process memory leaks and accumulate stats
        UINT numLeaks = m_pActiveAllocMap->Size();
        CAMX_ASSERT(numLeaks == m_totalNumAllocsInUse);

        // Print out leak report
        CAMX_LOG_INFO(CamxLogGroupMemSpy,
                      "================= Memory Leak Detection Report =================");

        // Print high-level results
        CAMX_LOG_INFO(CamxLogGroupMemSpy,
                      "Number of leaks:            %d",
                      numLeaks);

        CAMX_LOG_INFO(CamxLogGroupMemSpy,
                      "Total system leaked memory: %d",
                      m_totalNumBytesInUse);

        CAMX_LOG_INFO(CamxLogGroupMemSpy,
                      "");

        // Print list of specific allocations that were leaked
        CAMX_LOG_INFO(CamxLogGroupMemSpy,
                      "Leaked allocation list:");

        CAMX_LOG_INFO(CamxLogGroupMemSpy,
                      "-----------------------------------------------------------------------------------------------");

        CAMX_LOG_INFO(CamxLogGroupMemSpy,
                      "| Alloc Size|            Pointer|          CamxMemType|                      File Name:LineNum|");

        CAMX_LOG_INFO(CamxLogGroupMemSpy,
                      "-----------------------------------------------------------------------------------------------");

        result = m_pActiveAllocMap->GetValList(activeAllocList);

        if (CamxResultSuccess == result)
        {
            // Sort the active allocation list by the allocation size
            activeAllocList.Sort(&CompareAllocSize);

            // Add up the leaked memory to make sure the total matches our own internal tracking
            SIZE_T leakedMemory = 0;


            for (LDLLNode* pNode = activeAllocList.Tail();
                 NULL != pNode;
                 pNode = LightweightDoublyLinkedList::PrevNode(pNode))
            {
                MemSpyAllocDesc* pAllocDesc = static_cast<MemSpyAllocDesc*>(pNode->pData);
                if (NULL != pAllocDesc)
                {
                    MemSpyAllocCallsiteDesc* pAllocCallsiteDesc = reinterpret_cast<MemSpyAllocCallsiteDesc*>
                                                                                   (pAllocDesc->pAllocCallsiteDesc);
                    CAMX_LOG_INFO(CamxLogGroupMemSpy,
                                  "| %10d| %18p| %20s| %30s:%7d|",
                                  pAllocDesc->allocSize,
                                  pAllocDesc->pClientPtr,
                                  NULL != CamxMemTypeToString(pAllocCallsiteDesc->type) ?
                                                              CamxMemTypeToString(pAllocCallsiteDesc->type) : "<Unknown>",
                                  0 != pAllocCallsiteDesc->filename[0] ? pAllocCallsiteDesc->filename : "<Unknown>",
                                  0 != pAllocCallsiteDesc->lineNum     ? pAllocCallsiteDesc->lineNum  : -1);
                    leakedMemory += pAllocDesc->allocSize;
                }
            }

            CAMX_LOG_INFO(CamxLogGroupMemSpy,
                          "-----------------------------------------------------------------------------------------------");

            CAMX_ASSERT(leakedMemory == m_totalNumBytesInUse);

            CAMX_LOG_INFO(CamxLogGroupMemSpy,
                          "");
        }

        activeAllocList.FreeAllNodesAndTheirClientData();

        // Print list of specific callsites that leaked, sorted by bytes leaked
        CAMX_LOG_INFO(CamxLogGroupMemSpy,
                      "Allocation callsites that had leaks:");

        CAMX_LOG_INFO(CamxLogGroupMemSpy,
                      "----------------------------------------------------------------------------------------------");

        CAMX_LOG_INFO(CamxLogGroupMemSpy,
                      "| Bytes Leaked| # Allocs Leaked|          CamxMemType|                      File Name:LineNum|");

        CAMX_LOG_INFO(CamxLogGroupMemSpy,
                      "----------------------------------------------------------------------------------------------");

        result = m_pAllocCallsiteMap->GetValList(allocCallsiteList);

        if (CamxResultSuccess == result)
        {
            // Sort the allocation callsite list by the bytes currently in use
            allocCallsiteList.Sort(&CompareNumBytesInUse);

            for (LDLLNode* pNode = allocCallsiteList.Tail();
                 NULL != pNode;
                 pNode = LightweightDoublyLinkedList::PrevNode(pNode))
            {
                MemSpyAllocCallsiteDesc* pAllocDesc = static_cast<MemSpyAllocCallsiteDesc*>(pNode->pData);

                if ((NULL != pAllocDesc) && (pAllocDesc->numBytesInUse > 0))
                {
                    CAMX_LOG_INFO(CamxLogGroupMemSpy,
                                  "| %12d| %15d| %20s| %30s:%7d|",
                                  pAllocDesc->numBytesInUse,
                                  pAllocDesc->numAllocsInUse,
                                  CamxMemTypeToString(pAllocDesc->type),
                                  pAllocDesc->filename,
                                  pAllocDesc->lineNum);
                }
            }
        }

        allocCallsiteList.FreeAllNodesAndTheirClientData();

        CAMX_LOG_INFO(CamxLogGroupMemSpy,
                      "----------------------------------------------------------------------------------------------");

        CAMX_LOG_INFO(CamxLogGroupMemSpy,
                      "============== End of Memory Leak Detection Report =============");
    }

}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// MemSpy::CheckMemAccumulation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID MemSpy::CheckMemAccumulation()
{
    CamxResult result = CamxResultSuccess;
    LightweightDoublyLinkedList allocCallsiteList;

    result = m_pAllocCallsiteMap->GetValList(allocCallsiteList);
    if (CamxResultSuccess == result)
    {
        // Sort the allocation callsite list by the bytes used in the high watermark
        allocCallsiteList.Sort(&CompareNumBytesWatermark);

        BOOL printedHeader = FALSE;
        for (LDLLNode* pNode = allocCallsiteList.Head(); NULL != pNode; pNode = LightweightDoublyLinkedList::PrevNode(pNode))
        {
            MemSpyAllocCallsiteDesc* pAllocCallsiteData = static_cast<MemSpyAllocCallsiteDesc*>(pNode->pData);
            if (NULL != pAllocCallsiteData)
            {
                FLOAT freedAtEndRatio =
                    static_cast<FLOAT>(pAllocCallsiteData->numLastFreeAllocs) / pAllocCallsiteData->numAllocsLifetime;
                FLOAT avgPtrLiveTime =
                    static_cast<FLOAT>(pAllocCallsiteData->totalLiveTime) / pAllocCallsiteData->numAllocsLifetime;

                if ((pAllocCallsiteData->lastFreeTime == m_currentTime) &&
                    (pAllocCallsiteData->numBytesWatermark > (m_totalNumBytesWatermark * MemSpyWatermarkThreshold)) &&
                    (freedAtEndRatio > MemSpyFreedAtEndThreshold) &&
                    ((avgPtrLiveTime / m_currentTime) > MemSpyAvgPtrTimeThreshold) &&
                    (pAllocCallsiteData->numAllocsLifetime > MemSpyNumAllocsThreshold))
                {
                    if (FALSE == printedHeader)
                    {
                        printedHeader = TRUE;

                        CAMX_LOG_INFO(CamxLogGroupMemSpy,
                                    "============= Memory Accumulation Detection Report =============");

                        CAMX_LOG_INFO(CamxLogGroupMemSpy,
                                        "Total live time of program:                %lld",
                                        m_currentTime);

                        CAMX_LOG_INFO(CamxLogGroupMemSpy,
                                        "Total number of bytes used high watermark: %lld",
                                        m_totalNumBytesWatermark);

                        CAMX_LOG_INFO(CamxLogGroupMemSpy,
                                        "Freed at end ratio threshold:              %f",
                                        MemSpyFreedAtEndThreshold);

                        CAMX_LOG_INFO(CamxLogGroupMemSpy,
                                        "Average pointer live time ratio threshold: %f",
                                        MemSpyAvgPtrTimeThreshold);

                        CAMX_LOG_INFO(CamxLogGroupMemSpy,
                                        "Watermark ratio threshold:                 %f",
                                        MemSpyWatermarkThreshold);

                        CAMX_LOG_INFO(CamxLogGroupMemSpy,
                                        "Number of allocations threshold:           %d",
                                        MemSpyNumAllocsThreshold);

                        CAMX_LOG_INFO(CamxLogGroupMemSpy,
                                        "");

                        CAMX_LOG_INFO(CamxLogGroupMemSpy,
                                        "Allocation callsites with possible accumulation:");

                        CAMX_LOG_INFO(CamxLogGroupMemSpy,
                "----------------------------------------------------------------------------------------------"
                "----------------------------------------------------------------------------------------------");

                        CAMX_LOG_INFO(CamxLogGroupMemSpy,
                                        "| HW in Bytes| Avg Ptr Live Time| Lifetime Allocs| # Frees @ End| # Leaks"
                                        "|          CamxMemType|                      File Name:LineNum|");

                        CAMX_LOG_INFO(CamxLogGroupMemSpy,
                "----------------------------------------------------------------------------------------------"
                "----------------------------------------------------------------------------------------------");
                    }

                    CAMX_LOG_INFO(CamxLogGroupMemSpy,
                                    "| %11d| %16d| %15d| %13d| %7d| %20s| %30s:%7d|",
                                    pAllocCallsiteData->numBytesWatermark,
                                    Utils::RoundFLOAT(avgPtrLiveTime),
                                    pAllocCallsiteData->numAllocsLifetime,
                                    pAllocCallsiteData->numLastFreeAllocs,
                                    pAllocCallsiteData->numAllocsInUse,
                                    CamxMemTypeToString(pAllocCallsiteData->type),
                                    pAllocCallsiteData->filename,
                                    pAllocCallsiteData->lineNum);
                }
            }
        }

        allocCallsiteList.FreeAllNodesAndTheirClientData();

        if (TRUE == printedHeader)
        {
            CAMX_LOG_INFO(CamxLogGroupMemSpy,
            "----------------------------------------------------------------------------------------------"
            "----------------------------------------------------------------------------------------------");

            CAMX_LOG_INFO(CamxLogGroupMemSpy,
                            "========== End of Memory Accumulation Detection Report =========");
        }

    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// MemSpy::GenerateReport
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID MemSpy::GenerateReport()
{
    PrintMemStats();
    CheckMemAccumulation();
    CheckMemLeaks();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// MemSpy::GenerateRuntimeReport
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID MemSpy::GenerateRuntimeReport()
{
    CAMX_LOG_INFO(CamxLogGroupMemSpy,
                  "Total current (bytes, numAllocs) high (bytes, numAllocs): %d,%d,%d,%d",
                  m_totalNumBytesInUse,
                  m_totalNumAllocsInUse,
                  m_totalNumBytesWatermark,
                  m_totalNumAllocsWatermark);
}

CAMX_NAMESPACE_END

#elif defined (_WINDOWS) // CAMX_USE_MEMSPY

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// MemSpyStub
//
// @note MSVC gets upset if a .cpp file doesn't emit any code. This avoids the error: "warning LNK4221: This object file does
//       not define any previously undefined public symbols, so it will not be used by any link operation that consumes this
//       library"
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID MemSpyStub()
{
}

#endif // CAMX_USE_MEMSPY
