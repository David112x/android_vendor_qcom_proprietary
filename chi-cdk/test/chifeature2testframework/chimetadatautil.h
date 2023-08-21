////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2018-2020 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  chimetadatautil.h
/// @brief Declarations for chimetadatautil.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef CHIMETADATAUTIL_H
#define CHIMETADATAUTIL_H

#include "camxcdktypes.h"
#include "chi.h"
#include "chimodule.h"
#include "chxmetadata.h"

#include <string>
#include <sstream>

const std::string inputMetaPath     = "/data/vendor/camera/";
const std::string inputMetaBinPath  = "/data/vendor/camera/metadata/";

class ChiMetadataUtil
{
public:
    ChiMetadataUtil() = default;
    ~ChiMetadataUtil() = default;

    static ChiMetadataUtil*  GetInstance();
    static VOID  DestroyInstance();
    CDKResult ChiModuleInitialize();

    CHIMETADATAOPS GetMetadataOps();

    /* Methods to help test the metadataops */
    CDKResult CreateMetabuffer(CHIMETAHANDLE* pChiMetaHandle);
    CDKResult DestroyMetabuffer(CHIMETAHANDLE pChiMetaHandle);
    CDKResult DestroyMetabuffer(ChiMetadata* pMetadata);
    CDKResult CreateMetabufferWithAndroidMetadata(CHIMETAHANDLE* pChiMetaHandle);
    CDKResult CreateMetabufferWithTagList(const UINT32*  pTagList,
        UINT32         tagListCount,
        CHIMETAHANDLE* pMetaHandle);
    VOID CreateChiRationalIdentityMatrix(CHIRATIONAL* matrix);
    CDKResult SetTag(CHIMETAHANDLE pChiMetaHandle);
    CDKResult GetTag(CHIMETAHANDLE pChiMetaHandle, UINT32 tagId, VOID** tagContent);
    CDKResult GetPipelineinfo(CHIHANDLE hChiContext,
        CHIHANDLE                   hSession,
        CHIPIPELINEDESCRIPTOR       hPipelineDescriptor,
        CHIPIPELINEMETADATAINFO*    pPipelineMetadataInfo);

    CDKResult CreateDefaultMetadata(int cameraId, ChiMetadata** ppChiMetadata);

    CDKResult CopyMetaBuffer(CHIMETAHANDLE& pSourceMetaHandle, CHIMETAHANDLE& pDestinationMetaHandle);
    CDKResult CopyMetaBufferDisjoint(CHIMETAHANDLE& pDestinationMetaHandle, CHIMETAHANDLE& pSourceMetaHandle);
    CDKResult DeleteTagFromMetaBuffer(CHIMETAHANDLE& pSourceMetaHandle, UINT32 tagId);
    CDKResult MergeMetaBuffer(CHIMETAHANDLE& pDestinationMetaHandle, CHIMETAHANDLE& pSourceMetaHandle);
    CDKResult MergeMetaBufferDisjoint(CHIMETAHANDLE& pDestinationMetaHandle, CHIMETAHANDLE& pSourceMetaHandle);
    CDKResult CloneMetaBuffer(CHIMETAHANDLE pSourceMetaHandle, CHIMETAHANDLE* pDestinationMetaHandle);
    bool CompareSizeOfTagList(CHIMETAHANDLE& pDestinationMetaHandle, CHIMETAHANDLE& pSourceMetaHandle);
    bool CheckMetadataOps(CHIMETADATAOPS* metadataOps);

    CDKResult VerifyCaptureResultMetadata(CHIMETAHANDLE pResultMetadata);
    CDKResult DumpMetaHandle(CHIMETAHANDLE metadata, const CHAR* fileName);
    int GetTagCount(CHIMETAHANDLE hMetaHandle, UINT32* count);
    CDKResult GetTagEntry(CHIMETAHANDLE hMetaHandle, UINT32 tagId, CHIMETADATAENTRY* tagEntry);
    CDKResult GetTagCapacity(CHIMETAHANDLE hMetaHandle, UINT32* pCapacity);

    CDKResult CreateIterator(CHIMETAHANDLE hMetaHandle, CHIMETADATAITERATOR* iterator);
    CDKResult BeginIterator(CHIMETADATAITERATOR iterator);
    CDKResult IteratorGetEntry(CHIMETADATAITERATOR iterator, CHIMETADATAENTRY* metadataEntry);
    CDKResult NextIterator(CHIMETADATAITERATOR iterator);
    CDKResult DestroyIterator(CHIMETADATAITERATOR iterator);
    CDKResult AddReference(CHIMETAHANDLE hMetahandle, CHIMETADATACLIENTID clientId, UINT32* pCount);
    CDKResult ReleaseReference(CHIMETAHANDLE hMetahandle, CHIMETADATACLIENTID clientId, UINT32* pCount);
    CDKResult ReleaseAllReferences(CHIMETAHANDLE hMetahandle, bool bCHIAndCAMXReferences);
    CDKResult GetReferenceCount(CHIMETAHANDLE hMetahandle, UINT32* pCount);
    CDKResult InvalidateTags(CHIMETAHANDLE hMetaHandle);
    CDKResult CreateRandomMetadata(CHIMETAHANDLE* hMetaHandle);
    CDKResult GetMetadataTable(CHIMETADATAENTRY* hMetadataTable);
    CDKResult GetVendorTag(CHIMETAHANDLE hMetaHandle, const CHAR* sectionName, const CHAR* pTagName, VOID** ppData);
    CDKResult SetVendorTag(CHIMETAHANDLE hMetaHandle, const CHAR* pSectionName, const CHAR* pTagName, const VOID* pData, UINT32 count);
    CDKResult GetVendorTagEntry(CHIMETAHANDLE hMetaHandle, const CHAR* pSectionName, const CHAR*   pTagName, CHIMETADATAENTRY* pMetadataEntry);
    bool CompareMetaBuffers(CHIMETAHANDLE hDefaultMetaHandle, CHIMETAHANDLE hMetaHandle);
    CDKResult PrintMetaHandle(CHIMETAHANDLE metadata);
    CDKResult CreateInputMetabufferPool(int poolSize, const CHAR* pFileName = NULL, bool multiInput = FALSE); // Circular buffer to hold input metabuffers
    CDKResult PatchingMetabufferPool(const CHAR* pFileName, bool multiFrame = FALSE);
    CDKResult PatchingMetabufferPool(const CHAR* pSection, const CHAR* pTag, const VOID* pData, UINT32 count);
    CDKResult PatchingMetabufferPool(UINT tagId, const VOID* pData, UINT32 count);
    CDKResult DeleteTagFromMetabufferPool(const CHAR* pSection, const CHAR* pTag);
    CDKResult PatchingMetabufferPoolStats(const CHAR* pStatsVendorTagSection);
    CDKResult CreateOutputMetabufferPool(int poolSize);     // Circular buffer to hold input metabuffers
    ChiMetadata* GetInputMetabufferFromPool(int index);
    CHIMETAHANDLE GetOutputMetabufferFromPool(int index, UINT32* tagArray, UINT32 tagCount);
    CHIMETAHANDLE GetExistingOutputMetabufferFromPool(int index, bool invalidate);
    CDKResult DestroyMetabufferPools();

private:
    ChiModule*              m_pChiModule;
    CHIMETADATAOPS          m_pMetadataOps;
    static ChiMetadataUtil* m_pMetadataUtilInstance;
    ChiMetadata**            m_pInputMetabufferPool;
    CHIMETAHANDLE*           m_pOutputMetabufferPool;
    int                      m_bufferPoolSize;
};

#endif // CHIMETADATAUTIL_H
