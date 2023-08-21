////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2017-2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  depthsensorutil.h
/// @brief CamX CHI node specific utility implementation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#ifndef DEPTHSENSORUTIL_H
#define DEPTHSENSORUTIL_H

#include "camxcdktypes.h"

#if defined (_WIN32)
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/// File system path separator character

#ifndef CONFIG_DIRS_DEFINED
#define CONFIG_DIRS_DEFINED
static const CHAR PathSeparator[]           = "\\";
static const CHAR SharedLibraryExtension[]  = "dll";
static const CHAR VendorLibPath[]           = ".";
static const CHAR CameraComponentLibPath[]  = ".";
static const CHAR FileDumpPath[]            = ".";
#endif // CONFIG_DIRS_DEFINED

#elif defined (_LINUX)
#include <log/log.h>
#include <stdarg.h>
#include <stdio.h>
#include <dlfcn.h>
#include <time.h>
#include <stdlib.h>
#include <string.h>

/// File system path separator character
static const CHAR PathSeparator[] = "/";
static const CHAR SharedLibraryExtension[] = "so";

#if defined(_LP64)
static const CHAR VendorLibPath[]           = "/vendor/lib64";
static const CHAR CameraComponentLibPath[]  = "/vendor/lib64/camera/components";
#else
static const CHAR VendorLibPath[]           = "/vendor/lib";
static const CHAR CameraComponentLibPath[]  = "/vendor/lib/camera/components";
#endif // defined(_LP64)

/// The directory from which to read configuration files
#ifndef CONFIG_DIRS_DEFINED
#define CONFIG_DIRS_DEFINED
#if defined (CAMX_ANDROID_API) && (CAMX_ANDROID_API >= 28) // NOWHINE PR002 <- Win32 definition
static const CHAR ConfigFileDirectory[] = "/data/vendor/camera";
static const CHAR FileDumpPath[]        = "/data/vendor/camera";
#else
static const CHAR ConfigFileDirectory[] = "/data/misc/camera";
static const CHAR FileDumpPath[]        = "/data/misc/camera";
#endif // Android-P or later
#endif // CONFIG_DIRS_DEFINED

#endif // (_WIN32) (_LINUX)

/// Define target type
#if defined(__aarch64__)
#define TARGET_ARM64
#elif defined(__arm__)
#define TARGET_ARM
#endif

/// Dynamic library handle
typedef VOID* CHILIBRARYHANDLE;

/// Constant offset between android elapsedRealTimeNano clock
static const UINT64 NSEC_PER_SEC = 1000000000ull;

/// Default flush repsonse time (in milliseconds)
static const UINT32 DEFAULT_FLUSH_RESPONSE_TIME = 5;

/// Convert from nano secs to micro secs
#define NanoToMicro(x) (x / 1000)

/// Convert from micro secs to nano secs
#define MicroToNano(x) (x * 1000)

/// Convert from nano secs to secs
#define NanoToSec(x) (x / 1000000000)

// Unreferenced macro helper
#define CHI_UNREFERENCED_PARAM(param) (void)param

// NOWHINE FILE CP049: Use static_assert instead of CAMX_STATIC_ASSERT(_MESSAGE)
#define CHI_STATIC_ASSERT(condition) static_assert(condition, #condition)

// NOWHINE FILE CP049: Use assert instead of CAMX_ASSERT(_MESSAGE)
#define CHI_ASSERT(condition) assert(condition)

// Macros for calloc and free
#define CHI_CALLOC ChiNodeUtils::MemCalloc
#define CHI_FREE   ChiNodeUtils::MemFree

typedef VOID* OSLIBRARYHANDLE;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief General CHI Node specific utility class implementation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class DepthSensorUtil
{
public:
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// SNPrintF
    ///
    /// @brief Write formatted data from variable argument list to sized buffer
    ///
    /// @param  pDst     Destination buffer
    /// @param  sizeDst  Size of the destination buffer
    /// @param  pFormat  Format string, printf style
    /// @param  ...      Parameters required by format
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
    static CDK_VISIBILITY_PUBLIC INT VSNPrintF(
        CHAR*       pDst,
        SIZE_T      sizeDst,
        const CHAR* pFormat,
        va_list     argptr);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// LibMap
    ///
    /// @brief  Static function to obtain a handle to the named library (.dll, .so etc.)
    ///         Recommended not use this function, instead use LibMapFullName
    ///
    /// @param  pLibraryName  Name of library to map
    /// @param: pLibraryPath  Path of the lib
    ///
    /// @return Handle to named module or NULL if failure
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static CDK_VISIBILITY_PUBLIC OSLIBRARYHANDLE LibMap(
        const CHAR* pLibraryName,
        const CHAR* pLibraryPath);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// LibMapFullName
    ///
    /// @brief  Static function to obtain a handle to the named library (.dll, .so etc.)
    ///
    /// @param  pLibraryName  Name of library to map
    /// @param: pLibraryPath  Path of the lib
    ///
    /// @return Handle to named module or NULL if failure
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static CDK_VISIBILITY_PUBLIC OSLIBRARYHANDLE LibMapFullName(
        const CHAR* pLibraryName);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// LibUnmap
    ///
    /// @brief  Static function to unmap a library previously mapped by LibMap()
    ///
    /// @param  hLibrary  Handle to library from LibMap()
    ///
    /// @return CDKResultSuccess on success, CDKResultEFailed on failure
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static CDK_VISIBILITY_PUBLIC CDKResult LibUnmap(
        CHILIBRARYHANDLE hLibrary);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// LibGetAddr
    ///
    /// @brief  Static function find an entry point in a library previously loaded by LibMap()
    ///
    /// @param  hLibrary  Handle to library from LibMap()
    /// @param  pProcName Name of entry point to find
    ///
    /// @return Pointer to named entry point in library, or NULL on failure
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static CDK_VISIBILITY_PUBLIC VOID* LibGetAddr(
        CHILIBRARYHANDLE hLibrary,
        const CHAR*      pProcName);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// FloatToHalf
    ///
    /// @brief  Converts Float value to half float
    ///
    /// @param  f      Float Value to be converted to half float
    ///
    /// @return UINT16 The converted value
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static CDK_VISIBILITY_PUBLIC UINT16 FloatToHalf(
        FLOAT f);

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
    static CDK_VISIBILITY_PUBLIC FILE* FOpen(
        const CHAR* pFilename,
        const CHAR* pMode);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// FileNo
    ///
    /// @brief  gets the File Handle for the Descriptor
    ///
    /// @param  pFile File Handle
    ///
    /// @return File Descriptor
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static CDK_VISIBILITY_PUBLIC  INT FileNo(
    FILE* pFile);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// FClose
    ///
    /// @brief  Closes the given file handle
    ///
    /// @param  pFile File handle to close
    ///
    /// @return CDKResultSuccess on success, CDKResultEFailed otherwise
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static CDK_VISIBILITY_PUBLIC CDKResult FClose(
        FILE* pFile);

    static const INT SeekSet = SEEK_SET;    ///< Beginning of file origin for Fseek
    static const INT SeekEnd = SEEK_END;    ///< End of file origin for Fseek
    static const INT SeekCur = SEEK_CUR;    ///< Current position of the file pointer origin for Fseek

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// FSeek
    ///
    /// @brief  Moves the file pointer to a specified location
    ///
    /// @param  pFile    File handle to seek
    /// @param  offset   Number of bytes from origin
    /// @param  origin   Initial position
    ///
    /// @return CDKResultSuccess on success, CDKResultEFailed otherwise
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static CDK_VISIBILITY_PUBLIC CDKResult FSeek(
        FILE*   pFile,
        INT64   offset,
        INT     origin);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// FTell
    ///
    /// @brief  Gets the current position of a file pointer
    ///
    /// @param  pFile File handle to tell
    ///
    /// @return Current file position or -1 on error
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static CDK_VISIBILITY_PUBLIC INT64 FTell(
        FILE* pFile);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// FRead
    ///
    /// @brief  Reads data from a file handle
    ///
    /// @param  pDst     Destination memory
    /// @param  dstSize  Destination memory size
    /// @param  size     Size of objects to read
    /// @param  count    Number of objects to read
    /// @param  pFile    File handle to read from
    ///
    /// @return Number of objects read successfully
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static CDK_VISIBILITY_PUBLIC SIZE_T FRead(
        VOID*   pDst,
        SIZE_T  dstSize,
        SIZE_T  size,
        SIZE_T  count,
        FILE*   pFile);

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
    static CDK_VISIBILITY_PUBLIC SIZE_T FWrite(
        const VOID* pSrc,
        SIZE_T      size,
        SIZE_T      count,
        FILE*       pFile);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// FFlush
    ///
    /// @brief  Flushes a file handle
    ///
    /// @param  pFile File handle to flush
    ///
    /// @return CDKResultSuccess on success, CDKResultEFailed otherwise
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static CDK_VISIBILITY_PUBLIC CDKResult FFlush(
        FILE* pFile);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// MaxUINT32
    ///
    /// @brief  Returns the larger of two UINT32 values
    ///
    /// @param  input1 The first number to compare
    /// @param  input2 The second number to compare
    ///
    /// @return The larger of the two input values
    ///
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static CDK_VISIBILITY_PUBLIC inline UINT32 MaxUINT32(
        UINT32 input1,
        UINT32 input2)
    {
        return ((input1 > input2) ? input1 : input2);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// MaxUINT32
    ///
    /// @brief  Returns the larger of two UINT32 values
    ///
    /// @param  input1 The first number to compare
    /// @param  input2 The second number to compare
    ///
    /// @return The larger of the two input values
    ///
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static CDK_VISIBILITY_PUBLIC inline UINT32 MinUINT32(
        UINT32 input1,
        UINT32 input2)
    {
        return ((input1 < input2) ? input1 : input2);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// AlignGeneric32
    ///
    /// @brief  This function is used to align up any UINT32 value to any alignment value
    ///
    /// @param  operand     value to be aligned
    /// @param  alignment   desired alignment
    ///
    /// @return Value aligned as specified
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static CDK_VISIBILITY_PUBLIC UINT32 AlignGeneric32(
        UINT32 operand,
        UINT   alignment)
    {
        UINT remainder = (operand % alignment);

        return (0 == remainder) ? operand : operand - remainder + alignment;
    }

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
    static CDK_VISIBILITY_PUBLIC inline UINT32 DivideAndCeil(
        UINT32 val,
        UINT32 divisor)
    {
        return (val + (divisor - 1)) / divisor;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// StrTokReentrant
    ///
    /// @brief  Parses a string into a sequence of tokens. On the first call, the string to be parsed is specified in pSrc and
    ///         the value of pContext should be valid, but is ignored. In each subsequent call that should parse the same
    ///         string, pSrc should be NULL and pContext should be unchanged since the previous call. This function is
    ///         reentrant and different strings may be parsed concurrently using sequences of calls to strtok_r() that specify
    ///         different saveptr arguments.
    ///
    /// @param  pSrc        The string to search
    /// @param  pDelimiter  Specifies a set of bytes that delimit the tokens in the parsed string. The caller may specify
    ///                     different delimiter strings in successive calls that parse the same string.
    /// @param  ppContext   A pointer to a CHAR* variable that is used internally in order to maintain context between
    ///                     successive calls that parse the same string.
    ///
    /// @return A pointer to a null-terminated string containing the next token, or NULL if there are no more tokens.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static CDK_VISIBILITY_PUBLIC CHAR* StrTokReentrant(
        CHAR*       pSrc,
        const CHAR* pDelimiter,
        CHAR**      ppContext)
    {
#if defined (_LINUX)
        return strtok_r(pSrc, pDelimiter, ppContext);
#elif defined (_WINDOWS)
        return strtok_s(pSrc, pDelimiter, ppContext);
#else
#error Unsupported target defined
#endif // defined(_LINUX) || defined(_WINDOWS)
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// StrStrip
    ///
    /// @brief  Strip all whitespace characters (space, tab, newline, carriage return, form feed, vertical tab) from a string.
    ///
    /// @param  pDst     The string to write to
    /// @param  pSrc     The string to read from
    /// @param  sizeDst  Size of the destination string
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static CDK_VISIBILITY_PUBLIC VOID StrStrip(
        CHAR*   pDst,
        CHAR*   pSrc,
        SIZE_T  sizeDst);

private:
};

#endif
