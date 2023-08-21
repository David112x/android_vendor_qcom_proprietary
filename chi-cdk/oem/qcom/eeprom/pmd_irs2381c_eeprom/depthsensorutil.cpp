////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2017-2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  depthsensorutil.cpp
/// @brief CHI Node common utility class implementation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "depthsensorutil.h"

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// DepthSensorUtil::VSNPrintF
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
INT DepthSensorUtil::VSNPrintF(
    CHAR*       pDst,
    SIZE_T      sizeDst,
    const CHAR* pFormat,
    va_list     argptr)
{
    INT numCharWritten = 0;

    numCharWritten = vsnprintf(pDst, sizeDst, pFormat, argptr);

    if ((numCharWritten >= static_cast<INT>(sizeDst)) && (sizeDst > 0))
    {
        numCharWritten = -1;
    }
    return numCharWritten;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// DepthSensorUtil::LibMap
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
OSLIBRARYHANDLE DepthSensorUtil::LibMap(
    const CHAR* pLibraryName,
    const CHAR* pLibraryPath)
{
    OSLIBRARYHANDLE hLibrary = NULL;
    CHAR   libFilename[FILENAME_MAX];
    INT    numCharWritten = 0;

    numCharWritten = SNPrintF(libFilename,
                              FILENAME_MAX,
                              "%s%s.%s",
                              pLibraryPath,
                              pLibraryName,
                              SharedLibraryExtension);

#if defined (_LINUX)
    const UINT bind_flags = RTLD_NOW | RTLD_LOCAL;
    hLibrary = dlopen(libFilename, bind_flags);

#elif defined (_WINDOWS)
    hLibrary = LoadLibrary(libFilename);
#else
#error Unsupported target defined
#endif // defined(_LINUX) || defined(_WINDOWS)

    if (NULL == hLibrary)
    {
        // LOG_ERROR(CamxLogGroupChi, , "Error Loading Library: %s", libFilename);
#if defined (_LINUX)
        // LOG_ERROR(CamxLogGroupChi, "dlopen error %s", dlerror());
#endif // _LINUX
    }
    return hLibrary;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// DepthSensorUtil::LibMap
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
OSLIBRARYHANDLE DepthSensorUtil::LibMapFullName(
    const CHAR* pLibraryName)
{
    OSLIBRARYHANDLE hLibrary = NULL;

#if defined (_LINUX)
    const UINT bind_flags = RTLD_NOW | RTLD_LOCAL;
    hLibrary = dlopen(pLibraryName, bind_flags);

#elif defined (_WINDOWS)
    hLibrary = LoadLibrary(pLibraryName);
#else
#error Unsupported target defined
#endif // defined(_LINUX) || defined(_WINDOWS)

    if (NULL == hLibrary)
    {
        // LOG_ERROR(CamxLogGroupChi, "Error Loading Library: %s", pLibraryName);
#if defined (_LINUX)
        // LOG_ERROR(CamxLogGroupChi, "dlopen error %s", dlerror());
#endif // _LINUX
    }

    return hLibrary;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// DepthSensorUtil::LibUnmap
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult DepthSensorUtil::LibUnmap(
    OSLIBRARYHANDLE hLibrary)
{
    CDKResult result = CDKResultSuccess;

    if (NULL != hLibrary)
    {
#if defined (_LINUX)
        if (0 != dlclose(hLibrary))
        {
            result = CDKResultEFailed;
        }
#elif defined (_WINDOWS)
        if (FALSE == FreeLibrary(static_cast<HMODULE>(hLibrary)))
        {
            result = CDKResultEFailed;
        }
#else
#error Unsupported target defined
#endif // defined(_LINUX) || defined(_WINDOWS)
    }
    else
    {
        result = CDKResultEInvalidArg;
    }

    if (CDKResultSuccess != result)
    {
        // LOG_ERROR(CamxLogGroupChi, "Failed to close CHI node Library %d", result);
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// DepthSensorUtil::LibGetAddr
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID* DepthSensorUtil::LibGetAddr(
    OSLIBRARYHANDLE hLibrary,
    const CHAR*      pProcName)
{
    VOID* pProcAddr = NULL;

    if (NULL != hLibrary)
    {
#if defined (_LINUX)
        pProcAddr = dlsym(hLibrary, pProcName);
#elif defined (_WINDOWS)
        pProcAddr = GetProcAddress(static_cast<HMODULE>(hLibrary), pProcName);
#else
#error Unsupported target defined
#endif // defined(_LINUX) || defined(_WINDOWS)
    }

    return pProcAddr;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// DepthSensorUtil::FOpen
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
FILE* DepthSensorUtil::FOpen(
    const CHAR* pFilename,
    const CHAR* pMode)
{
    FILE* pFile;
#if defined (_LINUX)
    pFile = fopen(pFilename, pMode);
#elif defined (_WINDOWS)
    if (0 != fopen_s(&pFile, pFilename, pMode))
    {
        pFile = NULL;
    }
#else
#error Unsupported target defined
#endif // defined(_LINUX) || defined(_WINDOWS)

    return pFile;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// DepthSensorUtil::FileNo
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static INT FileNo(
    FILE* pFile)
{
    return fileno(pFile);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// DepthSensorUtil::FClose
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult DepthSensorUtil::FClose(
    FILE* pFile)
{
    CDKResult result = CDKResultSuccess;
    if (0 != fclose(pFile))
    {
        result = CDKResultEFailed;
    }
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// DepthSensorUtil::FSeek
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult DepthSensorUtil::FSeek(
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

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// DepthSensorUtil::FTell
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
INT64 DepthSensorUtil::FTell(
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

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// DepthSensorUtil::FRead
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
SIZE_T DepthSensorUtil::FRead(
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

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// DepthSensorUtil::FWrite
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
SIZE_T DepthSensorUtil::FWrite(
    const VOID* pSrc,
    SIZE_T      size,
    SIZE_T      count,
    FILE*       pFile)
{
    return fwrite(pSrc, size, count, pFile);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// DepthSensorUtil::FFlush
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult DepthSensorUtil::FFlush(
    FILE* pFile)
{
    CDKResult result = CDKResultSuccess;
    if (0 != fflush(pFile))
    {
        result = CDKResultEFailed;
    }
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// FloatToHalf
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UINT16 DepthSensorUtil::FloatToHalf(
    FLOAT f)
{
    // For IEEE-754 float representation, the sign is in bit 31, biased exponent is in bits 23-30 and mantissa is in bits
    // 0-22. Using hex representation, extract sign, biased exponent and mantissa.
    UINT32 hex_32 = *(reinterpret_cast<UINT32*>(&f));
    UINT32 sign_32 = hex_32 & (0x80000000);
    UINT32 biased_exponent_32 = hex_32 & (0x7F800000);
    UINT32 mantissa_32 = hex_32 & (0x007FFFFF);

    // special case: 0 or denorm
    if (biased_exponent_32 == 0)
    {
        return 0;
    }

    // The exponent is stored in the range 1 .. 254 (0 and 255 have special meanings), and is biased by subtracting 127
    // to get an exponent value in the range -126 .. +127.
    // remove exp bias, adjust mantissa
    INT32 exponent_32 = (static_cast<INT32>(hex_32 >> 23)) - 127;
    mantissa_32 = ((mantissa_32 >> (23 - 10 - 1)) + 1) >> 1;    // shift with rounding to yield 10 MSBs

    if (mantissa_32 & 0x00000400)
    {
        mantissa_32 >>= 1;  // rounding resulted in overflow, so adjust mantissa and exponent
        exponent_32++;
    }

    // Assume exponent fits with 4 bits for exponent with half

    // compose
    UINT16 sign_16 = static_cast<UINT16>(sign_32 >> 16);
    UINT16 biased_exponent_16 = static_cast<UINT16>((exponent_32 + 15) << 10);
    UINT16 mantissa_16 = static_cast<UINT16>(mantissa_32);

    return (sign_16 | biased_exponent_16 | mantissa_16);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// DepthSensorUtil::StrStrip
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID DepthSensorUtil::StrStrip(
    CHAR* pDst,
    CHAR* pSrc,
    SIZE_T sizeDst)
{
    if ((NULL != pSrc) && (NULL != pDst) && (sizeDst > 0))
    {
        CHAR* pTokenString  = NULL;
        CHAR* pContext      = NULL;
        pDst[0]             = '\0';
        pTokenString        = StrTokReentrant(pSrc, " \t\n\r\f\v", &pContext);
        while ((pTokenString != NULL) && (pTokenString[0] != '\0'))
        {
#if defined (_LINUX)

#ifdef ANDROID
            strlcat(pDst, pTokenString, sizeDst);
#else // NOT ANDROID
            g_strlcat(pDst, pTokenString, sizeDst);
#endif // ANDROID

#elif defined (_WINDOWS)

            SIZE_T lengthSrc = strlen(pSrc);
            SIZE_T lengthDst = strlen(pDst);

            strncat_s(pDst, sizeDst, pSrc, _TRUNCATE);
#else
#error Unsupported target defined
#endif // defined(_LINUX) || defined(_WINDOWS)

            pTokenString = StrTokReentrant(NULL, " \t\n\r\f\v", &pContext);
        }
    }
}

