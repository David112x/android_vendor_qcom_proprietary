////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2019-2020 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  camxosutilslinuxembedded.cpp
/// @brief General OS specific utility class implementation for Linux
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// NOWHINE FILE CP040: This file defines memory utils.
// NOWHINE FILE PR007b: Whiner incorrectly concludes as non-library files
// NOWHINE FILE CP036a: For gralloc typecase

#if defined (_LINUX)                // This file for Linux build only

#include <dlfcn.h>                  // dynamic linking
#include <errno.h>                  // errno
#include <fcntl.h>
#include <limits.h>
#include <stdarg.h>
#include <stdlib.h>                 // posix_memalign, free
#include <string.h>                 // strlcat
#include <stdio.h>
#include <sys/mman.h>               // memory management
#include <sys/resource.h>
#include <sys/syscall.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>                 // library functions
#include <time.h>

#ifdef ANDROID
#include <android/log.h>            // Logcat logging functions
#include <cutils/properties.h>      // Android properties
#include <sync/sync.h>
#include <sys/prctl.h>

#define gettid() syscall(SYS_gettid)
typedef void gralloc1_device_t;
typedef enum {
   GRALLOC1_ERROR_NONE = 0,
} gralloc1_error_t;

#else

#include "glib.h"
// NOWHINE FILE NC006: glib specific definitions
#define gettid() syscall(SYS_gettid)
#define strlcpy g_strlcpy

#endif // ANDROID

#include "camxincs.h"
#include "camxosutils.h"
#include "camximageformatutils.h"

CAMX_NAMESPACE_BEGIN

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// File System
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// OsUtils::GetPropertyList
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID OsUtils::GetPropertyList(
    // NOWHINE CF003: Function callback don't want to put it on newlines.
    VOID(*pFunc)(const CHAR* pName, const CHAR* pValue, VOID* pData),
    VOID* pData)
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// OsUtils::GetPropertyString
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
INT OsUtils::GetPropertyString(
    const CHAR*  pName,
    CHAR*        pValue,
    const CHAR*  pDefault)
{
    INT result = 0;

    CAMX_UNREFERENCED_PARAM(pName);
    CAMX_UNREFERENCED_PARAM(pValue);
    CAMX_UNREFERENCED_PARAM(pDefault);
    CAMX_UNREFERENCED_PARAM(result);

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// OsUtils::GetPropertyBool
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL OsUtils::GetPropertyBool(
    const CHAR*  pName,
    BOOL         defaultValue)
{
    BOOL result = false;

    CAMX_UNREFERENCED_PARAM(pName);
    CAMX_UNREFERENCED_PARAM(defaultValue);
    CAMX_UNREFERENCED_PARAM(result);

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// OsUtils::GetPropertyINT64
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
INT64 OsUtils::GetPropertyINT64(
    const CHAR*  pName,
    INT64        defaultValue)
{
    INT64 result = 0;

    CAMX_UNREFERENCED_PARAM(pName);
    CAMX_UNREFERENCED_PARAM(defaultValue);
    CAMX_UNREFERENCED_PARAM(result);

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// OsUtils::GetPropertyINT32
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
INT32 OsUtils::GetPropertyINT32(
    const CHAR*  pName,
    INT32        defaultValue)
{
    INT32 result = 0;

    CAMX_UNREFERENCED_PARAM(pName);
    CAMX_UNREFERENCED_PARAM(defaultValue);
    CAMX_UNREFERENCED_PARAM(result);

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// OsUtils::Stat
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult OsUtils::Stat(
    const CHAR* pPath,
    StatType*   pBuffer)
{
    CamxResult result = CamxResultSuccess;
    if (0 != stat64(pPath, pBuffer))
    {
        result = CamxResultEFailed;
    }
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// OsUtils::FOpen
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
FILE* OsUtils::FOpen(
    const CHAR* pFilename,
    const CHAR* pMode)
{
    return fopen(pFilename, pMode);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// OsUtils::FileNo
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
INT OsUtils::FileNo(
    FILE* pFile)
{
    return fileno(pFile);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// OsUtils::FClose
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult OsUtils::FClose(
    FILE* pFile)
{
    CamxResult result = CamxResultSuccess;
    if (0 != fclose(pFile))
    {
        result = CamxResultEFailed;
    }
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// OsUtils::FSeek
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult OsUtils::FSeek(
    FILE*   pFile,
    INT64   offset,
    INT     origin)
{
    CamxResult result = CamxResultSuccess;
    if (0 != fseeko(pFile, static_cast<off_t>(offset), origin))
    {
        result = CamxResultEFailed;
    }
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// OsUtils::FTell
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
INT64 OsUtils::FTell(
    FILE* pFile)
{
    return static_cast<INT64>(ftello(pFile));
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// OsUtils::FPrintF
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
INT OsUtils::FPrintF(
    FILE*       pFile,
    const CHAR* pFormat,
                ...)
{
    INT     result;
    va_list argptr;

    va_start(argptr, pFormat);
    result = vfprintf(pFile, pFormat, argptr);
    va_end(argptr);

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// OsUtils::FScanF
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
INT OsUtils::FScanF(
    FILE*       pFile,
    const CHAR* pFormat,
    ...)
{
    INT     result;
    va_list argptr;

    va_start(argptr, pFormat);
    result = vfscanf(pFile, pFormat, argptr);
    va_end(argptr);

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// OsUtils::FRead
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
SIZE_T OsUtils::FRead(
    VOID*   pDst,
    SIZE_T  dstSize,
    SIZE_T  size,
    SIZE_T  count,
    FILE*   pFile)
{
    CAMX_UNREFERENCED_PARAM(dstSize);

    return fread(pDst, size, count, pFile);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// OsUtils::FWrite
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
SIZE_T OsUtils::FWrite(
    const VOID* pSrc,
    SIZE_T      size,
    SIZE_T      count,
    FILE*       pFile)
{
    return fwrite(pSrc, size, count, pFile);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// OsUtils::FFlush
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult OsUtils::FFlush(
    FILE* pFile)
{
    CamxResult result = CamxResultSuccess;
    if (0 != fflush(pFile))
    {
        result = CamxResultEFailed;
    }
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Logging functions
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// OsUtils::GetFileName
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
const CHAR* OsUtils::GetFileName(
    const CHAR* pFilePath)
{
    const CHAR* pFileName = StrRChr(pFilePath, '/');
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
// OsUtils::GetFileNameToken
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL OsUtils::GetFileNameToken(
    CHAR*  pFilePath,
    UINT32 tokenInputNum,
    CHAR*  pOutToken,
    SIZE_T maxTokenLen)
{
    UINT  tokenCount = 0;
    CHAR* pToken     = NULL;
    INT   result     = FALSE;
    CHAR* pContext   = NULL;
    CHAR  outTokenString[FILENAME_MAX];

    OsUtils::StrLCpy(outTokenString, pFilePath, strlen(pFilePath));
    pToken = OsUtils::StrTokReentrant(outTokenString, ".", &pContext);

    // The binary name is of format com.<vendor>.<category>.<algorithm>.<extension>
    while (NULL != pToken)
    {
        tokenCount++;
        if (tokenInputNum == tokenCount)
        {
            OsUtils::StrLCpy(pOutToken, pToken, maxTokenLen);
            result = TRUE;
            break;
        }
        pToken = OsUtils::StrTokReentrant(NULL, ".", &pContext);
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// OsUtils::GetFilesFromPath
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UINT16 OsUtils::GetFilesFromPath(
    const CHAR* pFileSearchPath,
    SIZE_T      maxFileNameLength,
    CHAR*       pFileNames,
    const CHAR* pVendorName,
    const CHAR* pCategoryName,
    const CHAR* pModuleName,
    const CHAR* pTargetName,
    const CHAR* pExtension)
{
    UINT16      fileCount        = 0;
    DIR*        pDirectory       = NULL;
    dirent*     pDirent          = NULL;
    const CHAR* pToken           = NULL;
    CHAR*       pContext         = NULL;
    UINT        tokenCount       = 0;
    BOOL        isValid          = TRUE;
    const CHAR* pCom             = static_cast<const CHAR*>("com");
    UINT8       maxFileNameToken = static_cast<UINT8>(FileNameToken::Max);
    const CHAR* pTokens[maxFileNameToken];
    CHAR        fileName[maxFileNameLength];

    pDirectory = opendir(pFileSearchPath);
    if (NULL != pDirectory)
    {
        while (NULL != (pDirent = readdir(pDirectory)))
        {
            OsUtils::StrLCpy(fileName, pDirent->d_name, maxFileNameLength);
            tokenCount     = 0;
            pToken         = OsUtils::StrTokReentrant(fileName, ".", &pContext);
            isValid        = TRUE;
            for (UINT8 i = 0; i < maxFileNameToken; i++)
            {
                pTokens[i] = NULL;
            }

            // The binary name is of format com.<vendor>.<category>.<module>.<extension>
            // Read all the tokens
            while ((NULL != pToken) && (tokenCount < maxFileNameToken))
            {
                pTokens[tokenCount++] = pToken;
                pToken = OsUtils::StrTokReentrant(NULL, ".", &pContext);
            }

            // token validation
            if ((NULL == pToken) && (maxFileNameToken - 1 <= tokenCount) && (maxFileNameToken >= tokenCount))
            {
                UINT8 index = 0;
                for (UINT8 i = 0; (i < maxFileNameToken) && (isValid == TRUE); i++)
                {
                    switch (static_cast<FileNameToken>(i))
                    {
                        case FileNameToken::Com:
                            pToken = pCom;
                            index  = i;
                            break;
                        case FileNameToken::Vendor:
                            pToken = pVendorName;
                            index  = i;
                            break;
                        case FileNameToken::Category:
                            pToken = pCategoryName;
                            index  = i;
                            break;
                        case FileNameToken::Module:
                            pToken = pModuleName;
                            index  = i;
                            break;
                        case FileNameToken::Target:
                            if ((maxFileNameToken == tokenCount + 1) && (0 == StrCmp(pTargetName, "*")))
                            {
                                continue;
                            }
                            else
                            {
                                pToken = pTargetName;
                                index  = i;
                            }
                            break;
                        case FileNameToken::Extension:
                            if (maxFileNameToken == tokenCount + 1)
                            {
                                pToken = pExtension;
                                index  = i-1;
                            }
                            else
                            {
                                pToken = pExtension;
                                index  = i;
                            }
                            break;
                        default:
                            break;
                    }

                    if (0 != OsUtils::StrCmp(pToken, "*"))
                    {
                        if (0 == OsUtils::StrCmp(pToken, pTokens[index]))
                        {
                            isValid = TRUE;
                        }
                        else
                        {
                            isValid = FALSE;
                        }
                    }
                }
                if (TRUE == isValid)
                {
                    OsUtils::SNPrintF(pFileNames + (fileCount * maxFileNameLength),
                                      maxFileNameLength,
                                      "%s%s%s",
                                      pFileSearchPath,
                                      PathSeparator,
                                      pDirent->d_name);
                    fileCount++;

                }
            }
        }
        closedir(pDirectory);
    }

    return fileCount;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// OsUtils::GetFileSize
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UINT64 OsUtils::GetFileSize(
    const CHAR* pFilename)
{
    UINT64      size     = 0;
    struct stat statBuf;

    if (0 == stat(pFilename, &statBuf))
    {
        size = statBuf.st_size;
    }

    return size;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// OsUtils::LogSystem
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID OsUtils::LogSystem(
    CamxLog     level,
    const CHAR* pText)
{
#ifdef ANDROID
    switch (level)
    {
        case CamxLogDebug:
            __android_log_write(ANDROID_LOG_DEBUG, "CamX", pText);
            break;
        case CamxLogError:
            __android_log_write(ANDROID_LOG_ERROR, "CamX", pText);
            break;
        case CamxLogWarning:
        case CamxLogPerfWarning:
            __android_log_write(ANDROID_LOG_WARN, "CamX", pText);
            break;
        case CamxLogConfig:
        case CamxLogEntryExit:
        case CamxLogInfo:
        case CamxLogPerfInfo:
        case CamxLogDRQ:
        case CamxLogMeta:
            __android_log_write(ANDROID_LOG_INFO, "CamX", pText);
            break;
        case CamxLogVerbose:
            __android_log_write(ANDROID_LOG_VERBOSE, "CamX", pText);
            break;
        default:
            // Unknown level, consider the message an error
            __android_log_write(ANDROID_LOG_ERROR, "CamX", pText);
            CAMX_ASSERT_ALWAYS();
            break;
    }

#else
    switch (level)
    {
        case CamxLogDebug:
        case CamxLogError:
            fprintf(stderr, "%s\n", pText);
            break;
        case CamxLogConfig:
        case CamxLogWarning:
        case CamxLogEntryExit:
        case CamxLogInfo:
        case CamxLogVerbose:
        case CamxLogPerfInfo:
        case CamxLogPerfWarning:
        case CamxLogDRQ:
        case CamxLogMeta:
            printf("%s\n", pText);
            break;
        default:
            // Unknown level, consider the message an error
            fprintf(stderr, "%s\n", pText);
            CAMX_ASSERT_ALWAYS();
            break;
    }
#endif // defined (ANDROID)
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// OsUtils::SetFDLimit
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult OsUtils::SetFDLimit(
    UINT32 maxFDNum)
{
    CamxResult    result = CamxResultSuccess;
    struct rlimit limitOpenFiles;

    if (getrlimit(RLIMIT_NOFILE, &limitOpenFiles) == 0)
    {
        CAMX_LOG_INFO(CamxLogGroupCore, "curren limit (RLIMIT_NOFILE):%d", limitOpenFiles.rlim_cur);

        if (limitOpenFiles.rlim_cur < maxFDNum)
        {
            limitOpenFiles.rlim_cur = maxFDNum;
        }

        if (setrlimit(RLIMIT_NOFILE, &limitOpenFiles) != 0)
        {
            CAMX_LOG_ERROR(CamxLogGroupCore, "fail to setrlimit(RLIMIT_NOFILE), errno:%d", errno);
            result = CamxResultEFailed;
        }

        getrlimit(RLIMIT_NOFILE, &limitOpenFiles);
        CAMX_LOG_INFO(CamxLogGroupCore, "new limit (RLIMIT_NOFILE):%d", limitOpenFiles.rlim_cur);
    }
    else
    {
        CAMX_LOG_ERROR(CamxLogGroupCore, "getrlimit(RLIMIT_NOFILE)");
        result = CamxResultEFailed;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// OsUtils::LoadPreBuiltLibrary
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID* OsUtils::LoadPreBuiltLibrary(
    const CHAR*      pLibName,
    const CHAR*      pFuncName,
    OSLIBRARYHANDLE* phLibHandle)
{
    VOID* pFunc = NULL;
    INT   numOfCharWritten = 0;
    CHAR  libFileName[FILENAME_MAX] = { "" };

    CAMX_ASSERT(NULL != pLibName);
    CAMX_ASSERT(NULL != pFuncName);
    CAMX_ASSERT(NULL != phLibHandle);

    numOfCharWritten = OsUtils::SNPrintF(libFileName,
        FILENAME_MAX,
        "%s%s%s.%s",
        VendorLibPath,
        PathSeparator,
        pLibName,
        SharedLibraryExtension);

    // Load the Library
    *phLibHandle = OsUtils::LibMap(libFileName);

    CAMX_ASSERT(NULL != *phLibHandle);
    // Get the pointer to the cunction
    pFunc = OsUtils::LibGetAddr(*phLibHandle, pFuncName);
    CAMX_ASSERT(NULL != pFunc);
    return pFunc;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Memory functions
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// OsUtils::MallocAligned
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID* OsUtils::MallocAligned(
    SIZE_T size,
    SIZE_T alignment)
{
    VOID* pMem = NULL;

    if ((0 == alignment) || (TRUE == CamX::Utils::IsPowerOfTwo(alignment)))
    {
        if (alignment < sizeof(VOID*))
        {
            alignment = sizeof(VOID*);
        }

        if (posix_memalign(&pMem, alignment, size) != 0)
        {
            pMem = NULL;
        }
    }

    return pMem;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// OsUtils::FreeAligned
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID OsUtils::FreeAligned(
    VOID* pMem)
{
    free(pMem);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// OsUtils::PageSize
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
INT OsUtils::PageSize()
{
    return sysconf(_SC_PAGESIZE);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// OsUtils::MemoryProtect
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult OsUtils::MemoryProtect(
    VOID*               pMem,
    SIZE_T              size,
    CamxProtectionType  accessRights)
{
    CamxResult result = CamxResultSuccess;

    // Validate pointer/size alignment
    INT pageSize = PageSize();
    if ((Utils::ByteAlign(size, pageSize) != size) || (Utils::ByteAlignPtr(pMem, pageSize) != pMem))
    {
        result = CamxResultEInvalidArg;
    }
    else
    {
        // Determine access rights
        INT flags = PROT_NONE;
        flags |= ((accessRights.read == TRUE) ? PROT_READ : 0);
        flags |= ((accessRights.write == TRUE) ? PROT_WRITE : 0);

        // Set access rights
        INT resultCode = mprotect(pMem, size, flags);
        if (0 != resultCode)
        {
            result = CamxResultEFailed;
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// OsUtils::MemMap
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID* OsUtils::MemMap(
    INT     bufferFD,
    SIZE_T  bufferLength,
    SIZE_T  offset)
{
    VOID* pMem = NULL;

    CAMX_ASSERT(bufferFD >= 0);
    if ((bufferLength > 0) && (bufferFD >= 0))
    {
        pMem = mmap(NULL, bufferLength, (PROT_READ | PROT_WRITE), MAP_SHARED, bufferFD, offset);
        if (MAP_FAILED == pMem)
        {
            CAMX_LOG_ERROR(CamxLogGroupCSL, "mmap() failed! errno=%d, bufferFD=%zd, bufferLength=%d offset=%zd",
                           errno, bufferFD, bufferLength, offset);
            pMem = NULL;
        }
    }

    return pMem;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// OsUtils::MemUnmap
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
INT OsUtils::MemUnmap(
    VOID*   pAddr,
    SIZE_T  size)
{
    INT result = -1;

    CAMX_ASSERT(NULL != pAddr);
    if (NULL != pAddr)
    {
        result = munmap(pAddr, size);
        if (0 != result)
        {
            CAMX_LOG_ERROR(CamxLogGroupCSL, "munmap() failed! errno %d", errno);
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// String functions
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// OsUtils::DPrintF
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
INT OsUtils::DPrintF(
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
// OsUtils::StrError
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID OsUtils::StrError(
    CHAR*   pErrorMsg,
    SIZE_T  errorMsgSize,
    INT     errorNum)
{
    (void)StrLCpy(pErrorMsg, strerror(errorNum), errorMsgSize);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Thread functions
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// OsUtils::ThreadCreate
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult OsUtils::ThreadCreate(
    OSThreadFunc    threadEntryFunction,
    VOID*           pThreadData,
    OSThreadHandle* phThread)
{
    CamxResult result = CamxResultSuccess;

    if (pthread_create(phThread, 0, threadEntryFunction, pThreadData) != 0)
    {
        // Failed to create thread
        CAMX_ASSERT_ALWAYS();
        result = CamxResultEFailed;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// OsUtils::ThreadSetName
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult OsUtils::ThreadSetName(
    OSThreadHandle  hThread,
    const CHAR*     pName)
{
    CamxResult result = CamxResultSuccess;

    if (pthread_setname_np(hThread, pName) != 0)
    {
        result = CamxResultEFailed;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// OsUtils::ThreadWait
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID OsUtils::ThreadWait(
    OSThreadHandle hThread)
{
    pthread_join(hThread, NULL);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// OsUtils::SleepMicroseconds
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID OsUtils::SleepMicroseconds(
    UINT microseconds)
{
    CAMX_ENTRYEXIT_SCOPE(CamxLogGroupUtils, SCOPEEventOsUtilsSleepMicroseconds);

    // sleep in us
    usleep(microseconds);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// OsUtils::LibMap
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
OSLIBRARYHANDLE OsUtils::LibMap(
    const CHAR* pLibraryName)
{
    OSLIBRARYHANDLE hLibrary = NULL;
    const UINT bind_flags = RTLD_NOW | RTLD_LOCAL;

    CAMX_ASSERT(NULL != pLibraryName);

    hLibrary = dlopen(pLibraryName, bind_flags);

    if (NULL == hLibrary)
    {
        const CHAR* pErrorMessage = dlerror();

        CAMX_LOG_ERROR(CamxLogGroupUtils,
                       "dlopen: %s, on '%s'",
                       pErrorMessage ? pErrorMessage : "N/A",
                       pLibraryName);
        CAMX_ASSERT_ALWAYS();
    }

    return hLibrary;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// OsUtils::LibUnmap
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult OsUtils::LibUnmap(
    OSLIBRARYHANDLE hLibrary)
{
    CamxResult result = CamxResultSuccess;

    CAMX_ASSERT(NULL != hLibrary);

    if (NULL != hLibrary)
    {
        if (0 != dlclose(hLibrary))
        {
            const CHAR* pErrorMessage = dlerror();

            CAMX_LOG_ERROR(CamxLogGroupUtils, "dlclose error: %s", pErrorMessage ? pErrorMessage
                                                                            : "N/A");
            result = CamxResultEFailed;
            CAMX_ASSERT_ALWAYS();
        }
    }
    else
    {
        result = CamxResultEInvalidArg;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// OsUtils::LibGetAddr
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID* OsUtils::LibGetAddr(
    OSLIBRARYHANDLE hLibrary,
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
// OsUtils::GetThreadID
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UINT OsUtils::GetThreadID()
{
    return gettid();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// OsUtils::GetProcessID
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UINT OsUtils::GetProcessID()
{
    return getpid();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// String Functions
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// OsUtils::StrNICmp
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
INT OsUtils::StrNICmp(
    const CHAR* pString1,
    const CHAR* pString2,
    SIZE_T      maxCount)
{
    return strncasecmp(pString1, pString2, maxCount);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// OsUtils::StrLCat
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
SIZE_T OsUtils::StrLCat(
    CHAR* pDst,
    const CHAR* pSrc,
    SIZE_T sizeDst)
{
#ifdef ANDROID
    return strlcat(pDst, pSrc, sizeDst);
#else // ANDROID
    return g_strlcat(pDst, pSrc, sizeDst);
#endif // ANDROID
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// OsUtils::StrLCpy
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
SIZE_T OsUtils::StrLCpy(
    CHAR*       pDst,
    const CHAR* pSrc,
    SIZE_T      sizeDst)
{
#ifdef ANDROID
    return strlcpy(pDst, pSrc, sizeDst);
#else // ANDROID
    return g_strlcpy(pDst, pSrc, sizeDst);
#endif // ANDROID
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// OsUtils::StrTokReentrant
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CHAR* OsUtils::StrTokReentrant(
    CHAR*       pSrc,
    const CHAR* pDelimiter,
    CHAR**      ppContext)
{
    return strtok_r(pSrc, pDelimiter, ppContext);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Native Fence Functions
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// OsUtils::NativeFenceWait
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult OsUtils::NativeFenceWait(
    NativeFence hFence,
    UINT32      timeoutMilliseconds)
{
    CamxResult result = CamxResultEFailed;

#ifdef ANDROID
    INT status = sync_wait(hFence, timeoutMilliseconds);
    if (0 == status)
    {
        result = CamxResultSuccess;
    }
#endif // ANDROID

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// OsUtils::GetTime
///
/// @brief  Gets the current time
///
/// @param  pTime Output time structure
///
/// @return None
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID OsUtils::GetTime(
    CamxTime* pTime)
{
    CAMX_ASSERT(NULL != pTime);

    timespec time;
    clock_gettime(CLOCK_BOOTTIME, &time);

    pTime->seconds      = static_cast<UINT32>(time.tv_sec);
    pTime->nanoSeconds  = static_cast<UINT32>(time.tv_nsec);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// OsUtils::GetDateTime
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID OsUtils::GetDateTime(
    CamxDateTime* pDateTime)
{
    CAMX_ASSERT(NULL != pDateTime);
    struct timeval  timeValue;
    struct tm*      pTimeInfo   = NULL;
    INT             result      = 0;

    result = gettimeofday(&timeValue, NULL);
    if (0 == result)
    {
        pTimeInfo = localtime(&timeValue.tv_sec);
        if (NULL != pTimeInfo)
        {
            pDateTime->seconds                  = static_cast<UINT32>(pTimeInfo->tm_sec);
            pDateTime->microseconds             = static_cast<UINT32>(timeValue.tv_usec);
            pDateTime->minutes                  = static_cast<UINT32>(pTimeInfo->tm_min);
            pDateTime->hours                    = static_cast<UINT32>(pTimeInfo->tm_hour);
            pDateTime->dayOfMonth               = static_cast<UINT32>(pTimeInfo->tm_mday);
            pDateTime->month                    = static_cast<UINT32>(pTimeInfo->tm_mon);
            pDateTime->year                     = static_cast<UINT32>(pTimeInfo->tm_year);
            pDateTime->weekday                  = static_cast<UINT32>(pTimeInfo->tm_wday);
            pDateTime->yearday                  = static_cast<UINT32>(pTimeInfo->tm_yday);
            pDateTime->dayLightSavingTimeFlag   = static_cast<UINT32>(pTimeInfo->tm_isdst);
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// OsUtils::GetGMTime
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
INT OsUtils::GetGMTime(
    const time_t*   pTime,
    struct tm*      pResult)
{
    CAMX_ASSERT(NULL != pTime);
    CAMX_ASSERT(NULL != pResult);
    INT        result        = 0;
    struct tm* pReturnValue  = NULL;

    pReturnValue = gmtime_r(pTime, pResult);
    if (NULL == pReturnValue)
    {
        result = -1;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// OsUtils::Close
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult OsUtils::Close(
    INT FD)
{
    CamxResult result = CamxResultSuccess;
    if (-1 != FD)
    {
        if (0 != close(FD))
        {
            result = CamxResultEFailed;
        }
    }
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// OsUtils::InitializeFormatAlignment
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID OsUtils::InitializeFormatAlignment(
    ImageFormat*       pImageFormat,
    FormatParamInfo*   pFormatParamInfo)
{
#ifdef ANDROID
    if (FALSE == pFormatParamInfo->isImplDefined)
    {
        pImageFormat->formatParams.yuvFormat[0].width = pImageFormat->width;
        pImageFormat->formatParams.yuvFormat[0].height = pImageFormat->height;
        pImageFormat->formatParams.yuvFormat[1].width = pImageFormat->width;
        pImageFormat->formatParams.yuvFormat[1].height = pImageFormat->height >> 1;

        if (pFormatParamInfo->sliceHeight != 0 && pFormatParamInfo->planeStride != 0)
        {
            pImageFormat->formatParams.yuvFormat[0].planeStride = pFormatParamInfo->planeStride;
            pImageFormat->formatParams.yuvFormat[0].sliceHeight = pFormatParamInfo->sliceHeight;
            pImageFormat->formatParams.yuvFormat[1].planeStride = pFormatParamInfo->planeStride;
            pImageFormat->formatParams.yuvFormat[1].sliceHeight = pFormatParamInfo->sliceHeight >> 1;
        }
        else if (GrallocUsageHwImageEncoder == (pFormatParamInfo->grallocUsage & GrallocUsageHwImageEncoder))
        {
            pImageFormat->formatParams.yuvFormat[0].planeStride = Utils::AlignGeneric32(pImageFormat->width, 512);
            pImageFormat->formatParams.yuvFormat[0].sliceHeight = Utils::AlignGeneric32(pImageFormat->height, 512);
            pImageFormat->formatParams.yuvFormat[1].planeStride = Utils::AlignGeneric32(pImageFormat->width, 512);
            pImageFormat->formatParams.yuvFormat[1].sliceHeight = Utils::AlignGeneric32(pImageFormat->height >> 1, 256);
        }
        else if (GrallocUsageHwVideoEncoder == (pFormatParamInfo->grallocUsage & GrallocUsageHwVideoEncoder))
        {
            pImageFormat->formatParams.yuvFormat[0].planeStride = Utils::AlignGeneric32(pImageFormat->width, 128);
            pImageFormat->formatParams.yuvFormat[0].sliceHeight = Utils::AlignGeneric32(pImageFormat->height, 32);
            pImageFormat->formatParams.yuvFormat[1].planeStride = Utils::AlignGeneric32(pImageFormat->width, 128);
            pImageFormat->formatParams.yuvFormat[1].sliceHeight = Utils::AlignGeneric32(pImageFormat->height >> 1, 16);
        }
        else
        {
            pImageFormat->formatParams.yuvFormat[0].planeStride = Utils::AlignGeneric32(pImageFormat->width, 64);
            pImageFormat->formatParams.yuvFormat[0].sliceHeight = Utils::AlignGeneric32(pImageFormat->height, 64);
            pImageFormat->formatParams.yuvFormat[1].planeStride = Utils::AlignGeneric32(pImageFormat->width, 64);
            pImageFormat->formatParams.yuvFormat[1].sliceHeight = Utils::AlignGeneric32(pImageFormat->height, 64) >> 1;
        }
        pImageFormat->formatParams.yuvFormat[0].planeSize = ImageFormatUtils::GetPlaneSize(pImageFormat, 0);
        pImageFormat->formatParams.yuvFormat[1].planeSize = ImageFormatUtils::GetPlaneSize(pImageFormat, 1);
    }
    else
    {
        if (GrallocUsageHwVideoEncoder == (pFormatParamInfo->grallocUsage & GrallocUsageHwVideoEncoder))
        {
            pImageFormat->formatParams.yuvFormat[0].width = pImageFormat->width;
            pImageFormat->formatParams.yuvFormat[0].height = pImageFormat->height;
            pImageFormat->formatParams.yuvFormat[1].width = pImageFormat->width;
            pImageFormat->formatParams.yuvFormat[1].height = pImageFormat->height >> 1;
            if (pFormatParamInfo->sliceHeight != 0 && pFormatParamInfo->planeStride != 0)
            {
                pImageFormat->formatParams.yuvFormat[0].planeStride = pFormatParamInfo->planeStride;
                pImageFormat->formatParams.yuvFormat[0].sliceHeight = pFormatParamInfo->sliceHeight;
                pImageFormat->formatParams.yuvFormat[1].planeStride = pFormatParamInfo->planeStride;
                pImageFormat->formatParams.yuvFormat[1].sliceHeight = pFormatParamInfo->sliceHeight >> 1;
            }
            else if (GrallocUsageHwImageEncoder == (pFormatParamInfo->grallocUsage & GrallocUsageHwImageEncoder))
            {
                pImageFormat->formatParams.yuvFormat[0].planeStride = Utils::AlignGeneric32(pImageFormat->width, 512);
                pImageFormat->formatParams.yuvFormat[0].sliceHeight = Utils::AlignGeneric32(pImageFormat->height, 512);
                pImageFormat->formatParams.yuvFormat[1].planeStride = Utils::AlignGeneric32(pImageFormat->width, 512);
                pImageFormat->formatParams.yuvFormat[1].sliceHeight = Utils::AlignGeneric32(pImageFormat->height >> 1, 256);
            }
            else
            {
                pImageFormat->formatParams.yuvFormat[0].planeStride = Utils::AlignGeneric32(pImageFormat->width, 128);
                pImageFormat->formatParams.yuvFormat[0].sliceHeight = Utils::AlignGeneric32(pImageFormat->height, 32);
                pImageFormat->formatParams.yuvFormat[1].planeStride = Utils::AlignGeneric32(pImageFormat->width, 128);
                pImageFormat->formatParams.yuvFormat[1].sliceHeight = Utils::AlignGeneric32(pImageFormat->height >> 1, 16);
            }
            pImageFormat->formatParams.yuvFormat[0].planeSize = ImageFormatUtils::GetPlaneSize(pImageFormat, 0);
            pImageFormat->formatParams.yuvFormat[1].planeSize = ImageFormatUtils::GetPlaneSize(pImageFormat, 1);
        }
        else if (GrallocUsageHwCameraZSL == (pFormatParamInfo->grallocUsage & GrallocUsageHwCameraZSL))
        {
            pImageFormat->formatParams.yuvFormat[0].width = pImageFormat->width;
            pImageFormat->formatParams.yuvFormat[0].height = pImageFormat->height;
            pImageFormat->formatParams.yuvFormat[1].width = pImageFormat->width;
            pImageFormat->formatParams.yuvFormat[1].height = pImageFormat->height >> 1;
            if (pFormatParamInfo->sliceHeight != 0 && pFormatParamInfo->planeStride != 0)
            {
                pImageFormat->formatParams.yuvFormat[0].planeStride = pFormatParamInfo->planeStride;
                pImageFormat->formatParams.yuvFormat[0].sliceHeight = pFormatParamInfo->sliceHeight;
                pImageFormat->formatParams.yuvFormat[1].planeStride = pFormatParamInfo->planeStride;
                pImageFormat->formatParams.yuvFormat[1].sliceHeight = pFormatParamInfo->sliceHeight >> 1;
            }
            else
            {
                pImageFormat->formatParams.yuvFormat[0].planeStride = Utils::AlignGeneric32(pImageFormat->width, 64);
                pImageFormat->formatParams.yuvFormat[0].sliceHeight = Utils::AlignGeneric32(pImageFormat->height, 64);
                pImageFormat->formatParams.yuvFormat[1].planeStride = Utils::AlignGeneric32(pImageFormat->width, 64);
                pImageFormat->formatParams.yuvFormat[1].sliceHeight = Utils::AlignGeneric32(pImageFormat->height, 64) >> 1;
            }
            pImageFormat->formatParams.yuvFormat[0].planeSize = ImageFormatUtils::GetPlaneSize(pImageFormat, 0);
            pImageFormat->formatParams.yuvFormat[1].planeSize = ImageFormatUtils::GetPlaneSize(pImageFormat, 1);
        }
        else
        {
            pImageFormat->formatParams.yuvFormat[0].width = pImageFormat->width;
            pImageFormat->formatParams.yuvFormat[0].height = pImageFormat->height;
            pImageFormat->formatParams.yuvFormat[1].width = pImageFormat->width;
            pImageFormat->formatParams.yuvFormat[1].height = pImageFormat->height >> 1;
            if (pFormatParamInfo->sliceHeight != 0 && pFormatParamInfo->planeStride != 0)
            {
                pImageFormat->formatParams.yuvFormat[0].planeStride = pFormatParamInfo->planeStride;
                pImageFormat->formatParams.yuvFormat[0].sliceHeight = pFormatParamInfo->sliceHeight;
                pImageFormat->formatParams.yuvFormat[1].planeStride = pFormatParamInfo->planeStride;
                pImageFormat->formatParams.yuvFormat[1].sliceHeight = pFormatParamInfo->sliceHeight >> 1;
            }
            else
            {
                pImageFormat->formatParams.yuvFormat[0].planeStride = Utils::AlignGeneric32(pImageFormat->width, 128);
                pImageFormat->formatParams.yuvFormat[0].sliceHeight = Utils::AlignGeneric32(pImageFormat->height, 32);
                pImageFormat->formatParams.yuvFormat[1].planeStride = Utils::AlignGeneric32(pImageFormat->width, 128);
                pImageFormat->formatParams.yuvFormat[1].sliceHeight = Utils::AlignGeneric32(pImageFormat->height >> 1, 16);
            }
            pImageFormat->formatParams.yuvFormat[0].planeSize = ImageFormatUtils::GetPlaneSize(pImageFormat, 0);
            pImageFormat->formatParams.yuvFormat[1].planeSize = ImageFormatUtils::GetPlaneSize(pImageFormat, 1);
        }
    }
#else
    CAMX_UNREFERENCED_PARAM(pImageFormat);
    CAMX_UNREFERENCED_PARAM(pFormatParamInfo);

    CAMX_NOT_IMPLEMENTED();
#endif // ANDROID
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// OsUtils::SetGrallocMetaData
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult OsUtils::SetGrallocMetaData(
    const NativeHandle* phNativehandle,
    UINT32              metadatatype,
    VOID*               pMetadata)
{
    CamxResult result = CamxResultSuccess;

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// OsUtils::RaiseSignalAbort
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID OsUtils::RaiseSignalAbort()
{
    raise(SIGABRT);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Mutex class
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Mutex::Create
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
Mutex* Mutex::Create(
    const CHAR* pResourceName)
{
    Mutex* pMutex = NULL;

    pMutex = CAMX_NEW Mutex();
    if (NULL != pMutex)
    {
        if (CamxResultSuccess != pMutex->Initialize(pResourceName))
        {
            CAMX_DELETE pMutex;
            pMutex = NULL;
        }
    }

    return pMutex;
}

#if CAMX_USE_MEMSPY
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Mutex::CreateNoSpy
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
Mutex* Mutex::CreateNoSpy(
    const CHAR* pResourceName)
{
    Mutex* pMutex = NULL;

    pMutex = CAMX_NEW_NO_SPY Mutex();
    if (NULL != pMutex)
    {
        if (CamxResultSuccess != pMutex->Initialize(pResourceName))
        {
            CAMX_DELETE pMutex;
            pMutex = NULL;
        }
    }

    return pMutex;
}
#endif // CAMX_USE_MEMSPY

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Mutex::Initialize
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult Mutex::Initialize(
    const CHAR* pResourceName)
{
    CamxResult          result      = CamxResultSuccess;
    pthread_mutexattr_t attr;
    BOOL                bValidAttr  = FALSE;      // TRUE once attr has been initialized

    CAMX_ASSERT(pResourceName != NULL);

    OsUtils::StrLCpy(m_pResourceName, pResourceName, MaxResourceNameSize);

    if (pthread_mutexattr_init(&attr) == 0)
    {
        bValidAttr = TRUE;
    }
    else
    {
        result = CamxResultEFailed;
    }

    // Using re-entrant mutexes
    if ((CamxResultSuccess == result) &&
        (pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE_NP) != 0))
    {
        result = CamxResultEFailed;
    }

    if ((CamxResultSuccess == result) &&
        (pthread_mutex_init(&m_mutex, &attr) == 0))
    {
        CAMX_TRACE_MESSAGE_F(CamxLogGroupSync, "Mutex Init");
        m_validMutex = TRUE;
    }
    else
    {
        result = CamxResultEFailed;
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

    CAMX_DELETE this;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Mutex::Lock
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID Mutex::Lock()
{
    CAMX_TRACE_SYNC_BEGIN_F(CamxLogGroupSync, "%s: Mutex::Lock", m_pResourceName);
    pthread_mutex_lock(&m_mutex);
    CAMX_TRACE_ASYNC_BEGIN_F(CamxLogGroupSync, Utils::VoidPtrToUINT64(GetNativeHandle()), "%s: Mutex Held", m_pResourceName);
    CAMX_TRACE_SYNC_END(CamxLogGroupSync);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Mutex::TryLock
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult Mutex::TryLock()
{
    CamxResult result     = CamxResultSuccess;
    INT        returnCode = 0;

    CAMX_TRACE_SYNC_BEGIN_F(CamxLogGroupSync, "%s: Mutex::Lock", m_pResourceName);
    returnCode = pthread_mutex_trylock(&m_mutex);
    if (0 != returnCode)
    {
        if (EBUSY == returnCode)
        {
            result = CamxResultEBusy;
        }
        else
        {
            result = CamxResultEFailed;
        }
    }
    else
    {
        CAMX_TRACE_ASYNC_BEGIN_F(CamxLogGroupSync, Utils::VoidPtrToUINT64(GetNativeHandle()),
                                 "%s: Mutex Held", m_pResourceName);
    }
    CAMX_TRACE_SYNC_END(CamxLogGroupSync);

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Mutex::Unlock
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID Mutex::Unlock()
{
    pthread_mutex_unlock(&m_mutex);
    CAMX_TRACE_ASYNC_END_F(CamxLogGroupSync, Utils::VoidPtrToUINT64(GetNativeHandle()), "%s: Mutex Held", m_pResourceName);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Mutex::GetNativeHandle
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
OSMutexHandle* Mutex::GetNativeHandle()
{
    return ((TRUE == m_validMutex) ? &m_mutex : NULL);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ReadWriteLock class
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ReadWriteLock::Create
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
ReadWriteLock* ReadWriteLock::Create(
    const CHAR* pResourceName)
{
    ReadWriteLock* pReadWriteLock = NULL;

    pReadWriteLock = CAMX_NEW ReadWriteLock();

    if (NULL != pReadWriteLock)
    {
        if (CamxResultSuccess != pReadWriteLock->Initialize(pResourceName))
        {
            CAMX_DELETE pReadWriteLock;
            pReadWriteLock = NULL;
        }
    }

    return pReadWriteLock;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ReadWriteLock::Initialize
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult ReadWriteLock::Initialize(
    const CHAR* pResourceName)
{
    CamxResult result = CamxResultSuccess;

    if (pthread_rwlock_init(&m_readWriteLock, NULL) == 0)
    {
        CAMX_ASSERT(pResourceName != NULL);

        OsUtils::StrLCpy(m_pResourceName, pResourceName, MaxResourceNameSize);
        m_validReadWriteLock    = TRUE;
        m_lockCount             = 0;
    }
    else
    {
        result = CamxResultEFailed;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ReadWriteLock::Destroy
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID ReadWriteLock::Destroy()
{
    if (TRUE == m_validReadWriteLock)
    {
        pthread_rwlock_destroy(&m_readWriteLock);
    }

    CAMX_DELETE this;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ReadWriteLock::ReadLock
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID ReadWriteLock::ReadLock()
{
    CAMX_TRACE_SYNC_BEGIN_F(CamxLogGroupSync, "%s: RWLock::ReadLock", m_pResourceName);
    pthread_rwlock_rdlock(&m_readWriteLock);
    INT lockCount = CamxAtomicInc(&m_lockCount);
    CAMX_TRACE_INT32_F(CamxLogGroupSync, lockCount, "%s: RWLock::LockCount" , m_pResourceName);
    CAMX_TRACE_SYNC_END(CamxLogGroupSync);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ReadWriteLock::WriteLock
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID ReadWriteLock::WriteLock()
{
    CAMX_TRACE_SYNC_BEGIN_F(CamxLogGroupSync, "%s: RWLock::WriteLock", m_pResourceName);
    pthread_rwlock_wrlock(&m_readWriteLock);
    INT lockCount = CamxAtomicInc(&m_lockCount);
    CAMX_TRACE_INT32_F(CamxLogGroupSync, lockCount, "%s: RWLock::LockCount" , m_pResourceName);
    CAMX_TRACE_SYNC_END(CamxLogGroupSync);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ReadWriteLock::TryReadLock
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL ReadWriteLock::TryReadLock()
{
    BOOL isAcquired = FALSE;

    CAMX_TRACE_SYNC_BEGIN_F(CamxLogGroupSync, "%s: RWLock::TryReadLock", m_pResourceName);
    if (0 == pthread_rwlock_tryrdlock(&m_readWriteLock))
    {
        isAcquired = TRUE;
        INT lockCount = CamxAtomicInc(&m_lockCount);
        CAMX_TRACE_INT32_F(CamxLogGroupSync, lockCount, "%s: RWLock::LockCount" , m_pResourceName);
        CAMX_TRACE_MESSAGE_F(CamxLogGroupSync, "%s: Acquired lock", m_pResourceName);
    }
    CAMX_TRACE_SYNC_END(CamxLogGroupSync);

    return isAcquired;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ReadWriteLock::TryWriteLock
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL ReadWriteLock::TryWriteLock()
{
    CAMX_ENTRYEXIT_SCOPE_ID(CamxLogGroupSync, SCOPEEventOsUtilsReadWriteLockWaitOnWriteLock, Utils::VoidPtrToUINT64(this));
    BOOL isAcquired = FALSE;

    CAMX_TRACE_SYNC_BEGIN_F(CamxLogGroupSync, "%s: RWLock::TryWriteLock", m_pResourceName);
    if (0 == pthread_rwlock_trywrlock(&m_readWriteLock))
    {
        isAcquired = TRUE;
        INT lockCount = CamxAtomicInc(&m_lockCount);
        CAMX_TRACE_INT32_F(CamxLogGroupSync, lockCount, "%s: RWLock::LockCount" , m_pResourceName);
        CAMX_TRACE_MESSAGE_F(CamxLogGroupSync, "%s: Acquired lock", m_pResourceName);
    }
    CAMX_TRACE_SYNC_END(CamxLogGroupSync);

    return isAcquired;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ReadWriteLock::Unlock
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID ReadWriteLock::Unlock()
{
    INT lockCount = CamxAtomicDec(&m_lockCount);
    CAMX_TRACE_INT32_F(CamxLogGroupSync, lockCount, "%s: RWLock::LockCount" , m_pResourceName);
    CAMX_TRACE_MESSAGE_F(CamxLogGroupSync, "%s: Unlock", m_pResourceName);
    pthread_rwlock_unlock(&m_readWriteLock);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Semaphore class
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Semaphore::Create
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
Semaphore* Semaphore::Create()
{
    Semaphore* pSemaphore = NULL;

    pSemaphore = CAMX_NEW Semaphore();

    if (NULL != pSemaphore)
    {
        if (CamxResultSuccess != pSemaphore->Initialize())
        {
            CAMX_DELETE pSemaphore;
            pSemaphore = NULL;
        }
    }

    return pSemaphore;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Semaphore::Initialize
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult Semaphore::Initialize()
{
    CamxResult result = CamxResultSuccess;

    m_validSemaphore = FALSE;

    if (sem_init(&m_semaphore, 0, 0) == 0)
    {
        m_validSemaphore = TRUE;
    }
    else
    {
        result = CamxResultEFailed;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Semaphore::Destroy
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID Semaphore::Destroy()
{
    if (TRUE == m_validSemaphore)
    {
        sem_destroy(&m_semaphore);
    }

    CAMX_DELETE this;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Semaphore::Wait
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID Semaphore::Wait()
{
    CAMX_ENTRYEXIT_SCOPE_ID(CamxLogGroupSync, SCOPEEventOsUtilsSemaphoreWait, Utils::VoidPtrToUINT64(this));
    if (TRUE == m_validSemaphore)
    {
        sem_wait(&m_semaphore);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Semaphore::Signal
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID Semaphore::Signal()
{
    CAMX_ENTRYEXIT_SCOPE_ID(CamxLogGroupSync, SCOPEEventOsUtilsSemaphoreSignal, Utils::VoidPtrToUINT64(this));
    if (TRUE == m_validSemaphore)
    {
        sem_post(&m_semaphore);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Semaphore::TimedWait
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult Semaphore::TimedWait(
    UINT timeoutMilliseconds)
{
    CAMX_ENTRYEXIT_SCOPE_ID(CamxLogGroupSync, SCOPEEventOsUtilsSemaphoreWait, Utils::VoidPtrToUINT64(this));
    CamxResult      result              = CamxResultSuccess;
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

    waitResult = sem_timedwait(&m_semaphore, &timeout);
    if (waitResult != 0)
    {
        // Check errno for reason for failure
        if (ETIMEDOUT == waitResult)
        {
            result = CamxResultETimeout;
        }
        else
        {
            result = CamxResultEFailed;
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Condition class
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Condition::Create
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
Condition* Condition::Create(
    const CHAR* pResource)
{
    Condition* pCondition = NULL;

    pCondition = CAMX_NEW Condition();

    if (NULL != pCondition)
    {
        if (CamxResultSuccess != pCondition->Initialize(pResource))
        {
            CAMX_DELETE pCondition;
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

    CAMX_DELETE this;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Condition::Initialize
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult Condition::Initialize(
    const CHAR* pResource)
{
    CamxResult result = CamxResultSuccess;

    m_pResource = pResource;

    if (pthread_cond_init(&m_conditionVar, NULL) == 0)
    {
        m_validConditionVar = TRUE;
    }
    else
    {
        result = CamxResultEFailed;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Condition::Wait
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult Condition::Wait(
    OSMutexHandle* phMutex)
{
    CAMX_TRACE_SYNC_BEGIN_F(CamxLogGroupSync, "Condition::Wait %s", m_pResource);
    CAMX_ASSERT(NULL != phMutex);

    INT         rc      = 0;
    CamxResult  result  = CamxResultEFailed;

    CAMX_TRACE_ASYNC_END_F(CamxLogGroupSync, Utils::VoidPtrToUINT64(phMutex), "ConditionLock %s", m_pResource);
    rc = pthread_cond_wait(&m_conditionVar, phMutex);
    CAMX_TRACE_ASYNC_BEGIN_F(CamxLogGroupSync, Utils::VoidPtrToUINT64(phMutex), "ConditionLock %s", m_pResource);

    if (0 == rc)
    {
        result = CamxResultSuccess;
    }

    CAMX_TRACE_SYNC_END(CamxLogGroupSync);
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Condition::TimedWait
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult Condition::TimedWait(
    OSMutexHandle*  phMutex,
    UINT            timeoutMilliseconds)
{
    CAMX_TRACE_SYNC_BEGIN_F(CamxLogGroupSync, "Condition::TimedWait %s Timeout: %d", m_pResource, timeoutMilliseconds);
    CAMX_ASSERT(NULL != phMutex);

    CamxResult      result              = CamxResultSuccess;
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

    CAMX_TRACE_ASYNC_END_F(CamxLogGroupSync, Utils::VoidPtrToUINT64(phMutex), "ConditionLock %s", m_pResource);
    waitResult = pthread_cond_timedwait(&m_conditionVar, phMutex, &timeout);
    CAMX_TRACE_ASYNC_BEGIN_F(CamxLogGroupSync, Utils::VoidPtrToUINT64(phMutex), "ConditionLock %s", m_pResource);
    if (waitResult != 0)
    {
        // Check errno for reason for failure
        if (ETIMEDOUT == waitResult)
        {
            result = CamxResultETimeout;
        }
        else
        {
            result = CamxResultEFailed;
        }
    }

    CAMX_TRACE_SYNC_END(CamxLogGroupSync);

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Condition::Signal
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID Condition::Signal()
{
    CAMX_TRACE_MESSAGE_F(CamxLogGroupSync, "Condition::Signal %s", m_pResource);
    pthread_cond_signal(&m_conditionVar);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Condition::Broadcast
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID Condition::Broadcast()
{
    CAMX_TRACE_MESSAGE_F(CamxLogGroupSync, "Condition::Broadcast %s", m_pResource);
    pthread_cond_broadcast(&m_conditionVar);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Gralloc class
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

struct Gralloc1Interface
{
    gralloc1_device_t*                      pGralloc1Device;      ///< Gralloc1 device
};

static Gralloc1Interface g_gralloc1Interface = { 0 };

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Gralloc::Gralloc
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
Gralloc::Gralloc()
{
    //stubbed out
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Gralloc::SetupGralloc1Device
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult Gralloc::SetupGralloc1Device()
{
    CamxResult result = CamxResultEFailed;
    //stubbed out
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Gralloc::SetupGralloc1Interface
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
INT32 Gralloc::SetupGralloc1Interface()
{
    INT32 result = GRALLOC1_ERROR_NONE;
    //stubbed out
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Gralloc::GetInstance
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
Gralloc* Gralloc::GetInstance()
{
    static Gralloc s_GrallocSingleton;

    Gralloc* pGrallocSingleton = NULL;
    //stubbed out
    return pGrallocSingleton;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Gralloc::ConvertCamxFormatToGrallocFormat
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
INT32 Gralloc::ConvertCamxFormatToGrallocFormat(
    CamX::Format    camxFormat,
    UINT32*         pGrallocFormat)
{
    INT32 result = GRALLOC1_ERROR_NONE;
    //stubbed out
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Gralloc::AllocateGrallocBuffer
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult Gralloc::AllocateGrallocBuffer(
    UINT32              width,
    UINT32              height,
    UINT32              grallocFormat,
    BufferHandle*       phAllocatedBuffer,
    UINT32*             pStride,
    UINT64              producerFlags,
    UINT64              consumerFlags)
{
    INT32  result = GRALLOC1_ERROR_NONE;
    //stubbed out
    return (GRALLOC1_ERROR_NONE == result) ? CamxResultSuccess : CamxResultENoMemory;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Gralloc::Destroy
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult Gralloc::Destroy(
    BufferHandle hAllocatedBuffer)
{
    INT32 result = GRALLOC1_ERROR_NONE;
    //stubbed out
    return (GRALLOC1_ERROR_NONE == result) ? CamxResultSuccess : CamxResultENoMemory;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Gralloc::Retain
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult Gralloc::Retain(
    BufferHandle hAllocatedBuffer)
{
    INT32 result = GRALLOC1_ERROR_NONE;
    //stubbed out
    return (GRALLOC1_ERROR_NONE == result) ? CamxResultSuccess : CamxResultENoMemory;
}
CAMX_NAMESPACE_END

#endif // defined (_LINUX)
