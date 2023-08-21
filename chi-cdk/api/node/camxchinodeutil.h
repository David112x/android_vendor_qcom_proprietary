////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2017-2020 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  camxchinodeutil.h
/// @brief CamX CHI node specific utility implementation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#ifndef CAMXCHINODEUTILS_H
#define CAMXCHINODEUTILS_H

#include "camxcdktypes.h"
#include "chinode.h"
#include "chxdebugprint.h"

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

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief General CHI Node specific utility class implementation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class ChiNodeUtils
{
public:
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetMetaData
    ///
    /// @brief  Get the needed metadata from camera system
    ///
    /// @param requestId         The request id for current request
    /// @param tagId             The id of tag to get from camera system
    /// @param metadataType      Metadata type Static/Dynamic
    /// @param pChiNodeInterface Instance of the chi node interface
    /// @param hChiSession       The Chi session handle
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static CDK_VISIBILITY_PUBLIC VOID* GetMetaData(
        UINT64            requestId,
        UINT32            tagId,
        CHIMETADATATYPE   metadataType,
        ChiNodeInterface* pChiNodeInterface,
        CHIHANDLE         hChiSession);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetMulticamDynamicMetaByCamId
    ///
    /// @brief  Get the needed dynamic metadata for a camera id.
    ///
    /// @param requestId         The request id for current request
    /// @param tagId             The id of tag to get from camera system
    /// @param metadataType      Metadata type Static/Dynamic
    /// @param pChiNodeInterface Instance of the chi node interface
    /// @param hChiSession       The Chi session handle
    /// @param cameraId          CameraId associated with the metadata from which the list of tags must be fetched.
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static CDK_VISIBILITY_PUBLIC VOID* GetMulticamDynamicMetaByCamId(
        UINT64            requestId,
        UINT32            tagId,
        CHIMETADATATYPE   metadataType,
        ChiNodeInterface* pChiNodeInterface,
        CHIHANDLE         hChiSession,
        UINT32            cameraId);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetStaticMetaData
    ///
    /// @brief  Get the needed static metadata from camera system
    ///
    /// @param cameraId          The camera id for the current request
    /// @param tagId             The id of tag to get from camera system
    /// @param pChiNodeInterface Instance of the chi node interface
    /// @param hChiSession       The Chi session handle
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static CDK_VISIBILITY_PUBLIC VOID* GetStaticMetaData(
        UINT32            cameraId,
        UINT32            tagId,
        ChiNodeInterface* pChiNodeInterface,
        CHIHANDLE         hChiSession);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetPSMetaData
    ///
    /// @brief  Get the needed port-specific metadata from camera system
    ///
    /// @param requestId         The request id for current request
    /// @param tagId             The id of tag to get from camera system
    /// @param inputPortId       Input port Id corresponding to the request
    /// @param pChiNodeInterface Instance of the chi node interface
    /// @param hChiSession       The Chi session handle
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static CDK_VISIBILITY_PUBLIC VOID* GetPSMetaData(
        UINT64            requestId,
        UINT32            tagId,
        UINT32            inputPortId,
        ChiNodeInterface* pChiNodeInterface,
        CHIHANDLE         hChiSession);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// SetNodeInterface
    ///
    /// @brief  Implementation of generic method to handle PFCHINODESETNODEINTERFACE defined in chinode.h for all chi nodes
    ///
    /// @param  pNodeInterface             Pointer to the chinode interface structure passed as part of the
    ///                                    PFCHINODESETNODEINTERFACE
    /// @param  componentName              Name of the Node instance
    /// @param  pNodeInstChiNodeInterface  Pointer to the chinode interface associated with the instance of the calling Chi Node
    /// @param  pNodeVendorTagBase         Pointer to the chi node's vendor tag base
    ///
    /// @return CDKResultSuccess if success or appropriate error code.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static CDK_VISIBILITY_PUBLIC CDKResult SetNodeInterface(
        ChiNodeInterface* pNodeInterface,
        const CHAR*       pComponentName,
        ChiNodeInterface* pNodeInstChiNodeInterface,
        UINT32*           pNodeVendorTagBase);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// QueryVendorTag
    ///
    /// @brief  Implementation of PFNCHIQUERYVENDORTAG defined in chinode.h
    ///
    /// @param  pQueryVendorTag    Pointer to a structure that returns the exported vendor tag
    /// @param  pNodeVendorTagInfo Vendor Tag Info of the Node
    ///
    /// @return CDKResultSuccess if success or appropriate error code.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static CDK_VISIBILITY_PUBLIC CDKResult QueryVendorTag(
        CHIQUERYVENDORTAG* pQueryVendorTag,
        ChiVendorTagInfo*  pNodeVendorTagInfo);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// DefaultBufferNegotiation
    ///
    /// @brief  Implement the default buffer negotiation logic.
    ///
    /// @param  pQueryBufferInfo  Pointer to a structure that has the information of required output/input buffer
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static CDK_VISIBILITY_PUBLIC VOID DefaultBufferNegotiation(
        CHINODEQUERYBUFFERINFO* pQueryBufferInfo);

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
    static CDK_VISIBILITY_PUBLIC CHILIBRARYHANDLE LibMap(
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
    static CDK_VISIBILITY_PUBLIC CHILIBRARYHANDLE LibMapFullName(
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
    /// GetVendorTagBase
    ///
    /// @brief  Static function to get the vendor tag base from the section and tag name
    ///
    /// @param  pSectionName      Defined Vendor tag section name
    /// @param  pTagname          Defined Vendor tag name
    /// @param  pChiNodeInterface Pointer to the chinode interface associated with the instance of the calling Chi Node
    /// @param  pVendorTagBase    Pointer to vendor tag base to be filled in
    ///
    /// @return CDKResultSuccess on success, CDKResultEFailed on failure
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static CDK_VISIBILITY_PUBLIC inline CDKResult GetVendorTagBase(
        const CHAR*           pSectionName,
        const CHAR*           pTagname,
        ChiNodeInterface*     pChiNodeInterface,
        CHIVENDORTAGBASEINFO* pVendorTagBase)
    {
        CDKResult result = CDKResultSuccess;

        pVendorTagBase->size = sizeof(CHIVENDORTAGBASEINFO);
        pVendorTagBase->pComponentName = pSectionName;
        pVendorTagBase->pTagName = pTagname;
        result = pChiNodeInterface->pGetVendorTagBase(pVendorTagBase);

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
    /// MinUINT32
    ///
    /// @brief  Returns the smallest of two UINT32 values
    ///
    /// @param  input1 The first number to compare
    /// @param  input2 The second number to compare
    ///
    /// @return The smallest of the two input values
    ///
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static CDK_VISIBILITY_PUBLIC inline UINT32 MinUINT32(
        UINT32 input1,
        UINT32 input2)
    {
        return ((input1 < input2) ? input1 : input2);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// MaxFLOAT
    ///
    /// @brief  Returns the larger of two FLOAT values
    ///
    /// @param  input1 The first number to compare
    /// @param  input2 The second number to compare
    ///
    /// @return The larger of the two input values
    ///
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static CDK_VISIBILITY_PUBLIC inline FLOAT MaxFLOAT(
        FLOAT input1,
        FLOAT input2)
    {
        return ((input1 > input2) ? input1 : input2);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// MinFLOAT
    ///
    /// @brief  Returns the smallest of two FLOAT values
    ///
    /// @param  input1 The first number to compare
    /// @param  input2 The second number to compare
    ///
    /// @return The smallest of the two input values
    ///
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static CDK_VISIBILITY_PUBLIC inline FLOAT MinFLOAT(
        FLOAT input1,
        FLOAT input2)
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
    /// GetDateTime
    ///
    /// @brief  Get Date Time
    ///
    /// @param  pDateTime Chi Date time structure to be filled
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static CDK_VISIBILITY_PUBLIC VOID GetDateTime(
        CHIDateTime* pDateTime);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetRequestIdOffset
    ///
    /// @brief  Compute the offset from the lastFlushedRequestId and the current request id
    ///
    /// @return The Offset from the lastFlushedRequestId
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static CDK_VISIBILITY_PUBLIC UINT64 GetRequestIdOffset(
        UINT64 requestId,
        UINT64 lastFlushedRequestId)
    {
        return (requestId > lastFlushedRequestId) ? (requestId - lastFlushedRequestId) : requestId;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// MemCalloc
    ///
    /// @brief  Memory calloc util for chi nodes
    ///
    /// @param  count Number of elements to allocate
    /// @param  size  Size of element to allocate
    ///
    /// @return Pointer to allocated memory
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static CDK_VISIBILITY_PUBLIC VOID* MemCalloc(
        SIZE_T count,
        SIZE_T size)
    {
        return calloc(count, size);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// MemFree
    ///
    /// @brief  Memory free util for chi nodes
    ///
    /// @param  ptr Pointer to allocated memory
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static CDK_VISIBILITY_PUBLIC VOID MemFree(
        VOID* ptr)
    {
        free(ptr);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// Memcpy
    ///
    /// @brief  Wraps the standard memcpy function; copies "numBytes" from source to destination
    ///
    /// @param  pDst       pointer to destination of copy
    /// @param  pSrc       pointer to source of copy
    /// @param  numBytes   size of data pointed at by pSrc
    ///
    /// @return Dest Pointer
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static CDK_VISIBILITY_PUBLIC VOID* Memcpy(
        VOID*       pDst,
        const VOID* pSrc,
        SIZE_T      numBytes)
    {
        return memcpy(pDst, pSrc, numBytes);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// Memset
    ///
    /// @brief  Wraps the standard memset function; sets "numBytes" at the destination pointer to the specified pattern
    ///
    /// @param  pDst        pointer to destination of copy
    /// @param  setVal      character valueto set
    /// @param  numBytes    number of characters (bytes) to copy from pSrc to pDst
    ///
    /// @return Destination pointer
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static CDK_VISIBILITY_PUBLIC VOID* Memset(
        VOID*    pDst,
        INT      setVal,
        SIZE_T   numBytes)
    {
        return memset(pDst, setVal, numBytes);
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

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetAlignment
    ///
    /// @brief  Get stride and scanline alignment for a given format and resolution
    ///
    /// @param  width            Width of a buffer in pixel
    /// @param  height           Height of a buffer in pixel
    /// @param  planeId          Plane index
    /// @param  format           Format of the buffer
    /// @param  pStrideAlignment Pointer to storage for returned stride alignment
    /// @param  pStrideAlignment Pointer to storage for returned scanline alignment
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static CDK_VISIBILITY_PUBLIC VOID GetAlignment(
        INT          width,
        INT          height,
        UINT         planeId,
        ChiFormat    format,
        INT*         pStrideAlignment,
        INT*         pScanlineAlignment);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetVendorTagBase
    ///
    /// @brief  Get the needed metadata vendor tag base from camera system
    ///
    /// @param pComponentName    The name of the metadata component
    /// @param pTagName          The name of the metadata tag
    /// @param pChiNodeInterface Instance of the chi node interface
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static CDK_VISIBILITY_PUBLIC UINT32 GetVendorTagBase(
        const CHAR*       pComponentName,
        const CHAR*       pTagName,
        ChiNodeInterface* pChiNodeInterface);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// SleepMicroseconds
    ///
    /// @brief  Put the current thread to sleep
    ///
    /// @param  microseconds  Sleep duration in microsecond
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static CDK_VISIBILITY_PUBLIC VOID SleepMicroseconds(
        UINT microseconds);
private:

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// CreateMetadataInfo
    ///
    /// @brief  create metadatainfo object
    ///
    /// @param requestId         The request id for current request
    /// @param pTagId            The id of tag to get from camera system
    /// @param metadataType      Metadata type Static/Dynamic
    /// @param hChiSession       The Chi session handle
    /// @param pTagData          Chi tag data structure
    /// @param pOutMetadataInfo  Output MetadataInfo
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static VOID CreateMetadataInfo(
    UINT64            requestId,
    UINT32*           pTagId,
    CHIMETADATATYPE   metadataType,
    CHIHANDLE         hChiSession,
    CHITAGDATA*       pTagData,
    CHIMETADATAINFO*  pOutMetadataInfo);
};

#endif
