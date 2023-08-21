////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2017-2020 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  chxutils.cpp
/// @brief CHX utility functions
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include <system/camera_metadata.h>
#include <chrono>

#include "chxutils.h"
#include "chxusecaseutils.h"
#include <sys/prctl.h>
#include <stdio.h>
#include <unistd.h>                 // library functions

// NOWHINE ENTIRE FILE - Temporarily bypassing for existing CHI files

const UINT SemaphoreTimeout = 10;

#ifndef CDK_VISIBILITY_PUBLIC
#define CDK_VISIBILITY_PUBLIC __attribute__ ((visibility ("default")))
#endif

extern UINT32 g_enableChxLogs;
extern BOOL   g_logRequestMapping;
extern BOOL   g_enableSystemLog;

#if defined (_LINUX)

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Operator new
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID* operator new(
    size_t numBytes)    ///< Number of bytes to allocate
{
    return calloc(1, numBytes);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Operator delete
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID operator delete(
    VOID* pMem)    ///< Memory pointer
{
    free(pMem);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Mutex::Create
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
Mutex* Mutex::Create()
{
    Mutex* pMutex = NULL;

    pMutex = CHX_NEW Mutex();
    if (NULL != pMutex)
    {
        if (CDKResultSuccess != pMutex->Initialize())
        {
            CHX_DELETE pMutex;
            pMutex = NULL;
        }
    }

    return pMutex;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Mutex::Initialize
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult Mutex::Initialize()
{
    CDKResult          result      = CDKResultSuccess;
    pthread_mutexattr_t attr;
    BOOL                bValidAttr  = FALSE;      // TRUE once attr has been initialized

    if (pthread_mutexattr_init(&attr) == 0)
    {
        bValidAttr = TRUE;
    }
    else
    {
        result = CDKResultEFailed;
    }

    // Using re-entrant mutexes
    if ((result == CDKResultSuccess) &&
        (pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE_NP) != 0))
    {
        result = CDKResultEFailed;
    }

    if ((result == CDKResultSuccess) &&
        (pthread_mutex_init(&m_mutex, &attr) == 0))
    {
        m_validMutex = TRUE;
    }
    else
    {
        result = CDKResultEFailed;
    }

    if (TRUE == bValidAttr)
    {
        pthread_mutexattr_destroy(&attr);
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Mutex::Destroy
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID Mutex::Destroy()
{
    if (TRUE == m_validMutex)
    {
        pthread_mutex_destroy(&m_mutex);
    }

    CHX_DELETE this;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Mutex::Lock
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID Mutex::Lock()
{
    pthread_mutex_lock(&m_mutex);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Mutex::TryLock
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult Mutex::TryLock()
{
    CDKResult result     = CDKResultSuccess;
    INT        returnCode = 0;

    returnCode = pthread_mutex_trylock(&m_mutex);
    if (0 != returnCode)
    {
        if (EBUSY == returnCode)
        {
            result = CDKResultEBusy;
        }
        else
        {
            result = CDKResultEFailed;
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Mutex::Unlock
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID Mutex::Unlock()
{
    pthread_mutex_unlock(&m_mutex);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Mutex::GetNativeHandle
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
OSMutexHandle* Mutex::GetNativeHandle()
{
    return ((TRUE == m_validMutex) ? &m_mutex : NULL);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Condition::Create
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
Condition* Condition::Create()
{
    Condition* pCondition = NULL;

    pCondition = CHX_NEW Condition();

    if (NULL != pCondition)
    {
        if (CDKResultSuccess != pCondition->Initialize())
        {
            CHX_DELETE pCondition;
            pCondition = NULL;
        }
    }

    return pCondition;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Condition::Destroy
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID Condition::Destroy()
{
    if (TRUE == m_validConditionVar)
    {
        pthread_cond_destroy(&m_conditionVar);
    }

    CHX_DELETE this;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Condition::Initialize
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult Condition::Initialize()
{
    CDKResult result = CDKResultSuccess;

    if (pthread_cond_init(&m_conditionVar, NULL) == 0)
    {
        m_validConditionVar = TRUE;
    }
    else
    {
        result = CDKResultEFailed;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Condition::Wait
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult Condition::Wait(
    OSMutexHandle* phMutex)
{
    CHX_ASSERT(NULL != phMutex);

    INT         rc      = 0;
    CDKResult  result  = CDKResultEFailed;

    rc = pthread_cond_wait(&m_conditionVar, phMutex);

    if (0 == rc)
    {
        result = CDKResultSuccess;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Condition::TimedWait
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult Condition::TimedWait(
    OSMutexHandle*  phMutex,
    UINT            timeoutMilliseconds)
{
    CHX_ASSERT(NULL != phMutex);

    CDKResult      result              = CDKResultSuccess;
    INT             waitResult          = 0;
    UINT            timeoutSeconds      = (timeoutMilliseconds / 1000UL);
    UINT            timeoutNanoseconds  = (timeoutMilliseconds % 1000UL) * 1000000UL;
    struct timespec timeout             = {0};

    // Calculate the timeout time
    clock_gettime(CLOCK_REALTIME, &timeout);
    timeoutSeconds      += (static_cast<UINT>(timeout.tv_nsec) + timeoutNanoseconds) / 1000000000UL;
    timeoutNanoseconds  =  (static_cast<UINT>(timeout.tv_nsec) + timeoutNanoseconds) % 1000000000UL;
    timeout.tv_sec      += static_cast<INT>(timeoutSeconds);
    timeout.tv_nsec     =  static_cast<INT>(timeoutNanoseconds);

    waitResult = pthread_cond_timedwait(&m_conditionVar, phMutex, &timeout);
    if (waitResult != 0)
    {
        // Check errno for reason for failure
        if (ETIMEDOUT == waitResult)
        {
            result = CDKResultETimeout;
        }
        else
        {
            result = CDKResultEFailed;
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Signal the condition variable
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID Condition::Signal()
{
    pthread_cond_signal(&m_conditionVar);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Broadcast the signal to all
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID Condition::Broadcast()
{
    pthread_cond_broadcast(&m_conditionVar);
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Semaphore::Create
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
Semaphore* Semaphore::Create(
    INT count)
{
    Semaphore* pSemaphore = NULL;

    pSemaphore = CHX_NEW Semaphore();

    if (NULL != pSemaphore)
    {
        if (CDKResultSuccess != pSemaphore->Initialize(count))
        {
            CHX_DELETE pSemaphore;
            pSemaphore = NULL;
        }
    }

    return pSemaphore;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Semaphore::Initialize
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult Semaphore::Initialize(
    INT count)
{
    CDKResult result = CDKResultSuccess;

    m_semaphoreCount    = count;
    m_maxSemaphoreCount = count;

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Semaphore::NotifyAllThreads
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID Semaphore::NotifyAllThreads()
{
    std::unique_lock<std::mutex> lock(m_mutex);
    m_semaphoreCount++;
    if (m_semaphoreCount > m_maxSemaphoreCount)
    {
        CHX_LOG_WARN("Semaphore count exceeds the allocated number");
    }

    // Notify all the waiting threads
    m_semaphoreCv.notify_all();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Semaphore::Reset
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID Semaphore::Reset()
{
    std::unique_lock<std::mutex> lock(m_mutex);
    m_semaphoreCount = m_maxSemaphoreCount;

    CHX_LOG_INFO("Reset called on semaphore");

    // Notify the waiting threads
    m_semaphoreCv.notify_all();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Semaphore::WaitForSemaphore
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID Semaphore::WaitForSemaphore()
{
    std::unique_lock<std::mutex> lock(m_mutex);

    while (m_semaphoreCount == 0)
    {
        if(m_semaphoreCv.wait_for(lock, std::chrono::seconds(SemaphoreTimeout)) == std::cv_status::timeout)
        {
            CHX_LOG_ERROR("Wait for Semaphore timed out");
        }
        else
        {
            CHX_LOG_INFO("Waiting on the mutex until NotifyAllThreads() is called");
        }
    }
    m_semaphoreCount--;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Semaphore::Destroy
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID Semaphore::Destroy()
{
    CHX_DELETE this;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// AlignGeneric32
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UINT32 ChxUtils::AlignGeneric32(
    UINT32 operand,
    UINT   alignment)
{
    UINT remainder = (operand % alignment);

    return (0 == remainder) ? operand : operand - remainder + alignment;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// EvenCeilingUINT32
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UINT32 ChxUtils::EvenCeilingUINT32(
    UINT32 input)
{
    UINT32 result;

    if (0xFFFFFFFF == input)
    {
        result = input;
    }
    else if (0 != (input & 0x00000001))
    {
        result = input + 0x00000001;
    }
    else
    {
        result = input;
    }
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// EvenFloorUINT32
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UINT32 ChxUtils::EvenFloorUINT32(
    UINT32 input)
{
    return (input & 0xFFFFFFFE);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// FEqualCoarse
///
/// @brief  Compares the floating point numbers with reduced precision
///
/// @param  value1  The first number to compare
/// @param  value2  The second number to compare
///
/// @return BOOL TRUE if equal FALSE if not equal
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL ChxUtils::FEqualCoarse(
    FLOAT value1, FLOAT value2)
{
    return ((fabs(value1 - value2) < 1e-2) ? (TRUE) : (FALSE));
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Checks if a particular bit is set in a number
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL ChxUtils::IsBitSet(
    UINT32  number,
    UINT32  bit)
{
    static const UINT32 One = 1;

    CHX_ASSERT(bit < (sizeof(UINT32) * 8));

    return ((number & (One << bit)) != 0) ? TRUE : FALSE;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Resets a bit in a number
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UINT32 ChxUtils::BitReset(
    UINT32  number,
    UINT32  bit)
{
    static const UINT32 One = 1;

    CHX_ASSERT(bit < (sizeof(UINT32) * 8));

    return (number & ~(One << bit));
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Set a bit in a number
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UINT32 ChxUtils::BitSet(
    UINT32  number,
    UINT32  bit)
{
    static const UINT32 One = 1;

    CHX_ASSERT(bit < (sizeof(UINT32) * 8));

    return (number | (One << bit));
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ChxUtils::DPrintF
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
INT ChxUtils::DPrintF(
    INT         fd,
    const CHAR* pFormat,
                ...)
{
    INT     numCharWritten;
    va_list args;

    va_start(args, pFormat);
    numCharWritten = vdprintf(fd, pFormat, args);
    va_end(args);

    return numCharWritten;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// OsUtils::GetFileName
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
const CHAR* ChxUtils::GetFileName(
    const CHAR* pFilePath)
{
    const CHAR* pFileName = strrchr(pFilePath, '/');

    if (NULL != pFileName)
    {
        // StrRChr will return a pointer to the /, advance one to the filename
        pFileName += 1;
    }
    else
    {
        pFileName = pFilePath;
    }

    return pFileName;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// MemMap
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID* ChxUtils::MemMap(
    INT     fd,
    SIZE_T  length,
    SIZE_T  offset)
{
    VOID* pMem;

    pMem = mmap(NULL, length, (PROT_READ | PROT_WRITE), MAP_SHARED, fd, offset);
    if (MAP_FAILED == pMem)
    {
        pMem = NULL;
    }

    return pMem;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// MemUnmap
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
INT ChxUtils::MemUnmap(
    VOID*   pAddr,
    SIZE_T  length)
{
    return munmap(pAddr, length);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Memcpy
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID* ChxUtils::Memcpy(
    VOID*       pDst,
    const VOID* pSrc,
    SIZE_T      numBytes)
{
    CHX_ASSERT((pDst != NULL) && (pSrc != NULL));

    if ((pDst != NULL) && (pSrc != NULL))
    {
        return memcpy(pDst, pSrc, numBytes);
    }
    return NULL;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Memset
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID* ChxUtils::Memset(
    VOID*  pDst,
    INT    value,
    SIZE_T numBytes)
{
    return memset(pDst, value, numBytes);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Allocate zeroed out memory
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID* ChxUtils::Calloc(SIZE_T numBytes)
{
    VOID* pMem = malloc(numBytes);

    if (NULL != pMem)
    {
        Memset(pMem, 0, numBytes);
    }

    return pMem;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Free memory
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID ChxUtils::Free(VOID* pMem)
{
    free(pMem);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Function to create a thread
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ChxUtils::ThreadCreate(
    OSThreadFunc    threadEntryFunction,
    VOID*           pThreadData,
    OSThreadHandle* phThread)
{
    CDKResult result = CDKResultSuccess;

    if (pthread_create(phThread, 0, threadEntryFunction, pThreadData) != 0)
    {
        result = CDKResultEFailed;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Terminate a thread
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID ChxUtils::ThreadTerminate(OSThreadHandle hThread)
{
    pthread_join(hThread, NULL);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// 64 bit atomic store
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID ChxUtils::AtomicStore64(volatile INT64* pVar,
                             INT64           val)
{
    (VOID)__atomic_store_n(pVar, val, __ATOMIC_RELAXED);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// 64 bit atomic store
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID ChxUtils::AtomicStoreU64(volatile UINT64* pVar,
                                        UINT64           val)
{
    (VOID)__atomic_store_n(pVar, val, __ATOMIC_RELAXED);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// 64 bit atomic load
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UINT64 ChxUtils::AtomicLoadU64(volatile UINT64* pVar)
{
    return (__sync_add_and_fetch(pVar, 0));
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// 32-bit atomic store
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID ChxUtils::AtomicStoreU32(volatile UINT32* pVar,
                              UINT32           val)
{
    (VOID)__atomic_store_n(pVar, val, __ATOMIC_RELAXED);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// 32-bit atomic load
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UINT32 ChxUtils::AtomicLoadU32(volatile UINT32* pVar)
{
    return (__sync_add_and_fetch(pVar, 0));
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// 32-bit atomic increment
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UINT32 ChxUtils::AtomicIncU32(volatile UINT32* pVar)
{
    return (__sync_add_and_fetch(pVar, 1));
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// 32-bit atomic decrement
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UINT32 ChxUtils::AtomicDecU32(volatile UINT32* pVar)
{
    return (__sync_sub_and_fetch(pVar, 1));
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Sleep in microseconds
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID ChxUtils::SleepMicroseconds(UINT microseconds)
{
    CHX_LOG("Sleeping for %u microseconds", microseconds);
    usleep(microseconds);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Get the address of the function in the library
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID* ChxUtils::LibGetAddr(OSLIBRARYHANDLE hLibrary,
                           const CHAR*     pProcName)
{
    VOID* pProcAddr = NULL;

    if (hLibrary != NULL)
    {
        pProcAddr = dlsym(hLibrary, pProcName);
    }

    return pProcAddr;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Open the passed in library
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
OSLIBRARYHANDLE ChxUtils::LibMap(const CHAR* pLibraryName)
{
    OSLIBRARYHANDLE hLibrary  = NULL;

    const UINT bindFlags = RTLD_NOW | RTLD_LOCAL;

    hLibrary = dlopen(pLibraryName, bindFlags);

    if (NULL == hLibrary)
    {
        CHX_LOG_ERROR("Failed to load library %s error %s", pLibraryName, dlerror());
        CHX_ASSERT(0 == dlerror());
    }

    return hLibrary;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Check if a vendor tag is present or not
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL ChxUtils::AndroidMetadata::IsVendorTagPresent(
    const VOID* pMetadata,
    VendorTag   tagEnum)
{
    BOOL isPresent = FALSE;
    camera_metadata_entry_t entry;

    UINT32 tagId = ExtensionModule::GetInstance()->GetVendorTagId(tagEnum);

    if (0 == find_camera_metadata_entry((camera_metadata_t*)(pMetadata), tagId, &entry))
    {
        isPresent = TRUE;
    }

    return isPresent;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Check if snapshot is long exposure capture
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL ChxUtils::AndroidMetadata::IsLongExposureCapture(
     const VOID* pMetadata)
{
    BOOL isLongExposure = FALSE;
    UINT64 exposureTime = 0;
    camera_metadata_entry_t entry;

    if (0 == find_camera_metadata_entry((camera_metadata_t*)(pMetadata), ANDROID_SENSOR_EXPOSURE_TIME, &entry))
    {
        exposureTime = *(entry.data.i64);

        if (exposureTime / (1000 * 1000 * 1000) >= 2)
        {
            CHX_LOG("Long exposure capture %" PRIu64, exposureTime);
            isLongExposure = TRUE;
        }
    }
        return isLongExposure;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Get vendor tag value
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID ChxUtils::AndroidMetadata::GetVendorTagValue(
    const VOID* pMetadata,
    VendorTag   tagEnum,
    VOID**      ppData)
{
    camera_metadata_entry_t entry;

    *ppData = NULL;

    UINT32 tagId = ExtensionModule::GetInstance()->GetVendorTagId(tagEnum);

    if (0 == find_camera_metadata_entry((camera_metadata_t*)(pMetadata), tagId, &entry))
    {
        *ppData = static_cast<VOID*>(entry.data.u8);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Set vendor tag value
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ChxUtils::AndroidMetadata::SetVendorTagValue(
    VOID*     pMetadata,
    VendorTag tagEnum,
    UINT      dataCount,
    VOID*     pData)
{
    camera_metadata_entry_t entry;
    camera_metadata_t*      pDstMetadata = (camera_metadata_t*)(pMetadata);
    UINT status = 0;
    UINT32 tagId = ExtensionModule::GetInstance()->GetVendorTagId(tagEnum);

    if (0 == find_camera_metadata_entry(pDstMetadata, tagId, &entry))
    {
        camera_metadata_entry_t updatedEntry;

        status = update_camera_metadata_entry(pDstMetadata, entry.index, pData, dataCount, &updatedEntry);
    }
    else
    {
        status = add_camera_metadata_entry(pDstMetadata, tagId, pData, dataCount);
    }
    CHX_ASSERT(status == 0);

    return status;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Merge metadata
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
INT ChxUtils::AndroidMetadata::MergeMetadata(
    VOID* pMetadata1,
    VOID* pMetadata2)
{
    INT    status       = 0;
    SIZE_T totalEntries = 0;

    CHX_ASSERT(NULL != pMetadata1);
    CHX_ASSERT(NULL != pMetadata2);

    if ((NULL == pMetadata1) || (NULL == pMetadata2))
    {
        return status;
    }

    totalEntries = get_camera_metadata_entry_count(reinterpret_cast<const camera_metadata_t *>(pMetadata1));

    for (UINT i = 0; i < totalEntries; i++)
    {
        camera_metadata_entry_t srcEntry;
        camera_metadata_entry_t dstEntry;
        camera_metadata_entry_t updatedEntry;

        get_camera_metadata_entry(reinterpret_cast<camera_metadata*>(pMetadata1), i, &srcEntry);
        status = find_camera_metadata_entry(reinterpret_cast<camera_metadata*>(pMetadata2), srcEntry.tag, &dstEntry);
        if (0 != status)
        {
            status = add_camera_metadata_entry(reinterpret_cast<camera_metadata*>(pMetadata2),
                         srcEntry.tag,
                         srcEntry.data.i32,
                         srcEntry.count);
        }
        else
        {
            if (0 == srcEntry.count)
            {
                status = delete_camera_metadata_entry(GetMetadataType(pMetadata2), srcEntry.tag);
                if (0 == status)
                {
                    status = add_camera_metadata_entry(GetMetadataType(pMetadata2),
                                                       srcEntry.tag,
                                                       srcEntry.data.i32,
                                                       srcEntry.count);
                }
            }
            else if ((0 != memcmp(srcEntry.data.u8,
                             dstEntry.data.u8,
                             (camera_metadata_type_size[srcEntry.type] * srcEntry.count))) ||
                (srcEntry.count != dstEntry.count)                                         ||
                (srcEntry.type  != dstEntry.type))
            {
                status = update_camera_metadata_entry(reinterpret_cast<camera_metadata*>(pMetadata2),
                             dstEntry.index,
                             srcEntry.data.i32,
                             srcEntry.count,
                             &updatedEntry);
            }
        }

        if (0 != status)
        {
            break;
        }
    }

    return status;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// AndroidMetadata::FreeMetaData
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID ChxUtils::AndroidMetadata::FreeMetaData(
    VOID* pMetadata)
{
    if (pMetadata != NULL)
    {
        free_camera_metadata((camera_metadata_t *)pMetadata);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// AndroidMetadata::ResetMetadata
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
camera_metadata_t* ChxUtils::AndroidMetadata::ResetMetadata(
    camera_metadata* pMetadata)
{
    UINT entryCapacity = get_camera_metadata_entry_capacity(pMetadata);
    UINT dataCapacity  = get_camera_metadata_data_capacity(pMetadata);
    UINT metadataSize  = calculate_camera_metadata_size(entryCapacity, dataCapacity);

    CHX_LOG("Metadata entry capacity : %d, data capacity: %d, metaSize: %d", entryCapacity, dataCapacity, metadataSize);

    // Reset the metadata to empty
    return place_camera_metadata(pMetadata, metadataSize, entryCapacity, dataCapacity);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// AndroidMetadata::AllocateMetaData
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID* ChxUtils::AndroidMetadata::AllocateMetaData(UINT32  entry_count, UINT32  data_count)
{
    return allocate_camera_metadata(entry_count, data_count);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// AndroidMetadata::AllocateAndAppendMetaData
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID* ChxUtils::AndroidMetadata::AllocateAppendMetaData(
    const VOID* pMetadata,
    UINT32      entry_count,
    UINT32      data_count)
{
    int res;

    if (pMetadata == NULL)
        return NULL;

    camera_metadata_t *clone = allocate_camera_metadata(
        get_camera_metadata_entry_count((camera_metadata_t*)pMetadata)+entry_count,
        get_camera_metadata_data_count((camera_metadata_t*)pMetadata)+data_count);

    if (clone != NULL) {
        res = append_camera_metadata(clone, (camera_metadata_t*)pMetadata);
        if (res != 0) {
            free_camera_metadata(clone);
            clone = NULL;
        }
    }
    CHX_ASSERT(validate_camera_metadata_structure(clone, NULL) == 0);
    return clone;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// AndroidMetadata::AllocateCopyMetaData
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID* ChxUtils::AndroidMetadata::AllocateCopyMetaData(
    const VOID* pSrcMetadata)
{
    camera_metadata_t* pMetadata;
    pMetadata = (camera_metadata_t*)allocate_copy_camera_metadata_checked(reinterpret_cast<const camera_metadata_t*>(pSrcMetadata),
        get_camera_metadata_size(reinterpret_cast<const camera_metadata_t*>(pSrcMetadata)));
    return (VOID*)pMetadata;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// AndroidMetadata::GetFeature1Mode
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
ChiModeFeature1SubModeType  ChxUtils::AndroidMetadata::GetFeature1Mode(
    VOID *  pMetaData,
    UINT32* pFeature1Value)
{
    camera_metadata_entry_t    entry           = { 0 };
    ChiModeFeature1SubModeType feature1Mode    = ChiModeFeature1SubModeType::None;
    UINT32                     tagId           = ExtensionModule::GetInstance()->GetVendorTagId(VendorTag::Feature1Mode);

    if (0 == find_camera_metadata_entry((camera_metadata_t*)(pMetaData), tagId, &entry))
    {
        feature1Mode    = static_cast<ChiModeFeature1SubModeType>(*(entry.data.u8));
        *pFeature1Value = static_cast<unsigned int>(feature1Mode);
    }
    else
    {
        feature1Mode = static_cast<ChiModeFeature1SubModeType>(*pFeature1Value);
    }

    CHX_LOG("Utils Feature1 mode %d", feature1Mode);

    return feature1Mode;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// GetFeature2Mode
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
ChiModeFeature2SubModeType  ChxUtils::GetFeature2Mode(
    UINT32* pFeature2Value)
{
    ChiModeFeature2SubModeType feature2Mode = static_cast<ChiModeFeature2SubModeType>(*pFeature2Value);

    if (feature2Mode == ChiModeFeature2SubModeType::OISCapture)
    {
       return ChiModeFeature2SubModeType::OISCapture;
    }
    else if (feature2Mode == ChiModeFeature2SubModeType::HLG)
    {
        return ChiModeFeature2SubModeType::HLG;
    }
    else if (feature2Mode == ChiModeFeature2SubModeType::HDR10)
    {
        return ChiModeFeature2SubModeType::HDR10;
    }
    else if (feature2Mode == ChiModeFeature2SubModeType::MFNRBlend)
    {
        return ChiModeFeature2SubModeType::MFNRBlend;
    }
    else if (feature2Mode == ChiModeFeature2SubModeType::MFNRPostFilter)
    {
        return ChiModeFeature2SubModeType::MFNRPostFilter;
    }
    else if (feature2Mode == ChiModeFeature2SubModeType::MFSRBlend)
    {
        return ChiModeFeature2SubModeType::MFSRBlend;
    }
    else if (feature2Mode == ChiModeFeature2SubModeType::MFSRPostFilter)
    {
        return ChiModeFeature2SubModeType::MFSRPostFilter;
    }
    else if (feature2Mode == ChiModeFeature2SubModeType::OfflineNoiseReprocess)
    {
        return ChiModeFeature2SubModeType::OfflineNoiseReprocess;
    }
    else
    {
        return ChiModeFeature2SubModeType::None;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ChxUtils::PopulateHALToChiStreamBuffer
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID ChxUtils::PopulateHALToChiStreamBuffer(
    const camera3_stream_buffer_t*  pCamera3StreamBuffer,
    CHISTREAMBUFFER*                pChiStreamBuffer)
{
    CHX_ASSERT(NULL != pCamera3StreamBuffer);
    CHX_ASSERT(NULL != pChiStreamBuffer);

    pChiStreamBuffer->size                  = sizeof(CHISTREAMBUFFER);

    ///< @todo (CAMX-4113) Decouple CHISTREAM and camera3_stream
    pChiStreamBuffer->pStream               = reinterpret_cast<CHISTREAM*>(pCamera3StreamBuffer->stream);
    pChiStreamBuffer->bufferInfo.bufferType = HALGralloc;
    pChiStreamBuffer->bufferInfo.phBuffer   = reinterpret_cast<CHIBUFFERHANDLE>(pCamera3StreamBuffer->buffer);
    pChiStreamBuffer->bufferStatus          = pCamera3StreamBuffer->status;
    pChiStreamBuffer->acquireFence.valid    = FALSE;
    pChiStreamBuffer->releaseFence.valid    = FALSE;

    if (InvalidNativeFence != pCamera3StreamBuffer->acquire_fence)
    {
        pChiStreamBuffer->acquireFence.valid            = TRUE;
        pChiStreamBuffer->acquireFence.type             = ChiFenceTypeNative;
        pChiStreamBuffer->acquireFence.nativeFenceFD    = pCamera3StreamBuffer->acquire_fence;
    }

    if (InvalidNativeFence != pCamera3StreamBuffer->release_fence)
    {
        pChiStreamBuffer->releaseFence.valid            = TRUE;
        pChiStreamBuffer->releaseFence.type             = ChiFenceTypeNative;
        pChiStreamBuffer->releaseFence.nativeFenceFD    = pCamera3StreamBuffer->release_fence;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ChxUtils::PopulateChiToHALStreamBuffer
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID ChxUtils::PopulateChiToHALStreamBuffer(
    const CHISTREAMBUFFER*      pChiStreamBuffer,
    camera3_stream_buffer_t*    pCamera3StreamBuffer)
{
    CHX_ASSERT(NULL != pCamera3StreamBuffer);
    CHX_ASSERT(NULL != pChiStreamBuffer);

    if (pChiStreamBuffer->size != sizeof(CHISTREAMBUFFER))
    {
        CHX_LOG_ERROR("Size mismatch %u %u", pChiStreamBuffer->size, static_cast<UINT32>(sizeof(CHISTREAMBUFFER)));
    }
    else if ((HALGralloc != pChiStreamBuffer->bufferInfo.bufferType) &&
             (ChiGralloc != pChiStreamBuffer->bufferInfo.bufferType))
    {
        // We can actually allow internal native_buffers to be passed as camera3_buffers
        // but if internal buffer type is ImageBuffer, we can't convey that while populating camera3_stream_buffer_t
        CHX_LOG_ERROR("Incorrect buffer handle type %d", pChiStreamBuffer->bufferInfo.bufferType);
    }
    else
    {
        ///< @todo (CAMX-4113) Decouple CHISTREAM and camera3_stream
        pCamera3StreamBuffer->stream        = reinterpret_cast<camera3_stream*>(pChiStreamBuffer->pStream);
        pCamera3StreamBuffer->buffer        = reinterpret_cast<buffer_handle_t *>(pChiStreamBuffer->bufferInfo.phBuffer);
        pCamera3StreamBuffer->status        = pChiStreamBuffer->bufferStatus;
        pCamera3StreamBuffer->acquire_fence = InvalidNativeFence;
        pCamera3StreamBuffer->release_fence = InvalidNativeFence;

        if (TRUE == IsValidNativeFenceType(&pChiStreamBuffer->acquireFence))
        {
            pCamera3StreamBuffer->acquire_fence = pChiStreamBuffer->acquireFence.nativeFenceFD;
        }

        if (TRUE == IsValidNativeFenceType(&pChiStreamBuffer->releaseFence))
        {
            pCamera3StreamBuffer->release_fence = pChiStreamBuffer->releaseFence.nativeFenceFD;
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ChxUtils::DeepCopyCamera3CaptureRequest
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID ChxUtils::DeepCopyCamera3CaptureRequest(
    const camera3_capture_request_t*  pSrcReq,
    camera3_capture_request_t*        pDestReq)
{
    pDestReq->frame_number       = pSrcReq->frame_number;
    pDestReq->settings           = pSrcReq->settings;
    pDestReq->input_buffer       = pSrcReq->input_buffer;
    pDestReq->num_output_buffers = pSrcReq->num_output_buffers;

    CHX_ASSERT(NULL != pDestReq->output_buffers);

    if (0 < pSrcReq->num_output_buffers)
    {
        for (UINT i = 0; i < pSrcReq->num_output_buffers; i++)
        {
            const_cast<camera3_stream_buffer_t*>(&pDestReq->output_buffers[i])->stream        =
                pSrcReq->output_buffers[i].stream;
            const_cast<camera3_stream_buffer_t*>(&pDestReq->output_buffers[i])->buffer        =
                pSrcReq->output_buffers[i].buffer;
            const_cast<camera3_stream_buffer_t*>(&pDestReq->output_buffers[i])->status        =
                pSrcReq->output_buffers[i].status;
            const_cast<camera3_stream_buffer_t*>(&pDestReq->output_buffers[i])->acquire_fence =
                pSrcReq->output_buffers[i].acquire_fence;
            const_cast<camera3_stream_buffer_t*>(&pDestReq->output_buffers[i])->release_fence =
                pSrcReq->output_buffers[i].release_fence;
        }
    }
}



////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ChxUtils::GetUsecaseMode
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
ChiModeUsecaseSubModeType ChxUtils::GetUsecaseMode(
    camera3_capture_request_t* pRequest)
{
    UINT                      tuningUsecaseMask    = 0;
    ChiModeUsecaseSubModeType chiTuningUsecaseMode = ChiModeUsecaseSubModeType::Preview;

    for (UINT streamIndex = 0; streamIndex < pRequest->num_output_buffers; streamIndex++)
    {
        if (UsecaseSelector::IsVideoStream(pRequest->output_buffers[streamIndex].stream))
        {
            tuningUsecaseMask |= tuningUsecaseVideoMask;
        }
        else if (UsecaseSelector::IsYUVSnapshotStream(pRequest->output_buffers[streamIndex].stream) ||
                 UsecaseSelector::IsJPEGSnapshotStream(pRequest->output_buffers[streamIndex].stream))
        {
            tuningUsecaseMask |= tuningUsecaseSnapshotMask;
        }
        else if (UsecaseSelector::IsPreviewStream(pRequest->output_buffers[streamIndex].stream))
        {
            tuningUsecaseMask |= tuningUsecasePreviewMask;
        }
        /// @todo (CAMX-2924) check ZSL usecase as well from app.
    }

    if ((tuningUsecaseMask & tuningUsecaseVideoMask) &&
        (tuningUsecaseMask & tuningUsecaseSnapshotMask))
    {
        chiTuningUsecaseMode = ChiModeUsecaseSubModeType::Liveshot;
    }
    else if (tuningUsecaseMask & (tuningUsecaseSnapshotMask))
    {
        chiTuningUsecaseMode = ChiModeUsecaseSubModeType::Snapshot;
    }
    else if (tuningUsecaseMask & (tuningUsecaseVideoMask))
    {
        chiTuningUsecaseMode = ChiModeUsecaseSubModeType::Video;
    }
    else if (tuningUsecaseMask & (tuningUsecaseZSLMask))
    {
        chiTuningUsecaseMode = ChiModeUsecaseSubModeType::ZSL;
    }
    else if (tuningUsecaseMask & (tuningUsecasePreviewMask))
    {
        chiTuningUsecaseMode = ChiModeUsecaseSubModeType::Preview;
    }

    return chiTuningUsecaseMode;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ChxUtils::SkipFrame
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID ChxUtils::SkipFrame(
    camera3_stream_buffer_t* pBuffer)
{
    // set buffer status to error, so framework will drop this frame
    pBuffer->status = CAMERA3_BUFFER_STATUS_ERROR;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Usecase::AndroidMetadata::GetSceneMode
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
ChiModeSceneSubModeType ChxUtils::AndroidMetadata::GetSceneMode(
    VOID *  pMetaData,
    UINT32* pEffectModeValue)
{
    camera_metadata_entry_t entry     = { 0 };
    UINT32 modeValue                  = ANDROID_CONTROL_SCENE_MODE_DISABLED;

    if (0 == find_camera_metadata_entry((camera_metadata_t*)(pMetaData), ANDROID_CONTROL_MODE, &entry))
    {
        UINT32 controlModeValue = *(entry.data.u8);
        if ((ANDROID_CONTROL_MODE_USE_SCENE_MODE == controlModeValue) &&
            (0 == find_camera_metadata_entry((camera_metadata_t*)(pMetaData), ANDROID_CONTROL_SCENE_MODE, &entry)))
        {
            modeValue = *(entry.data.u8);
            *pEffectModeValue = modeValue;
        }
        else
        {
            modeValue = *pEffectModeValue;
        }
    }
    else
    {
        modeValue = *pEffectModeValue;
    }

    return SceneMap[modeValue].to;
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Get vendor tag value
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID ChxUtils::GetVendorTagValue(
    ChiMetadata* pMetadata,
    VendorTag   tagEnum,
    VOID**      ppData)
{
    *ppData = NULL;

    UINT32 tagId = ExtensionModule::GetInstance()->GetVendorTagId(tagEnum);

    *ppData = reinterpret_cast<UINT*>(pMetadata->GetTag(tagId));

    if (NULL == *ppData)
    {
        CHX_LOG("Failed to get data for vendor tagId: %x", tagId);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ChxUtils::AndroidMetadata::GetEffectMode
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
ChiModeEffectSubModeType ChxUtils::AndroidMetadata::GetEffectMode(
    VOID*   pMetaData,
    UINT32* pEffectModeValue)
{
    camera_metadata_entry_t entry      = { 0 };
    UINT32 modeValue                   = ANDROID_CONTROL_EFFECT_MODE_OFF;

    if (0 == find_camera_metadata_entry((camera_metadata_t*)(pMetaData), ANDROID_CONTROL_EFFECT_MODE, &entry))
    {
        modeValue = *(entry.data.u8);
        *pEffectModeValue = modeValue;
    }
    else
    {
        modeValue = *pEffectModeValue;
    }

    return EffectMap[modeValue].to;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// NativeFenceWait
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ChxUtils::NativeFenceWait(
    NativeFence hFence,
    UINT32 timeoutMilliseconds)
{
    CDKResult result = CamxResultEFailed;

#ifdef ANDROID
    INT status = sync_wait(hFence, timeoutMilliseconds);
    if (0 == status)
    {
        result = CDKResultSuccess;
    }
#endif // ANDROID

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Close
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ChxUtils::Close(INT FD)
{
    CDKResult result = CDKResultSuccess;
    if (-1 != FD)
    {
        if (0 != close(FD))
        {
            result = CDKResultEFailed;
        }
    }
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// readSocID
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
INT32 ChxUtils::ReadSocID()
{
    int fd;
    int soc = -1;
    if (!access("/sys/devices/soc0/soc_id", F_OK))
    {
        fd = open("/sys/devices/soc0/soc_id", O_RDONLY);
    }
    else
    {
        fd = open("/sys/devices/system/soc/soc0/id", O_RDONLY);
    }
    if (fd != -1)
    {
        char raw_buf[5];
        read(fd, raw_buf,4);
        raw_buf[4] = 0;
        soc = atoi(raw_buf);
        close(fd);
    }
    else
    {
        close(fd);
    }

    return soc;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// WaitOnAcquireFence
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID ChxUtils::WaitOnAcquireFence(
    const CHISTREAMBUFFER* pBuffer)
{
    if (TRUE == IsValidNativeFenceType(&pBuffer->acquireFence))
    {
        NativeFence acquireFence = pBuffer->acquireFence.nativeFenceFD;

        CHX_LOG("Wait on acquireFence %d ", acquireFence);

        INT error = 1;

        if (error < 0)
        {
            CHX_LOG_ERROR("sync_wait timedout! error = %s", strerror(errno));
        }

        CHX_LOG("Close fence fd %d ", acquireFence);

        close(acquireFence);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// WaitOnAcquireFence
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID ChxUtils::WaitOnAcquireFence(
    const camera3_stream_buffer* pBuffer)
{
    if (-1 != pBuffer->acquire_fence)
    {
        NativeFence acquireFence = pBuffer->acquire_fence;

        CHX_LOG("Wait on acquireFence %d ", acquireFence);

        CDKResult result = CDKResultSuccess;

        //TODO: backporting-flush resolve this
        result = ChxUtils::NativeFenceWait(acquireFence, 5000);

        if (CDKResultSuccess != result)
        {
            CHX_LOG_ERROR("sync_wait timedout! error = %s", strerror(errno));
        }

        CHX_LOG_INFO("Close fence fd %d ", acquireFence);
        close(acquireFence);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ChxUtils::AndroidMetadata::FillCameraId
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID ChxUtils::AndroidMetadata::FillCameraId(
    VOID*   pMetaData,
    UINT32  cameraId)
{
    if (pMetaData != NULL)
    {
        MultiCameraIds* pData         = NULL;
        MultiCameraIds  inputMetadata = {0, 0};

        ChxUtils::AndroidMetadata::GetVendorTagValue(pMetaData, VendorTag::MultiCamera, (VOID**)&pData);
        if (NULL != pData)
        {
            inputMetadata = *pData;
        }
        inputMetadata.currentCameraId = cameraId;
        CHX_LOG_INFO("FillCameraId %d ", cameraId);
        ChxUtils::AndroidMetadata::SetVendorTagValue(pMetaData, VendorTag::MultiCamera, sizeof(inputMetadata), &inputMetadata);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ChxUtils::AndroidMetadata::GetFpsRange
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ChxUtils::AndroidMetadata::GetFpsRange(
    const VOID*     pMetaData,
    INT32*          pMinFps,
    INT32*          pMaxFps)
{
    CDKResult               result = CDKResultENoSuch;
    camera_metadata_entry_t entry = { 0 };

    if (0 == find_camera_metadata_entry((camera_metadata_t*)(pMetaData), ANDROID_CONTROL_AE_TARGET_FPS_RANGE, &entry))
    {
        INT32* pRange  = entry.data.i32;
        *pMinFps = pRange[0];
        *pMaxFps = pRange[1];
        result   = CDKResultSuccess;

        CHX_LOG_INFO("FPS Range %d %d", pRange[0], pRange[1]);
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ChxUtils::AndroidMetadata::GetZSLMode
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
INT ChxUtils::AndroidMetadata::GetZSLMode(
    VOID*  pMetaData)
{
    camera_metadata_entry_t entry           = { 0 };
    UINT32 controlZSLValue                  = ANDROID_CONTROL_ENABLE_ZSL_TRUE;

    if (0 == find_camera_metadata_entry((camera_metadata_t*)(pMetaData), ANDROID_CONTROL_ENABLE_ZSL, &entry))
    {
        controlZSLValue = *(entry.data.u8);
    }

    CHX_LOG("Utils ZslMode Value %d", controlZSLValue);

    return controlZSLValue;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ChxUtils::AndroidMetadata::FillTuningModeData
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID ChxUtils::AndroidMetadata::FillTuningModeData(
    VOID*                      pMetaData,
    camera3_capture_request_t* pRequest,
    UINT32                     sensorModeIndex,
    UINT32*                    pEffectModeValue,
    UINT32*                    pSceneModeValue,
    UINT32*                    pFeature1Value,
    UINT32*                    pFeature2Value)
{
    if (pMetaData != NULL)
    {
        ChiTuningModeParameter chiTuningModeParameter = {0};

        chiTuningModeParameter.noOfSelectionParameter = MaxTuningMode;

        chiTuningModeParameter.TuningMode[0].mode              = ChiModeType::Default;
        chiTuningModeParameter.TuningMode[0].subMode.value     = 0;
        chiTuningModeParameter.TuningMode[1].mode              = ChiModeType::Sensor;
        chiTuningModeParameter.TuningMode[1].subMode.value     = sensorModeIndex;
        chiTuningModeParameter.TuningMode[2].mode              = ChiModeType::Usecase;
        chiTuningModeParameter.TuningMode[2].subMode.usecase   = GetUsecaseMode(pRequest);
        chiTuningModeParameter.TuningMode[3].mode              = ChiModeType::Feature1;
        chiTuningModeParameter.TuningMode[3].subMode.feature1  = GetFeature1Mode(pMetaData, pFeature1Value);
        chiTuningModeParameter.TuningMode[4].mode              = ChiModeType::Feature2;
        chiTuningModeParameter.TuningMode[4].subMode.feature2  = GetFeature2Mode(pFeature2Value);
        chiTuningModeParameter.TuningMode[5].mode              = ChiModeType::Scene;
        chiTuningModeParameter.TuningMode[5].subMode.scene     = GetSceneMode(pMetaData, pSceneModeValue);
        chiTuningModeParameter.TuningMode[6].mode              = ChiModeType::Effect;
        chiTuningModeParameter.TuningMode[6].subMode.effect    = GetEffectMode(pMetaData, pEffectModeValue);

        ChxUtils::AndroidMetadata::SetVendorTagValue(pMetaData, VendorTag::TuningMode, sizeof(ChiTuningModeParameter),
            &chiTuningModeParameter);

        CHX_LOG_VERBOSE("SensorMode %d UC %u feature %u %d scene %d effect %d", sensorModeIndex,
                        chiTuningModeParameter.TuningMode[2].subMode.usecase,
                        chiTuningModeParameter.TuningMode[3].subMode.feature1,
                        chiTuningModeParameter.TuningMode[3].subMode.feature2,
                        chiTuningModeParameter.TuningMode[5].subMode.scene,
                        chiTuningModeParameter.TuningMode[6].subMode.effect);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ChxUtils::AndroidMetadata::UpdateTimeStamp
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ChxUtils::AndroidMetadata::UpdateTimeStamp(
   camera_metadata_t*  pMetadata,
   UINT64              timestamp,
   UINT32              frameNum)
{
    camera_metadata_entry_t entry           = { 0 };
    INT32  status;

    if (0 == find_camera_metadata_entry((camera_metadata_t*)(pMetadata), ANDROID_SENSOR_TIMESTAMP, &entry))
    {
        status = delete_camera_metadata_entry(pMetadata, entry.index);
    }

    status = add_camera_metadata_entry(pMetadata,
                                       ANDROID_SENSOR_TIMESTAMP,
                                       &timestamp,
                                       1);

    CHX_LOG("Update Timestamp %" PRIu64 " frameNum %u status %d timestamp %" PRIu64,
            timestamp, frameNum, status, GetTimeStamp(pMetadata));

    return (0 == status) ? CDKResultSuccess : CDKResultEFailed;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ChxUtils::AndroidMetadata::GetTimeStamp
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UINT64 ChxUtils::AndroidMetadata::GetTimeStamp(
   camera_metadata_t*  pMetadata)
{
    camera_metadata_entry_t entry     = { 0 };
    UINT64                  timestamp = ANDROID_SENSOR_TIMESTAMP;
    INT32                   status;

    if (0 == find_camera_metadata_entry((camera_metadata_t*)(pMetadata), ANDROID_SENSOR_TIMESTAMP, &entry))
    {
        timestamp = *(entry.data.i64);
    }

    return timestamp;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ChxUtils::GetZSLMode
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
INT ChxUtils::GetZSLMode(
    ChiMetadata*  pMetaData)
{
    UINT32 controlZSLValue = ANDROID_CONTROL_ENABLE_ZSL_TRUE;

    INT* pControlZSL = static_cast<INT*>(pMetaData->GetTag(ANDROID_CONTROL_ENABLE_ZSL));
    if (NULL != pControlZSL)
    {
        controlZSLValue = *pControlZSL;
    }

    CHX_LOG("Utils ZslMode Value %d", controlZSLValue);

    return controlZSLValue;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ChxUtils::GetFlashMode
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL ChxUtils::GetFlashMode(
    ChiMetadata*  pMetaData)
{
    UINT32 controlFLASHMode = ANDROID_CONTROL_AE_MODE_OFF;
    BOOL   isFlashMode      = FALSE;

    INT* pControlFLASH = static_cast<INT*>(pMetaData->GetTag(ANDROID_CONTROL_AE_MODE));
    if (NULL != pControlFLASH)
    {
        controlFLASHMode = *pControlFLASH;
    }

    CHX_LOG_INFO("Utils FLASH mode from APP %d", controlFLASHMode);

    isFlashMode = (ANDROID_CONTROL_AE_MODE_ON_ALWAYS_FLASH == controlFLASHMode) ? TRUE : FALSE;

    return isFlashMode;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ChxUtils::GetFlashFiredState
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL ChxUtils::GetFlashFiredState(
    ChiMetadata*  pMetaData)
{
    UINT32 controlFLASHValue = ANDROID_FLASH_STATE_UNAVAILABLE;
    BOOL   isFlashFired      = FALSE;

    INT* pControlFLASH = static_cast<INT*>(pMetaData->GetTag(ANDROID_FLASH_STATE));
    if (NULL != pControlFLASH)
    {
        controlFLASHValue = *pControlFLASH;
    }

    CHX_LOG_INFO("Utils flashfired state Value %d", controlFLASHValue);

    isFlashFired = (ANDROID_FLASH_STATE_FIRED == controlFLASHValue) ? TRUE : FALSE;

    return isFlashFired;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ChxUtils::SetVendorTagValue
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ChxUtils::SetVendorTagValue(
    ChiMetadata*     pMetadata,
    VendorTag        tagEnum,
    UINT             dataCount,
    VOID*            pData)
{
    CDKResult result = CDKResultSuccess;
    UINT32 tagId = ExtensionModule::GetInstance()->GetVendorTagId(tagEnum);

    result = pMetadata->SetTag(tagId, pData, dataCount);

    CHX_ASSERT(result == CDKResultSuccess);

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ChxUtils::GetSceneMode
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
ChiModeSceneSubModeType ChxUtils::GetSceneMode(
    ChiMetadata* pMetaData,
    UINT32 * pSceneModeValue)
{
    UINT32* pData;
    UINT32 modeValue = ANDROID_CONTROL_SCENE_MODE_DISABLED;

    pData = reinterpret_cast<UINT32*>(pMetaData->GetTag(ANDROID_CONTROL_MODE));

    if (NULL != pData)
    {
        UINT32 controlModeValue = *pData;
        pData = reinterpret_cast<UINT32*>(pMetaData->GetTag(ANDROID_CONTROL_SCENE_MODE));
        if ((ANDROID_CONTROL_MODE_USE_SCENE_MODE == controlModeValue) &&
            (NULL != pData))
        {
            modeValue = *pData;
            *pSceneModeValue = modeValue;
        }
        else
        {
            modeValue = *pSceneModeValue;
        }
    }
    else
    {
        modeValue = *pSceneModeValue;
    }

    return SceneMap[modeValue].to;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ChxUtils::GetEffectMode
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
ChiModeEffectSubModeType ChxUtils::GetEffectMode(
    ChiMetadata* pMetaData,
    UINT32* pEffectModeValue)
{
    UINT8* pData;
    UINT8 modeValue = ANDROID_CONTROL_EFFECT_MODE_OFF;

    pData = static_cast<UINT8*>(pMetaData->GetTag(ANDROID_CONTROL_EFFECT_MODE));

    if (NULL != pData)
    {
        modeValue = *pData;
        *pEffectModeValue = modeValue;
    }
    else
    {
        modeValue = *pEffectModeValue;
    }

    return EffectMap[modeValue].to;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ChxUtils::GetFeature1Mode
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
ChiModeFeature1SubModeType ChxUtils::GetFeature1Mode(
    ChiMetadata* pMetaData,
    UINT32*      pFeature1Value)
{
    UINT8* pData;
    ChiModeFeature1SubModeType feature1Mode = ChiModeFeature1SubModeType::None;
    UINT32                     tagId = ExtensionModule::GetInstance()->GetVendorTagId(VendorTag::Feature1Mode);

    pData = static_cast<UINT8*>(pMetaData->GetTag(tagId));

    if (NULL != pData)
    {
        feature1Mode = static_cast<ChiModeFeature1SubModeType>(*pData);
        *pFeature1Value = static_cast<unsigned int>(feature1Mode);
    }
    else
    {
        feature1Mode = static_cast<ChiModeFeature1SubModeType>(*pFeature1Value);
    }

    CHX_LOG("Utils Feature1 mode %d", feature1Mode);

    return feature1Mode;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ChxUtils::FillCameraId
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID ChxUtils::FillCameraId(
    ChiMetadata* pMetaData,
    UINT32       cameraId)
{
    if (pMetaData != NULL)
    {
        MultiCameraIds* pData         = NULL;
        MultiCameraIds  inputMetadata = {0, 0};

        ChxUtils::GetVendorTagValue(pMetaData, VendorTag::MultiCamera, (VOID**)&pData);
        if (NULL != pData)
        {
            inputMetadata = *pData;
        }
        inputMetadata.currentCameraId = cameraId;
        CHX_LOG("FillCameraId %d ", cameraId);
        ChxUtils::SetVendorTagValue(pMetaData, VendorTag::MultiCamera, sizeof(inputMetadata), &inputMetadata);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ChxUtils::FillTuningModeData
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID ChxUtils::FillTuningModeData(
    ChiMetadata*               pMetaData,
    ChiModeUsecaseSubModeType  usecaseMode,
    UINT32                     sensorModeIndex,
    UINT32*                    pEffectModeValue,
    UINT32*                    pSceneModeValue,
    UINT32*                    pFeature1Value,
    UINT32*                    pFeature2Value)
{
    if (pMetaData != NULL)
    {
        ChiTuningModeParameter chiTuningModeParameter = { 0 };
        chiTuningModeParameter.noOfSelectionParameter = MaxTuningMode;

        chiTuningModeParameter.TuningMode[0].mode               = ChiModeType::Default;
        chiTuningModeParameter.TuningMode[0].subMode.value      = 0;
        chiTuningModeParameter.TuningMode[1].mode               = ChiModeType::Sensor;
        chiTuningModeParameter.TuningMode[1].subMode.value      = sensorModeIndex;
        chiTuningModeParameter.TuningMode[2].mode               = ChiModeType::Usecase;
        chiTuningModeParameter.TuningMode[2].subMode.usecase    = usecaseMode;
        chiTuningModeParameter.TuningMode[3].mode               = ChiModeType::Feature1;
        chiTuningModeParameter.TuningMode[3].subMode.feature1   = GetFeature1Mode(pMetaData, pFeature1Value);
        chiTuningModeParameter.TuningMode[4].mode               = ChiModeType::Feature2;
        chiTuningModeParameter.TuningMode[4].subMode.feature2   = GetFeature2Mode(pFeature2Value);
        chiTuningModeParameter.TuningMode[5].mode               = ChiModeType::Scene;
        chiTuningModeParameter.TuningMode[5].subMode.scene      = GetSceneMode(pMetaData, pSceneModeValue);
        chiTuningModeParameter.TuningMode[6].mode               = ChiModeType::Effect;
        chiTuningModeParameter.TuningMode[6].subMode.effect     = GetEffectMode(pMetaData, pEffectModeValue);

        ChxUtils::SetVendorTagValue(pMetaData, VendorTag::TuningMode, sizeof(ChiTuningModeParameter), &chiTuningModeParameter);

        CHX_LOG_VERBOSE("SensorMode %d UC %u feature %u %d scene %d effect %d", sensorModeIndex,
                        chiTuningModeParameter.TuningMode[2].subMode.usecase,
                        chiTuningModeParameter.TuningMode[3].subMode.feature1,
                        chiTuningModeParameter.TuningMode[4].subMode.feature2,
                        chiTuningModeParameter.TuningMode[5].subMode.scene,
                        chiTuningModeParameter.TuningMode[6].subMode.effect);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// DebugDataSize
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
SIZE_T ChxUtils::DebugDataSize(
    DebugDataType debugDataType)
{
    SIZE_T              debugDataSize       = 0;
    ExtensionModule*    pExtensionModule    = ExtensionModule::GetInstance();

    // Get 3A size only if enable
    if (TRUE == pExtensionModule->Enable3ADebugData())
    {
        switch (debugDataType)
        {
        case DebugDataType::AEC:
            debugDataSize = pExtensionModule->DebugDataSizeAEC();
            break;
        case DebugDataType::AWB:
            debugDataSize = pExtensionModule->DebugDataSizeAWB();
            break;
        case DebugDataType::AF:
            debugDataSize = pExtensionModule->DebugDataSizeAF();
            break;
        default:
            break;
        }
    }
    else if (TRUE == pExtensionModule->EnableConcise3ADebugData())
    {
        switch (debugDataType)
        {
        case DebugDataType::AEC:
            debugDataSize = pExtensionModule->ConciseDebugDataSizeAEC();
            break;
        case DebugDataType::AWB:
            debugDataSize = pExtensionModule->ConciseDebugDataSizeAWB();
            break;
        case DebugDataType::AF:
            debugDataSize = pExtensionModule->ConciseDebugDataSizeAF();
            break;
        default:
            break;
        }
    }

    // Get tuning size if enable
    if (TRUE == pExtensionModule->EnableTuningMetadata())
    {
        switch (debugDataType)
        {
        case DebugDataType::IFETuning:
            debugDataSize = pExtensionModule->TuningDumpDataSizeIFE();
            break;
        case DebugDataType::IPETuning:
            debugDataSize = pExtensionModule->TuningDumpDataSizeIPE();
            break;
        case DebugDataType::BPSTuning:
            debugDataSize = pExtensionModule->TuningDumpDataSizeBPS();
            break;
        default:
            break;
        }
    }

    // Get total size
    if (DebugDataType::AllTypes == debugDataType)
    {
        SIZE_T size3A       = 0;
        SIZE_T sizeTuning   = 0;

        if (TRUE == pExtensionModule->Enable3ADebugData())
        {
            size3A = pExtensionModule->DebugDataSizeAEC() +
                pExtensionModule->DebugDataSizeAWB() +
                pExtensionModule->DebugDataSizeAF();
        }
        else if (TRUE == pExtensionModule->EnableConcise3ADebugData())
        {
            size3A = pExtensionModule->ConciseDebugDataSizeAEC() +
                pExtensionModule->ConciseDebugDataSizeAWB() +
                pExtensionModule->ConciseDebugDataSizeAF();
        }

        if (TRUE == pExtensionModule->EnableTuningMetadata())
        {
            sizeTuning = pExtensionModule->TuningDumpDataSizeIFE() +
                pExtensionModule->TuningDumpDataSizeIPE() +
                pExtensionModule->TuningDumpDataSizeBPS();
        }

        debugDataSize = size3A + sizeTuning;
    }

    return debugDataSize;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ChxUtils::DebugDataAllocBuffer
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ChxUtils::DebugDataAllocBuffer(
    DebugData* pDebugData)
{
    CDKResult   result          = CDKResultSuccess;
    SIZE_T      debugDataSize   = ChxUtils::DebugDataSize(DebugDataType::AllTypes);

    if (NULL == pDebugData)
    {
        result = CDKResultEInvalidArg;
        return result;
    }

    if (0 < debugDataSize)
    {
        pDebugData->pData = ChxUtils::Calloc(debugDataSize);
        if (NULL != pDebugData->pData)
        {
            pDebugData->size = debugDataSize;
        }
        else
        {
            pDebugData->size = 0;
            result = CDKResultENoMemory;
            CHX_LOG_WARN("No memory for offline debug-data");
        }
    }

    return result;
}

#else

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Main thread function called by the OS for all created threads
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static DWORD WINAPI OSThreadLauncher(
    LPVOID pPrivateThreadData)
{
    OSThreadParams* pParams             = static_cast<OSThreadParams*>(pPrivateThreadData);
    OSThreadFunc    threadEntryFunction = pParams->threadEntryFunction;
    VOID*           pThreadData         = pParams->pThreadData;

    // Once this function returns it means we need to terminate the thread
    threadEntryFunction(pThreadData);

    CHX_FREE(pParams);

    return 0;   // 0 indicates the thread is terminated
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Returns a pointer to the global mutex, initializing it if necessary
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static Mutex* Initialize();

// Singleton global mutex for all atomic operations
static Mutex* g_pMutex = Initialize();

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Initialize
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static Mutex* Initialize()
{
    if (NULL == g_pMutex)
    {
        g_pMutex = Mutex::Create();
    }

    return g_pMutex;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Operator new
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID* operator new(
    size_t numBytes)    ///< Number of bytes to allocate
{
    /// @todo Hardcoded alignment
    VOID* pMem = _aligned_malloc(numBytes, 4096);

    if (NULL != pMem)
    {
        memset(pMem, 0, numBytes);
    }

    return pMem;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Operator delete
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID operator delete(
    VOID* pMem)    ///< Memory pointer
{
    _aligned_free(pMem);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Mutex::Create
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
Mutex* Mutex::Create()
{
    Mutex* pMutex = NULL;

    pMutex = CHX_NEW Mutex();

    if (NULL != pMutex)
    {
        if (CDKResultSuccess != pMutex->Initialize())
        {
            CHX_DELETE pMutex;
            pMutex = NULL;
        }
    }

    return pMutex;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Mutex::Initialize
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult Mutex::Initialize()
{
    CDKResult result = CDKResultSuccess;

    InitializeCriticalSection(&m_criticalSection);

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Mutex::Destroy
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID Mutex::Destroy()
{
    DeleteCriticalSection(&m_criticalSection);
    CHX_DELETE this;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Mutex::Lock
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID Mutex::Lock()
{
    EnterCriticalSection(&m_criticalSection);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Mutex::TryLock
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult Mutex::TryLock()
{
    CDKResult result = CDKResultSuccess;

    if (FALSE == TryEnterCriticalSection(&m_criticalSection))
    {
        result = CDKResultEBusy;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Mutex::Unlock
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID Mutex::Unlock()
{
    LeaveCriticalSection(&m_criticalSection);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Mutex::GetNativeHandle
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
OSMutexHandle* Mutex::GetNativeHandle()
{
    return &m_criticalSection;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Condition::Create
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
Condition* Condition::Create()
{
    Condition* pCondition = NULL;

    pCondition = CHX_NEW Condition();

    if (NULL != pCondition)
    {
        if (CDKResultSuccess != pCondition->Initialize())
        {
            CHX_DELETE pCondition;
            pCondition = NULL;
        }
    }

    return pCondition;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Condition::Destroy
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID Condition::Destroy()
{
    WakeAllConditionVariable(&m_conditionVar);
    CHX_DELETE this;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Condition::Initialize
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult Condition::Initialize()
{
    InitializeConditionVariable(&m_conditionVar);

    return CDKResultSuccess;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Condition::Wait
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult Condition::Wait(
    OSMutexHandle* phMutex)
{
    SleepConditionVariableCS(&m_conditionVar, phMutex, INFINITE);

    return CDKResultSuccess;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Condition::TimedWait
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult Condition::TimedWait(
    OSMutexHandle*  phMutex,
    UINT            timeoutMilliseconds)
{
    CDKResult result     = CDKResultSuccess;
    BOOL      waitResult = FALSE;

    waitResult = SleepConditionVariableCS(&m_conditionVar, phMutex, timeoutMilliseconds);

    if (FALSE == waitResult)
    {
        result = CDKResultEFailed;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Condition::Signal
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID Condition::Signal()
{
    WakeConditionVariable(&m_conditionVar);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Condition::Broadcast
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID Condition::Broadcast()
{
    WakeAllConditionVariable(&m_conditionVar);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// AlignGeneric32
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UINT32 ChxUtils::AlignGeneric32(
    UINT32 operand,
    UINT   alignment)
{
    UINT remainder = (operand % alignment);

    return (0 == remainder) ? operand : operand - remainder + alignment;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// EvenCeilingUINT32
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UINT32 ChxUtils::EvenCeilingUINT32(
    UINT32 input)
{
    UINT32 result;

    if (0xFFFFFFFF == input)
    {
        result = input;
    }
    else if (0 != (input & 0x00000001))
    {
        result = input + 0x00000001;
    }
    else
    {
        result = input;
    }
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// EvenFloorUINT32
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UINT32 ChxUtils::EvenFloorUINT32(
    UINT32 input)
{
    return (input & 0xFFFFFFFE);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// FEqualCoarse
///
/// @brief  Compares the floating point numbers with reduced precision
///
/// @param  value1  The first number to compare
/// @param  value2  The second number to compare
///
/// @return BOOL TRUE if equal FALSE if not equal
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL ChxUtils::FEqualCoarse(
    FLOAT value1, FLOAT value2)
{
    return ((fabs(value1 - value2) < 1e-2) ? (TRUE) : (FALSE));
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Checks if a particular bit is set in a number
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL ChxUtils::IsBitSet(
    UINT32  number,
    UINT32  bit)
{
    static const UINT32 One = 1;

    CHX_ASSERT(bit < (sizeof(UINT32) * 8));

    return ((number & (One << bit)) != 0) ? TRUE : FALSE;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Resets a bit in a number
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UINT32 ChxUtils::BitReset(
    UINT32  number,
    UINT32  bit)
{
    static const UINT32 One = 1;

    CHX_ASSERT(bit < (sizeof(UINT32) * 8));

    return (number & ~(One << bit));
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// OsUtils::GetFileName
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
const CHAR* ChxUtils::GetFileName(
    const CHAR* pFilePath)
{
    const CHAR* pFileName = strrchr(pFilePath, '/');

    if (NULL != pFileName)
    {
        // StrRChr will return a pointer to the /, advance one to the filename
        pFileName += 1;
    }
    else
    {
        pFileName = pFilePath;
    }

    return pFileName;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// MemMap
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID* ChxUtils::MemMap(
    INT     fd,
    SIZE_T  length,
    SIZE_T  offset)
{
    // Unimplemented.
    return NULL;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// MemUnmap
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
INT ChxUtils::MemUnmap(
    VOID*   pAddr,
    SIZE_T  length)
{
    // Unimplemented.
    return 0;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Memcpy
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID* ChxUtils::Memcpy(
    VOID*       pDst,
    const VOID* pSrc,
    SIZE_T      numBytes)
{
    CHX_ASSERT((pDst != NULL) && (pSrc != NULL));

    return memcpy(pDst, pSrc, numBytes);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Memset
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID* ChxUtils::Memset(
    VOID*  pDst,
    INT    value,
    SIZE_T numBytes)
{
    return memset(pDst, value, numBytes);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Allocate zeroed out memory
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID* ChxUtils::Calloc(SIZE_T numBytes)
{
    VOID* pMem = malloc(numBytes);
    if (NULL != pMem)
    {
        Memset(pMem, 0, numBytes);
    }

    return pMem;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Free memory
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID ChxUtils::Free(VOID* pMem)
{
    free(pMem);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Function to create a thread
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ChxUtils::ThreadCreate(
    OSThreadFunc    threadEntryFunction,
    VOID*           pThreadData,
    OSThreadHandle* phThread)
{
    HANDLE          hNewThread = NULL;
    OSThreadParams* pParams    = static_cast<OSThreadParams*>(CHX_CALLOC(sizeof(OSThreadParams)));
    CDKResult       result     = CDKResultSuccess;

    if (NULL != pParams)
    {
        pParams->threadEntryFunction = threadEntryFunction;
        pParams->pThreadData         = pThreadData;

        hNewThread = CreateThread(NULL, 0, OSThreadLauncher, pParams, 0, NULL);

        if (hNewThread == NULL)
        {
            CHX_FREE(pParams);
            result = CDKResultEFailed;
        }
        else
        {
            *phThread = hNewThread;
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Terminate a thread
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID ChxUtils::ThreadTerminate(OSThreadHandle hThread)
{
    WaitForSingleObject(hThread, INFINITE);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// 64 bit atomic store
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID ChxUtils::AtomicStoreU64(volatile UINT64* pVar,
                              UINT64           val)
{
    g_pMutex->Lock();
    *pVar = val;
    g_pMutex->Unlock();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// 64 bit atomic load
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UINT64 ChxUtils::AtomicLoadU64(volatile UINT64* pVar)
{
    UINT64 val = 0;

    g_pMutex->Lock();
    val = *pVar;
    g_pMutex->Unlock();

    return val;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// 32-bit atomic store
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID ChxUtils::AtomicStoreU32(volatile UINT32* pVar,
                                        UINT32           val)
{
    g_pMutex->Lock();
    *pVar = val;
    g_pMutex->Unlock();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// 32-bit atomic load
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UINT32 ChxUtils::AtomicLoadU32(volatile UINT32* pVar)
{
    UINT32 val = 0;

    g_pMutex->Lock();
    val = *pVar;
    g_pMutex->Unlock();

    return val;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// 32-bit atomic Increment
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UINT32 ChxUtils::AtomicIncU32(volatile UINT32* pVar)
{
    UINT32 val = 0;

    g_pMutex->Lock();
    *pVar++;
    val = *pVar;
    g_pMutex->Unlock();

    return val;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// 32-bit atomic decrement
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UINT32 ChxUtils::AtomicDecU32(volatile UINT32* pVar)
{
    UINT32 val = 0;

    g_pMutex->Lock();
    *pVar--;
    val = *pVar;
    g_pMutex->Unlock();

    return val;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Sleep in microseconds
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID ChxUtils::SleepMicroseconds(UINT microseconds)
{
    ::Sleep(microseconds / 1000);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Get the address of the function in the library
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID* ChxUtils::LibGetAddr(OSLIBRARYHANDLE hLibrary,
                           const CHAR*     pProcName)
{
    FARPROC pProcAddr = NULL;

    if (hLibrary != NULL)
    {
        pProcAddr = GetProcAddress(static_cast<HMODULE>(hLibrary), pProcName);
    }

    return pProcAddr;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Open the passed in library
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
OSLIBRARYHANDLE ChxUtils::LibMap(const CHAR* pLibraryName)
{
    OSLIBRARYHANDLE hLibrary = NULL;

    hLibrary = LoadLibrary(pLibraryName);

    return hLibrary;
}

#endif // WIN32

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Get the Partial result count for the sender
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
PartialResultCount ChxUtils::GetPartialResultCount(
    PartialResultSender sender
)
{
    MetaDataResultCount totalMetaDataCount =
        static_cast<MetaDataResultCount>(ExtensionModule::GetInstance()->GetNumMetadataResults());
    PartialMetaSupport  partialMetaSupport =
        ExtensionModule::GetInstance()->EnableCHIPartialData();
    PartialResultCount  resultCount        = PartialResultCount::FirstPartialResult;

    if (MetaDataResultCount::TwoMetaDataCount == totalMetaDataCount)
    {
        // When the Total Metadata Count is equal to 2 then we need to put
        // the CHI/Driver partial Metadata as the intial partial result
        // and the Final Metadata as the second partial result
        if ((PartialResultSender::DriverPartialData == sender) ||
            (PartialResultSender::CHIPartialData == sender))
        {
            resultCount = PartialResultCount::FirstPartialResult;
        }
        else
        {
            resultCount = PartialResultCount::SecondPartialResult;
        }
    }
    else if (MetaDataResultCount::ThreeMetaDataCount == totalMetaDataCount)
    {
        // When the Total Metadata Count is equal to 3 then we are putting
        // the CHI partial Metadata as the intial partial result
        // and the Driver partial Metadata as the second partial result
        // and the Final Metadata as the third partial result.
        // Please note that The first and second partial result could be
        // interchanged i.e. CHI Partial result could be the second and the
        // Driver Partial result could be the first.
        if (PartialResultSender::CHIPartialData == sender)
        {
            resultCount = PartialResultCount::FirstPartialResult;
        }
        else if (PartialResultSender::DriverPartialData == sender)
        {
            resultCount = PartialResultCount::SecondPartialResult;
        }
        else
        {
            resultCount = PartialResultCount::ThirdPartialResult;
        }
    }
    return resultCount;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// MatchAspectRatio
///
/// @brief  Matches output dimensions aspect ratio to reference dimensions aspect ratio
///
/// @param  pReferenceDimension     Pointer to reference dimension
/// @param  pUpdateDimension        Pointer to dimension that needs to be updated
///
/// @return None
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID ChxUtils::MatchAspectRatio(
    const CHIDimension *pReferenceDimension,
    CHIDimension       *pUpdateDimension)
{
    FLOAT referenceAspectRatio = 0.0f;
    FLOAT currentAspectRatio = 0.0f;

    referenceAspectRatio = static_cast<FLOAT>(pReferenceDimension->width) / static_cast<FLOAT>(pReferenceDimension->height);
    currentAspectRatio = static_cast<FLOAT>(pUpdateDimension->width) / static_cast<FLOAT>(pUpdateDimension->height);

    if (currentAspectRatio < referenceAspectRatio)
    {
        pUpdateDimension->height = static_cast<UINT32>(static_cast<FLOAT>(pUpdateDimension->width) / referenceAspectRatio);
    }
    else if (currentAspectRatio > referenceAspectRatio)
    {
        pUpdateDimension->width = static_cast<UINT32>(static_cast<FLOAT>(pUpdateDimension->height) * referenceAspectRatio);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ChxUtils::UpdateTimeStamp
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID ChxUtils::UpdateTimeStamp(
   ChiMetadata* pChiMetadata,
   UINT64       shutterTimestamp,
   UINT32       frameNum)
{
    if (NULL != pChiMetadata)
    {
        UINT64* pSensorTimestamp = static_cast<UINT64*>(pChiMetadata->GetTag(ANDROID_SENSOR_TIMESTAMP));
        UINT64  sensorTimeStamp  = (NULL != pSensorTimestamp) ? *pSensorTimestamp : 0LL;

        if (shutterTimestamp != sensorTimeStamp)
        {
            pChiMetadata->SetTag(ANDROID_SENSOR_TIMESTAMP, &shutterTimestamp, 1);

            CHX_LOG_INFO("Chi Timestamp sensor %" PRIu64 " shutter %" PRIu64 " metadata %pK frameNum %d",
                         sensorTimeStamp,
                         shutterTimestamp,
                         pChiMetadata->GetHandle(),
                         frameNum);
        }
    }
    else
    {
        CHX_LOG_WARN("Chi OutputMetadata is Null");
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ChxUtils::IsVendorTagPresent
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL ChxUtils::IsVendorTagPresent(
    ChiMetadata* pMetadata,
    VendorTag   tagEnum)
{
    BOOL isPresent = FALSE;

    if (NULL != pMetadata)
    {
        UINT32* pData;

        UINT32 tagId = ExtensionModule::GetInstance()->GetVendorTagId(tagEnum);

        pData = reinterpret_cast<UINT32*>(pMetadata->GetTag(tagId));

        if (NULL != pData)
        {
            isPresent = TRUE;
        }
    }

    return isPresent;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChxUtils::HasInputBufferError
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL ChxUtils::HasInputBufferError(
    CHIPIPELINEREQUEST* pSubmitRequest)
{
    BOOL inputBufferError = FALSE;

    for (UINT requestIdx = 0; requestIdx < pSubmitRequest->numRequests; requestIdx++)
    {
        const CHICAPTUREREQUEST* captureRequest = &pSubmitRequest->pCaptureRequests[requestIdx];
        for (UINT bufferIdx = 0; bufferIdx < captureRequest->numInputs; bufferIdx++)
        {
            if (captureRequest->pInputBuffers[bufferIdx].bufferStatus != CAMERA3_BUFFER_STATUS_OK)
            {
                inputBufferError = TRUE;
                break;
            }
        }

        if (TRUE == inputBufferError)
        {
            break;
        }
    }

    return inputBufferError;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChxUtils::FillDefaultTuningMetadata
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID ChxUtils::FillDefaultTuningMetadata(
    ChiMetadata*                pMetaData,
    ChiModeUsecaseSubModeType   usecaseMode,
    UINT32                      modeIndex)
{
    if (pMetaData != NULL)
    {
        UINT32 feature1     = 0;
        UINT32 feature2     = 0;
        UINT32 sceneMode    = 0;
        UINT32 effectMode   = 0;

        ChiTuningModeParameter chiTuningModeParameter = { 0 };
        chiTuningModeParameter.noOfSelectionParameter = MaxTuningMode;

        chiTuningModeParameter.TuningMode[0].mode               = ChiModeType::Default;
        chiTuningModeParameter.TuningMode[0].subMode.value      = 0;
        chiTuningModeParameter.TuningMode[1].mode               = ChiModeType::Sensor;
        chiTuningModeParameter.TuningMode[1].subMode.value      = modeIndex;
        chiTuningModeParameter.TuningMode[2].mode               = ChiModeType::Usecase;
        chiTuningModeParameter.TuningMode[2].subMode.usecase    = usecaseMode;
        chiTuningModeParameter.TuningMode[3].mode               = ChiModeType::Feature1;
        chiTuningModeParameter.TuningMode[3].subMode.feature1   = GetFeature1Mode(pMetaData, &feature1);
        chiTuningModeParameter.TuningMode[4].mode               = ChiModeType::Feature2;
        chiTuningModeParameter.TuningMode[4].subMode.feature2   = GetFeature2Mode(&feature2);
        chiTuningModeParameter.TuningMode[5].mode               = ChiModeType::Scene;
        chiTuningModeParameter.TuningMode[5].subMode.scene      = GetSceneMode(pMetaData, &sceneMode);
        chiTuningModeParameter.TuningMode[6].mode               = ChiModeType::Effect;
        chiTuningModeParameter.TuningMode[6].subMode.effect     = GetEffectMode(pMetaData, &effectMode);
        ChxUtils::SetVendorTagValue(pMetaData, VendorTag::TuningMode, sizeof(ChiTuningModeParameter), &chiTuningModeParameter);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ChxUtils::GetCameraIdFromStream
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UINT32 ChxUtils::GetCameraIdFromStream(
    ChiStream* pStream)
{
    UINT32 PhyicalCameraId = InvalidCameraId;
    if ((NULL != pStream) && (NULL != pStream->physicalCameraId))
    {
        PhyicalCameraId = static_cast<UINT32>(atoi(pStream->physicalCameraId));
    }
    return PhyicalCameraId;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ChxUtils::UpdateMetadataWithInputSettings
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ChxUtils::UpdateMetadataWithInputSettings(
   ChiMetadata& rInputMetadata,
   ChiMetadata& rOutputMetadata)
{
    CDKResult result = CDKResultSuccess;

    UINT8* pHotPixelMode = static_cast<UINT8*>(rInputMetadata.GetTag(ANDROID_HOT_PIXEL_MODE));
    if (NULL != pHotPixelMode)
    {
        rOutputMetadata.SetTag(ANDROID_HOT_PIXEL_MODE, pHotPixelMode, 1);
    }

    INT32* pJpegOrientation = static_cast<INT32*>(rInputMetadata.GetTag(ANDROID_JPEG_ORIENTATION));
    if (NULL != pJpegOrientation)
    {
        rOutputMetadata.SetTag(ANDROID_JPEG_ORIENTATION, pJpegOrientation, 1);
    }

    INT32* pEnableZSL = static_cast<INT32*>(rInputMetadata.GetTag(ANDROID_CONTROL_ENABLE_ZSL));
    if (NULL != pEnableZSL)
    {
        rOutputMetadata.SetTag(ANDROID_CONTROL_ENABLE_ZSL, pEnableZSL, 1);
    }

    UINT8* pJpegQuality = static_cast<UINT8*>(rInputMetadata.GetTag(ANDROID_JPEG_QUALITY));
    if (NULL != pJpegQuality)
    {
        rOutputMetadata.SetTag(ANDROID_JPEG_QUALITY, pJpegQuality, 1);
    }

    INT32* pThumbnailSize = static_cast<INT32*>(rInputMetadata.GetTag(ANDROID_JPEG_THUMBNAIL_SIZE));
    if (NULL != pThumbnailSize)
    {
        rOutputMetadata.SetTag(ANDROID_JPEG_THUMBNAIL_SIZE, pThumbnailSize, 2);
    }

    INT32* pThumbnailQuality = static_cast<INT32*>(rInputMetadata.GetTag(ANDROID_JPEG_THUMBNAIL_QUALITY));
    if (NULL != pThumbnailQuality)
    {
        rOutputMetadata.SetTag(ANDROID_JPEG_THUMBNAIL_QUALITY, pThumbnailQuality, 1);
    }

    INT32* pNoiseReductionMode = static_cast<INT32*>(rInputMetadata.GetTag(ANDROID_NOISE_REDUCTION_MODE));
    if (NULL != pNoiseReductionMode)
    {
        rOutputMetadata.SetTag(ANDROID_NOISE_REDUCTION_MODE, pNoiseReductionMode, 1);
    }

    ChiTuningModeParameter* pData   = NULL;
    ChxUtils::GetVendorTagValue(&rInputMetadata, VendorTag::TuningMode, (VOID**)&pData);
    if (NULL != pData)
    {
        ChxUtils::SetVendorTagValue(&rOutputMetadata, VendorTag::TuningMode, sizeof(ChiTuningModeParameter), pData);
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CameraUsecaseBase::UpdateMetadataWithSnapshotSettings
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ChxUtils::UpdateMetadataWithSnapshotSettings(
   ChiMetadata& rMetadata)
{
    CDKResult result = CDKResultSuccess;
    INT       status = 0;

    // Add default bps sensor metdata for gain if not present
    if (NULL == rMetadata.GetTag("com.qti.sensorbps", "mode_index"))
    {
        CHX_LOG("BPS gain not present. Filling with default");
        UINT  sensorBpsModeIndex = 0;
        rMetadata.SetTag("com.qti.sensorbps", "mode_index", &sensorBpsModeIndex, 1);
    }

    if (NULL == rMetadata.GetTag("com.qti.sensorbps", "gain"))
    {
        CHX_LOG("BPS gain not present. Filling with default");
        FLOAT sensorBpsGainValue = 1.0f;
        rMetadata.SetTag("com.qti.sensorbps", "gain", &sensorBpsGainValue, 1);
    }

    // Add DebugData
    if (NULL == rMetadata.GetTag("org.quic.camera.debugdata", "DebugDataAll"))
    {
        CHX_LOG("DebugData was not present in the original request. Reprocess likely to fail");
    }

    return result;
}

