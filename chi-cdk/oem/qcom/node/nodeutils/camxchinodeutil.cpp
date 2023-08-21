////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2017-2020 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  camxchinodeutil.cpp
/// @brief CHI Node common utility class implementation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "camxchinodeutil.h"
#if defined (_LINUX)
#include <media/msm_media_info.h>
#endif // defined(_LINUX)

#undef LOG_TAG
#define LOG_TAG "CHINODEUTIL"

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ChiNodeUtils::GetVendorTagBase
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UINT32 ChiNodeUtils::GetVendorTagBase(
    const CHAR*       pComponentName,
    const CHAR*       pTagName,
    ChiNodeInterface* pChiNodeInterface)
{
    CHIVENDORTAGBASEINFO vendorTagBaseInfo;
    UINT                 result      = 0;
    CDKResult            getResult;

    if ((NULL == pComponentName) || (NULL == pChiNodeInterface))
    {
        getResult = CDKResultEInvalidArg;
    }
    else
    {
        vendorTagBaseInfo.size           = sizeof(CHIVENDORTAGBASEINFO);
        vendorTagBaseInfo.pComponentName = pComponentName;
        vendorTagBaseInfo.pTagName       = pTagName;
        getResult                        = pChiNodeInterface->pGetVendorTagBase(&vendorTagBaseInfo);
    }

    if (CDKResultSuccess == getResult)
    {
        result = vendorTagBaseInfo.vendorTagBase;
    }
    else
    {
        LOG_ERROR(CamxLogGroupChi,
                  "Error: %u during GetVendorTagBase for: %s %s Interface: %p",
                  getResult,
                  (NULL == pComponentName) ? "Invalid Component" : pComponentName,
                  (NULL == pTagName)       ? "NULL"              : pTagName,
                  pChiNodeInterface);
    }
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ChiNodeUtils::GetMetaData
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID* ChiNodeUtils::GetMetaData(
    UINT64            requestId,
    UINT32            tagId,
    CHIMETADATATYPE   metadataType,
    ChiNodeInterface* pChiNodeInterface,
    CHIHANDLE         hChiSession)
{
    CHIMETADATAINFO metadataInfo = { 0 };
    CHITAGDATA      tagData      = { 0 };

    CreateMetadataInfo(requestId, &tagId, metadataType, hChiSession, &tagData, &metadataInfo);
    pChiNodeInterface->pGetMetadata(&metadataInfo);
    return metadataInfo.pTagData->pData;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ChiNodeUtils::GetMulticamDynamicMetaByCamId
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID* ChiNodeUtils::GetMulticamDynamicMetaByCamId(
    UINT64            requestId,
    UINT32            tagId,
    CHIMETADATATYPE   metadataType,
    ChiNodeInterface* pChiNodeInterface,
    CHIHANDLE         hChiSession,
    UINT32            cameraId)
{
    CHIMETADATAINFO metadataInfo = { 0 };
    CHITAGDATA      tagData      = { 0 };

    CreateMetadataInfo(requestId, &tagId, metadataType, hChiSession, &tagData, &metadataInfo);
    pChiNodeInterface->pGetMultiCamDynamicMetaByCamId(&metadataInfo, cameraId);
    return metadataInfo.pTagData->pData;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ChiNodeUtils::GetPSMetaData
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID* ChiNodeUtils::GetPSMetaData(
    UINT64            requestId,
    UINT32            tagId,
    UINT32            inputPortId,
    ChiNodeInterface* pChiNodeInterface,
    CHIHANDLE         hChiSession)
{
    CHIPSMETADATA   metadataInfo = { 0 };
    CHITAGDATA      tagData      = { 0 };

    tagData.requestId = requestId;
    tagData.offset    = 0;
    tagData.negate    = FALSE;

    metadataInfo.size         = sizeof(CHIPSMETADATA);
    metadataInfo.chiSession   = hChiSession;
    metadataInfo.portId       = inputPortId;
    metadataInfo.tagCount     = 1;
    metadataInfo.portId       = inputPortId;
    metadataInfo.pTagList     = &tagId;
    metadataInfo.pTagData     = &tagData;

    pChiNodeInterface->pGetPSMetadata(&metadataInfo);
    return metadataInfo.pTagData->pData;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ChiNodeUtils::CreateMetadataInfo
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID ChiNodeUtils::CreateMetadataInfo(
    UINT64            requestId,
    UINT32*           pTagId,
    CHIMETADATATYPE   metadataType,
    CHIHANDLE         hChiSession,
    CHITAGDATA*       pTagData,
    CHIMETADATAINFO*  pOutMetadataInfo)
{
        const UINT32    tagSize      = 1;

        pTagData->requestId = requestId;
        pTagData->offset    = 0;
        pTagData->negate    = FALSE;

        pOutMetadataInfo->size         = sizeof(CHIMETADATAINFO);
        pOutMetadataInfo->chiSession   = hChiSession;
        pOutMetadataInfo->metadataType = metadataType;
        pOutMetadataInfo->tagNum       = tagSize;
        pOutMetadataInfo->pTagList     = pTagId;
        pOutMetadataInfo->pTagData     = pTagData;
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ChiNodeUtils::GetStaticMetaData
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID* ChiNodeUtils::GetStaticMetaData(
    UINT32            cameraId,
    UINT32            tagId,
    ChiNodeInterface* pChiNodeInterface,
    CHIHANDLE         hChiSession)
{
    CHIMETADATAINFO metadataInfo = { 0 };
    const UINT32    tagSize      = 1;
    CHITAGDATA      tagData      = { 0 };
    UINT32          tagList      = tagId;

    tagData.requestId = 0;

    metadataInfo.size           = sizeof(CHIMETADATAINFO);
    metadataInfo.chiSession     = hChiSession;
    metadataInfo.metadataType   = ChiMetadataStatic;
    metadataInfo.tagNum         = tagSize;
    metadataInfo.pTagList       = &tagList;
    metadataInfo.pTagData       = &tagData;

    pChiNodeInterface->pGetStaticMetadata(&metadataInfo, cameraId);
    return tagData.pData;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ChiNodeUtils::SetNodeInterface
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ChiNodeUtils::SetNodeInterface(
    ChiNodeInterface* pNodeInterface,
    const CHAR*       pComponentName,
    ChiNodeInterface* pNodeInstChiNodeInterface,
    UINT32*           pNodeVendorTagBase)
{
    CDKResult result = CDKResultSuccess;

    if (NULL == pNodeInterface)
    {
        result = CDKResultEInvalidPointer;
        LOG_ERROR(CamxLogGroupChi, "Invalid argument: pNodeInterface is NULL");
    }

    if (CDKResultSuccess == result)
    {
        if (pNodeInterface->size >= sizeof(ChiNodeInterface))
        {
            pNodeInstChiNodeInterface->pGetMetadata                     = pNodeInterface->pGetMetadata;
            pNodeInstChiNodeInterface->pGetMultiCamDynamicMetaByCamId   = pNodeInterface->pGetMultiCamDynamicMetaByCamId;
            pNodeInstChiNodeInterface->pSetMetadata                     = pNodeInterface->pSetMetadata;
            pNodeInstChiNodeInterface->pGetVendorTagBase                = pNodeInterface->pGetVendorTagBase;
            pNodeInstChiNodeInterface->pProcessRequestDone              = pNodeInterface->pProcessRequestDone;
            pNodeInstChiNodeInterface->pCreateFence                     = pNodeInterface->pCreateFence;
            pNodeInstChiNodeInterface->pWaitFenceAsync                  = pNodeInterface->pWaitFenceAsync;
            pNodeInstChiNodeInterface->pSignalFence                     = pNodeInterface->pSignalFence;
            pNodeInstChiNodeInterface->pGetFenceStatus                  = pNodeInterface->pGetFenceStatus;
            pNodeInstChiNodeInterface->pReleaseFence                    = pNodeInterface->pReleaseFence;
            pNodeInstChiNodeInterface->pGetDataSource                   = pNodeInterface->pGetDataSource;
            pNodeInstChiNodeInterface->pPutDataSource                   = pNodeInterface->pPutDataSource;
            pNodeInstChiNodeInterface->pGetData                         = pNodeInterface->pGetData;
            pNodeInstChiNodeInterface->pPutData                         = pNodeInterface->pPutData;
            pNodeInstChiNodeInterface->pCacheOps                        = pNodeInterface->pCacheOps;
            pNodeInstChiNodeInterface->pProcessMetadataDone             = pNodeInterface->pProcessMetadataDone;
            pNodeInstChiNodeInterface->pGetFlushInfo                    = pNodeInterface->pGetFlushInfo;
            pNodeInstChiNodeInterface->pGetStaticMetadata               = pNodeInterface->pGetStaticMetadata;
            pNodeInstChiNodeInterface->pGetSupportedPSMetadataList      = pNodeInterface->pGetSupportedPSMetadataList;
            pNodeInstChiNodeInterface->pGetPSMetadata                   = pNodeInterface->pGetPSMetadata;
            pNodeInstChiNodeInterface->pSetPSMetadata                   = pNodeInterface->pSetPSMetadata;
            pNodeInstChiNodeInterface->pPublishPSMetadata               = pNodeInterface->pPublishPSMetadata;

            if (NULL != pNodeVendorTagBase)
            {
                // get the vendor tag base
                CHIVENDORTAGBASEINFO vendorTagBase = { 0 };
                vendorTagBase.size = sizeof(CHIVENDORTAGBASEINFO);
                vendorTagBase.pComponentName = pComponentName;

                result = pNodeInstChiNodeInterface->pGetVendorTagBase(&vendorTagBase);
                if (CDKResultSuccess == result)
                {
                    *pNodeVendorTagBase = vendorTagBase.vendorTagBase;
                }
            }
        }
        else
        {
            LOG_ERROR(CamxLogGroupChi, "CHINODEPROCESSREQUESTINFO is smaller than expected");
            result = CDKResultEFailed;
        }
    }
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ChiNodeUtils::QueryVendorTag
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ChiNodeUtils::QueryVendorTag(
    CHIQUERYVENDORTAG* pQueryVendorTag,
    ChiVendorTagInfo*  pNodeVendorTagInfo)
{
    CDKResult result = CDKResultSuccess;
    if (NULL == pQueryVendorTag )
    {
        result = CDKResultEInvalidPointer;
        LOG_ERROR(CamxLogGroupChi, "Invalide argument: pQueryVendorTag is NULL");
    }

    if (NULL ==  pNodeVendorTagInfo)
    {
        result = CDKResultEInvalidPointer;
        LOG_ERROR(CamxLogGroupChi, "Invalide argument: pNodeVendorTagInfo is NULL");
    }

    if (CDKResultSuccess == result)
    {
        if (pQueryVendorTag->size >= sizeof(CHIQUERYVENDORTAG))
        {
            pQueryVendorTag->pVendorTagInfo = pNodeVendorTagInfo;
        }
        else
        {
            LOG_ERROR(CamxLogGroupChi, "pQueryVendorTag is smaller than expected");
            result = CDKResultEFailed;
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ChiNodeUtils::DefaultBufferNegotiation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID ChiNodeUtils::DefaultBufferNegotiation(
    CHINODEQUERYBUFFERINFO* pQueryBufferInfo)
{
    UINT      optimalInputWidth = 0;
    UINT      optimalInputHeight = 0;
    UINT32    minInputWidth = 0;
    UINT32    minInputHeight = 0;
    UINT32    maxInputWidth = 0xffff;
    UINT32    maxInputHeight = 0xffff;
    UINT      perOutputPortOptimalWidth = 0;
    UINT      perOutputPortOptimalHeight = 0;
    UINT32    perOutputPortMinWidth = 0;
    UINT32    perOutputPortMinHeight = 0;
    UINT32    perOutputPortMaxWidth = 0xffff;
    UINT32    perOutputPortMaxHeight = 0xffff;
    BOOL      isSinkNoBuffer         = FALSE;

    if (NULL != pQueryBufferInfo)
    {
        for (UINT outputIndex = 0; outputIndex < pQueryBufferInfo->numOutputPorts; outputIndex++)
        {
            ChiOutputPortQueryBufferInfo* pOutputPort = &pQueryBufferInfo->pOutputPortQueryInfo[outputIndex];

            perOutputPortOptimalWidth = 0;
            perOutputPortOptimalHeight = 0;

            for (UINT input = 0; input < pOutputPort->numConnectedInputPorts; input++)
            {
                ChiNodeBufferRequirement* pInputPortRequirement = &pOutputPort->pBufferRequirement[input];

                // Optimal width per port is the super resolution of all the connected destination ports' optimal needs .
                perOutputPortOptimalWidth = (pInputPortRequirement->optimalW >= perOutputPortOptimalWidth) ?
                    pInputPortRequirement->optimalW : perOutputPortOptimalWidth;
                perOutputPortOptimalHeight = (pInputPortRequirement->optimalH >= perOutputPortOptimalHeight) ?
                    pInputPortRequirement->optimalH : perOutputPortOptimalHeight;
                perOutputPortMinWidth = (pInputPortRequirement->minW >= perOutputPortMinWidth) ?
                    pInputPortRequirement->minW : perOutputPortMinWidth;
                perOutputPortMinHeight = (pInputPortRequirement->minH >= perOutputPortMinHeight) ?
                    pInputPortRequirement->minH : perOutputPortMinHeight;
                perOutputPortMaxWidth = (pInputPortRequirement->maxW <= perOutputPortMaxWidth) ?
                    pInputPortRequirement->maxW : perOutputPortMaxWidth;
                perOutputPortMaxHeight = (pInputPortRequirement->maxH <= perOutputPortMaxHeight) ?
                    pInputPortRequirement->maxH : perOutputPortMaxHeight;
            }

            // Ensure optimal dimensions are within min and max dimensions. There are chances that the optimal dimension goes
            // over the max. Correct for the same
            perOutputPortOptimalWidth = (((perOutputPortOptimalWidth) <= (perOutputPortMinWidth)) ? (perOutputPortMinWidth) :
                (((perOutputPortOptimalWidth) >= (perOutputPortMaxWidth)) ? (perOutputPortMaxWidth) :
                (perOutputPortOptimalWidth)));

            perOutputPortOptimalHeight = (((perOutputPortOptimalHeight) <= (perOutputPortMinHeight)) ?
                (perOutputPortMinHeight) :
                (((perOutputPortOptimalHeight) >= (perOutputPortMaxHeight)) ?
                (perOutputPortMaxHeight) :
                    (perOutputPortOptimalHeight)));

            pOutputPort->outputBufferOption.minW = perOutputPortMinWidth;
            pOutputPort->outputBufferOption.minH = perOutputPortMinHeight;
            pOutputPort->outputBufferOption.maxW = perOutputPortMaxWidth;
            pOutputPort->outputBufferOption.maxH = perOutputPortMaxHeight;
            pOutputPort->outputBufferOption.optimalW = perOutputPortOptimalWidth;
            pOutputPort->outputBufferOption.optimalH = perOutputPortOptimalHeight;

            optimalInputWidth = ((perOutputPortOptimalWidth > optimalInputWidth) ?
                perOutputPortOptimalWidth : optimalInputWidth);
            optimalInputHeight = ((perOutputPortOptimalHeight > optimalInputHeight) ?
                perOutputPortOptimalHeight : optimalInputHeight);
            minInputWidth = ((perOutputPortMinWidth > minInputWidth) ?
                perOutputPortMinWidth : minInputWidth);
            minInputHeight = ((perOutputPortMinHeight > minInputHeight) ?
                perOutputPortMinHeight : minInputHeight);
            maxInputWidth = ((perOutputPortMaxWidth < maxInputWidth) ? perOutputPortMaxWidth : maxInputWidth);
            maxInputHeight = ((perOutputPortMaxHeight < maxInputHeight) ? perOutputPortMaxHeight : maxInputHeight);

            LOG_INFO(CamxLogGroupChi, "Input:  (w x h) optimal %d x %d min %d x %d max %d x %d", optimalInputWidth,
                optimalInputHeight, minInputWidth, minInputHeight, maxInputWidth, maxInputHeight);
            LOG_INFO(CamxLogGroupChi, "Output: (w x h) optimal %d x %d min %d x %d max %d x %d", perOutputPortOptimalWidth,
                perOutputPortOptimalHeight, perOutputPortMinWidth, perOutputPortMinHeight,
                perOutputPortMaxWidth, perOutputPortMaxHeight);
        }

        for (UINT outputIndex = 0; outputIndex < pQueryBufferInfo->numOutputPorts; outputIndex++)
        {
            isSinkNoBuffer = pQueryBufferInfo->pOutputPortQueryInfo[outputIndex].outputPortflags.isSinkNoBuffer;
            if (isSinkNoBuffer == TRUE)
            {
                LOG_INFO(CamxLogGroupChi, "Output port: %d is connected to SinkNoBuffer", outputIndex);
                minInputWidth      = 0;
                minInputHeight     = 0;
                maxInputWidth      = 0xFFFFFFFF;
                maxInputHeight     = 0xFFFFFFFF;
                optimalInputWidth  = 0;
                optimalInputHeight = 0;
                break;
            }
        }
        for (UINT inputIndex = 0; inputIndex < pQueryBufferInfo->numInputPorts; inputIndex++)
        {
            ChiInputPortQueryBufferInfo* pInputOptions = &pQueryBufferInfo->pInputOptions[inputIndex];

            pInputOptions->inputBufferOption.minW = minInputWidth;
            pInputOptions->inputBufferOption.minH = minInputHeight;
            pInputOptions->inputBufferOption.maxW = maxInputWidth;
            pInputOptions->inputBufferOption.maxH = maxInputHeight;
            pInputOptions->inputBufferOption.optimalW = optimalInputWidth;
            pInputOptions->inputBufferOption.optimalH = optimalInputHeight;
        }
    }
    else
    {
        LOG_ERROR(CamxLogGroupChi, "Invalide argument: pQueryBufferInfo is NULL");
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiNodeUtils::VSNPrintF
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
INT ChiNodeUtils::VSNPrintF(
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
/// ChiNodeUtils::LibMap
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CHILIBRARYHANDLE ChiNodeUtils::LibMap(
    const CHAR* pLibraryName,
    const CHAR* pLibraryPath)
{
    CHILIBRARYHANDLE hLibrary = NULL;
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
        LOG_ERROR(CamxLogGroupChi, "Error Loading Library: %s", libFilename);
#if defined (_LINUX)
        LOG_ERROR(CamxLogGroupChi, "dlopen error %s", dlerror());
#endif // _LINUX
    }
    return hLibrary;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ChiNodeUtils::LibMap
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CHILIBRARYHANDLE ChiNodeUtils::LibMapFullName(
    const CHAR* pLibraryName)
{
    CHILIBRARYHANDLE hLibrary = NULL;

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
        LOG_ERROR(CamxLogGroupChi, "Error Loading Library: %s", pLibraryName);
#if defined (_LINUX)
        LOG_ERROR(CamxLogGroupChi, "dlopen error %s", dlerror());
#endif // _LINUX
    }

    return hLibrary;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ChiNodeUtils::LibUnmap
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ChiNodeUtils::LibUnmap(
    CHILIBRARYHANDLE hLibrary)
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
        LOG_ERROR(CamxLogGroupCHI, "Failed to close CHI node Library %d", result);
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ChiNodeUtils::LibGetAddr
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID* ChiNodeUtils::LibGetAddr(
    CHILIBRARYHANDLE hLibrary,
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
// ChiNodeUtils::FOpen
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
FILE* ChiNodeUtils::FOpen(
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
// ChiNodeUtils::FileNo
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static INT FileNo(
    FILE* pFile)
{
    return fileno(pFile);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiNodeUtils::FClose
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ChiNodeUtils::FClose(
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
// ChiNodeUtils::FSeek
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ChiNodeUtils::FSeek(
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
// ChiNodeUtils::FTell
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
INT64 ChiNodeUtils::FTell(
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
// ChiNodeUtils::FRead
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
SIZE_T ChiNodeUtils::FRead(
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
// ChiNodeUtils::FWrite
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
SIZE_T ChiNodeUtils::FWrite(
    const VOID* pSrc,
    SIZE_T      size,
    SIZE_T      count,
    FILE*       pFile)
{
    return fwrite(pSrc, size, count, pFile);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiNodeUtils::FFlush
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ChiNodeUtils::FFlush(
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
UINT16 ChiNodeUtils::FloatToHalf(
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

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiNodeUtils::GetDateTime
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID ChiNodeUtils::GetDateTime(
    CHIDateTime* pDateTime)
{
    if (NULL != pDateTime)
    {
#if defined (_LINUX)
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
        LOG_ERROR(CamxLogGroupChi, "GetDateTime not implemented");
#endif // defined(_LINUX)
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ChiNodeUtils::StrStrip
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID ChiNodeUtils::StrStrip(
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

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ChiNodeUtils::GetAlignment
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID ChiNodeUtils::GetAlignment(
    INT          width,
    INT          height,
    UINT         planeId,
    ChiFormat    format,
    INT*         pStrideAlignment,
    INT*         pScanlineAlignment)
{
    // Get alignment info from msm_media_info.h

    // Set default stride and scanline alignments
    INT        strideAlignment   = 128;
    INT        scanlineAlignment = (0 == planeId) ? 32 : 16;

#if defined (_LINUX)
    BOOL       useDefault        = FALSE;
    // Venus color formats
    color_fmts mediaColorFmt     = COLOR_FMT_NV12;

    switch (format)
    {
    case ChiFormat::YUV420NV12:
        mediaColorFmt = COLOR_FMT_NV12;
        break;
    case ChiFormat::YUV420NV21:
        mediaColorFmt = COLOR_FMT_NV21;
        break;
    case ChiFormat::P010:
        mediaColorFmt = COLOR_FMT_P010;
        break;
    case ChiFormat::UBWCNV12:
        mediaColorFmt = COLOR_FMT_NV12_UBWC;
        break;
    case ChiFormat::UBWCTP10:
        mediaColorFmt = COLOR_FMT_NV12_BPP10_UBWC;
        break;
    default:
        useDefault = TRUE;
        LOG_WARN(CamxLogGroupChi, "Unsupported format for media color format: %d, using default alignments", format);
        break;
    }

    if (FALSE == useDefault)
    {
        INT alignedWidth  = 0;
        INT alignedHeight = 0;
        INT tempAlignedW  = 0;
        INT tempAlignedH  = 0;
        INT tempWidth     = width;
        INT tempHeight    = height;
        if (0 == planeId)
        {
            alignedWidth = VENUS_Y_STRIDE(mediaColorFmt, width);
            do
            {
                tempAlignedW = VENUS_Y_STRIDE(mediaColorFmt, --tempWidth);
            } while ((alignedWidth == tempAlignedW) && (0 != tempWidth));

            alignedHeight = VENUS_Y_SCANLINES(mediaColorFmt, height);
            do
            {
                tempAlignedH = VENUS_Y_SCANLINES(mediaColorFmt, --tempHeight);
            } while ((alignedHeight == tempAlignedH) && (0 != tempHeight));

            strideAlignment   = alignedWidth  - tempAlignedW;
            scanlineAlignment = alignedHeight - tempAlignedH;
        }
        else
        {
            alignedWidth = VENUS_UV_STRIDE(mediaColorFmt, width);
            do
            {
                tempAlignedW = VENUS_UV_STRIDE(mediaColorFmt, --tempWidth);
            } while ((alignedWidth == tempAlignedW) && (0 != tempWidth));

            alignedHeight = VENUS_UV_SCANLINES(mediaColorFmt, height);
            do
            {
                tempAlignedH = VENUS_UV_SCANLINES(mediaColorFmt, --tempHeight);
            } while ((alignedHeight == tempAlignedH) && (0 != tempHeight));

            strideAlignment   = alignedWidth  - tempAlignedW;
            scanlineAlignment = alignedHeight - tempAlignedH;
        }

        if ((0 == tempWidth) || (0 == tempHeight))
        {
            LOG_ERROR(CamxLogGroupChi,
                "Stride or scanline alignment calculation may be incorrect!, strideAlignment = %d, scanlineAlignment = %d",
                strideAlignment, scanlineAlignment);
        }
    }

    LOG_VERBOSE(CamxLogGroupChi,
        "Format = %d, mediaColorFmt = %d, planeId = %d, strideAlignment = %d, scanlineAlignment = %d",
        format, mediaColorFmt, planeId, strideAlignment, scanlineAlignment);
#endif // defined(_LINUX)

    *pStrideAlignment   = strideAlignment;
    *pScanlineAlignment = scanlineAlignment;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ChiNodeUtils::SleepMicroseconds
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID ChiNodeUtils::SleepMicroseconds(
    UINT microseconds)
{
    // sleep in us
    usleep(microseconds);
}
