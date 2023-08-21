////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2017-2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  cdkutils.h
/// @brief CDK OS specific utility implementation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef CDKUTILS_H
#define CDKUTILS_H

#include <stdio.h>

#include "camxcdktypes.h"
#include "chxutils.h"

#if defined (_WIN32)
#include <windows.h>
#elif defined (_LINUX)
#include <pthread.h>
#include <time.h>
#endif // defined(_LINUX) || defined(_WINDOWS)

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief CDK OS specific utility class implementation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// NOWHINE CP017,CP018: All static class does not need copy/assignment overrides
class CdkUtils
{
public:
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// FileNo
    ///
    /// @brief  gets the File Handle for the Descriptor
    ///
    /// @param  pFile File Handle
    ///
    /// @return File Descriptor
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static CDK_VISIBILITY_PUBLIC inline INT FileNo(
        FILE* pFile)
    {
#if defined (_LINUX)
        return fileno(pFile);
#endif // _LINUX
#if defined (_WIN32)
        return fileno(pFile);
#endif // _WIN32
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// FTell
    ///
    /// @brief  Gets the current position in the file
    ///
    /// @param  pFile File Handle
    ///
    /// @return INT64 position value
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static CDK_VISIBILITY_PUBLIC inline INT64 FTell(
        FILE* pFile)
    {
#if defined (_LINUX)
        return static_cast<INT64>(ftello(pFile));
#elif defined (_WINDOWS)
        return _ftelli64(pFile);
#else
#error Unsupported target defined
#endif // defined(_LINUX) || defined(_WINDOWS)
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// FileExist
    ///
    /// @brief  check if file exists on file system
    ///
    /// @param  pFileName  the full path of a file
    ///
    /// @return True, if file exists, otherwise FALSE
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static CDK_VISIBILITY_PUBLIC inline BOOL FileExist(
        const CHAR* pFileName)
    {
#if defined (_LINUX)
        return (!access(pFileName, F_OK));
#endif // _LINUX
#if defined (_WIN32)
        return (!_access(pFileName, F_OK));
#endif // _WIN32
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// FRead
    ///
    /// @brief  Reads a block of data from given file handle
    ///
    /// @param  pDst    Pointer to destination memory
    /// @param  dstSize Destination memory size
    /// @param  size    Bytes of each element to read
    /// @param  count   Number of elements to read
    /// @param  pFile   File Handle
    ///
    /// @return File Descriptor
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static CDK_VISIBILITY_PUBLIC inline SIZE_T FRead(
        VOID*   pDst,
        SIZE_T  dstSize,
        SIZE_T  size,
        SIZE_T  count,
        FILE*   pFile)
    {
#if defined (_LINUX)
        CDK_UNUSED_PARAM(dstSize);

        return fread(pDst, size, count, pFile);
#elif defined (_WINDOWS)
        return fread_s(pDst, dstSize, size, count, pFile);
#else
#error Unsupported target defined
#endif // defined(_LINUX) || defined(_WINDOWS)
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// FSeek
    ///
    /// @brief  Seek the file position indicator
    ///
    /// @param  pFile  File Handle
    /// @param  offset Number of bytes to offset from origin
    /// @param  origin Position to use as reference for origin
    ///
    /// @return File Descriptor
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static CDK_VISIBILITY_PUBLIC inline  CDKResult FSeek(
        FILE*   pFile,
        INT64   offset,
        INT     origin)
    {
        CDKResult result = CDKResultSuccess;
#if defined (_LINUX)
        if (0 != fseeko(pFile, static_cast<off_t>(offset), origin))
        {
            result = CDKResultEFailed;
        }
#elif defined (_WINDOWS)
        if (0 != _fseeki64(pFile, offset, origin))
        {
            result = CDKResultEFailed;
        }
#else
#error Unsupported target defined
#endif // defined(_LINUX) || defined(_WINDOWS)

        return result;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// FOpen
    ///
    /// @brief  Opens a file on the filesystem
    ///
    /// @param  pFilename File to open
    /// @param  pMode     File access mode string
    ///
    /// @return File handle or NULL on failure
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static CDK_VISIBILITY_PUBLIC inline FILE* FOpen(
        const CHAR* pFilename,
        const CHAR* pMode)
    {
#if defined (_LINUX)
        return fopen(pFilename, pMode);
#endif // _LINUX
#if defined (_WIN32)
        FILE* pFile;
        if (0 != fopen_s(&pFile, pFilename, pMode))
        {
            pFile = NULL;
        }
        return pFile;
#endif // _WIN32
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// FClose
    ///
    /// @brief  Closes the given file handle
    ///
    /// @param  pFile File handle to close
    ///
    /// @return CDKResultSuccess on success, CamxResultEFailed otherwise
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static CDK_VISIBILITY_PUBLIC inline CDKResult FClose(
        FILE* pFile)
    {
        CDKResult result = CDKResultSuccess;

        if (0 != fclose(pFile))
        {
            result = CDKResultEFailed;
        }

        return result;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// FWrite
    ///
    /// @brief  Writes data to a file handle
    ///
    /// @param  pSrc     Source memory
    /// @param  size     Size of objects to write
    /// @param  count    Number of objects to write
    /// @param  pFile    File handle to write to
    ///
    /// @return Number of objects written successfully
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static CDK_VISIBILITY_PUBLIC inline SIZE_T FWrite(
        const VOID* pSrc,
        SIZE_T      size,
        SIZE_T      count,
        FILE*       pFile)
    {
        return fwrite(pSrc, size, count, pFile);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// SNPrintF
    ///
    /// @brief Write formatted data from variable argument list to sized buffer
    ///
    /// @param pDst     Destination buffer
    /// @param sizeDst  Size of the destination buffer
    /// @param pFormat  Format string, printf style
    /// @param ...      Parameters required by format
    ///
    /// @return Number of characters written
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static CDK_VISIBILITY_PUBLIC inline INT SNPrintF(
        CHAR*       pDst,
        SIZE_T      sizeDst,
        const CHAR* pFormat,
        ...)
    {
        INT     numCharWritten;
        va_list args;

        va_start(args, pFormat);
        numCharWritten = VSNPrintF(pDst, sizeDst, pFormat, args);
        va_end(args);

        return numCharWritten;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// VSNPrintF
    ///
    /// @brief  Write formatted data from variable argument list to sized buffer
    ///
    /// @param  pDst     Destination buffer
    /// @param  sizeDst  Size of the destination buffer
    /// @param  pFormat  Format string, printf style
    /// @param  argptr   Parameters required by format
    ///
    /// @return Number of characters written
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static INT CDK_VISIBILITY_PUBLIC inline VSNPrintF(
        CHAR*       pDst,
        SIZE_T      sizeDst,
        const CHAR* pFormat,
        va_list     argptr)
    {
        INT numCharWritten = 0;
#if defined (_LINUX)
        numCharWritten = vsnprintf(pDst, sizeDst, pFormat, argptr);
#endif // _LINUX
#if defined (_WIN32)
        numCharWritten = vsnprintf_s(pDst, sizeDst, _TRUNCATE, pFormat, argptr);
#endif // _WIN32
        CHX_ASSERT(numCharWritten >= 0);
        if ((numCharWritten >= static_cast<INT>(sizeDst)) && (sizeDst > 0))
        {
            // Message length exceeds the buffer limit size
            CHX_ASSERT(FALSE);
            numCharWritten = -1;
        }

        return numCharWritten;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// StrLCpy
    ///
    /// @brief  Copy string from source pointer to destination
    ///
    /// @param  pDst     Destination buffer
    /// @param  pSrc     Source buffer
    /// @param  sizeDst  Size of the destination buffer
    ///
    /// @return Number of characters written
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static SIZE_T CDK_VISIBILITY_PUBLIC inline StrLCpy(
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

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// StrLCat
    ///
    /// @brief  Appends a copy of the source string to the destination strings
    ///
    /// @param  pDst     Destination buffer
    /// @param  pSrc     Source buffer
    /// @param  sizeDst  Size of the destination buffer
    ///
    /// @return Number of characters written
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static SIZE_T CDK_VISIBILITY_PUBLIC inline StrLCat(
        CHAR*       pDst,
        const CHAR* pSrc,
        SIZE_T      sizeDst)
    {
        SIZE_T lengthSrc = strlen(pSrc);
        SIZE_T lengthDst = strlen(pDst);

#ifdef ANDROID
        strlcat(pDst, pSrc, sizeDst);
#else // NOT ANDROID
        g_strlcat(pDst, pSrc, sizeDst);
#endif // ANDROID

        return lengthSrc + lengthDst;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// StrLen
    ///
    /// @brief  Get the length of the stream
    ///
    /// @param  pStr     The pointer of string
    ///
    /// @return Length of pStr
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static SIZE_T CDK_VISIBILITY_PUBLIC inline StrLen(
        CHAR*       pStr)
    {
        return strlen(pStr);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// StrStr
    ///
    /// @brief  Returns a pointer to the first occurrence of pString2 in pString1, or a NULL pointer if pString2 is not part of
    ///         pString1. The matching process stops at the first terminating null-characters and it is not considered in the
    ///         comparison.
    ///
    /// @param  pString1    The string to search
    /// @param  pString2    The string containing the sequence of characters to match
    ///
    /// @return A pointer to the first occurrence in pString1 of the entire sequence of characters specified in pString2, or a
    ///         NULL pointer if the sequence is not present in pString1.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static CDK_VISIBILITY_PUBLIC inline const CHAR*  StrStr(
        const CHAR* pString1,
        const CHAR* pString2)
    {
        return strstr(pString1, pString2);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// StrCmp
    ///
    /// @brief  Compares two strings.
    ///
    /// @param  pString1 First string to compare
    /// @param  pString2 Second string to compare
    ///
    /// @return Returns a signed integer indicating the relationship between the input strings:
    ///         <0  the first character that does not match has a lower value in string1 than in string
    ///         0   the contents of both strings are equal
    ///         >0  the first character that does not match has a greater value in string1 than in string2
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static CDK_VISIBILITY_PUBLIC inline INT StrCmp(
        const CHAR* pString1,
        const CHAR* pString2)
    {
        return strcmp(pString1, pString2);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetNanosecondsSinceEpoch
    ///
    /// @brief  Get the number of nanoseconds that have passed between now and Jan 1 00:00:00 1970
    ///
    /// @return a UINT64 value representing nanoseconds since the 1970 epoch
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static CDK_VISIBILITY_PUBLIC inline UINT64 GetNanosecondsSinceEpoch()
    {
#if defined (ANDROID)
        constexpr UINT64 NanoSecondsPerSecond = 1000000000ULL;
        struct timespec  duration            = { 0 };
        clock_gettime(CLOCK_REALTIME, &duration);
        return (static_cast<UINT64>(duration.tv_sec) * NanoSecondsPerSecond) + static_cast<UINT>(duration.tv_nsec);
#else
#error Unsupported operation.
#endif
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetDateTime
    ///
    /// @brief  Get the date and time
    ///
    /// @param  pDateTime Chi Date time structure to be filled
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static CDK_VISIBILITY_PUBLIC inline VOID GetDateTime(
        CHIDateTime* pDateTime)
    {
        if (NULL != pDateTime)
        {
#if defined (ANDROID)
            struct timeval timeValue;
            struct tm*     pTimeInfo = NULL;

            CDKResult result = gettimeofday(&timeValue, NULL);
            if (0 == result)
            {
                pTimeInfo = localtime(&timeValue.tv_sec);
                if (NULL != pTimeInfo)
                {
                    pDateTime->seconds                = static_cast<UINT32>(pTimeInfo->tm_sec);
                    pDateTime->microseconds           = static_cast<UINT32>(timeValue.tv_usec);
                    pDateTime->minutes                = static_cast<UINT32>(pTimeInfo->tm_min);
                    pDateTime->hours                  = static_cast<UINT32>(pTimeInfo->tm_hour);
                    pDateTime->dayOfMonth             = static_cast<UINT32>(pTimeInfo->tm_mday);
                    pDateTime->month                  = static_cast<UINT32>(pTimeInfo->tm_mon);
                    pDateTime->year                   = static_cast<UINT32>(pTimeInfo->tm_year);
                    pDateTime->weekday                = static_cast<UINT32>(pTimeInfo->tm_wday);
                    pDateTime->yearday                = static_cast<UINT32>(pTimeInfo->tm_yday);
                    pDateTime->dayLightSavingTimeFlag = static_cast<UINT32>(pTimeInfo->tm_isdst);
                }
            }
#else
#error Unsupported operation.
#endif // defined(_LINUX)
        }
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetThreadID
    ///
    /// @brief  Get the current thread ID
    ///
    /// @return the current thread id
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static CDK_VISIBILITY_PUBLIC inline UINT GetThreadID()
    {
#if defined (ANDROID)
        return static_cast<UINT>(pthread_self());
#else
#error Unsupported operation.
#endif
    }

    /// @brief File Name Token
    enum class FileNameToken
    {
        Com,       ///< com
        Vendor,    ///< vendor name
        Category,  ///< category name
        Module,    ///< module name
        Target,    ///< Target name
        Extension, ///< File Extension
        Max        ///< Max tokens for file name
    };

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetFilesFromPath
    ///
    /// @brief  Reads the directory and returns the binary file names
    ///
    /// @param  pFileSearchPath   Path to the directory
    /// @param  maxFileNameLength Maximum length of file name
    /// @param  pFileNames        Pointer to store an array of full file names at the return of function
    /// @param  pVendorName       Vendor name
    /// @param  pCategoryName     Category of binary file
    /// @param  pModuleName       Name of the module
    /// @param  pExtension        Pointer to file extension
    ///
    /// @return Number of binary files in the directory
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static CDK_VISIBILITY_PUBLIC inline UINT16 GetFilesFromPath(
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
                StrLCpy(fileName, pDirent->d_name, maxFileNameLength);
                tokenCount     = 0;
                pToken         = strtok_r(fileName, ".", &pContext);
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
                    pToken = strtok_r(NULL, ".", &pContext);
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

                        if (0 != StrCmp(pToken, "*"))
                        {
                            if (0 == StrCmp(pToken, pTokens[index]))
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
                        SNPrintF(pFileNames + (fileCount * maxFileNameLength),
                                          maxFileNameLength,
                                          "%s%s%s",
                                          pFileSearchPath,
                                          "/",
                                          pDirent->d_name);
                        fileCount++;

                    }
                }
            }
            closedir(pDirectory);
        }

        return fileCount;
    }

private:
    CdkUtils() = default;
};

#endif // CDKUTILS_H
