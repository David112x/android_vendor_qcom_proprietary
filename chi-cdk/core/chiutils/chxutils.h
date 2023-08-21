////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2017-2020 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  chxutils.h
/// @brief CHX utility functions
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef CHXUTILS_H
#define CHXUTILS_H

#include "chxdefs.h"
#include <math.h>
#include <inttypes.h>
#include <mutex>
#include <condition_variable>
#include <signal.h>

#include "chituningmodeparam.h"
#include "chxdebugprint.h"


#define DEBUGGER_SIGNAL (__SIGRTMIN + 3)

// NOWHINE ENTIRE FILE - Temporarily bypassing for existing CHI files

// Forward decl
class ChiMetadata;

#define CHX_CALLOC   ChxUtils::Calloc
#define CHX_FREE     ChxUtils::Free
#define CHX_FILENAME ChxUtils::GetFileName

/// The directory from which to read configuration files
#ifndef CONFIG_DIRS_DEFINED
#define CONFIG_DIRS_DEFINED
#if defined (_WIN32)
static const CHAR PathSeparator[] = "\\";
static const CHAR SharedLibraryExtension[] = "dll";
static const CHAR VendorLibPath[] = ".";
static const CHAR VendorLibPath[] = ".";
static const CHAR FileDumpPath[] = ".";
#elif defined (CAMX_ANDROID_API) && (CAMX_ANDROID_API >= 28) // NOWHINE PR002 <- Win32 definition
static const CHAR ConfigFileDirectory[] = "/data/vendor/camera";
static const CHAR FileDumpPath[]        = "/data/vendor/camera";
#else
static const CHAR ConfigFileDirectory[] = "/data/misc/camera";
static const CHAR FileDumpPath[]        = "/data/misc/camera";
#endif // Android-P or later
#endif // CONFIG_DIRS_DEFINED

/// Thread entry function type
typedef VOID* (*OSThreadFunc)(VOID* pArg);
/// @brief Function pointer type for a job function
typedef VOID* (*JobFunc)(VOID* pArg);
/// Dynamic library handle
typedef VOID* OSLIBRARYHANDLE;

#if defined (_LINUX)
#include <dlfcn.h>                  // dynamic linking
#include <errno.h>                  // errno
#include <fcntl.h>
#include <stdarg.h>
#include <stdlib.h>                 // posix_memalign, free
#include <string.h>                 // strlcat
#include <sys/mman.h>               // memory management
#include <sys/syscall.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>                 // library functions
#include <dirent.h>
#include <pthread.h>
#include <semaphore.h>
#include <signal.h>
#include <string.h>
#include <unistd.h>
#include <sync/sync.h>

/// Thread handle type
typedef pthread_t OSThreadHandle;
/// Mutex handle type
typedef pthread_mutex_t OSMutexHandle;
/// Native fence type
typedef INT NativeFence;
#elif defined (_WIN32)
/// Thread handle type
typedef HANDLE OSThreadHandle;
/// Mutex handle type
typedef CRITICAL_SECTION OSMutexHandle;
/// Native fence type
typedef VOID* NativeFence;

/// @brief This structure is passed to the OS thread create function. This gets passed back to the thread by the OS when it
///        calls the thread main function entry point registered during thread create.
struct OSThreadParams
{
    OSThreadFunc    threadEntryFunction;    ///< Function to execute in new thread
    VOID*           pThreadData;            ///< Data passed to threadEntryFunction()
};

#else
#error Unsupported target defined
#endif // defined(_LINUX) || defined(_WIN32)

/// @brief Information about one thread
struct PerThreadData
{
    UINT32          threadId;               ///< Logical thread number
    OSThreadHandle  hThreadHandle;          ///< Thread handle
    JobFunc         mainThreadFunc;         ///< Thread main entry function
    VOID*           pPrivateData;           ///< Private data to be interpreted by the thread
};

#define CHX_NEW     new
#define CHX_DELETE  delete
#define CHX_ASSERT  (VOID)
#define CHX_INLINE  __inline
#define INVALID_THREAD_HANDLE (-1)

static const INT InvalidNativeFence = -1;   ///< Invalid native hFence

/// @brief Information about android to native scene.
typedef struct _AndroidToNativeScene {
    camera_metadata_enum_android_control_scene_mode_t from;
    ChiModeSceneSubModeType to;
} AndroidToCamxScene;

/// @brief Information about android to native effect.
typedef struct _AndroidToNativeEffect {
    camera_metadata_enum_android_control_effect_mode_t from;
    ChiModeEffectSubModeType to;
} AndroidToCamxEffect;

static AndroidToCamxScene SceneMap[] =
{
    { ANDROID_CONTROL_SCENE_MODE_DISABLED,ChiModeSceneSubModeType::None },
    { ANDROID_CONTROL_SCENE_MODE_FACE_PRIORITY,ChiModeSceneSubModeType::FacePriority },
    { ANDROID_CONTROL_SCENE_MODE_ACTION,ChiModeSceneSubModeType::Action },
    { ANDROID_CONTROL_SCENE_MODE_PORTRAIT,ChiModeSceneSubModeType::Portrait },
    { ANDROID_CONTROL_SCENE_MODE_LANDSCAPE,ChiModeSceneSubModeType::Landscape },
    { ANDROID_CONTROL_SCENE_MODE_NIGHT,ChiModeSceneSubModeType::Night },
    { ANDROID_CONTROL_SCENE_MODE_NIGHT_PORTRAIT,ChiModeSceneSubModeType::NightPortrait },
    { ANDROID_CONTROL_SCENE_MODE_THEATRE,ChiModeSceneSubModeType::Theater },
    { ANDROID_CONTROL_SCENE_MODE_BEACH,ChiModeSceneSubModeType::Beach },
    { ANDROID_CONTROL_SCENE_MODE_SNOW,ChiModeSceneSubModeType::Snow },
    { ANDROID_CONTROL_SCENE_MODE_SUNSET,ChiModeSceneSubModeType::Sunset },
    // No STEADYPHOTO in ChiModeSceneSubModeType !!!
    { ANDROID_CONTROL_SCENE_MODE_STEADYPHOTO,ChiModeSceneSubModeType::None },
    { ANDROID_CONTROL_SCENE_MODE_FIREWORKS,ChiModeSceneSubModeType::Fireworks },
    { ANDROID_CONTROL_SCENE_MODE_SPORTS,ChiModeSceneSubModeType::Sports },
    { ANDROID_CONTROL_SCENE_MODE_PARTY,ChiModeSceneSubModeType::Party },
    { ANDROID_CONTROL_SCENE_MODE_CANDLELIGHT,ChiModeSceneSubModeType::CandleLight },
    { ANDROID_CONTROL_SCENE_MODE_BARCODE,ChiModeSceneSubModeType::Barcode },
    // No SPEED_VIDEO in ChiModeSceneSubModeType !!!
    { ANDROID_CONTROL_SCENE_MODE_HIGH_SPEED_VIDEO,ChiModeSceneSubModeType::None },
    // No HDR in ChiModeSceneSubModeType !!!
    { ANDROID_CONTROL_SCENE_MODE_HDR, ChiModeSceneSubModeType::None },
    // No FACE_PRIORITY_LOW_LIGHT in ChiModeSceneSubModeType !!!
    { ANDROID_CONTROL_SCENE_MODE_FACE_PRIORITY_LOW_LIGHT,ChiModeSceneSubModeType::None },
};

static AndroidToCamxEffect EffectMap[] =
{
    { ANDROID_CONTROL_EFFECT_MODE_OFF, ChiModeEffectSubModeType::None },
    { ANDROID_CONTROL_EFFECT_MODE_MONO, ChiModeEffectSubModeType::Mono },
    { ANDROID_CONTROL_EFFECT_MODE_NEGATIVE, ChiModeEffectSubModeType::Negative },
    { ANDROID_CONTROL_EFFECT_MODE_SOLARIZE, ChiModeEffectSubModeType::Solarize },
    { ANDROID_CONTROL_EFFECT_MODE_SEPIA, ChiModeEffectSubModeType::Sepia },
    { ANDROID_CONTROL_EFFECT_MODE_POSTERIZE, ChiModeEffectSubModeType::Posterize },
    { ANDROID_CONTROL_EFFECT_MODE_WHITEBOARD, ChiModeEffectSubModeType::Whiteboard },
    { ANDROID_CONTROL_EFFECT_MODE_BLACKBOARD, ChiModeEffectSubModeType::Blackboard },
    { ANDROID_CONTROL_EFFECT_MODE_AQUA, ChiModeEffectSubModeType::Aqua },
};


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief General portable mutex class implementation
///
/// Basic wrapping of OS mutex to provide abstraction
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class CDK_VISIBILITY_PUBLIC Mutex final
{
public:
    /// Static method to create an instance of Mutex
    static Mutex* Create();

    /// Method to delete an instance of Mutex
    VOID Destroy();

    /// Lock/acquire an Mutex
    VOID Lock();

    ///Try to acquire a Mutex lock
    CDKResult TryLock();

    /// Unlock/release an Mutex
    VOID Unlock();

    /// Get the native mutex handle, needed for other system calls which takes native handle
    OSMutexHandle* GetNativeHandle();

private:
    /// Default constructor for Mutex object
    Mutex() = default;

    /// Default destructor for Mutex object.
    ~Mutex() = default;

    /// Initialize a newly created Mutex object
    CDKResult Initialize();

    Mutex(const Mutex&) = delete;                ///< Disallow the copy constructor
    Mutex& operator=(const Mutex&) = delete;     ///< Disallow assignment operator

#if defined(_LINUX)
    pthread_mutex_t     m_mutex;                ///< (Linux) Mutex
    BOOL                m_validMutex;           ///< (Linux) Indicates if underlying mutex is valid
#else
    CRITICAL_SECTION    m_criticalSection;      ///< (Win32) Critical Section implementing Mutex
#endif // defined(_LINUX)
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief General portable condition class implementation
///
/// Basic wrapping of OS condition variable to provide abstraction
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class CDK_VISIBILITY_PUBLIC Condition final
{
public:
    /// Static method to create an instance of Condition
    static Condition* Create();

    /// Method to delete an instance of Semaphore
    VOID Destroy();

    /// Wait on the condition
    CDKResult Wait(OSMutexHandle* phMutex);

    /// Wait with timeout on the Condition
    CDKResult TimedWait(OSMutexHandle*  phMutex,
                        UINT            timeoutMilliseconds);

    /// Signal the condition
    VOID Signal();

    /// Broadcast the condition
    VOID Broadcast();

private:
    Condition()  = default;
    ~Condition() = default;

    /// Initialize a newly created Condition object
    CDKResult Initialize();

    Condition(const Condition&) = delete;                ///< Disallow the copy constructor
    Condition& operator=(const Condition&) = delete;     ///< Disallow assignment operator

#if defined(_LINUX)
    pthread_cond_t      m_conditionVar;                  ///< (Linux) Underlying conditional variable
    BOOL                m_validConditionVar;             ///< (Linux) Indicates if condVar is valid
#else
    CONDITION_VARIABLE  m_conditionVar;                  ///< (Windows) Underlying conditional variable
#endif // defined(_LINUX)
};


class CDK_VISIBILITY_PUBLIC Semaphore final
{
public:

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// Create
    ///
    /// @brief  Static method to create an instance of counting Semaphore
    ///
    /// @param count    semaphore count
    ///
    /// @return Pointer to newly created object on success, NULL on failure.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static Semaphore* Create(
        INT count = 0);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// Destroy
    ///
    /// @brief  Method to delete an instance of Semaphore
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID Destroy();

    ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// NotifyAllThreads
    ///
    /// @brief  Notify all the threads that are waiting for the semaphore
    ///
    /// @param  None
    ///
    /// @return None.
    ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID NotifyAllThreads();

    ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// Reset
    ///
    /// @brief  Reset the semaphore count and notify semaphore
    ///
    /// @param  None
    ///
    /// @return None.
    ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID Reset();

    ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// WaitForSemaphore
    ///
    /// @brief  Wait for semaphore if its not available
    ///
    /// @param  None
    ///
    /// @return FPS value for preview stream
    ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID WaitForSemaphore();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetSemaphoreCount
    ///
    /// @brief  Get the current semaphore count value
    ///
    /// @return count value
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    UINT GetSemaphoreCount()
    {
        return m_semaphoreCount;
    }

private:

    /// @brief Constructor for Semaphore object.
    Semaphore() = default;

    /// @brief Destructor for Semaphore object.
    ~Semaphore() = default;

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// Initialize
    ///
    /// @brief  Initialize a newly created Semaphore object
    ///
    /// @return Success if successful, a failure code otherwise
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CDKResult Initialize(
    INT count);

    Semaphore(const Semaphore&) = delete;                ///< Disallow the copy constructor
    Semaphore& operator=(const Semaphore&) = delete;     ///< Disallow assignment operator

    std::mutex                m_mutex;                     ///< Mutex required for critical section
    std::condition_variable   m_semaphoreCv;               ///< condition variable required for semaphore
    UINT                      m_semaphoreCount;            ///< counting semaphore count
    UINT                      m_maxSemaphoreCount;         ///< Max count initialized during creation
};


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Utils
///
/// @brief General utility class implementation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class CDK_VISIBILITY_PUBLIC ChxUtils
{
public:

    static VOID MatchAspectRatio(
        const CHIDimension* pReferenceDimension,
        CHIDimension*       pUpdateDimension);

    static UINT32 AlignGeneric32(UINT32 operand,
                                 UINT   alignment);

    static UINT32 EvenCeilingUINT32(UINT32 input);

    static UINT32 EvenFloorUINT32(UINT32 input);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// DivideAndCeil
    ///
    /// @brief  calculates the ceil after dividing a UINT32
    ///
    /// @param  val     UINT32 to be divided and ceiled
    /// @param  divisor UINT32 to use as the divisor in the operation
    ///
    /// @return UINT32  ceil of the quotient
    ///
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static CHX_INLINE UINT32 DivideAndCeil(
        UINT32 val,
        UINT32 divisor)
    {
        UINT32 result = 0;

        if (0 != divisor)
        {
            result = (val + (divisor - 1)) / divisor;
        }
        else
        {
            CHX_LOG_ERROR("Dividing by zero.");
        }

        return result;
    }

    static BOOL FEqualCoarse(
        FLOAT value1, FLOAT value2);


    static BOOL IsBitSet(UINT32  number,
                         UINT32  bit);

    static UINT32 BitReset(UINT32  number,
                           UINT32  bit);

    static UINT32 BitSet(UINT32  number,
                           UINT32  bit);

    static const CHAR* GetFileName(
        const CHAR* pFilePath);

    static VOID* MemMap(INT     fd,
                        SIZE_T  length,
                        SIZE_T  offset);

    static INT MemUnmap(VOID*   pAddr,
                        SIZE_T  length);

    static CDKResult NativeFenceWait(NativeFence hFence,
                                             UINT32     timeoutMilliseconds);

    static CDKResult Close(   INT FD);

    static VOID* Memcpy(VOID*       pDst,
                        const VOID* pSrc,
                        SIZE_T      numBytes);

    static VOID* Memset(VOID*  pDst,
                        INT    value,
                        SIZE_T numBytes);

    static VOID* Calloc(SIZE_T numBytes);

    static VOID Free(VOID* pMem);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// DumpCallStacksForAllThreads
    ///
    /// @brief  Raises signal 35 to generate tombstones in /data/tombstones Tombstone file gives thread callstacks.
    ///         We have check for all newly generated tombstone files. We have to see the reason for tombstone generation
    ///         within the initial few lines of tombstone file. Search for "signal 35". That helps to identify the particular
    ///         file.
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static VOID DumpCallStacksForAllThreads()
    {
#if defined(ANDROID)
        raise(DEBUGGER_SIGNAL);
#endif // defined(ANDROID)
    }

    static  CDKResult ThreadCreate(OSThreadFunc    threadEntryFunction,
                                   VOID*           pThreadData,
                                   OSThreadHandle* phThread);

    static VOID ThreadTerminate(OSThreadHandle hThread);

    static VOID AtomicStore64(volatile INT64* pVar,
                              INT64           val);

    static VOID AtomicStoreU64(volatile UINT64* pVar,
                               UINT64           val);

    static UINT64 AtomicLoadU64(volatile UINT64* pVar);

    static VOID AtomicStoreU32(volatile UINT32* pVar,
                               UINT32           val);

    static UINT32 AtomicLoadU32(volatile UINT32* pVar);

    static UINT32 AtomicIncU32(volatile UINT32* pVar);
    static UINT32 AtomicDecU32(volatile UINT32* pVar);

    static VOID SleepMicroseconds(UINT microseconds);

    static VOID* LibGetAddr(OSLIBRARYHANDLE hLibrary,
                            const CHAR*     pProcName);

    static OSLIBRARYHANDLE LibMap(const CHAR* pLibraryName);


    static CHX_INLINE DOUBLE AbsoluteFLOAT(
        FLOAT input)
    {
        return fabs(input);
    }

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// DPrintF
///
/// @brief  Write formatted data from variable argument list to sized buffer
///
/// @param  fd       File descriptor to print to
/// @param  pFormat  Format string, printf style
/// @param  ...      Parameters required by format
///
/// @return Number of characters written
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static INT DPrintF(
        INT         fd,
        const CHAR* pFormat,
                    ...);

    /// CHX utilies based on Android Camera metadata ops.
    /// Existing APIs retained for backward compatibility
    struct AndroidMetadata
    {
    public:
        /// checks if the vendor tag specified by the tag value is present
        static BOOL IsVendorTagPresent(
            const VOID* pMetadata,
            VendorTag   tag);

        /// checks if long exposure capture setting is present in the metadata
        static BOOL IsLongExposureCapture(
            const VOID* pMetadata);

        ///  gets the tag data given the vendor tag from the metadata
        static VOID GetVendorTagValue(
            const VOID* pMetadata,
            VendorTag   tag,
            VOID**      ppData);

        ///  sets the tag data specified the vendor tag to the metadata
        static CDKResult SetVendorTagValue(
            VOID*       pMetadata,
            VendorTag   tag,
            UINT        dataCount,
            VOID*       ppData);

        ///  merges two metadata
        static INT MergeMetadata(
            VOID* pMetadata1,
            VOID* pMetadata2);

        ///  allocates and append metadata
        static VOID* AllocateAppendMetaData(
            const VOID* pMetadata,
            UINT32      entry_count,
            UINT32      data_count);

        ///  allocates and copies metadata
        static VOID* AllocateCopyMetaData(
            const VOID* pSrcMetadata);

        ///  allocates metadata
        static VOID* AllocateMetaData(
            UINT32  entry_count,
            UINT32  data_count);

        ///  frees metadata
        static VOID FreeMetaData(
            VOID* pMetadata);

        /// Fill Tuning mode selection data.
        static VOID FillTuningModeData(
            VOID*                      pMetaData,
            camera3_capture_request_t* pRequest,
            UINT32                     sensorModeIndex,
            UINT32*                    pEffectModeValue,
            UINT32*                    pSceneModeValue,
            UINT32*                    pFeature1Value,
            UINT32*                    pFeature2Value);

        /// Add timestamp to metadata
        static CDKResult UpdateTimeStamp(
            camera_metadata_t*  pMetadata,
            UINT64              timestamp,
            UINT32              frameNum);

        /// Get timestamp from metadata
        static UINT64 GetTimeStamp(
            camera_metadata_t*  pMetadata);

        /// Resets the metadata
        static camera_metadata_t* ResetMetadata(
            camera_metadata* pMetadata);

        /// Fill cameraid.
        static VOID FillCameraId(
            VOID*  pMetaData,
            UINT32 cameraId);

        /// Get scene mode from metadata
        static ChiModeSceneSubModeType GetSceneMode(
            VOID*   pMetaData,
            UINT32* pSceneModeValue);

        /// Get ZSL mode from metadata
        static INT GetZSLMode(
            VOID* pMetaData);

        /// Get Fps Range
        static CDKResult GetFpsRange(
            const VOID* pMetaData,
            INT32*      pMinFps,
            INT32*      pMaxFps);

        /// Get effect mode from the metadata
        static ChiModeEffectSubModeType GetEffectMode(
            VOID*   pMetaData,
            UINT32* pEffectModeValue);

        /// Get feature1 for tuning mode
        static ChiModeFeature1SubModeType GetFeature1Mode(
            VOID*   pMetaData,
            UINT32* pFeature1Value);
    };

    static CHX_INLINE camera_metadata* GetMetadataType(VOID* pMetadata)
    {
        return static_cast<camera_metadata*>(pMetadata);
    }

    /// CHI metadata function equivalents of Android metadata
    static BOOL IsVendorTagPresent(
       ChiMetadata* pMetadata,
       VendorTag    tag);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// DebugDataSize
    ///
    /// @brief  Get debug data size
    ///
    /// @param  debugDataType   Debug-data type
    ///
    /// @return Size required to allocate all debug data
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static SIZE_T DebugDataSize(
        DebugDataType debugDataType);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// DebugDataAllocBuffer
    ///
    /// @brief  Allocate memory for offline processing and copy incoming data.
    ///
    /// @param  pDebugData*    Allocated memory will be provided in this structure.
    ///
    /// @return CDKResultSuccess if allocation succeed.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static CDKResult DebugDataAllocBuffer(
        DebugData* pDebugData);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ErrorMessageCodeToString
    ///
    /// @brief  Translate an error message code into a string
    ///
    /// @param  code The integer value of a CHIERRORMESSAGECODE
    ///
    /// @return The string value of the error message
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static CHX_INLINE const CHAR* ErrorMessageCodeToString(UINT code)
    {
        static const CHAR* messageArray[] =
        {
            "ERROR_DEVICE",
            "ERROR_REQUEST",
            "ERROR_RESULT",
            "ERROR_BUFFER",
            "ERROR_RECOVERY"
        };
        UINT idx = code - 1;
        return (idx < CHX_ARRAY_SIZE(messageArray)) ? messageArray[idx] : "ERROR_UNKNOWN";
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ErrorMessageCodeToString
    ///
    /// @brief  Translate an error message code into a string
    ///
    /// @param  code An error message code
    ///
    /// @return The string value of the error message
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static CHX_INLINE const CHAR* ErrorMessageCodeToString(CHIERRORMESSAGECODE code)
    {
        return ErrorMessageCodeToString(static_cast<INT>(code));
    }

    /// Gets the vendor tag data specified by tag value from the metadata
    static VOID GetVendorTagValue(
       ChiMetadata* pMetadata,
       VendorTag    tag,
       VOID**       ppData);

    /// Sets the vendor tag data specified by tag value to the metadata
    static CDKResult SetVendorTagValue(
       ChiMetadata* pMetadata,
        VendorTag   tag,
        UINT        dataCount,
        VOID*       ppData);

    /// Fill Tuning mode selection data.
    static CHX_INLINE VOID FillTuningModeData(
        ChiMetadata*                pMetaData,
        camera3_capture_request_t*  pRequest,
        UINT32                      sensorModeIndex,
        UINT32*                     pEffectModeValue,
        UINT32*                     pSceneModeValue,
        UINT32*                     pFeature1Value,
        UINT32*                     pFeature2Value)
    {
        return FillTuningModeData( pMetaData,
                                   GetUsecaseMode(pRequest),
                                   sensorModeIndex,
                                   pEffectModeValue,
                                   pSceneModeValue,
                                   pFeature1Value,
                                   pFeature2Value);
    }

    /// Fill Tuning mode selection data.
    static VOID FillTuningModeData(
        ChiMetadata*                pMetaData,
        ChiModeUsecaseSubModeType   usecaseMode,
        UINT32                      sensorModeIndex,
        UINT32*                     pEffectModeValue,
        UINT32*                     pSceneModeValue,
        UINT32*                     pFeature1Value,
        UINT32*                     pFeature2Value);

    /// Fill cameraid to the CHI metadata
    static VOID FillCameraId(
        ChiMetadata* pMetaData,
        UINT32       cameraId);

    /// Get scene mode from CHI Metadata
    static ChiModeSceneSubModeType GetSceneMode(
        ChiMetadata* pMetaData,
        UINT32*      pSceneModeValue);

    /// Get ZSL mode from CHI Metadata
    static INT GetZSLMode(
        ChiMetadata* pMetaData);

    /// Get Flash fired state from CHI Metadata
    static BOOL GetFlashFiredState(
        ChiMetadata* pMetaData);

    /// Get Flash mode from APP Metadata
    static BOOL GetFlashMode(
        ChiMetadata* pMetaData);

    /// Get effect mode from CHI Metadata
    static ChiModeEffectSubModeType GetEffectMode(
        ChiMetadata* pMetaData,
        UINT32*      pEffectModeValue);

    /// Get feture1 for tuning mode.
    static ChiModeFeature1SubModeType GetFeature1Mode(
        ChiMetadata* pMetaData,
        UINT32*      pFeature1Value);

    // update the timestamp
    static VOID UpdateTimeStamp(
       ChiMetadata* pMetadata,
       UINT64       timestamp,
       UINT32       frameNum);

    /// Deep copy camera3_capture_request_t data
    static VOID DeepCopyCamera3CaptureRequest(
        const camera3_capture_request_t* pSrcReq,
        camera3_capture_request_t*       pDestReq);

    /// Get usecase from request
    static ChiModeUsecaseSubModeType GetUsecaseMode(
        camera3_capture_request_t* pRequest);

    static INT32  ReadSocID();

    static VOID  WaitOnAcquireFence(
        const CHISTREAMBUFFER* pBuffer);

    static VOID  WaitOnAcquireFence(
        const camera3_stream_buffer* pBuffer);

    /// SkipFrame
    static VOID SkipFrame(
        camera3_stream_buffer_t* pBuffer);

    /// Get feature2 for tuning mode.
    static ChiModeFeature2SubModeType GetFeature2Mode(
        UINT32* pFeature2Value);

    /// Check whether input ChiFence is a valid native fence type
    static CHX_INLINE BOOL IsValidNativeFenceType(
        const CHIFENCEINFO* pChiFence)
    {
        BOOL bValid = FALSE;

        if ((TRUE               == pChiFence->valid) &&
            (ChiFenceTypeNative == pChiFence->type)  &&
            (InvalidNativeFence != pChiFence->nativeFenceFD))
        {
            bValid = TRUE;
        }

        return bValid;
    }

    /// Check whether input ChiFence is a valid internal fence type
    static CHX_INLINE BOOL IsValidInternalFenceType(
        const CHIFENCEINFO* pChiFence)
    {
        BOOL bValid = FALSE;

        if ((TRUE                   == pChiFence->valid) &&
            (ChiFenceTypeInternal   == pChiFence->type)  &&
            (NULL                   != pChiFence->hChiFence))
        {
            bValid = TRUE;
        }

        return bValid;
    }

    /// Populate HAL Buffer structure to Chi Stream Buffer
    static VOID PopulateHALToChiStreamBuffer(
        const camera3_stream_buffer_t*  pCamera3StreamBuffer,
        CHISTREAMBUFFER*                pChiStreamBuffer);

    /// Populate Chi Stream Buffer structure to HAL Buffer
    static VOID PopulateChiToHALStreamBuffer(
        const CHISTREAMBUFFER*      pChiStreamBuffer,
        camera3_stream_buffer_t*    pCamera3StreamBuffer);

    /// Get The Partial Result Count of the Sender
    static PartialResultCount GetPartialResultCount(
        PartialResultSender sender);

    /// Get The corresponding CHI frame index for the frame number
    static CHX_INLINE UINT32 GetResultFrameIndexChi(UINT32 frameNumber)
    {
        return frameNumber % MaxOutstandingRequests;
    }

    /// Returns the larger of two INT32 values
    static CHX_INLINE INT32 MaxINT32(
        INT32 input1,
        INT32 input2)
    {
        return ((input1 > input2) ? input1 : input2);
    }

    /// Returns the larger of two UINT values
    static CHX_INLINE UINT MaxUINT(
        UINT input1,
        UINT input2)
    {
        return ((input1 > input2) ? input1 : input2);
    }

    /// Returns the larger of two INT64 values
    static CHX_INLINE INT64 MaxINT64(
        INT64 input1,
        INT64 input2)
    {
        return ((input1 > input2) ? input1 : input2);
    }

    /// Returns the smaller of two INT64 values
    static CHX_INLINE INT64 MinINT64(
        INT64 input1,
        INT64 input2)
    {
        return ((input1 < input2) ? input1 : input2);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// HasInputBufferError
    ///
    /// @brief  Check to see if input buffer of a CHIPIPELINEREQUEST* has a buffer error
    ///
    /// @param  CHIPIPELINEREQUEST* The request we want to submit to the pipeline
    ///
    /// @return TRUE if input buffer, FALSE otherwise
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static BOOL HasInputBufferError(
        CHIPIPELINEREQUEST* pSubmitRequest);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// FillDefaultTuningMetadata
    ///
    /// @brief  Fill Default tuning metadata
    ///
    /// @param  ChiMetadata*                Input Metadata on which to set default tuning
    /// @param  ChiModeUsecaseSubModeType   Usecase mode
    /// @param  modeIndex                   mode index of the sensor
    ///
    /// @return VOID
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static VOID FillDefaultTuningMetadata(
        ChiMetadata*                pMetaData,
        ChiModeUsecaseSubModeType   usecaseMode,
        UINT32                      modeIndex);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetCameraIdFromStream
    ///
    /// @brief  Get camera ID from stream
    ///
    /// @param  ChiStream*   Stream pointer
    ///
    /// @return CameraId
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static UINT32 GetCameraIdFromStream(
        ChiStream* pStream);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// UpdateMetadataWithInputSettings
    ///
    /// @brief Replace the capture result metadata with input request metadata
    ///
    /// @param  ChiMetadata& Src Metadata
    /// @param  ChiMetadata& Dst Metadata
    ///
    /// @return CDKResultSuccess if successful
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static CDKResult UpdateMetadataWithInputSettings(
       ChiMetadata& rInputMetadata,
       ChiMetadata& rOutputMetadata);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// UpdateMetadataWithSnapshotSettings
    ///
    /// @brief Replace the capture request metadata with a copy of it + additional new settings inserted by the Chi override
    ///
    /// @param  ChiMetadata& Metadata to update
    ///
    /// @return CDKResultSuccess if successful
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static CDKResult UpdateMetadataWithSnapshotSettings(
        ChiMetadata& rMetadata);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// RaiseSignalAbort
    ///
    /// @brief Raise sigabort in fatal scenario
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static CHX_INLINE VOID RaiseSignalAbort()
    {
        raise(SIGABRT);
    }
 };

#endif // CHXUTILS_H
