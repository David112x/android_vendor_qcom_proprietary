////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2016-2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  camxstatscommon.cpp
/// @brief Stats common implementation.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "camxhal3metadatautil.h"
#include "camxstatscommon.h"
#include "camxmetadatapool.h"
#include "camxvendortags.h"
#include "camxsettingsmanager.h"

CAMX_NAMESPACE_BEGIN

static const INT    AlgorithmFilePathSize                   = 256;   ///< Maximum custom algorithm file path size


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// StatsUtil::LoadAlgorithmLib
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID* StatsUtil::LoadAlgorithmLib(
    CamX::OSLIBRARYHANDLE* pHandle,
    const CHAR* pAlgorithmPath,
    const CHAR* pAlgorithmName,
    const CHAR* pFunctionName)
{
    VOID*  pAddr = NULL;
    INT    numCharWritten = 0;
    CHAR   libFilename[FILENAME_MAX];

    numCharWritten = OsUtils::SNPrintF(libFilename,
                                       FILENAME_MAX,
                                       "%s%s.%s",
                                       pAlgorithmPath,
                                       pAlgorithmName,
                                       SharedLibraryExtension);

    CAMX_ASSERT((numCharWritten < AlgorithmFilePathSize) && (numCharWritten != -1));

    *pHandle = CamX::OsUtils::LibMap(libFilename);

    if (NULL == *pHandle)
    {
        numCharWritten = OsUtils::SNPrintF(libFilename,
                                           FILENAME_MAX,
                                           "%s%s%s.%s",
                                           VendorLibPath,
                                           PathSeparator,
                                           pAlgorithmName,
                                           SharedLibraryExtension);

        CAMX_ASSERT((numCharWritten < AlgorithmFilePathSize) && (numCharWritten != -1));

        *pHandle = CamX::OsUtils::LibMap(libFilename);
    }

    if (NULL != (*pHandle))
    {
        CAMX_LOG_VERBOSE(CamxLogGroupStats, "Loaded Custom algo library: %s", libFilename);

        pAddr = CamX::OsUtils::LibGetAddr(*pHandle, pFunctionName);
    }

    return pAddr;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ChiStatsSession::ChiStatsSession
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
ChiStatsSession::ChiStatsSession()
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ChiStatsSession::GetVendorTagBase
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ChiStatsSession::GetVendorTagBase(
    CHIVENDORTAGBASEINFO* pVendorTagBaseInfo)
{
    CDKResult result = CDKResultSuccess;

    if (NULL != pVendorTagBaseInfo)
    {
        if (pVendorTagBaseInfo->size <= sizeof(CHIVENDORTAGBASEINFO))
        {
            UINT32 sectionBase = 0;
            CamxResult resultCamx = CamxResultSuccess;
            resultCamx = VendorTagManager::QueryVendorTagSectionBase(pVendorTagBaseInfo->pComponentName, &sectionBase);
            if (CamxResultSuccess != resultCamx)
            {
                result = CDKResultEFailed;
            }
            else
            {
                pVendorTagBaseInfo->vendorTagBase = sectionBase;
            }
        }
        else
        {
            CAMX_LOG_ERROR(CamxLogGroupChi, "CHIVENDORTAGBASEINFO size mismatch");
            result = CDKResultEFailed;
        }
    }
    else
    {
        CAMX_LOG_ERROR(CamxLogGroupStats, "pVendorTagBaseInfo is NULL");
        result = CDKResultEFailed;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ChiStatsSession::FNGetMetadata
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ChiStatsSession::FNGetMetadata(
    CHIMETADATAINFO* pMetadataInfo)
{
    CDKResult result = CDKResultSuccess;

    if (NULL != pMetadataInfo && NULL != pMetadataInfo->chiSession)
    {
        if ((CDKResultSuccess == result) && (pMetadataInfo->size <= sizeof(CHIMETADATAINFO)))
        {
            ChiStatsSession* pStatsSession = static_cast<ChiStatsSession*>(pMetadataInfo->chiSession);

            if (ChiMetadataStatic == pMetadataInfo->metadataType)
            {
                result = pStatsSession->GetStaticMetadata(pMetadataInfo);
            }
            else if (ChiMetadataDynamic == pMetadataInfo->metadataType)
            {
                result = pStatsSession->GetDynamicMetadata(pMetadataInfo);
            }
            else
            {
                CAMX_LOG_ERROR(CamxLogGroupChi, "Invalid input metadata type");
                result = CDKResultEFailed;
            }
        }
        else
        {
            CAMX_LOG_ERROR(CamxLogGroupChi, "MedatadataInfo size mismatch or result is error:%d", result);
            result = CDKResultEFailed;
        }
    }
    else
    {
        CAMX_LOG_ERROR(CamxLogGroupChi, "pMetadataInfo or pMetadataInfo->chiSession is NULL");
        result = CDKResultEFailed;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ChiStatsSession::FNSetMetadata
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ChiStatsSession::FNSetMetadata(
    CHIMETADATAINFO* pMetadataInfo)
{
    CDKResult result = CDKResultSuccess;

    if (NULL != pMetadataInfo && NULL != pMetadataInfo->chiSession)
    {
        if ((CDKResultSuccess == result) && (pMetadataInfo->size <= sizeof(CHIMETADATAINFO)))
        {
            ChiStatsSession* pStatsSession = static_cast<ChiStatsSession*>(pMetadataInfo->chiSession);
            if (ChiMetadataStatic == pMetadataInfo->metadataType)
            {
                result = pStatsSession->SetStaticMetadata(pMetadataInfo);
            }
            else if (ChiMetadataDynamic == pMetadataInfo->metadataType)
            {
                result = pStatsSession->SetDynamicMetadata(pMetadataInfo);
            }
            else
            {
                CAMX_LOG_ERROR(CamxLogGroupChi, "Invalid input metadata type");
                result = CDKResultEFailed;
            }
        }
        else
        {
            CAMX_LOG_ERROR(CamxLogGroupChi, "MedatadataInfo size mismatch or result is error:%d", result);
            result = CDKResultEFailed;
        }
    }
    else
    {
        CAMX_LOG_ERROR(CamxLogGroupChi, "pMetadataInfo or pMetadataInfo->chiSession is NULL");
        result = CDKResultEFailed;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ChiStatsSession::GetStaticMetadata
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ChiStatsSession::GetStaticMetadata(
    CHIMETADATAINFO* pMetadataInfo)
{
    CDKResult       result  = CDKResultSuccess;
    MetadataSlot*   pSlot   = m_pStaticPool->GetSlot(0);

    if ((NULL == pMetadataInfo)                            ||
        (NULL == pMetadataInfo->pTagData)                  ||
        (NULL == pMetadataInfo->pTagList)                  ||
        (pMetadataInfo->size < sizeof(CHIMETADATAINFO)))
    {
        if (NULL != pMetadataInfo)
        {
            CAMX_LOG_ERROR(CamxLogGroupStats,
                           "Invalid arguments. pMetadataInfo: %p size: %d, pTagData: %p, pTagList: %p",
                           pMetadataInfo,
                           pMetadataInfo->size,
                           pMetadataInfo->pTagData,
                           pMetadataInfo->pTagList);
        }
        else
        {
            CAMX_LOG_ERROR(CamxLogGroupStats, "Invalid argument. pMetadataInfo: %p", pMetadataInfo);
        }

        result = CDKResultEInvalidArg;
    }


    if (NULL == pSlot)
    {
        result = CDKResultEFailed;
        CAMX_LOG_ERROR(CamxLogGroupStats, "Failed to retrieve static metadata slot.");
    }

    if (CDKResultSuccess == result)
    {
        pSlot->ReadLock();
        for (UINT32 i = 0; i < pMetadataInfo->tagNum; i++)
        {
            CHITAGDATA* pTagData = &pMetadataInfo->pTagData[i];
            if (NULL == pTagData)
            {
                CAMX_LOG_ERROR(CamxLogGroupStats, "Invalid argument. pTagData[%d] = %p", i, pTagData);
                result = CDKResultEInvalidArg;
                break;
            }

            pTagData->pData    = pSlot->GetMetadataByTag(pMetadataInfo->pTagList[i]);
            pTagData->dataSize = static_cast<UINT32>(HAL3MetadataUtil::GetMaxSizeByTag(pMetadataInfo->pTagList[i]));
        }
        pSlot->Unlock();
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ChiStatsSession::SetStaticMetadata
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ChiStatsSession::SetStaticMetadata(
    CHIMETADATAINFO* pMetadataInfo)
{
    CDKResult       cdkResult   = CDKResultSuccess;
    MetadataSlot*   pSlot       = m_pStaticPool->GetSlot(0);

    if ((NULL == pMetadataInfo)                           ||
        (NULL == pMetadataInfo->pTagData)                 ||
        (NULL == pMetadataInfo->pTagList)                 ||
        (pMetadataInfo->size < sizeof(CHIMETADATAINFO)))
    {
        if (NULL != pMetadataInfo)
        {
            CAMX_LOG_ERROR(CamxLogGroupStats,
                           "Invalid arguments. pMetadataInfo: %p size: %d, pTagData: %p, pTagList: %p",
                           pMetadataInfo,
                           pMetadataInfo->size,
                           pMetadataInfo->pTagData,
                           pMetadataInfo->pTagList);
        }
        else
        {
            CAMX_LOG_ERROR(CamxLogGroupStats, "Invalid argument. pMetadataInfo: %p", pMetadataInfo);
        }

        cdkResult = CDKResultEInvalidArg;
    }

    if (NULL == pSlot)
    {
        cdkResult = CDKResultEFailed;
        CAMX_LOG_ERROR(CamxLogGroupStats, "Failed to retrieve static metadata slot.");
    }

    if (CDKResultSuccess == cdkResult)
    {
        CamxResult  result = CamxResultSuccess;

        for (UINT32 i = 0; i < pMetadataInfo->tagNum; i++)
        {
            CHITAGDATA* pTagData = &pMetadataInfo->pTagData[i];
            if (NULL == pTagData)
            {
                CAMX_LOG_ERROR(CamxLogGroupStats, "Invalid argument. pTagData[%d] = %p", i, pTagData);
                result = CDKResultEInvalidArg;
                break;
            }

            result = pSlot->SetMetadataByTag(pMetadataInfo->pTagList[i], pTagData->pData, pTagData->dataSize, "statsCommon");

            if (result != CamxResultSuccess)
            {
                CAMX_LOG_ERROR(CamxLogGroupStats,
                               "Failed to publish VendorTag(%u) reqID(%llu) size(%d) pData(%p). Result = %d",
                               pMetadataInfo->pTagList[i],
                               pTagData->requestId,
                               pTagData->dataSize,
                               pTagData->pData,
                               result);
                cdkResult = CDKResultEFailed;
                break;
            }
        }
    }

    return cdkResult;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ChiStatsSession::GetDynamicMetadata
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ChiStatsSession::GetDynamicMetadata(
    CHIMETADATAINFO* pMetadataInfo)
{
    CDKResult result = CDKResultSuccess;
    CAMX_ASSERT((NULL != pMetadataInfo) && (NULL != pMetadataInfo->chiSession));

    UINT64* pOffset = static_cast<UINT64 *>(CAMX_CALLOC(sizeof(UINT64) * pMetadataInfo->tagNum));
    VOID**  ppData  = static_cast<VOID **>(CAMX_CALLOC(sizeof(VOID *) * pMetadataInfo->tagNum));

    if (NULL == pOffset || NULL == ppData)
    {
        result = CDKResultENoMemory;
    }

    if (CDKResultSuccess == result)
    {
        for (UINT32 i = 0; i < pMetadataInfo->tagNum; i++)
        {
            if (NULL != m_pStatsProcessRequestData)
            {
                if (pMetadataInfo->pTagData[i].requestId >= m_pStatsProcessRequestData->requestId)
                {
                    pOffset[i] = 0;
                }
                else
                {
                    pOffset[i] = m_pStatsProcessRequestData->requestId - pMetadataInfo->pTagData[i].requestId;
                }
            }
            else
            {
                pOffset[i] = 0;
            }
        }

        m_pNode->GetDataList(pMetadataInfo->pTagList, ppData, pOffset, pMetadataInfo->tagNum);

        for (UINT32 i = 0; i < pMetadataInfo->tagNum; i++)
        {
            pMetadataInfo->pTagData[i].pData = ppData[i];
            pMetadataInfo->pTagData[i].dataSize =
                static_cast<UINT32>(HAL3MetadataUtil::GetMaxSizeByTag(pMetadataInfo->pTagList[i]));
        }
    }

    if (NULL != ppData)
    {
        CAMX_FREE(ppData);
        ppData = NULL;
    }

    if (NULL != pOffset)
    {
        CAMX_FREE(pOffset);
        pOffset = NULL;
    }


    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ChiStatsSession::SetDynamicMetadata
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ChiStatsSession::SetDynamicMetadata(
    CHIMETADATAINFO* pMetadataInfo)
{
    UINT32 tagNum                   = pMetadataInfo->tagNum;
    CDKResult result                = CDKResultSuccess;

    UINT*         pDataSize = static_cast<UINT *>(CAMX_CALLOC(sizeof(UINT) * tagNum));
    const VOID**  ppData = static_cast<const VOID **>(CAMX_CALLOC(sizeof(VOID *) * tagNum));

    if (NULL == pDataSize || NULL == ppData)
    {
        result = CDKResultENoMemory;
    }

    if (CDKResultSuccess == result)
    {
        for (UINT32 i = 0; i < pMetadataInfo->tagNum; i++)
        {
            ppData[i] = pMetadataInfo->pTagData[i].pData;
            pDataSize[i] = static_cast<UINT>(pMetadataInfo->pTagData[i].dataSize);
        }

        m_pNode->WriteDataList(pMetadataInfo->pTagList, ppData, pDataSize, tagNum);
    }

    if (NULL != ppData)
    {
        CAMX_FREE(ppData);
        ppData = NULL;
    }

    if (NULL != pDataSize)
    {
        CAMX_FREE(pDataSize);
        pDataSize = NULL;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ChiStatsSession::QueryVendorTagLocation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ChiStatsSession::QueryVendorTagLocation(
    const CHAR* pSectionName,
    const CHAR* pTagName,
    UINT32*     pTagLocation)

{
    return VendorTagManager::QueryVendorTagLocation(pSectionName, pTagName, pTagLocation);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// StatsUtil::GetDebugDataBuffer
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult StatsUtil::GetDebugDataBuffer(
    MetadataPool*       pDebugDataPool,
    UINT64              requestId,
    UINT                tagId,
    DebugData**         ppDebugDataOut)
{
    CamxResult      result          = CamxResultSuccess;
    MetadataSlot*   pMetadataSlot   = NULL;
    DebugData*      pDebugData      = NULL;

    if ((NULL == pDebugDataPool) ||
        (NULL == ppDebugDataOut))
    {
        result = CamxResultEInvalidArg;
    }
    else
    {
        pMetadataSlot = pDebugDataPool->GetSlot(requestId);
        if (NULL != pMetadataSlot)
        {
            VOID* pBlob = NULL;
            pMetadataSlot->GetPropertyBlob(&pBlob);
            pDebugData = reinterpret_cast<DebugData*>(
               Utils::VoidPtrInc(pBlob, DebugDataPropertyOffsets[tagId & ~DriverInternalGroupMask]));

        }

        if ((NULL               != pDebugData)          &&
            (NULL               != pDebugData->pData)   &&
            (0                  <  pDebugData->size))
        {
            (*ppDebugDataOut) = pDebugData;

            CAMX_LOG_VERBOSE(CamxLogGroupDebugData,
                             "[%d] Debug data for request (%llu): ptr: 0x%x, size: %u",
                             tagId,
                             requestId,
                             pDebugData->pData,
                             pDebugData->size);
        }
        else
        {
            (*ppDebugDataOut) = NULL;
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// StatsUtil::GetStatsStreamInitConfig
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult StatsUtil::GetStatsStreamInitConfig(
    MetadataPool*           pUsecasePool,
    StatsStreamInitConfig*  pStatsStreamInitConfig)
{
    CamxResult              result          = CamxResultSuccess;
    MetadataSlot*           pCurrentSlot    = pUsecasePool->GetSlot(0);

    if (NULL != pStatsStreamInitConfig)
    {
        StatsStreamInitConfig* pSrcConfig = static_cast<StatsStreamInitConfig*>(
            pCurrentSlot->GetMetadataByTag(PropertyIDUsecaseStatsStreamInitConfig));
        if (NULL != pSrcConfig)
        {
            Utils::Memcpy(pStatsStreamInitConfig, pSrcConfig, sizeof(StatsStreamInitConfig));
        }
        else
        {
            result = CamxResultENoSuch;
        }
    }
    else
    {
        CAMX_LOG_ERROR(CamxLogGroupStats, "Invalid pointer FastStatsData: %p", pStatsStreamInitConfig);
        result = CamxResultEInvalidPointer;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// StatsUtil::GetRoleName()
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
const CHAR* StatsUtil::GetRoleName(
    StatsAlgoRole role)
{
    switch (role)
    {
        case StatsAlgoRole::StatsAlgoRoleDefault:
            return "Default";
        case StatsAlgoRole::StatsAlgoRoleMaster:
            return "Master";
        case StatsAlgoRole::StatsAlgoRoleSlave:
            return "Slave";
        default:
            return "Unknown";
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// StatsUtil::CdkResultToCamXResult
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult  StatsUtil::CdkResultToCamXResult(
    CDKResult cdkResult)
{
    switch (cdkResult)
    {
        case CDKResultSuccess:
            return CamxResultSuccess;
        case CDKResultEUnsupported:
            return CamxResultEUnsupported;
        case CDKResultEInvalidState:
            return CamxResultEInvalidState;
        case CDKResultEInvalidArg:
            return CamxResultEInvalidArg;
        case CDKResultEInvalidPointer:
            return CamxResultEInvalidPointer;
        case CDKResultENoSuch:
            return CamxResultENoSuch;
        case CDKResultEOutOfBounds:
            return CamxResultEOutOfBounds;
        case CDKResultENoMemory:
            return CamxResultENoMemory;
        case CDKResultETimeout:
            return CamxResultETimeout;
        case CDKResultENoMore:
            return  CamxResultENoMore;
        case CDKResultENeedMore:
            return CamxResultENeedMore;
        case CDKResultEExists:
            return CamxResultEExists;
        case CDKResultEPrivLevel:
            return  CamxResultEPrivLevel;
        case CDKResultEResource:
            return CamxResultEResource;
        case CDKResultEUnableToLoad:
            return CamxResultEUnableToLoad;
        case CDKResultEInProgress:
            return CamxResultEInProgress;
        case CDKResultETryAgain:
            return CamxResultETryAgain;
        case CDKResultEBusy:
            return  CamxResultEBusy;
        case CDKResultEReentered:
            return CamxResultEReentered;
        case CDKResultEReadOnly:
            return CamxResultEReadOnly;
        case CDKResultEOverflow:
            return CamxResultEOverflow;
        case CDKResultEOutOfDomain:
            return CamxResultEOutOfDomain;
        case CDKResultEInterrupted:
            return CamxResultEInterrupted;
        case CDKResultEWouldBlock:
            return CamxResultEWouldBlock;
        case CDKResultETooManyUsers:
            return CamxResultETooManyUsers;
        case CDKResultENotImplemented:
            return CamxResultENotImplemented;
        case CDKResultEFailed:
        default:
            return CamxResultEFailed;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// StatsUtil::ReadVendorTag
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult StatsUtil::ReadVendorTag(
    Node*       pNode,
    const CHAR* pSectionName,
    const CHAR* pTagName,
    VOID**      ppArg)
{
    UINT32     tag;
    CamxResult result;
    UINT       props[1] = { 0 };
    UINT64     offsets[1] = { 0 };
    VOID*      pData[1] = { 0 };

    result = VendorTagManager::QueryVendorTagLocation(pSectionName, pTagName, &tag);
    if (result != CamxResultSuccess)
    {
        CAMX_LOG_WARN(CamxLogGroupStats, "%s: %s tag not defined", pSectionName, pTagName);
        return CamxResultEFailed;
    }
    props[0] = tag | InputMetadataSectionMask;

    pNode->GetDataList(props, pData, offsets, 1);

    if (!pData[0])
    {
        CAMX_LOG_VERBOSE(CamxLogGroupStats, "%s: %s tag not published", pSectionName, pTagName);
        return CamxResultEFailed;
    }
    *ppArg = pData[0];

    return CamxResultSuccess;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// StatsUtil::WriteVendorTag
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult StatsUtil::WriteVendorTag(
    Node*       pNode,
    const CHAR* pSectionName,
    const CHAR* pTagName,
    const VOID* pArg,
    const UINT  argCount)
{
    UINT32      tag;
    CamxResult  result = CamxResultSuccess;
    UINT        props[1] = { 0 };
    UINT        pDataCount[1] = { 0 };
    const VOID* pData[1];
    result = VendorTagManager::QueryVendorTagLocation(pSectionName, pTagName, &tag);
    if (CamxResultSuccess != result)
    {
        CAMX_LOG_ERROR(CamxLogGroupStats, "%s: %s tag not defined", pSectionName, pTagName);
        return CamxResultEFailed;
    }
    props[0] = tag;
    pData[0] = pArg;
    pDataCount[0] = argCount;
    if (!pData[0])
    {
        CAMX_LOG_ERROR(CamxLogGroupStats, "Vendor tag data not available for writing");
        return CamxResultEFailed;
    }
    result = pNode->WriteDataList(props, pData, pDataCount, 1);
    if (CamxResultSuccess != result)
    {
        CAMX_LOG_ERROR(CamxLogGroupStats, "Unable to write for vendor tag");
        return CamxResultEFailed;
    }
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// StatsPropertyReadAndWrite::StatsPropertyReadAndWrite
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
StatsPropertyReadAndWrite::StatsPropertyReadAndWrite(
    const StatsInitializeData* pInitializeData)
{
    CAMX_ASSERT(NULL != pInitializeData);
    m_pNode = pInitializeData->pNode;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// StatsPropertyReadAndWrite::~StatsPropertyReadAndWrite
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
StatsPropertyReadAndWrite::~StatsPropertyReadAndWrite()
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// StatsPropertyReadAndWrite::SetReadProperties
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID StatsPropertyReadAndWrite::SetReadProperties(
    ReadProperty* pReadProperty)
{
    CAMX_ASSERT(NULL != pReadProperty);
    m_pReadProperty = pReadProperty;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// StatsPropertyReadAndWrite::SetWriteProperties
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID StatsPropertyReadAndWrite::SetWriteProperties(
    WriteProperty* pWriteProperty)
{
    CAMX_ASSERT(NULL != pWriteProperty);
    m_pWriteProperty = pWriteProperty;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// StatsPropertyReadAndWrite::GetReadProperties
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID StatsPropertyReadAndWrite::GetReadProperties(
    SIZE_T          count,
    UINT64          requestIdOffsetFromLastFlush)
{
    UINT64      pPropertyReadTagsOffset[MaxReadProperties]  = { 0 };
    UINT32      pPropertyReadTags[MaxReadProperties]        = { 0 };

    for (UINT i = 0; i < count; i++)
    {
        pPropertyReadTags[i]             = m_pReadProperty[i].propertyID;
        pPropertyReadTagsOffset[i]       = m_pReadProperty[i].offset;

        if (m_pReadProperty[i].offset < requestIdOffsetFromLastFlush)
        {
            m_pPropertyReadData[i] = m_pReadProperty[i].pReadData;
        }
        else
        {
            m_pPropertyReadData[i] = NULL;
        }
    }
    m_pNode->GetDataList(pPropertyReadTags, m_pPropertyReadData, pPropertyReadTagsOffset, count);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// StatsPropertyReadAndWrite::WriteProperties
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID StatsPropertyReadAndWrite::WriteProperties(
    SIZE_T       count)
{
    const VOID*                pPropertyWriteData[MaxWriteProperties];
    UINT32                     pPropertyWriteTags[MaxWriteProperties];
    UINT                       pWriteDataCount[MaxWriteProperties];

    for (UINT i = 0; i < count; i++)
    {
        pPropertyWriteTags[i]  = m_pWriteProperty[i].propertyID;
        pWriteDataCount[i]     = m_pWriteProperty[i].writtenDataSize;
        pPropertyWriteData[i]  = m_pWriteProperty[i].pWriteData;
    }
    m_pNode->WriteDataList(pPropertyWriteTags, pPropertyWriteData, pWriteDataCount, count);
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// StatsPropertyReadAndWrite::GetPropertyData
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID*  StatsPropertyReadAndWrite::GetPropertyData(
    const UINT id
    )const
{
    return m_pPropertyReadData[id];
}


CAMX_NAMESPACE_END
