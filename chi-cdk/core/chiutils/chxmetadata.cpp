////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2018-2020 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  chxmetadata.cpp
/// @brief CHX Metadata class implementation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include <assert.h>
#include <chrono>
#include <string>

#include "cdkutils.h"
#include "chxincs.h"
#include "chxmetadata.h"

#if defined (_LINUX)
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#endif

// NOWHINE ENTIRE FILE - Temporarily bypassing for existing CHI files

// #define __ENABLE_META_PROFILING__

// logging macros
#define CHX_LOG_META_DBG  CHX_LOG_INFO

#ifdef __ENABLE_META_PROFILING__
#define CHX_LOG_META_PROF CHX_LOG
#else
#define CHX_LOG_META_PROF(...) do {} while(0);
#endif

// dumping macros
#define __DUMP_META__ 0

// namespaces
using namespace std;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Constants
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static const UINT32 InvalidRefCount         = 0xFFFFFFFF;
static const UINT32 ChxMaxMetadataBuffers   = 150;
static const UINT32 ChxMaxRequestKeys       = 100;
static const UINT16 ChxInvalidSubId         = 0xFFFF;
static const UINT32 KEntryCount             = 256;
static const UINT32 KEntrySize              = 1024;
static const UINT32 KEntryCountSparse       = 2;
static const UINT32 KEntrySizeSparse        = sizeof(UINT64);
static const UINT32 ChxMaxFWOSize           = 150;
static const UINT32 ChxDefaultFWOSize       = 8;
static const UINT32 ChxDefaultFWOSparseSize = 8;
static const UINT32 MaxFileLen              = 256;
static const UINT32 ChxMaxMetaHoldTime      = 9000; ///< Maximum hold time for metadata buffers in ms for debugging
static const UINT32 ChxTrackerFrameDelta    = 100;  ///< Frame delta at which tracker needs to run
static const UINT32 ChxMetaHeaderStringSize = 4;    ///< Header String size for the metadata

static const UINT32 ChxMaxOfflineMetadataBuffers   = 150; ///< Maximum offline metadata buffers

static const UINT32 ChxCameraMetaStatusOk       = 0;
static const UINT32 ChxCameraMetaStatusError    = 1;
static const UINT32 ChxCameraMetaStatusNotFound = -ENOENT;

// Function to validate. Print error incase of failure
#define CHX_MD_VALIDATE_FUNC_PTR(pOps, pFn, result) { \
    if ((CDKResultSuccess == result) && (NULL == (pOps->pFn))) \
    { \
       CHX_LOG("[%s] NULL ", #pFn); \
       result = CDKResultEFailed; \
    }; \
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiMetadata::Create
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
ChiMetadata* ChiMetadata::Create(
    const UINT32*       pTagList,
    UINT32              tagCount,
    bool                useDefaultKeys,
    ChiMetadataManager* pManager)
{
    ChiMetadata*   pMetadata = CHX_NEW ChiMetadata;
    CDKResult      result    = CDKResultSuccess;
    vector<UINT32> tagList;

    if (NULL != pMetadata)
    {
        pMetadata->m_pManager = pManager;

        result = InitializeMetadataOps(&pMetadata->m_metadataOps);

        if (CDKResultSuccess == result)
        {
            pMetadata->m_metadataManagerClientId = ChiMetadataManager::InvalidClientId;

            // update with default keys
            if ((NULL == pTagList) && useDefaultKeys)
            {
                tagList.resize(ChxMaxRequestKeys);

                result = ExtensionModule::GetInstance()->GetAvailableRequestKeys(
                    0,
                    tagList.data(),
                    tagList.size(),
                    &tagCount);

                if (CDKResultSuccess == result)
                {
                    pTagList = tagList.data();
                }
            }

            result = pMetadata->Initialize(pTagList, tagCount);
        }
    }

    if (CDKResultSuccess != result)
    {
        CHX_DELETE pMetadata;
        pMetadata = NULL;
    }

    return pMetadata;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiMetadata::ReadDataFromFile
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ChiMetadata::ReadDataFromFile(
    const CHAR*        pMetadataFileName,
    std::vector<BYTE>& rBuffer)
{
    FILE*       pFp     = CdkUtils::FOpen(pMetadataFileName, "rb");
    INT         fileLen = 0;
    CDKResult   result  = CDKResultEFailed;

    if (pFp)
    {
        CdkUtils::FSeek(pFp, 0, SEEK_END);
        fileLen = CdkUtils::FTell(pFp);
        CdkUtils::FSeek(pFp, 0, SEEK_SET);

        if (0 < fileLen)
        {
            rBuffer.resize(fileLen);

            INT bytesRead = CdkUtils::FRead(rBuffer.data(), rBuffer.size(), fileLen, 1, pFp);

            if (0 < bytesRead)
            {
                result = CDKResultSuccess;
            }
            else
            {
                 CHX_LOG_ERROR("[CMB_ERROR] Zero bytes read for %s", pMetadataFileName);
            }
        }
        CdkUtils::FClose(pFp);
    }
    else
    {
        CHX_LOG_ERROR("[CMB_ERROR] Cannot open file %s", pMetadataFileName);
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiMetadata::ParseAndSetMetadata
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ChiMetadata::ParseAndSetMetadata(
    std::vector<BYTE>& rBuffer)
{
    CHAR        headerString[] = { 'M', 'E', 'T', 'A' };
    BYTE*       pBuffer        = rBuffer.data();
    UINT        parsedLen      = 0;
    CDKResult   result         = CDKResultSuccess;

    if (0 == memcmp(headerString, pBuffer, ChxMetaHeaderStringSize))
    {
        // Skip the version
        parsedLen += ChxMetaHeaderStringSize + 4;

        /// extract each metadata
        UINT tagId;
        UINT size;
        UINT count;
        UINT tagCount = 0;

        while (parsedLen + 12 < rBuffer.size())
        {
            memcpy(&tagId, &pBuffer[parsedLen], 4);
            parsedLen += 4;
            memcpy(&size, &pBuffer[parsedLen], 4);
            parsedLen += 4;
            memcpy(&count, &pBuffer[parsedLen], 4);
            parsedLen += 4;

            if ((0 < count) && (parsedLen + count < rBuffer.size()))
            {
                SetTag(tagId, &pBuffer[parsedLen], count);
                parsedLen += size;
                tagCount++;
            }
            else
            {
                break;
            }
        }
        CHX_LOG_INFO("[CMB_DEBUG] Parsed tag count %d parsed_len %d buffer_size %zu",
            tagCount, parsedLen, rBuffer.size());
    }
    else
    {
        result = CDKResultEInvalidArg;
        CHX_LOG_ERROR("[CMB_ERROR] Invalid metadata buffer header");
    }
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiMetadata::Create
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
ChiMetadata* ChiMetadata::Create(
    const CHAR* pMetadataFileName)
{
    ChiMetadata*   pMetadata = NULL;
    CDKResult      result    = CDKResultEFailed;
    vector<BYTE>   buffer;

    if (NULL != pMetadataFileName)
    {
        result = ReadDataFromFile(pMetadataFileName, buffer);

        if (CDKResultSuccess == result)
        {
            pMetadata = CHX_NEW ChiMetadata;

            if (NULL != pMetadata)
            {
                pMetadata->m_pManager = NULL;

                result = InitializeMetadataOps(&pMetadata->m_metadataOps);

                if (CDKResultSuccess == result)
                {
                    pMetadata->m_metadataManagerClientId = ChiMetadataManager::InvalidClientId;

                    result = pMetadata->Initialize(NULL, 0);

                    if (CDKResultSuccess == result)
                    {
                        result = pMetadata->ParseAndSetMetadata(buffer);
                    }
                    else
                    {
                        CHX_LOG_ERROR("[CMB_ERROR] Parsing failed");
                    }
                }
                else
                {
                    CHX_LOG_ERROR("[CMB_ERROR] Initialize metadata ops failed");
                }
            }
            else
            {
                result = CDKResultENoMemory;
                CHX_LOG_ERROR("[CMB_ERROR] NULL metadata");
            }
        }

        if ((CDKResultSuccess != result) && (NULL != pMetadata))
        {
            CHX_DELETE pMetadata;
            pMetadata = NULL;
        }
    }

    return pMetadata;
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiMetadata::InitializeMetadataOps
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ChiMetadata::InitializeMetadataOps(
    CHIMETADATAOPS* pMetadataOps)
{
    CDKResult result = CDKResultSuccess;
    ExtensionModule::GetInstance()->GetMetadataOps(pMetadataOps);

    CHX_MD_VALIDATE_FUNC_PTR(pMetadataOps, pCreate, result);
    CHX_MD_VALIDATE_FUNC_PTR(pMetadataOps, pCreateWithTagArray, result);
    CHX_MD_VALIDATE_FUNC_PTR(pMetadataOps, pCreateWithAndroidMetadata, result);
    CHX_MD_VALIDATE_FUNC_PTR(pMetadataOps, pDestroy, result);
    CHX_MD_VALIDATE_FUNC_PTR(pMetadataOps, pGetTag, result);
    CHX_MD_VALIDATE_FUNC_PTR(pMetadataOps, pGetTagEntry, result);
    CHX_MD_VALIDATE_FUNC_PTR(pMetadataOps, pGetVendorTag, result);
    CHX_MD_VALIDATE_FUNC_PTR(pMetadataOps, pGetVendorTagEntry, result);
    CHX_MD_VALIDATE_FUNC_PTR(pMetadataOps, pSetTag, result);
    CHX_MD_VALIDATE_FUNC_PTR(pMetadataOps, pSetVendorTag, result);
    CHX_MD_VALIDATE_FUNC_PTR(pMetadataOps, pSetAndroidMetadata, result);
    CHX_MD_VALIDATE_FUNC_PTR(pMetadataOps, pDeleteTag, result);
    CHX_MD_VALIDATE_FUNC_PTR(pMetadataOps, pInvalidate, result);
    CHX_MD_VALIDATE_FUNC_PTR(pMetadataOps, pMerge, result);
    CHX_MD_VALIDATE_FUNC_PTR(pMetadataOps, pCopy, result);
    CHX_MD_VALIDATE_FUNC_PTR(pMetadataOps, pClone, result);
    CHX_MD_VALIDATE_FUNC_PTR(pMetadataOps, pCapacity, result);
    CHX_MD_VALIDATE_FUNC_PTR(pMetadataOps, pCount, result);
    CHX_MD_VALIDATE_FUNC_PTR(pMetadataOps, pPrint, result);
    CHX_MD_VALIDATE_FUNC_PTR(pMetadataOps, pDump, result);
    CHX_MD_VALIDATE_FUNC_PTR(pMetadataOps, pBinaryDump, result);
    CHX_MD_VALIDATE_FUNC_PTR(pMetadataOps, pAddReference, result);
    CHX_MD_VALIDATE_FUNC_PTR(pMetadataOps, pReleaseReference, result);
    CHX_MD_VALIDATE_FUNC_PTR(pMetadataOps, pReleaseAllReferences, result);
    CHX_MD_VALIDATE_FUNC_PTR(pMetadataOps, pReferenceCount, result);
    CHX_MD_VALIDATE_FUNC_PTR(pMetadataOps, pGetMetadataTable, result);
    CHX_MD_VALIDATE_FUNC_PTR(pMetadataOps, pGetMetadataEntryCount, result);
    CHX_MD_VALIDATE_FUNC_PTR(pMetadataOps, pIteratorCreate, result);
    CHX_MD_VALIDATE_FUNC_PTR(pMetadataOps, pIteratorDestroy, result);
    CHX_MD_VALIDATE_FUNC_PTR(pMetadataOps, pIteratorBegin, result);
    CHX_MD_VALIDATE_FUNC_PTR(pMetadataOps, pIteratorNext, result);
    CHX_MD_VALIDATE_FUNC_PTR(pMetadataOps, pIteratorGetEntry, result);
    CHX_MD_VALIDATE_FUNC_PTR(pMetadataOps, pGetDefaultAndroidMeta, result);
    CHX_MD_VALIDATE_FUNC_PTR(pMetadataOps, pGetDefaultMetadata, result);
    CHX_MD_VALIDATE_FUNC_PTR(pMetadataOps, pFilter, result);
    CHX_MD_VALIDATE_FUNC_PTR(pMetadataOps, pGetPrivateData, result);
    CHX_MD_VALIDATE_FUNC_PTR(pMetadataOps, pGetPrivateData, result);
    CHX_MD_VALIDATE_FUNC_PTR(pMetadataOps, pMergeMultiCameraMeta, result);
    CHX_MD_VALIDATE_FUNC_PTR(pMetadataOps, pGetTagByCameraId, result);

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiMetadata::Initialize
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ChiMetadata::Initialize(
    const UINT32* pTagList,
    UINT32        tagCount)
{
    CDKResult result;

    if ((NULL != pTagList) && (0 < tagCount))
    {
        result = m_metadataOps.pCreateWithTagArray(pTagList, tagCount, &m_metaHandle, this);
    }
    else
    {
        result = m_metadataOps.pCreate(&m_metaHandle, this);
    }

    for (UINT32 index = 0; index < MaxChiMetaClients; ++index)
    {
        m_clientName[index] = NULL;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiMetadata::Destroy
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ChiMetadata::Destroy(
    bool force)
{
    CDKResult result = CDKResultEFailed;

    if ((ChiMetadataManager::InvalidClientId == m_metadataManagerClientId) &&
        (NULL != m_metaHandle))
    {
        result = DestroyInternal(force);
    }
    else
    {
        CHX_LOG_ERROR("[CMB_ERROR] Trying to delete the metadata owned by the manager this %p ops %p client %x frame %u",
            this,
            m_metaHandle,
            m_metadataClientId.clientIndex,
            m_metadataClientId.frameNumber);
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiMetadata::DestroyInternal
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ChiMetadata::DestroyInternal(
    bool force)
{

    CDKResult result = m_metadataOps.pDestroy(m_metaHandle, force);
    m_metaHandle = NULL;
    if (CDKResultSuccess == result)
    {
        CHX_DELETE this;
    }
    else
    {
        CHX_LOG_ERROR("[CMB_ERROR] Cannot destroy metadata client %x frame %u",
            m_metadataClientId.clientIndex,
            m_metadataClientId.frameNumber);
    }
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiMetadata::GetTag
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID* ChiMetadata::GetTag(
    UINT32 tagID)
{
    VOID* pData      = NULL;
    CDKResult result = m_metadataOps.pGetTag(m_metaHandle, tagID, &pData);
    CHX_ASSERT(CDKResultSuccess == result);
    return pData;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiMetadata::GetTag
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ChiMetadata::GetTag(
    UINT32            tagID,
    ChiMetadataEntry& entry)
{
    CDKResult result = m_metadataOps.pGetTagEntry(m_metaHandle, tagID, &entry);
    CHX_ASSERT(CDKResultSuccess == result);
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiMetadata::GetTag
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID* ChiMetadata::GetTag(
    const CHAR* pTagSectionName,
    const CHAR* pTagName)
{
    VOID*     pData  = NULL;
    CDKResult result = m_metadataOps.pGetVendorTag(m_metaHandle, pTagSectionName, pTagName, &pData);
    CHX_ASSERT(CDKResultSuccess == result);
    return pData;
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiMetadata::FindTag
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ChiMetadata::FindTag(
    uint32_t                 tag,
    camera_metadata_entry_t* pEntry)
{
    ChiMetadataEntry chiMetadataEntry;

    CDKResult result = m_metadataOps.pGetTagEntry(m_metaHandle, tag, &chiMetadataEntry);

    if (CDKResultSuccess == result)
    {
        pEntry->count   = chiMetadataEntry.count;
        pEntry->data.u8 = static_cast<BYTE*>(chiMetadataEntry.pTagData);
        pEntry->tag     = tag;
        pEntry->type    = chiMetadataEntry.type;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiMetadata::SetTag
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ChiMetadata::SetTag(
    UINT32       tagID,
    const VOID*  pData,
    UINT32       count)
{
    CDKResult result = m_metadataOps.pSetTag(m_metaHandle, tagID, pData, count);
    CHX_ASSERT(CDKResultSuccess == result);
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiMetadata::SetAndroidMetadata
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ChiMetadata::SetAndroidMetadata(
    const camera_metadata_t* pCameraMetadata)
{
    CDKResult result = m_metadataOps.pSetAndroidMetadata(m_metaHandle, pCameraMetadata);
    CHX_ASSERT(CDKResultSuccess == result);
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiMetadata::SetTag
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ChiMetadata::SetTag(
    const CHAR*   pTagSectionName,
    const CHAR*   pTagName,
    const VOID*   pData,
    UINT32        count)
{
    CDKResult result = m_metadataOps.pSetVendorTag(m_metaHandle, pTagSectionName, pTagName, pData, count);
    CHX_ASSERT(CDKResultSuccess == result);
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiMetadata::DeleteTag
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ChiMetadata::DeleteTag(
    UINT32 tagID)
{
    CDKResult result = m_metadataOps.pDeleteTag(m_metaHandle, tagID);
    CHX_ASSERT(CDKResultSuccess == result);
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiMetadata::Count
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UINT32 ChiMetadata::Count()
{
    UINT32 count = 0;
    CDKResult result = m_metadataOps.pCount(m_metaHandle, &count);
    CHX_ASSERT(CDKResultSuccess == result);
    return count;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiMetadata::Reset
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ChiMetadata::Reset()
{
    CDKResult result = m_metadataOps.pInvalidate(m_metaHandle);
    CHX_ASSERT(CDKResultSuccess == result);
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiMetadata::Merge
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ChiMetadata::Merge(
    ChiMetadata& srcMetadata,
    bool disjoint)
{
    CDKResult result = m_metadataOps.pMerge(m_metaHandle, srcMetadata.m_metaHandle, disjoint);

    CMBTimePoint curTimePoint = CMBTime::now();

    if (ChiMetadataManager::InvalidClientId != m_metadataManagerClientId)
    {
        m_timePoint = curTimePoint;
    }

    if (ChiMetadataManager::InvalidClientId != srcMetadata.m_metadataManagerClientId)
    {
        srcMetadata.m_timePoint = curTimePoint;
    }

    CHX_ASSERT(CDKResultSuccess == result);
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiMetadata::MergeMultiCameraMetadata
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ChiMetadata::MergeMultiCameraMetadata(
    UINT32        metadataCount,
    ChiMetadata** ppSrcMetadataArray,
    UINT32*       pCameraIdArray,
    UINT32        primaryCameraId)
{
    vector<CHIMETADATAHANDLE> metaHandleArray(metadataCount);
    for (UINT32 index = 0; index < metadataCount; ++index)
    {
        metaHandleArray[index] = ppSrcMetadataArray[index]->m_metaHandle;
    }

    CDKResult result = m_metadataOps.pMergeMultiCameraMeta(
        m_metaHandle,
        metadataCount,
        metaHandleArray.data(),
        pCameraIdArray,
        primaryCameraId);

    CHX_ASSERT(CDKResultSuccess == result);
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiMetadata::Copy
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ChiMetadata::Copy(
    ChiMetadata& srcMetadata,
    bool disjoint)
{
    CDKResult result = m_metadataOps.pCopy(m_metaHandle, srcMetadata.m_metaHandle, disjoint);
    CHX_ASSERT(CDKResultSuccess == result);
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiMetadata::Clone
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
ChiMetadata* ChiMetadata::Clone()
{
    ChiMetadata*      pDstMetadata = NULL;
    CHIMETADATAHANDLE hDstHandle;

    CDKResult result = m_metadataOps.pClone(m_metaHandle, &hDstHandle);
    CHX_ASSERT(CDKResultSuccess == result);

    if (CDKResultSuccess == result)
    {
        ChiMetadata* pDstMetadata = CHX_NEW ChiMetadata;
        if (NULL != pDstMetadata)
        {
            pDstMetadata->m_metadataOps = m_metadataOps;
            pDstMetadata->m_type        = ChiMetadataUsage::Generic;
            pDstMetadata->m_metaHandle  = hDstHandle;
        }
        else
        {
            m_metadataOps.pDestroy(hDstHandle, TRUE);
        }
    }
    return pDstMetadata;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiMetadata::TranslateToCameraPartialMetadata
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ChiMetadata::TranslateToCameraPartialMetadata(
    camera_metadata*            pDstCameraMetadata,
    UINT32*                     pPartial,
    UINT32                      partialCount)
{
    std::unordered_set<UINT32>::iterator partialIdIterator;
    ChiMetadataEntry                     chiMetaEntry = {0};
    INT                                  status;
    camera_metadata_entry_t              cameraMetaEntry;

    CDKResult result   = CDKResultSuccess;
    UINT      timeInMs = 0;

#ifdef __ENABLE_META_PROFILING__
    auto t0 = CMBTime::now();
#endif

    for(UINT i = 0; i < partialCount; i++)
    {
        result = GetTag(pPartial[i], chiMetaEntry);
        if (CDKResultSuccess == result && NULL != chiMetaEntry.pTagData)
        {
            if (0 == find_camera_metadata_entry(pDstCameraMetadata, chiMetaEntry.tagID, &cameraMetaEntry))
            {
                camera_metadata_entry_t updatedEntry;

                status = update_camera_metadata_entry(
                    pDstCameraMetadata,
                    cameraMetaEntry.index,
                    chiMetaEntry.pTagData,
                    chiMetaEntry.count,
                    &updatedEntry);
            }
            else
            {
                status = add_camera_metadata_entry(
                    pDstCameraMetadata,
                    chiMetaEntry.tagID,
                    chiMetaEntry.pTagData,
                    chiMetaEntry.count);
            }

            if (0 != status)
            {
                CHX_LOG_ERROR("[CMB_ERROR] Update failed for tag %x count %d capacity %zd",
                              chiMetaEntry.tagID, chiMetaEntry.count,
                              get_camera_metadata_entry_capacity(pDstCameraMetadata));
                break;
            }
        }
        else
        {
            CHX_LOG_META_DBG("SKIPPING Camera Entry!");
        }
    }

#ifdef __ENABLE_META_PROFILING__
    auto t1 = CMBTime::now();
    timeInMs = chrono::duration_cast<chrono::milliseconds>(t1 - t0).count();
#endif

    CHX_LOG_META_PROF("[CMB_PROF] Time to translate partial Meta data:%llu ms for a count:%u partialCount:%d status:%d this:%p",
        timeInMs,
        (INT32)get_camera_metadata_entry_count(pDstCameraMetadata),
        partialCount,
        result,
        this);

    return CDKResultSuccess;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiMetadata::TranslateToCameraMetadata
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ChiMetadata::TranslateToCameraMetadata(
    camera_metadata*  pDstCameraMetadata,
    bool              frameworkTagsOnly,
    bool              filterProperties,
    UINT32            filterTagCount,
    UINT32*           pFilterTagArray)
{
    UINT status   = 0;
    UINT timeInMs = 0;

#ifdef __ENABLE_META_PROFILING__
    auto t0 = CMBTime::now();
#endif

    m_metadataOps.pFilter(m_metaHandle,
                          pDstCameraMetadata,
                          frameworkTagsOnly,
                          filterProperties,
                          filterTagCount,
                          pFilterTagArray);

#ifdef __ENABLE_META_PROFILING__
    auto t1  = CMBTime::now();
    timeInMs = chrono::duration_cast<chrono::milliseconds>(t1 - t0).count();
#endif

    CHX_LOG_META_PROF("[CMB_PROF] Time to translate %llu ms count %u %d status %d",
        timeInMs,
        (INT32)get_camera_metadata_entry_count(pDstCameraMetadata),
        Count(), status);

    return CDKResultSuccess;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiMetadata::DumpDetailsToFile
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID ChiMetadata::DumpDetailsToFile(
    const CHAR* pFilename)
{
    CDKResult result = m_metadataOps.pDump(m_metaHandle, pFilename);
    CHX_ASSERT(CDKResultSuccess == result);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiMetadata::PrintDetails
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID ChiMetadata::PrintDetails()
{
    CDKResult result = m_metadataOps.pPrint(m_metaHandle);
    CHX_ASSERT(CDKResultSuccess == result);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiMetadata::Invalidate
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ChiMetadata::Invalidate()
{
    CDKResult result = m_metadataOps.pInvalidate(m_metaHandle);
    CHX_ASSERT(CDKResultSuccess == result);
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiMetadata::ReferenceCount
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UINT32 ChiMetadata::ReferenceCount()
{
    UINT32 refCount  = InvalidRefCount;
    CDKResult result = m_metadataOps.pReferenceCount(m_metaHandle, &refCount);
    CHX_ASSERT(CDKResultSuccess == result);
    return refCount;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiMetadata::AddReference
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UINT32 ChiMetadata::AddReference(
    const CHAR* pClientName)
{
    UINT32 refCount = InvalidRefCount;
    CDKResult result = m_metadataOps.pAddReference(m_metaHandle, m_metadataClientId, &refCount);
    CHX_ASSERT(CDKResultSuccess == result);

    if ((NULL != pClientName) && (CDKResultSuccess == result))
    {
        for (UINT32 index = 0; index < MaxChiMetaClients; ++index)
        {
            if (NULL == m_clientName[index])
            {
                m_clientName[index] = pClientName;
                break;
            }
        }
        CHX_LOG_META_DBG("[CMB_DEBUG_REF] add clientName %s handle %p", pClientName, m_metaHandle);
    }

    if (ChiMetadataManager::InvalidClientId != m_metadataManagerClientId)
    {
        m_timePoint = CMBTime::now();
    }

    return refCount;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiMetadata::ReleaseReference
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UINT32 ChiMetadata::ReleaseReference(
    const CHAR* pClientName)
{
    UINT32 refCount = InvalidRefCount;
    CDKResult result = m_metadataOps.pReleaseReference(m_metaHandle, m_metadataClientId, &refCount);
    CHX_ASSERT(CDKResultSuccess == result);

    if ((NULL != pClientName) && (CDKResultSuccess == result))
    {
        for (UINT32 index = 0; index < MaxChiMetaClients; ++index)
        {
            if (pClientName == m_clientName[index])
            {
                m_clientName[index] = NULL;
                break;
            }
        }
        CHX_LOG_META_DBG("[CMB_DEBUG_REF] release  %s handle %p", pClientName, m_metaHandle);
    }

    return refCount;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiMetadata::ReleaseAllReferences
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ChiMetadata::ReleaseAllReferences(
    BOOL bCHIAndCamXReferences)
{
    CDKResult result = m_metadataOps.pReleaseAllReferences(m_metaHandle, bCHIAndCamXReferences);
    CHX_ASSERT(CDKResultSuccess == result);
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiMetadata::Iterator::Iterator
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
ChiMetadata::Iterator::Iterator(
    ChiMetadata& metadata)
    : m_iterator(NULL)
    , m_metadata(metadata)
{
    CDKResult result = metadata.m_metadataOps.pIteratorCreate(metadata.GetHandle(), &m_iterator);
    CHX_ASSERT(CDKResultSuccess == result);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiMetadata::Iterator::~Iterator
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
ChiMetadata::Iterator::~Iterator()
{
    CDKResult result = m_metadata.m_metadataOps.pIteratorDestroy(m_iterator);
    CHX_ASSERT(CDKResultSuccess == result);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiMetadata::Iterator::Begin
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool ChiMetadata::Iterator::Begin()
{
    CDKResult result = m_metadata.m_metadataOps.pIteratorBegin(m_iterator);
    return (CDKResultSuccess == result);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiMetadata::Iterator::Next
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool ChiMetadata::Iterator::Next()
{
    CDKResult result = m_metadata.m_metadataOps.pIteratorNext(m_iterator);
    return (CDKResultSuccess == result);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiMetadata::Iterator::Get
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool ChiMetadata::Iterator::Get(
    ChiMetadataEntry& entry)
{
    CDKResult result = m_metadata.m_metadataOps.pIteratorGetEntry(m_iterator, &entry);
    return (CDKResultSuccess == result);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ChiMetadata::DumpAndroidMetadata
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID ChiMetadata::DumpAndroidMetadata(
    const camera_metadata* pAndroidMeta,
    const CHAR*            pFilename)
{
    if ((NULL != pAndroidMeta) && (NULL != pFilename))
    {
#if defined (_LINUX)
        INT32 fd = ::open(pFilename, O_RDWR | O_CREAT, 0666);
        if (0 < fd)
        {
            dump_camera_metadata(pAndroidMeta, fd, 2);

            ::close(fd);
        }
#endif
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiMetadataManager::MetaClient::MetaClient
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
ChiMetadataManager::MetaClient::MetaClient()
    : m_type(ChiMetadataUsage::Generic)
    , m_prevBufIndex(InvalidIndex)
{
    m_bufferList.reserve(ChxMaxMetadataBuffers);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiMetadataManager::MetaClient::~MetaClient
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
ChiMetadataManager::MetaClient::~MetaClient()
{
    if (NULL != m_pPartialTag)
    {
        CHX_DELETE[] m_pPartialTag;
        m_pPartialTag = NULL;
    }

    ReleaseBuffers();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ChiMetadataManager::TrackBuffers
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID ChiMetadataManager::MetaClient::TrackBuffers(
    CMBTimePoint& currentTimePoint,
    UINT32        timeoutValue)
{
    if (ChxInfiniteTimeout != timeoutValue)
    {
        // find a free buffer
        for (UINT32 bufIndex = 0; bufIndex < m_bufferList.size(); ++bufIndex)
        {
            MetaHolder& holder      = m_bufferList[bufIndex];
            UINT32      elapsedTime = chrono::duration_cast<chrono::milliseconds>(
                currentTimePoint - holder.pMetadata->m_timePoint).count();

            if (!holder.isFree &&
                (0 != holder.pMetadata->ReferenceCount()) &&
                (elapsedTime > timeoutValue))
            {
                CHX_LOG_META_DBG("[CMB_TRACK] Unreleased metadata %d from client %u elapsed time %u ms"
                    " timeout %u ms frameNumber %u handle %p %p max ref count %u",
                    bufIndex,
                    m_clientIndex,
                    elapsedTime,
                    timeoutValue,
                    holder.pMetadata->m_metadataClientId.frameNumber,
                    holder.pMetadata,
                    holder.pMetadata->m_metaHandle,
                    holder.pMetadata->ReferenceCount());

                // print necessary info
                holder.pMetadata->PrintDetails();
            }
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiMetadataManager::MetaClient::SubClient::SubClient
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
ChiMetadataManager::MetaClient::SubClient::SubClient()
    : pipelineId(0)
    , isUsed(false)
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiMetadataManager::MetaClient::MetaHolder::MetaHolder
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
ChiMetadataManager::MetaClient::MetaHolder::MetaHolder(
    ChiMetadata* pArgMetadata)
    : pMetadata(pArgMetadata)
    , isFree(true)
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiMetadataManager::MetaClient::ReleaseBuffers
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ChiMetadataManager::MetaClient::ReleaseBuffers()
{
    CDKResult result = CDKResultSuccess;

    CHX_LOG_META_DBG("[CMB_DEBUG] Release buffers for client %p type %d size %zd",
                     this, m_type, m_bufferList.size());

    for (UINT32 index = 0; index < m_bufferList.size(); ++index)
    {
        MetaHolder& holder = m_bufferList[index];
        if (NULL != holder.pMetadata)
        {
            CDKResult resultLocal = holder.pMetadata->DestroyInternal(TRUE);

            if (CDKResultSuccess != resultLocal)
            {
                result = resultLocal;
                CHX_LOG_ERROR("[CMB_ERROR] Cannot release buffer at index %d", index);
            }
            else
            {
                holder.pMetadata = NULL;
            }
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiMetadata::MetaClient::AllocateBuffers
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ChiMetadataManager::MetaClient::AllocateBuffers(
    UINT32* pTags,
    UINT32  tagCount,
    UINT32  partialTagCount,
    UINT32  bufferCount,
    UINT32  clientId)
{
    CDKResult result = CDKResultSuccess;

    if (0 < bufferCount)
    {
        UINT32 maxBufferCount = max(static_cast<UINT32>(m_bufferList.size()), bufferCount);

        for (UINT32 bufIndex = m_bufferList.size(); bufIndex < maxBufferCount; bufIndex++)
        {
            ChiMetadata* pInputMetadata = ChiMetadata::Create(pTags, tagCount);
            if (NULL != pInputMetadata)
            {
                pInputMetadata->m_metadataManagerClientId = clientId;
                m_bufferList.push_back(MetaHolder(pInputMetadata));
            }
            else
            {
                result = CDKResultEFailed;
                break;
            }
        }

        if (0 < partialTagCount)
        {
            m_PartialTagCount = partialTagCount;
            m_pPartialTag     = CHX_NEW UINT32[partialTagCount];
            ChxUtils::Memcpy(m_pPartialTag, pTags, sizeof(UINT32) * partialTagCount);
        }
        CHX_LOG_META_DBG("[CMB_DEBUG] Buffer count %u %u", bufferCount, maxBufferCount);
    }
    else
    {
        CHX_LOG_ERROR("[CMB_ERROR] Buffer count %u pTags %p count %u",
            bufferCount, pTags, tagCount);
        result = CDKResultEInvalidArg;
    }
    CHX_LOG_META_DBG("[CMB_DEBUG] Allocate buffers for client %p %zd", this, m_bufferList.size());

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiMetadataManager::AndroidMetadataHolder::AndroidMetadataHolder
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
ChiMetadataManager::AndroidMetadataHolder::AndroidMetadataHolder()
    : pMetadata(NULL)
    , isUsed(false)
    , isSparse(false)
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiMetadataManager::AndroidMetadataHolder::AndroidMetadataHolder
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
ChiMetadataManager::AndroidMetadataHolder::AndroidMetadataHolder(
    camera_metadata_t* pArgMetadata,
    bool               argIsUsed,
    bool               argIsEmpty)
    : pMetadata(pArgMetadata)
    , isUsed(argIsUsed)
    , isSparse(argIsEmpty)
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiMetadataManager::AndroidMetadataHolder::~AndroidMetadataHolder
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
ChiMetadataManager::AndroidMetadataHolder::~AndroidMetadataHolder()
{
    if (NULL != pMetadata)
    {
        CHX_LOG_INFO("Destroy FWO %p entry_cap %zd data_cap %zd", pMetadata,
                      get_camera_metadata_entry_capacity(pMetadata),
                      get_camera_metadata_data_capacity(pMetadata));
        ChxUtils::AndroidMetadata::FreeMetaData(pMetadata);
        pMetadata = NULL;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiMetadataManager::Create
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
ChiMetadataManager* ChiMetadataManager::Create(
    UINT32 inputFps)
{
    CDKResult           result = CDKResultSuccess;
    ChiMetadataManager* pMDM   = CHX_NEW ChiMetadataManager(0, inputFps);
    if (NULL != pMDM)
    {
        result = ChiMetadata::InitializeMetadataOps(&pMDM->m_metadataOps);
        if (CDKResultSuccess == result)
        {
            result = pMDM->Initialize();
        }
    }

    if (CDKResultSuccess != result)
    {
        CHX_DELETE pMDM;
        pMDM = NULL;
    }
    return pMDM;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiMetadataManager::Destroy
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID ChiMetadataManager::Destroy()
{
    CHX_DELETE this;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiMetadataManager::AllocateFrameworkMetadata
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ChiMetadataManager::AllocateFrameworkMetadata(
    UINT32 entryCapacity,
    UINT32 dataCapacity,
    bool   isSparse,
    UINT32 startIndex,
    UINT32 endIndex)
{
    CDKResult          result = CDKResultSuccess;
    camera_metadata_t* pMetadata;

    for (UINT32 index = startIndex; index < endIndex; ++index)
    {
        pMetadata = static_cast<camera_metadata_t*>(
            ChxUtils::AndroidMetadata::AllocateMetaData(entryCapacity, dataCapacity));

        if (NULL != pMetadata)
        {
            m_afwoMetaList[index].pMetadata = pMetadata;
            m_afwoMetaList[index].isUsed    = false;
            m_afwoMetaList[index].isSparse  = isSparse;

            CHX_LOG_META_DBG("Allocated FWO %p entry_cap %zd data_cap %zd", pMetadata,
                             get_camera_metadata_entry_capacity(pMetadata),
                             get_camera_metadata_data_capacity(pMetadata));
        }
        else
        {
            result = CDKResultENoMemory;
            break;
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiMetadataManager::Initialize
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ChiMetadataManager::Initialize()
{
    CDKResult          result = CDKResultSuccess;
    UINT32             count  = 0;

    result = m_metadataOps.pGetMetadataEntryCount(&count);
    if ((CDKResultSuccess == result) && (0 < count))
    {
        m_metadataTable.resize(count);
        result = m_metadataOps.pGetMetadataTable(m_metadataTable.data());

        if (CDKResultSuccess == result)
        {
            m_afwoMetaList.reserve(ChxMaxFWOSize);

            UINT32 listSize = ChxDefaultFWOSize + ChxDefaultFWOSparseSize;
            m_afwoMetaList.resize(listSize);

            UINT32 entryCapacity = KEntryCount;
            UINT32 dataCapacity  = KEntryCount * KEntrySize;

            result = AllocateFrameworkMetadata(KEntryCount,
                                               KEntryCount * KEntrySize,
                                               false,
                                               0,
                                               ChxDefaultFWOSize);

            if (CDKResultSuccess == result)
            {
                result = AllocateFrameworkMetadata(KEntryCountSparse,
                                                   KEntryCountSparse * KEntrySizeSparse,
                                                   true,
                                                   ChxDefaultFWOSize,
                                                   listSize);
            }
        }
    }

    if (CDKResultSuccess == result)
    {
        CHX_LOG_META_DBG("[CMB_DEBUG] Metadata manager initialized");
    }
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiMetadataManager::ChiMetadataManager
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
ChiMetadataManager::ChiMetadataManager(
    UINT32 cameraId,
    UINT32 inputFps)
    : m_stickyInput(NULL)
    , m_cameraId(cameraId)
    , m_InputFrameNumber(0)
    , m_enableTracking(false)
    , m_lastTrackedFrame(0)
    , m_inputFps(inputFps)
    , m_reuseBuffers(true)
{
    ChxUtils::Memset(&m_metadataOps, 0x0, sizeof(m_metadataOps));
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiMetadataManager::~ChiMetadataManager
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
ChiMetadataManager::~ChiMetadataManager()
{
    if (NULL != m_stickyInput)
    {
        m_stickyInput->DestroyInternal(TRUE);
        m_stickyInput = NULL;
    }

}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiMetadataManager::InitializeFrameworkInputClient
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ChiMetadataManager::InitializeFrameworkInputClient(
    UINT32 bufferCount,
    bool   bSupportMultipleInputs)
{
    CDKResult result = CDKResultSuccess;
    MetaClient& frameworkClient = m_clients[0];

    std::lock_guard<std::mutex> guard(m_lock);

    m_bUseMultipleInputs = bSupportMultipleInputs;

    if (!frameworkClient.m_isUsed || !bufferCount)
    {
        vector<UINT32> tagList(ChxMaxRequestKeys);
        UINT32 tagCount = 0;

        result = ExtensionModule::GetInstance()->GetAvailableRequestKeys(
            m_cameraId,
            tagList.data(),
            tagList.size(),
            &tagCount);

        if (CDKResultSuccess == result)
        {
            result = frameworkClient.AllocateBuffers(tagList.data(), tagCount, 0, bufferCount, 0);
            if (CDKResultSuccess == result)
            {
                frameworkClient.m_isUsed                  = true;
                frameworkClient.m_type                    = ChiMetadataUsage::FrameworkInput;
                frameworkClient.m_subClient[0].isUsed     = true;
                frameworkClient.m_subClient[0].pipelineId = 1;
            }
            else
            {
                CHX_LOG_ERROR("[CMB_ERROR] Error Allocatebuffers failed %d", result);
                result = CDKResultENoMemory;
            }
        }
        else
        {
            CHX_LOG_ERROR("[CMB_ERROR] Error GetAvailableRequestKeys failed %d", result);
            result = CDKResultENoMemory;
        }

        if (CDKResultSuccess == result)
        {
            CHIMETADATAHANDLE hStickyMetahandle;
            result = m_metadataOps.pGetDefaultMetadata(m_cameraId, &hStickyMetahandle);
            if (CDKResultSuccess == result)
            {
                m_stickyInput = CHX_NEW ChiMetadata;
                if (NULL != m_stickyInput)
                {
                    m_stickyInput->m_metadataOps = m_metadataOps;
                    m_stickyInput->m_pManager    = this;
                    m_stickyInput->m_type        = ChiMetadataUsage::FrameworkInput;
                    m_stickyInput->m_metaHandle  = hStickyMetahandle;
                }
            }
            if (NULL == m_stickyInput)
            {
                CHX_LOG_ERROR("[CMB_ERROR] Cannot create sticky input");
                result = CDKResultENoMemory;
            }
        }
    }
    else
    {
        CHX_LOG_META_DBG("[CMB_DEBUG] Framework client already initialized bufferCount %d", bufferCount);
        result = CDKResultEInvalidState;
    }

    CHX_LOG_META_DBG("[CMB_DEBUG] Framework client initialized bufferCount %d", bufferCount);

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiMetadataManager::RegisterClient
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UINT32 ChiMetadataManager::RegisterClient(
    BOOL             isExclusive,
    UINT32*          pTagList,
    UINT32           tagCount,
    UINT32           partialTagCount,
    UINT32           bufferCount,
    ChiMetadataUsage usage)
{
    return isExclusive ?
        RegisterExclusiveClient(pTagList, tagCount, partialTagCount, bufferCount, usage) :
        RegisterSharedClient(pTagList, tagCount, partialTagCount, bufferCount, usage);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiMetadataManager::IsFreeClient
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool ChiMetadataManager::IsFreeClient(
    const ChiMetadataManager::MetaClient& client)
{
    return !client.m_isUsed;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiMetadataManager::IsRealtimeClient
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool ChiMetadataManager::IsRealtimeClient(
    const ChiMetadataManager::MetaClient& client)
{
    return client.m_isUsed && client.m_type == ChiMetadataUsage::RealtimeOutput;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiMetadataManager::IsOfflineClient
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool ChiMetadataManager::IsOfflineClient(
    const ChiMetadataManager::MetaClient& client)
{
    return client.m_isUsed && client.m_type == ChiMetadataUsage::OfflineOutput;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiMetadataManager::IsFreeSubClient
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool ChiMetadataManager::IsFreeSubClient(
    const ChiMetadataManager::MetaClient::SubClient& client)
{
    return !client.isUsed;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiMetadataManager::RetrievePartialTag
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UINT32* ChiMetadataManager::RetrievePartialTags(
    UINT clientId)
{
    UINT32* pPartialTag;
    UINT32  index = clientId & 0xFFFF;

    if(ChxMaxMetadataClients <= clientId)
    {
        pPartialTag = NULL;
    }
    else
    {
        MetaClient& client = m_clients[index];
        pPartialTag = client.m_pPartialTag;
    }

    return pPartialTag;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiMetadataManager::RetrievePartialTag
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UINT32 ChiMetadataManager::RetrievePartialTagCount(
    UINT clientId)
{
    UINT32 partialCount;
    UINT32  index = clientId & 0xFFFF;

    if(ChxMaxMetadataClients <= clientId)
    {
        partialCount = 0;
    }
    else
    {
        MetaClient& client = m_clients[index];
        partialCount = client.m_PartialTagCount;
    }

    return partialCount;
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiMetadataManager::RegisterRealtimeClient
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UINT32 ChiMetadataManager::RegisterExclusiveClient(
    UINT32*          pTagList,
    UINT32           tagCount,
    UINT32           partialTagCount,
    UINT32           bufferCount,
    ChiMetadataUsage usage)
{
    UINT32 index;
    UINT32 clientId = InvalidClientId;

    std::lock_guard<std::mutex> guard(m_lock);

    for (index = 1; index < m_clients.size(); ++index)
    {
        if (!m_clients[index].m_isUsed)
        {
            break;
        }
    }

    if (index < m_clients.size())
    {
        MetaClient& client = m_clients[index];

        client.m_isUsed              = true;
        client.m_type                = usage;
        client.m_subClient[0].isUsed = true;
        client.m_isShared            = false;

        if (bufferCount > ChxMaxMetadataBuffers)
        {
            client.m_bufferList.reserve(bufferCount);
        }

        CDKResult result = client.AllocateBuffers(pTagList, tagCount, partialTagCount, bufferCount, index);

        if (CDKResultSuccess == result)
        {
            clientId = index;

            client.m_clientIndex = index;
        }
    }

    if (InvalidClientId == clientId)
    {
        CHX_LOG_ERROR("Register failed index %u", index);
    }
    else
    {
        CHX_LOG_META_DBG("[CMB_DEBUG] Register success index %u id %x %d usage %d",
                         index, clientId, bufferCount, usage);

    }
    return clientId;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiMetadataManager::RegisterOfflineClient
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UINT32 ChiMetadataManager::RegisterSharedClient(
    UINT32*          pTagList,
    UINT32           tagCount,
    UINT32           partialTagCount,
    UINT32           bufferCount,
    ChiMetadataUsage usage)
{
    UINT32 index    = ChxMaxMetadataClients;
    UINT32 subIndex = ChxMaxMetadataClients;
    UINT32 clientId = InvalidClientId;

    vector<UINT32> tagVector;

    std::lock_guard<std::mutex> guard(m_lock);

    auto clientIt = std::find_if(m_clients.begin()+1, m_clients.end(),
        [](const ChiMetadataManager::MetaClient& client) -> bool
        {
            return client.m_isUsed && client.m_isShared;
        }
    );

    if (clientIt != m_clients.end())
    {
        index = std::distance(m_clients.begin(), clientIt);

        auto subClientIt = std::find_if(clientIt->m_subClient.begin(), clientIt->m_subClient.end(), IsFreeSubClient);
        if (subClientIt != clientIt->m_subClient.end())
        {
            subIndex = std::distance(clientIt->m_subClient.begin(), subClientIt);
        }
    }
    else
    {
        clientIt = std::find_if(m_clients.begin()+1, m_clients.end(), IsFreeClient);

        if (clientIt != m_clients.end())
        {
            clientIt->m_isUsed   = true;
            clientIt->m_isShared = true;
            clientIt->m_type     = usage;
            index = std::distance(m_clients.begin(), clientIt);
            subIndex = 0;

            clientIt->m_clientIndex = index;

            clientIt->m_bufferList.reserve(ChxMaxOfflineMetadataBuffers);
        }
    }

    if ((ChxMaxMetadataClients > index) && (ChxMaxMetadataClients > subIndex))
    {
        UINT32 metaBufferClientId = (subIndex << 16) | index;

        // update taglist first
        for (UINT32 index = 0; index < tagCount; ++index)
        {
            clientIt->m_tagSet.insert(pTagList[index]);
        }
        tagVector.assign(clientIt->m_tagSet.begin(), clientIt->m_tagSet.end());

        CDKResult result = clientIt->AllocateBuffers(tagVector.data(),
                                                     tagVector.size(),
                                                     partialTagCount,
                                                     bufferCount,
                                                     metaBufferClientId);

        if (CDKResultSuccess == result)
        {
            clientId = metaBufferClientId;
            clientIt->m_subClient[subIndex].isUsed = true;
        }
    }

    if (InvalidClientId == clientId)
    {
        CHX_LOG_ERROR("Register failed index %u", index);
    }
    else
    {
        CHX_LOG_META_DBG("[CMB_DEBUG] Register success index %u id %x %d", index, clientId, bufferCount);
    }
    return clientId;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiMetadataManager::UnregisterClient
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ChiMetadataManager::UnregisterClient(
    UINT32  clientId)
{
    CDKResult result   = CDKResultEInvalidArg;
    UINT32    index    = clientId & 0xFFFF;
    UINT32    subIndex = clientId >> 16;

    std::lock_guard<std::mutex> guard(m_lock);

    if ((ChxMaxMetadataClients > index) && (0 < index) && (ChxMaxMetadataClients > subIndex))
    {
        m_clients[index].m_isUsed = false;
        m_clients[index].m_subClient[subIndex].isUsed = false;
        result = CDKResultSuccess;
    }
    else
    {
        CHX_LOG_ERROR("[CMB_ERROR] Unregister failed clientId %u", clientId);
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiMetadataManager::Flush
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ChiMetadataManager::Flush()
{
    std::lock_guard<std::mutex> guard(m_lock);

    for (UINT32 index = 0; index < m_clients.size(); ++index)
    {
        MetaClient& client = m_clients[index];

        if (client.m_isUsed)
        {
            std::lock_guard<std::mutex> bufferGuard(client.m_lock);

            for (UINT32 bufIndex = 0; bufIndex < client.m_bufferList.size(); ++bufIndex)
            {
                MetaClient::MetaHolder& holder = client.m_bufferList[bufIndex];

                holder.pMetadata->ReleaseAllReferences(TRUE);

                holder.isFree = true;
            }
        }
    }

    return CDKResultSuccess;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiMetadataManager::SetPipelineId
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ChiMetadataManager::SetPipelineId(
    UINT32 clientId,
    UINT32 pipelineId)
{
    CDKResult result   = CDKResultEInvalidArg;
    UINT32    index    = clientId & 0xFFFF;
    UINT32    subIndex = clientId >> 16;

    std::lock_guard<std::mutex> guard(m_lock);

    if ((ChxMaxMetadataClients > index) && (0 < index) && (ChxMaxMetadataClients > subIndex))
    {
        if (m_clients[index].m_subClient[subIndex].isUsed)
        {
            m_clients[index].m_subClient[subIndex].pipelineId = pipelineId;
            result = CDKResultSuccess;
        }
        else
        {
            CHX_LOG_ERROR("SetPipelineId failed clientId %u subclient not used", clientId);
        }
    }
    else
    {
        CHX_LOG_WARN("SetPipelineId failed clientId %u", clientId);
    }
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiMetadataManager::GetFreeHolder
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
ChiMetadata* ChiMetadataManager::GetFreeHolder(
    UINT32        index,
    UINT32        subIndex,
    UINT32        frameNumber,
    CMBTimePoint& timePoint)
{
    ChiMetadata*   pMetadata = NULL;
    UINT32         clientId = (subIndex << 16) | index;
    UINT32         bufIndex;
    vector<UINT32> tagVector;

    MetaClient& client = m_clients[index];
    if (client.m_isUsed && client.m_subClient[subIndex].isUsed)
    {
        std::lock_guard<std::mutex> guard(client.m_lock);

        // find a free buffer
        for (bufIndex = 0; bufIndex < client.m_bufferList.size(); ++bufIndex)
        {
            MetaClient::MetaHolder& holder = client.m_bufferList[bufIndex];
            if (holder.isFree)
            {
                pMetadata = holder.pMetadata;
                holder.isFree = false;
                break;
            }
        }

        if (NULL == pMetadata)
        {
            // check if a pending free buffer is released?
            for (bufIndex = 0; bufIndex < client.m_bufferList.size(); ++bufIndex)
            {
                MetaClient::MetaHolder& holder = client.m_bufferList[bufIndex];
                UINT32 refCount = holder.pMetadata->ReferenceCount();

                if (!refCount)
                {
                    pMetadata     = holder.pMetadata;
                    holder.isFree = false;

                    CHX_LOG_META_DBG("[CMB_DEBUG] Zero ref %d from client %u subclient %u frameNumber %d"
                                     " handle %p %p",
                                     bufIndex,
                                     index,
                                     subIndex,
                                     pMetadata->m_metadataClientId.frameNumber,
                                     pMetadata,
                                     pMetadata->m_metaHandle);
                    break;
                }
            }
        }

        // on-demand allocation
        if ((NULL == pMetadata) && (client.m_bufferList.capacity() > client.m_bufferList.size()))
        {
            tagVector.assign(client.m_tagSet.begin(), client.m_tagSet.end());
            bufIndex = client.m_bufferList.size();

            CHX_LOG_META_DBG("[CMB_DEBUG] Allocate on demand %x %u", clientId, bufIndex);

            CDKResult result = client.AllocateBuffers(tagVector.data(),
                                                      tagVector.size(),
                                                      0,
                                                      client.m_bufferList.size()+1,
                                                      clientId);
            if (CDKResultSuccess == result)
            {
                client.m_bufferList[bufIndex].isFree = false;
                pMetadata = client.m_bufferList[bufIndex].pMetadata;
            }
        }

        if ((NULL == pMetadata) && m_reuseBuffers && (0 < client.m_bufferList.size()))
        {
            CMBTimePoint minTimePoint = client.m_bufferList[0].pMetadata->m_timePoint;

            bufIndex = 0;

            for (UINT32 index = 1; index < client.m_bufferList.size(); ++index)
            {
                MetaClient::MetaHolder& holder = client.m_bufferList[index];

                if (holder.pMetadata->m_timePoint < minTimePoint)
                {
                    minTimePoint = holder.pMetadata->m_timePoint;
                    bufIndex = index;
                }
            }

            pMetadata = client.m_bufferList[bufIndex].pMetadata;

            pMetadata->ReleaseAllReferences(TRUE);
            pMetadata->Invalidate();

            CHX_LOG_META_DBG("[CMB_DEBUG] Reuse buffer %d from client %u subclient %u frameNumber %d"
                " handle %p %p",
                bufIndex,
                index,
                subIndex,
                pMetadata->m_metadataClientId.frameNumber,
                pMetadata,
                pMetadata->m_metaHandle);
        }

        if (NULL != pMetadata)
        {
            // found the buffer
            UINT32 packedIndex = index | (subIndex << 3); // 7 real time and 16 offline
            CHX_ASSERT(index < 7);
            CHX_ASSERT(subIndex < 16);
            pMetadata->m_metadataClientId.clientIndex = packedIndex;
            pMetadata->m_metadataClientId.frameNumber = frameNumber;
            pMetadata->AddReference();

            // update previous index
            client.m_prevBufIndex = bufIndex;

            // update time point
            pMetadata->m_timePoint = timePoint;

            CHX_LOG_META_DBG("[CMB_DEBUG] Get Buffer index %d from client %d subclient %u frameNumber %d handle %p %p",
                             bufIndex,
                             index,
                             subIndex,
                             pMetadata->m_metadataClientId.frameNumber,
                             pMetadata,
                             pMetadata->m_metaHandle);
        }
        else
        {
            CHX_LOG_ERROR("[CMB_ERROR] Cannot get metadata client %d subclient %d frameNumber %d",
                index, subIndex, frameNumber);
        }
    }
    else
    {
        CHX_LOG_ERROR("[CMB_ERROR] Cannot get metadata client %d subclient %d frameNumber %d",
            index, subIndex, frameNumber);
    }
    return pMetadata;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiMetadataManager::Get
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
ChiMetadata* ChiMetadataManager::Get(
    UINT32 clientId,
    UINT32 frameNumber)
{
    UINT32    index    = clientId & 0xFFFF;
    UINT32    subIndex = clientId >> 16;

    CMBTimePoint  timePoint = CMBTime::now();
    ChiMetadata*  pMetadata = NULL;

    if ((ChxMaxMetadataClients > index) && (0 < index) && (ChxMaxMetadataClients > subIndex))
    {
        pMetadata = GetFreeHolder(index, subIndex, frameNumber, timePoint);
    }
    else
    {
        CHX_LOG_ERROR("[CMB_ERROR] Cannot get metadata index %d subindex %d frameNumber %d",
            index, subIndex, frameNumber);
    }

    CHX_ASSERT(NULL != pMetadata);
    return pMetadata;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiMetadataManager::RestoreNonstickyMetadata
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID ChiMetadataManager::RestoreNonstickyMetadata(
    const camera_metadata_t* pFrameworkInput,
    ChiMetadata*             pInputMetadata)
{
    static const UINT32 nonStickyTagsList[] = { ANDROID_JPEG_GPS_COORDINATES,
                                                ANDROID_JPEG_GPS_PROCESSING_METHOD,
                                                ANDROID_JPEG_GPS_TIMESTAMP };
    UINT32              nonStickytagCount   = 0;
    UINT32              nonStickyTags[3];

    for (UINT numtags = 0; numtags < sizeof(nonStickyTagsList)/sizeof(UINT32); numtags++)
    {
        camera_metadata_entry_t entry = { 0 };
        entry.tag = nonStickyTagsList[numtags];

        if (0 != find_camera_metadata_entry(const_cast<camera_metadata_t*>(pFrameworkInput), entry.tag, &entry))
        {
            nonStickyTags[nonStickytagCount++] = nonStickyTagsList[numtags];
        }
    }

    for (UINT32 numtags = 0; numtags < nonStickytagCount; numtags++)
    {
        pInputMetadata->DeleteTag(nonStickyTags[numtags]);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiMetadataManager::Get
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
ChiMetadata* ChiMetadataManager::GetInput(
    const camera_metadata_t* pFrameworkInput,
    UINT32                   frameNumber,
    bool                     bUseSticky,
    bool                     bReuseBuffers)
{
    ChiMetadata* pMetadata = NULL;
    CDKResult    result    = CDKResultSuccess;
    CMBTimePoint timePoint = CMBTime::now();

    MetaClient& frameworkClient = m_clients[0];

    if (frameworkClient.m_isUsed)
    {
        UINT32& prevIndex = m_clients[0].m_prevBufIndex;

        // use the previous Input meta reference, if buffers are reusable and
        // 1. Input from framework is NULL for curent frame number (or)
        // 2. GetInput called more than once for the same frame number
        if (((NULL == pFrameworkInput) ||
            (frameNumber == m_InputFrameNumber)) &&
            (bReuseBuffers) &&
            (InvalidIndex != prevIndex) &&
            (0 < m_clients[0].m_bufferList[prevIndex].pMetadata->ReferenceCount()))
        {
            pMetadata = m_clients[0].m_bufferList[prevIndex].pMetadata;
            pMetadata->AddReference();
            pMetadata->m_timePoint = timePoint;

            CHX_LOG_META_DBG("[CMB_DEBUG] Get Buffer index %d from client 0 subclient 0 "
                             "frameNumber %u orig %u handle %p %p",
                             prevIndex,
                             frameNumber,
                             pMetadata->m_metadataClientId.frameNumber,
                             pMetadata,
                             pMetadata->m_metaHandle);

        }
        else
        {
            pMetadata = GetFreeHolder(0, 0, frameNumber, timePoint);

            if ((NULL != pMetadata) && (NULL != pFrameworkInput))
            {
                if (bUseSticky)
                {
                    result = m_stickyInput->SetAndroidMetadata(pFrameworkInput);

                    if (CDKResultSuccess == result)
                    {
                        result = pMetadata->Copy(*m_stickyInput);
                    }
                }
                else
                {
                    result = pMetadata->SetAndroidMetadata(pFrameworkInput);
                }
            }
            else if ((NULL != pMetadata) && bUseSticky)
            {
                result = pMetadata->Copy(*m_stickyInput);
            }
        }

        if (NULL == pMetadata)
        {
            result = CDKResultEFailed;
        }

        if (CDKResultSuccess == result)
        {
#if __DUMP_META__
            char filename[256];
            CdkUtils::SNPrintF(filename, sizeof(filename), "request_%u_b%d.txt",
                frameNumber,
                m_clients[0].m_prevBufIndex);
            pMetadata->DumpDetailsToFile(filename);

            CdkUtils::SNPrintF(filename, sizeof(filename), "sticky_%u_b%d.txt",
                frameNumber,
                m_clients[0].m_prevBufIndex);
            m_stickyInput->DumpDetailsToFile(filename);

            if (NULL != pFrameworkInput)
            {
                CdkUtils::SNPrintF(filename, sizeof(filename), "%s/settings_%u_b%d.txt",
                    FileDumpPath,
                    frameNumber,
                    m_clients[0].m_prevBufIndex);
                ChiMetadata::DumpAndroidMetadata(pFrameworkInput, filename);
            }
#endif
            m_InputFrameNumber = frameNumber;
            RestoreNonstickyMetadata(pFrameworkInput, pMetadata);
        }
        else
        {
            result = CDKResultEFailed;
            CHX_LOG_ERROR("[CMB_ERROR] Cannot set tags for frame %u copy failed", frameNumber);
        }
    }
    else
    {
        result = CDKResultEFailed;
        CHX_LOG_ERROR("[CMB_ERROR] Cannot get metadata for frame %u %d", frameNumber, m_InputFrameNumber);
    }

    if (m_enableTracking)
    {
        TrackBuffers(timePoint);
    }

    CHX_ASSERT(NULL != pMetadata);
    return pMetadata;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiMetadataManager::GetMetadataFromHandle
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
ChiMetadata* ChiMetadataManager::GetMetadataFromHandle(
    CHIMETADATAHANDLE hMetaHandle)
{
    ChiMetadata* pMetadata = NULL;

    CDKResult result = m_metadataOps.pGetPrivateData(hMetaHandle, reinterpret_cast<VOID**>(&pMetadata));

    if (CDKResultSuccess != result)
    {
        CHX_LOG_WARN("[CMB_ERROR] Cannot get metadata for handle %p", hMetaHandle);
    }

    return pMetadata;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiMetadataManager::ReleaseHolder
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ChiMetadataManager::ReleaseHolder(
    UINT32       index,
    UINT32       subIndex,
    ChiMetadata* pMetadata)
{
    CDKResult result = CDKResultEInvalidArg;

    MetaClient& client = m_clients[index];

    std::lock_guard<std::mutex> guard(client.m_lock);

    for (UINT32 bufIndex = 0; bufIndex < client.m_bufferList.size(); ++bufIndex)
    {
        MetaClient::MetaHolder& holder = client.m_bufferList[bufIndex];
        if (holder.pMetadata == pMetadata)
        {
            UINT32 refCount = pMetadata->ReleaseReference();

            // invalidate the buffer irrespective of refcount
            pMetadata->Invalidate();
            if (!refCount)
            {
                result = pMetadata->Invalidate();
                holder.isFree = true;
            }

            CHX_LOG_META_DBG("[CMB_DEBUG] Release buffer %d client %d subclient %d, frameNum %d refCount %d handle %p %p",
                            bufIndex,
                            index,
                            subIndex,
                            pMetadata->m_metadataClientId.frameNumber,
                            refCount,
                            pMetadata,
                            pMetadata->m_metaHandle);

            result = CDKResultSuccess;
        }
    }
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiMetadataManager::Release
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ChiMetadataManager::Release(
    ChiMetadata* pMetadata)
{
    CDKResult result    = CDKResultEInvalidArg;
    UINT32    clientId  = InvalidClientId;
    if (NULL != pMetadata)
    {
        clientId = pMetadata->GetClientId();
    }
    UINT32    index     = clientId & 0xFFFF;
    UINT32    subIndex  = clientId >> 16;

    if ((ChxMaxMetadataClients > index) && (ChxMaxMetadataClients > subIndex))
    {
        result = ReleaseHolder(index, subIndex, pMetadata);
    }
    else if ((NULL != pMetadata) && (ChiMetadataManager::InvalidClientId == clientId))
    {
        pMetadata->Invalidate();
    }
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiMetadataManager::ReleaseHolderByHandle
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ChiMetadataManager::ReleaseHolderByHandle(
    UINT32            index,
    UINT32            subIndex,
    CHIMETADATAHANDLE hMetadataHandle)
{
    CDKResult result = CDKResultEInvalidArg;

    MetaClient& client = m_clients[index];

    std::lock_guard<std::mutex> guard(client.m_lock);

    for (UINT32 bufIndex = 0; bufIndex < client.m_bufferList.size(); ++bufIndex)
    {
        MetaClient::MetaHolder& holder = client.m_bufferList[bufIndex];
        if (holder.pMetadata->m_metaHandle == hMetadataHandle)
        {
            UINT32 refCount = holder.pMetadata->ReleaseReference();

            // invalidate the buffer irrespective of refcount
            holder.pMetadata->Invalidate();
            if (!refCount)
            {
                holder.isFree = true;
            }

            CHX_LOG_META_DBG("[CMB_DEBUG] Release buffer %d client %d subclient %d, frameNum %d refCount %d handle %p %p",
                bufIndex,
                index,
                subIndex,
                holder.pMetadata->m_metadataClientId.frameNumber,
                refCount,
                holder.pMetadata,
                holder.pMetadata->m_metaHandle);

            result = CDKResultSuccess;
        }
    }
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiMetadataManager::GetAndroidFrameworkOutputMetadata
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
camera_metadata_t* ChiMetadataManager::GetAndroidFrameworkOutputMetadata(
   bool sparseMetadata)
{
    camera_metadata_t* pAndMetadata = NULL;
    UINT32             index;

    std::lock_guard<std::mutex> guard(m_afwomLock);
    // find a free entry
    for (index = 0; index < m_afwoMetaList.size(); ++index)
    {
        AndroidMetadataHolder& holder = m_afwoMetaList[index];
        if ((sparseMetadata == holder.isSparse) && !holder.isUsed)
        {
            CHX_ASSERT(NULL != holder.pMetadata);
            pAndMetadata = holder.pMetadata;
            m_afwoMetaList[index].isUsed = true;
            break;
        }
    }

    if ((NULL == pAndMetadata) && (m_afwoMetaList.capacity() > m_afwoMetaList.size()))
    {
        UINT32 entryCapacity;
        UINT32 dataCapacity;
        if (!sparseMetadata)
        {
            entryCapacity = KEntryCount;
            dataCapacity  = KEntryCount * KEntrySize;
        }
        else
        {
            entryCapacity = KEntryCountSparse;
            dataCapacity  = KEntryCountSparse * KEntrySizeSparse;
        }

        camera_metadata_t *pMetadata = static_cast<camera_metadata_t*>(
            ChxUtils::AndroidMetadata::AllocateMetaData(entryCapacity, dataCapacity));

        if (NULL != pMetadata)
        {
            m_afwoMetaList.push_back(AndroidMetadataHolder());
            m_afwoMetaList[index].pMetadata = pMetadata;
            m_afwoMetaList[index].isUsed    = true;
            m_afwoMetaList[index].isSparse  = sparseMetadata;
        }

        pAndMetadata = pMetadata;
    }

    if (NULL == pAndMetadata)
    {
        CHX_LOG_ERROR("[CMB_ERROR] Cannot get metadata for index %d", index);
    }

    CHX_LOG_META_DBG("[CMB_DEBUG_AFWO] Get framework output index %u address %p sparse %d", index,
                                                                                            pAndMetadata,
                                                                                            sparseMetadata);

    return pAndMetadata;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiMetadataManager::ReleaseAndroidFrameworkOutputMetadata
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ChiMetadataManager::ReleaseAndroidFrameworkOutputMetadata(
    const camera_metadata_t* pMetadata)
{
    UINT32    index;
    CDKResult result = CDKResultSuccess;

    std::lock_guard<std::mutex> guard(m_afwomLock);

    for (index = 0; index < m_afwoMetaList.size(); ++index)
    {
        if (pMetadata == m_afwoMetaList[index].pMetadata)
        {
            break;
        }
    }

    if (m_afwoMetaList.size() > index)
    {
        ChxUtils::AndroidMetadata::ResetMetadata(m_afwoMetaList[index].pMetadata);
        m_afwoMetaList[index].isUsed = false;
    }
    else
    {
        result = CDKResultENoSuch;
        CHX_LOG_ERROR("[CMB_ERROR] Cannot release metadata %p", pMetadata);
    }

    CHX_LOG_META_DBG("[CMB_DEBUG_AFWO] Release framework output index %u", index);
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiMetadataManager::PrintAllBuffers
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ChiMetadataManager::PrintAllBuffers(
    UINT32       index,
    UINT32       subIndex)
{
    CDKResult result   = CDKResultSuccess;
    MetaClient& client = m_clients[index];

    std::lock_guard<std::mutex> guard(client.m_lock);

    for (UINT32 bufIndex = 0; bufIndex < client.m_bufferList.size(); ++bufIndex)
    {
        MetaClient::MetaHolder& holder = client.m_bufferList[bufIndex];

        string allClients("");
        for (UINT32 index = 0; index < ChiMetadata::MaxChiMetaClients; ++index)
        {
            if (NULL != holder.pMetadata->m_clientName[index])
            {
                allClients += string(holder.pMetadata->m_clientName[index]) + string(";");
            }
        }

        holder.pMetadata->PrintDetails();

        CHX_LOG_INFO("[CMB_TRACK] Print meta %d client %d subclient %d, frameNum %d refCount %d"
                     " allClients %s handle chi %p %p",
                     bufIndex,
                     index,
                     subIndex,
                     holder.pMetadata->m_metadataClientId.frameNumber,
                     holder.pMetadata->ReferenceCount(),
                     allClients.c_str(),
                     holder.pMetadata,
                     holder.pMetadata->m_metaHandle);
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiMetadataManager::PrintAllBuffers
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ChiMetadataManager::PrintAllBuffers(
    UINT32       clientId)
{
    CDKResult result   = CDKResultEInvalidArg;
    UINT32    index    = clientId & 0xFFFF;
    UINT32    subIndex = clientId >> 16;

    if ((ChxMaxMetadataClients > index) && (ChxMaxMetadataClients > subIndex))
    {
        result = PrintAllBuffers(index, subIndex);
    }
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ChiMetadataManager::TrackBuffers
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID ChiMetadataManager::TrackBuffers(
    CMBTimePoint& currentTimePoint)
{
    if ((m_InputFrameNumber - m_lastTrackedFrame) > ChxTrackerFrameDelta)
    {
        for (UINT32 index = 0; index < m_clients.size(); ++index)
        {
            MetaClient& client = m_clients[index];

            if (client.m_isUsed)
            {
                client.TrackBuffers(currentTimePoint, ChxMaxMetaHoldTime);
            }
        }

        m_lastTrackedFrame = m_InputFrameNumber;
    }
}
