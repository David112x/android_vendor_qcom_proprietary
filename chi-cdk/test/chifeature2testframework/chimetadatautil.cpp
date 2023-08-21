////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2018-2020 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  chimetadatautil.cpp
/// @brief Implementation of chimetadatautil class.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "chimetadatautil.h"
#include "chifeature2test.h"
#include "camera_metadata.h"
#include "chituningmodeparam.h"
#include "cdkutils.h"

ChiMetadataUtil* ChiMetadataUtil::m_pMetadataUtilInstance = NULL;
/**************************************************************************************************************************
* ChiMetadataUtil::GetInstance
*
* @brief
*     Gets the singleton instance for ChiMetadataUtil
* @param
*     None
* @return
*     ChiModule singleton
**************************************************************************************************************************/
ChiMetadataUtil* ChiMetadataUtil::GetInstance()
{
    if (m_pMetadataUtilInstance == NULL)
    {
        m_pMetadataUtilInstance = CF2_NEW ChiMetadataUtil();

    }
    if (m_pMetadataUtilInstance != NULL)
    {
        if (m_pMetadataUtilInstance->ChiModuleInitialize() != CDKResultSuccess)
        {
            CF2_LOG_ERROR("Failed to initialize ChiModule!");
            return NULL;
        }

    }

    return m_pMetadataUtilInstance;
}

/**************************************************************************************************************************
* ChiMetadataUtil::DestroyInstance
*
* @brief
*     Destroy the singleton instance of the ChiMetadataUtil class
* @param
*     None
* @return
*     VOID
**************************************************************************************************************************/
VOID ChiMetadataUtil::DestroyInstance()
{
    if (m_pMetadataUtilInstance != NULL)
    {
        delete m_pMetadataUtilInstance;
        m_pMetadataUtilInstance = NULL;
    }
}
/**************************************************************************************************************************
* ChiMetadataUtil::ChiModuleInitialize
*
* @brief
*     Gets the ChiModule Instance and the MetadataOps struct.
* @param
*     None
* @return
*     CDKResult
**************************************************************************************************************************/
CDKResult ChiMetadataUtil::ChiModuleInitialize()
{
    if (m_pChiModule == NULL)
    {
        m_pChiModule = ChiModule::GetInstance();
    }
    if (m_pChiModule != NULL)
    {

        m_pMetadataUtilInstance->m_pChiModule->GetChiOps()->pMetadataOps(&m_pMetadataOps);
    }

    return (m_pChiModule != NULL) ? CDKResultSuccess : CDKResultEFailed;
}
/**************************************************************************************************************************
* ChiMetadataUtil::GetMetadataOps
*
* @brief
*     Gets the MetadataOps.
* @param
*     None
* @return
*     CHIMETADATAOPS
**************************************************************************************************************************/
CHIMETADATAOPS ChiMetadataUtil::GetMetadataOps()
{
    return m_pMetadataOps;

}

/**************************************************************************************************************************
* ChiMetadataUtil::CreateInputMetabufferPool
*
* @brief
*    Creates a circular buffer pool of Input metabuffers
* @param
*     [in] int poolSize                            number of buffers in the pool
* @return
*     CDKResult
**************************************************************************************************************************/
CDKResult ChiMetadataUtil::CreateInputMetabufferPool(int poolSize, const CHAR* pMetaName, bool multiInput)
{
    CDKResult result = CDKResultSuccess;

    if (m_pInputMetabufferPool == NULL)
    {
        m_bufferPoolSize = poolSize;
        m_pInputMetabufferPool = CF2_NEW ChiMetadata*[poolSize];

        if (NULL == pMetaName)
        {
            for (int poolIndex = 0; poolIndex < poolSize; poolIndex++)
            {
                result |= CreateDefaultMetadata(0, &m_pInputMetabufferPool[poolIndex]);
            }
        }
        else
        {
            for (int poolIndex = 0; poolIndex < poolSize; poolIndex++)
            {
                std::ostringstream completeFilePath;
                std::ostringstream completeBinFilePath;
                if (TRUE == multiInput)
                {
                    // Find the position where the extension starts in the metadata file name
                    std::string metadataName(pMetaName);
                    SIZE_T pos = metadataName.find("_0.bin");

                    // Erase the extension in the metadata file name as the index needs to be inserted
                    // after the file name
                    if (std::string::npos != pos)
                    {
                        metadataName.erase(pos);

                        if (std::string::npos == metadataName.find("/"))
                        {
                            completeFilePath    << inputMetaPath    << metadataName.c_str() << "_" << poolIndex << ".bin";
                            completeBinFilePath << inputMetaBinPath << metadataName.c_str() << "_" << poolIndex << ".bin";
                        }
                        else
                        {
                            completeFilePath << metadataName.c_str() << "_" << poolIndex << ".bin";
                        }
                    }
                    else
                    {
                        if (std::string::npos == metadataName.find("/"))
                        {
                            completeFilePath    << inputMetaPath    << pMetaName << "_" << poolIndex;
                            completeBinFilePath << inputMetaBinPath << pMetaName << "_" << poolIndex;
                        }
                        else
                        {
                            completeFilePath << pMetaName << "_" << poolIndex;
                        }
                    }
                }
                else
                {
                    if (NULL == strchr(pMetaName, '/'))
                    {
                        completeFilePath    << inputMetaPath    << pMetaName;
                        completeBinFilePath << inputMetaBinPath << pMetaName;
                    }
                    else
                    {
                        completeFilePath << pMetaName;
                    }
                }
                std::string  fileNameString = completeFilePath.str();
                ChiMetadata* pMetadata      = ChiMetadata::Create(fileNameString.c_str());
                if (NULL != pMetadata)
                {
                    m_pInputMetabufferPool[poolIndex] = pMetadata;
                }
                else
                {
                    // try other location
                    fileNameString = completeBinFilePath.str();
                    pMetadata      = ChiMetadata::Create(fileNameString.c_str());
                    if (NULL != pMetadata)
                    {
                        m_pInputMetabufferPool[poolIndex] = pMetadata;
                    }
                    else
                    {
                        CF2_LOG_WARN("%s not found, fill with default Metadata", fileNameString.c_str());
                        CreateDefaultMetadata(0, &m_pInputMetabufferPool[poolIndex]);
                    }
                }
            }
        }
    }
    return result;
}

/**************************************************************************************************************************
* ChiMetadataUtil::PatchingMetabufferPool
*
* @brief
*    Patching metadata buffers with tag value read from binary file
* @param
*     [in] const CHAR* pFileName                            File name to read tag value from
* @param
*     [in] bool multiFrame                                  Flag that indicates if multi-frame use case is active
* @return
*     CDKResult
**************************************************************************************************************************/
CDKResult ChiMetadataUtil::PatchingMetabufferPool(const CHAR* pFileName, bool multiFrame)
{
    CDKResult   result           = CDKResultSuccess;
    CHAR        lkgFileName[256] = { 0 };
    CHAR*       pLKGData         = NULL;

    std::string statsName(pFileName);
    std::size_t dotPosExt        = statsName.find_last_of(".");
    std::string dotfileExt       = statsName.substr(dotPosExt);
    std::size_t posOfLast_       = statsName.find_last_of("_");
    std::string nameBefore_Index = statsName.substr(0, posOfLast_);

    for (int poolIndex = 0; poolIndex < m_bufferPoolSize; poolIndex++)
    {
        std::ostringstream completeFilePath;

        if (TRUE == multiFrame)
        {
            if (std::string::npos == nameBefore_Index.find("/"))
            {
                completeFilePath << inputMetaPath << nameBefore_Index << "_" << poolIndex << dotfileExt;
            }
            else
            {
                completeFilePath << nameBefore_Index << "_" << poolIndex << dotfileExt;
            }
        }
        else
        {
            if (NULL == strchr(pFileName, '/'))
            {
                completeFilePath << inputMetaPath << pFileName;
            }
            else
            {
                completeFilePath << pFileName;
            }
        }

        std::string fileNameString = completeFilePath.str();
        FILE*       pFp            = CdkUtils::FOpen(fileNameString.c_str(), "rb");
        INT         fileLen        = 0;
        CHAR*       pData          = NULL;
        UINT        tagId          = 0;

        if (pFp)
        {
            CdkUtils::FSeek(pFp, 0, SEEK_END);
            fileLen = CdkUtils::FTell(pFp);
            CdkUtils::FSeek(pFp, 0, SEEK_SET);

            if (0 < fileLen)
            {
                pData = static_cast<CHAR*>(CHX_CALLOC(fileLen));
                if (NULL != pData)
                {
                    INT bytesRead = CdkUtils::FRead(pData, fileLen, 1, fileLen, pFp);

                    if (0 < bytesRead)
                    {
                        CdkUtils::StrLCpy(lkgFileName, fileNameString.c_str(), sizeof(lkgFileName));
                        pLKGData = pData;
                        result   = CDKResultSuccess;
                    }
                    else
                    {
                        CF2_LOG_ERROR("Zero bytes read for %s", fileNameString.c_str());
                        result = CDKResultENoMemory;
                    }
                }
                else
                {
                    CF2_LOG_ERROR("Fail to alloc %d bytes", fileLen);
                    result = CDKResultENoMemory;
                }
            }
            CdkUtils::FClose(pFp);
        }
        else if (0 != CdkUtils::StrLen(lkgFileName))
        {
            CF2_LOG_WARN("Cannot open file %s, loading %s instead", fileNameString.c_str(), lkgFileName);
            pData = pLKGData;
        }
        else
        {
            CF2_LOG_ERROR("File %s open failed!", fileNameString.c_str());
            result = CDKResultENoSuch;
        }

        if (CDKResultSuccess == result)
        {
            ChxUtils::Memcpy(&tagId, pData, sizeof(UINT));
            pData += sizeof(UINT);

            result |= m_pInputMetabufferPool[poolIndex]->SetTag(tagId, &pData, sizeof(VOID*));
        }
    }

    return result;
}

/**************************************************************************************************************************
* ChiMetadataUtil::PatchingMetabufferPool
*
* @brief
*    Patching metadata buffers
* @param
*     [in] pSection                         The tag section name
*     [in] pTag                             The tag name
*     [in] pData                            Point to the tag data
*     [in] count                            Size of metadata

* @return
*     CDKResult
**************************************************************************************************************************/
CDKResult ChiMetadataUtil::PatchingMetabufferPool(const CHAR* pSection, const CHAR* pTag, const VOID* pData, UINT32 count)
{
    CDKResult result = CDKResultSuccess;

    for (INT i = 0; i < m_bufferPoolSize; i++)
    {
        result |= m_pInputMetabufferPool[i]->SetTag(pSection, pTag, pData, count);
    }

    return result;
}

/**************************************************************************************************************************
* ChiMetadataUtil::PatchingMetabufferPool
*
* @brief
*    Patching metadata buffers
* @param
*     [in] tagId                            The tag ID
*     [in] pData                            Point to the tag data
*     [in] count                            Size of metadata

* @return
*     CDKResult
**************************************************************************************************************************/
CDKResult ChiMetadataUtil::PatchingMetabufferPool(UINT tagId, const VOID* pData, UINT32 count)
{
    CDKResult result = CDKResultSuccess;

    for (INT i = 0; i < m_bufferPoolSize; i++)
    {
        result |= m_pInputMetabufferPool[i]->SetTag(tagId, pData, count);
    }

    return result;
}

/**************************************************************************************************************************
* ChiMetadataUtil::PatchingMetabufferPoolStats
*
* @brief
*    Patching metadata buffers
* @param
*     [in] pStatsVendorTagSection                            The vendor tag section name of the stats to patch

* @return
*     CDKResult
**************************************************************************************************************************/
CDKResult ChiMetadataUtil::PatchingMetabufferPoolStats(const CHAR* pStatsVendorTagSection)
{
    CDKResult result = CDKResultSuccess;

    for (INT i = 0; i < m_bufferPoolSize; i++)
    {
        VOID* pData       = NULL;
        UINT  statsPropID = 0;

        pData = m_pInputMetabufferPool[i]->GetTag(pStatsVendorTagSection, "propertyID");
        if (NULL != pData)
        {
            statsPropID = *(static_cast<UINT*>(pData));
        }
        else
        {
            CF2_LOG_ERROR("VendorTag %s.propertyID does not exist for index %d!", pStatsVendorTagSection, i);
            result = CDKResultENoSuch;
        }

        if (CDKResultSuccess == result)
        {
            pData = m_pInputMetabufferPool[i]->GetTag(pStatsVendorTagSection, "stats");
            if (NULL != pData)
            {
                result |= m_pInputMetabufferPool[i]->SetTag(statsPropID, &pData, sizeof(VOID*));
            }
            else
            {
                CF2_LOG_ERROR("VendorTag %s.stats does not exist for index %d!", pStatsVendorTagSection, i);
                result = CDKResultENoSuch;
            }
        }
    }

    return result;
}

/**************************************************************************************************************************
* ChiMetadataUtil::DeleteTagFromMetabufferPool
*
* @brief
*    Delete tag from metadata buffers
* @param
*     [in] pSection                         The tag section name
*     [in] pTag                             The tag name

* @return
*     CDKResult
**************************************************************************************************************************/
CDKResult ChiMetadataUtil::DeleteTagFromMetabufferPool(const CHAR* pSection, const CHAR* pTag)
{
    CDKResult result = CDKResultSuccess;

    for (INT i = 0; i < m_bufferPoolSize; i++)
    {
        CHIMETAHANDLE    hMetahandle = m_pInputMetabufferPool[i]->GetHandle();
        CHIMETADATAENTRY metaEntry   = { 0 };

        result = GetVendorTagEntry(hMetahandle, pSection, pTag, &metaEntry);
        if (CDKResultSuccess == result)
        {
            result = m_pInputMetabufferPool[i]->DeleteTag(metaEntry.tagID);
        }

        if (CDKResultSuccess != result)
        {
            break;
        }
    }

    return result;
}

/**************************************************************************************************************************
* ChiMetadataUtil::GetInputMetabufferFromPool
*
* @brief
*    Returns a buffer from pool of Input metabuffers
* @param
*     [in] int index                            number of buffers in the pool
* @return
*     CHIMETAHANDLE
**************************************************************************************************************************/
ChiMetadata* ChiMetadataUtil::GetInputMetabufferFromPool(int index)
{
    if (m_pInputMetabufferPool != NULL)
    {
        return m_pInputMetabufferPool[index % m_bufferPoolSize];
    }
    return NULL;
}

/**************************************************************************************************************************
* ChiMetadataUtil::CreateOutputMetabufferPool
*
* @brief
*    Creates a circular buffer pool of Output metabuffers
* @param
*     [in] int poolSize                            number of buffers in the pool
* @return
*     CDKResult
**************************************************************************************************************************/
CDKResult ChiMetadataUtil::CreateOutputMetabufferPool(int poolSize)
{
    CDKResult result = CDKResultSuccess;
    if (m_pOutputMetabufferPool == NULL)
    {
        m_bufferPoolSize = poolSize;
        m_pOutputMetabufferPool = CF2_NEW CHIMETAHANDLE[poolSize];
        for (int poolIndex = 0; poolIndex < poolSize; poolIndex++)
        {
            m_pOutputMetabufferPool[poolIndex] = NULL;
        }
    }
    return result;
}

/**************************************************************************************************************************
* ChiMetadataUtil::GetOutputMetabufferFromPool
*
* @brief
*    Returns a buffer from pool of Output metabuffers. Invalidates an existing metabuffer or creates a new one if no
*    existing is found.
* @param
*     [in] int index                            number of buffers in the pool
*     [in] UINT32* tagArray                     list of tags queried from the pipeline
*     [in] UINT32 tagCount                      number of tags
* @return
*     CHIMETAHANDLE
**************************************************************************************************************************/
CHIMETAHANDLE ChiMetadataUtil::GetOutputMetabufferFromPool(int index, UINT32* tagArray, UINT32 tagCount)
{
    if (m_pOutputMetabufferPool != NULL)
    {
        if (m_pOutputMetabufferPool[index % m_bufferPoolSize] == NULL)
        {
            CreateMetabufferWithTagList(tagArray, tagCount, &m_pOutputMetabufferPool[index % m_bufferPoolSize]);
        }
        else
        {
            m_pMetadataUtilInstance->InvalidateTags(m_pOutputMetabufferPool[index % m_bufferPoolSize]);
        }
        return m_pOutputMetabufferPool[index % m_bufferPoolSize];
    }
    return NULL;
}

/**************************************************************************************************************************
* ChiMetadataUtil::GetExistingOutputMetabufferFromPool
*
* @brief
*    Returns an existing buffer from pool of Output metabuffers (does not try to create a new one)
* @param
*     [in] int index                            number of buffers in the pool
*     [in] bool invalidate                      invalidates metabuffer before returning it
* @return
*     CHIMETAHANDLE
**************************************************************************************************************************/
CHIMETAHANDLE ChiMetadataUtil::GetExistingOutputMetabufferFromPool(int index, bool invalidate)
{
    if (m_pOutputMetabufferPool != NULL && m_pOutputMetabufferPool[index % m_bufferPoolSize] != NULL)
    {
        if (invalidate)
        {
            m_pMetadataUtilInstance->InvalidateTags(m_pOutputMetabufferPool[index % m_bufferPoolSize]);
        }
        return m_pOutputMetabufferPool[index % m_bufferPoolSize];
    }
    return NULL;
}

/**************************************************************************************************************************
* ChiMetadataUtil::DestroyMetabufferPools
*
* @brief
*    Destroys the buffer pools of Input and Output metabuffers
* @param
*    None
* @return
*     CDKResult
**************************************************************************************************************************/
CDKResult ChiMetadataUtil::DestroyMetabufferPools()
{
    CDKResult result = CDKResultSuccess;
    if (m_pInputMetabufferPool != NULL)
    {
        for (int index = 0; index < m_bufferPoolSize; index++)
        {
            result |= DestroyMetabuffer(m_pInputMetabufferPool[index]);
        }

        delete[] m_pInputMetabufferPool;
        m_pInputMetabufferPool = NULL;
    }
    if (m_pOutputMetabufferPool != NULL)
    {
        for (int index = 0; index < m_bufferPoolSize; index++)
        {
            if(m_pOutputMetabufferPool[index] != NULL)
                result |= DestroyMetabuffer(m_pOutputMetabufferPool[index]);
        }

        delete[] m_pOutputMetabufferPool;
        m_pOutputMetabufferPool = NULL;
    }
    return result;
}

/**************************************************************************************************************************
* ChiMetadataUtil::CreateMetabuffer
*
* @brief
*     Creates the metabuffer using the pCreate function pointer in MetadataOps.
* @param
*     [out] CHIMETAHANDLE* pChiMetaHandle          pointer to the metabuffer that has to be created
* @return
*     CDKResult
**************************************************************************************************************************/
CDKResult ChiMetadataUtil::CreateMetabuffer(CHIMETAHANDLE* pChiMetaHandle)
{
    CDKResult result = CDKResultEFailed;

    if (m_pMetadataUtilInstance != NULL)
    {
        if (pChiMetaHandle != NULL)
        {
            return m_pMetadataUtilInstance->GetMetadataOps().pCreate(pChiMetaHandle, NULL);
        }
    }
    return result;
}

/**************************************************************************************************************************
* ChiMetadataUtil::DestroyMetabuffer
*
* @brief
*     Destroy the metabuffer using the pDestroy function pointer in MetadataOps.
* @param
*     [in] CHIMETAHANDLE pChiMetaHandle            metabuffer that has to be destroyed
* @return
*     CDKResult
**************************************************************************************************************************/
CDKResult ChiMetadataUtil::DestroyMetabuffer(CHIMETAHANDLE pChiMetaHandle)
{
    CDKResult result = CDKResultEFailed;
    if (m_pMetadataUtilInstance != NULL)
    {
        result = m_pMetadataUtilInstance->m_pMetadataOps.pDestroy(pChiMetaHandle, true);

    }
    return result;
}

/**************************************************************************************************************************
* ChiMetadataUtil::DestroyMetabuffer
*
* @brief
*     Destroy the metabuffer using the pDestroy function pointer in MetadataOps.
* @param
*     [in] ChiMetadata* pMetadata            metabuffer that has to be destroyed
* @return
*     CDKResult
**************************************************************************************************************************/
CDKResult ChiMetadataUtil::DestroyMetabuffer(ChiMetadata* pMetadata)
{
    CDKResult result = CDKResultEFailed;

    result = pMetadata->Destroy(true);

    return result;
}

/**************************************************************************************************************************
* ChiMetadataUtil::CreateMetabufferWithAndroidMetadata
*
* @brief
*     Create the metabuffer using the default android metadata
* @param
*     [out] CHIMETAHANDLE* pChiMetaHandle          pointer to the metabuffer that has to be created
* @return
*     CDKResult
**************************************************************************************************************************/
CDKResult ChiMetadataUtil::CreateMetabufferWithAndroidMetadata(CHIMETAHANDLE* pChiMetaHandle)
{
    CDKResult result = CDKResultEFailed;
    if (m_pMetadataUtilInstance != NULL)
    {
        CHIMETAHANDLE hAndroidDefaultMetadata;
        result = m_pMetadataUtilInstance->m_pMetadataOps.pGetDefaultAndroidMeta(0, (const VOID**)&hAndroidDefaultMetadata);
        if (result == CDKResultSuccess)
        {
            result|= m_pMetadataUtilInstance->m_pMetadataOps.pCreateWithAndroidMetadata(hAndroidDefaultMetadata, pChiMetaHandle, NULL);
        }
    }
    return result;
}

/**************************************************************************************************************************
* ChiMetadataUtil::CreateMetabufferWithTagList
*
* @brief
*     Create the metabuffer using the default android metadata
* @param
*       [in] const UINT32*   pTagList               list of tags for creating the metabuffer
*       [in] UINT32          tagListCount           count of tags sent
*       [out] CHIMETAHANDLE* pMetaHandle         created metabuffer with the provided tags
* @return
*     CDKResult
**************************************************************************************************************************/
CDKResult ChiMetadataUtil::CreateMetabufferWithTagList(const UINT32*  pTagList,
    UINT32         tagListCount,
    CHIMETAHANDLE* pMetaHandle)
{
    CDKResult result = CDKResultEFailed;
    if (m_pMetadataUtilInstance != NULL)
    {
        result = m_pMetadataUtilInstance->m_pMetadataOps.pCreateWithTagArray(pTagList, tagListCount, pMetaHandle, NULL);
    }
    return result;
}

/**************************************************************************************************************************
* ChiMetadataUtil::SetTag
*
* @brief
*     Sets the tag value in the meta buffer sent
* @param
*     [in] CHIMETAHANDLE pChiMetaHandle         metabuffer whose tags are to be set
* @return
*     CDKResult
**************************************************************************************************************************/
CDKResult ChiMetadataUtil::SetTag(CHIMETAHANDLE pChiMetaHandle)
{
    CDKResult result = CDKResultEFailed;
    CHIMETAHANDLE hAndroidDefaultMetadata;
    CHIRATIONAL colorTransformMatrix[9];

    UNUSED_PARAM(hAndroidDefaultMetadata);
    CreateChiRationalIdentityMatrix(colorTransformMatrix);

    result = m_pMetadataUtilInstance->m_pMetadataOps.pSetTag(pChiMetaHandle, ANDROID_COLOR_CORRECTION_TRANSFORM, // TODO make this function generic
        &colorTransformMatrix[0], 9);
    return result;
}

/**************************************************************************************************************************
* ChiMetadataUtil::GetTag
*
* @brief
*     Gets the requested tag value
* @param
*     [in] CHIMETAHANDLE pChiMetaHandle              metabuffer whose tags are requested
*     [in] UINT32 tagId                              tag ID
*     [out] CHIMETAHANDLE* tagContent                Pointer to tag data requested
* @return
*     CDKResult
**************************************************************************************************************************/
CDKResult ChiMetadataUtil::GetTag(CHIMETAHANDLE pChiMetaHandle, UINT32 tagId, VOID** tagContent)
{
    CDKResult result = CDKResultEFailed;
    result = m_pMetadataUtilInstance->m_pMetadataOps.pGetTag(pChiMetaHandle, tagId, tagContent);
    return result;
}

/**************************************************************************************************************************
* ChiMetadataUtil::GetPipelineinfo
*
* @brief
*     Queries the pipeline metadata info given the pipeline descriptor
* @param
*       [in] CHIHANDLE  hChiContext                                 chi context
*       [in] CHIHANDLE  hSession                                    session handle
*       [in] CHIPIPELINEDESCRIPTOR hPipelineDescriptor              pipeline handle
*       [out] CHIPIPELINEMETADATAINFO*    pPipelineMetadataInfo     the metadata info requested for the pipeline
*
* @return
*     CDKResult
**************************************************************************************************************************/
CDKResult ChiMetadataUtil::GetPipelineinfo(CHIHANDLE hChiContext,
    CHIHANDLE                   hSession,
    CHIPIPELINEDESCRIPTOR       hPipelineDescriptor,
    CHIPIPELINEMETADATAINFO*    pPipelineMetadataInfo)
{
    return m_pMetadataUtilInstance->m_pChiModule->GetChiOps()->pQueryPipelineMetadataInfo(hChiContext, hSession,
        hPipelineDescriptor, pPipelineMetadataInfo);
}

/**************************************************************************************************************************
* ChiMetadataUtil::CreateDefaultMetadata
*
* @brief
*     Creates the default metadata using the pGetDefaultAndroidMeta in MetadataOps.
* @param
*     [in]  int cameraId                        camera Id
*     [out] ChiMetadata* pChiMetadata         pointer to the metabuffer which contains the requested default metadata
* @return
*     CDKResult
**************************************************************************************************************************/
CDKResult ChiMetadataUtil::CreateDefaultMetadata(int cameraId, ChiMetadata** ppChiMetadata)
{
    CDK_UNUSED_PARAM(cameraId);
    CDKResult result = CDKResultSuccess;

    *ppChiMetadata = ChiMetadata::Create(NULL, 0, true, NULL);
    if (NULL != *ppChiMetadata)
    {
        ChiTuningModeParameter tagValue = { 0 };
        tagValue.noOfSelectionParameter = MaxTuningMode;

        tagValue.TuningMode[0].mode = ChiModeType::Default;
        tagValue.TuningMode[0].subMode.value = 0;
        tagValue.TuningMode[1].mode = ChiModeType::Sensor;
        tagValue.TuningMode[1].subMode.value = 0;
        tagValue.TuningMode[2].mode = ChiModeType::Usecase;
        tagValue.TuningMode[2].subMode.usecase = ChiModeUsecaseSubModeType::Preview;
        tagValue.TuningMode[3].mode = ChiModeType::Feature1;
        tagValue.TuningMode[3].subMode.feature1 = ChiModeFeature1SubModeType::None;
        tagValue.TuningMode[4].mode = ChiModeType::Feature2;
        tagValue.TuningMode[4].subMode.feature2 = ChiModeFeature2SubModeType::None;
        tagValue.TuningMode[5].mode = ChiModeType::Scene;
        tagValue.TuningMode[5].subMode.scene = ChiModeSceneSubModeType::None;
        tagValue.TuningMode[6].mode = ChiModeType::Effect;
        tagValue.TuningMode[6].subMode.effect = ChiModeEffectSubModeType::None;

        result |= (*ppChiMetadata)->SetTag(
                "org.quic.camera2.tuning.mode", "TuningMode", &tagValue, sizeof(ChiTuningModeParameter));
    }
    else
    {
        CF2_LOG_ERROR("Failed to create default metadata!");
        result = CDKResultEFailed;
    }

    return result;

}

/**************************************************************************************************************************
* ChiMetadataUtil::CopyMetaBuffer
*
* @brief
*     Creates the copy of metadata in the pSourceMetaHandle sent.
* @param
*     [in] CHIMETAHANDLE& pSourceMetaHandle           source metabuffer whose data is copied
*     [out] CHIMETAHANDLE& pDestinationMetaHandle     destination metabuffer which contains the copied metadata
* @return
*     CDKResult
**************************************************************************************************************************/
CDKResult ChiMetadataUtil::CopyMetaBuffer(CHIMETAHANDLE& pDestinationMetaHandle, CHIMETAHANDLE& pSourceMetaHandle)
{
    CDKResult result = CDKResultEFailed;
    if (m_pMetadataUtilInstance != NULL)
    {
        result = m_pMetadataUtilInstance->m_pMetadataOps.pCopy(pDestinationMetaHandle, pSourceMetaHandle, false);

    }
    return result;

}
/**************************************************************************************************************************
* ChiMetadataUtil::CopyMetaBufferDisjoint
*
* @brief
*     Creates the disjoint copy of metadata in the pSourceMetaHandle sent.
* @param
*    [in] CHIMETAHANDLE& pSourceMetaHandle           source metabuffer whose data is copied
*    [out] CHIMETAHANDLE& pDestinationMetaHandle     destination metabuffer which contains the copied metadata
* @return
*     CDKResult
**************************************************************************************************************************/
CDKResult ChiMetadataUtil::CopyMetaBufferDisjoint(CHIMETAHANDLE& pDestinationMetaHandle, CHIMETAHANDLE& pSourceMetaHandle)
{
    CDKResult result = CDKResultEFailed;
    if (m_pMetadataUtilInstance != NULL)
    {
        return m_pMetadataUtilInstance->m_pMetadataOps.pCopy(pDestinationMetaHandle, pSourceMetaHandle, true);

    }
    return result;

}

/**************************************************************************************************************************
* ChiMetadataUtil::DeleteTagFromMetaBuffer
*
* @brief
*    Deletes a tag from the metadata buffer sent.
* @param
*     [in] CHIMETAHANDLE& pSourceMetaHandle         metabuffer whose tag needs to be deleted
*     [in] UINT32 tagId                             tag id for the tag to be deleted
* @return
*     CDKResult
**************************************************************************************************************************/
CDKResult ChiMetadataUtil::DeleteTagFromMetaBuffer(CHIMETAHANDLE& pSourceMetaHandle, UINT32 tagId)
{
    CDKResult result = CDKResultEFailed;
    if (m_pMetadataUtilInstance != NULL)
    {
        result = m_pMetadataUtilInstance->m_pMetadataOps.pDeleteTag(pSourceMetaHandle, tagId);
    }
    return result;

}
/**************************************************************************************************************************
* ChiMetadataUtil::MergeMetaBuffer
*
* @brief
*     Performs a merge of the source and destination metadata handles.
* @param
*    [in] CHIMETAHANDLE& pSourceMetaHandle           source metabuffer whose data is merged
*    [out] CHIMETAHANDLE& pDestinationMetaHandle     destination metabuffer which contains the merged metadata
* @return
*     CDKResult
**************************************************************************************************************************/
CDKResult ChiMetadataUtil::MergeMetaBuffer(CHIMETAHANDLE& pDestinationMetaHandle, CHIMETAHANDLE& pSourceMetaHandle)
{
    CDKResult result = CDKResultEFailed;
    if (m_pMetadataUtilInstance != NULL)
    {
        return m_pMetadataUtilInstance->m_pMetadataOps.pMerge(pDestinationMetaHandle, pSourceMetaHandle, false);

    }
    return result;

}
/**************************************************************************************************************************
* ChiMetadataUtil::MergeMetaBufferDisjoint
*
* @brief
*     Performs a dijoint merge of the source and destination metadata handles.
* @param
*      [in] CHIMETAHANDLE& pSourceMetaHandle           source metabuffer whose data is merged
*      [out] CHIMETAHANDLE& pDestinationMetaHandle     destination metabuffer which contains the merged metadata
* @return
*     CDKResult
**************************************************************************************************************************/
CDKResult ChiMetadataUtil::MergeMetaBufferDisjoint(CHIMETAHANDLE& pDestinationMetaHandle, CHIMETAHANDLE& pSourceMetaHandle)
{
    CDKResult result = CDKResultEFailed;
    if (m_pMetadataUtilInstance != NULL)
    {
        result = m_pMetadataUtilInstance->m_pMetadataOps.pMerge(pDestinationMetaHandle, pSourceMetaHandle, true);

    }
    return result;
}

/**************************************************************************************************************************
* ChiMetadataUtil::CloneMetaBuffer
*
* @brief
*     Creates the clone of metadata in the pSourceMetaHandle sent.
* @param
*      [in] CHIMETAHANDLE& pSourceMetaHandle           source metabuffer whose data is to be cloned
*      [out] CHIMETAHANDLE* pDestinationMetaHandle     pointer to destination metabuffer which contains the cloned metadata
* @return
*     CDKResult
**************************************************************************************************************************/
CDKResult ChiMetadataUtil::CloneMetaBuffer(CHIMETAHANDLE pSourceMetaHandle, CHIMETAHANDLE* pDestinationMetaHandle)
{
    CDKResult result = CDKResultEFailed;
    if (m_pMetadataUtilInstance != NULL)
    {
        result = m_pMetadataUtilInstance->m_pMetadataOps.pClone(pSourceMetaHandle, pDestinationMetaHandle);

    }
    return result;

}

/**************************************************************************************************************************
* ChiMetadataUtil::CompareSizeOfTagList
*
* @brief
*     Compares the tag count in source and destination handles and returns true if equal.
* @param
*     [in] CHIMETAHANDLE& pSourceMetaHandle             source metadata
*     [in] CHIMETAHANDLE& pDestinationMetaHandle        destination metadata
* @return
*     bool
**************************************************************************************************************************/
bool ChiMetadataUtil::CompareSizeOfTagList(CHIMETAHANDLE& pDestinationMetaHandle, CHIMETAHANDLE& pSourceMetaHandle)
{
    if (m_pMetadataUtilInstance != NULL)
    {
        CDKResult result;
        UINT32 countOfTagsInDestinationHandle = 0;
        result = m_pMetadataUtilInstance->m_pMetadataOps.pCount(pDestinationMetaHandle, &countOfTagsInDestinationHandle);
        UINT32 countOfTagsInSourceHandle = 0;
        result |= m_pMetadataUtilInstance->m_pMetadataOps.pCount(pSourceMetaHandle, &countOfTagsInSourceHandle);
        if (result == CDKResultSuccess)
        {
            return countOfTagsInDestinationHandle == countOfTagsInSourceHandle;
        }
    }
    return false;

}

/**************************************************************************************************************************
* ChiMetadataUtil::VerifyCaptureResultMetadata
*
* @brief
*     Verifies the metadata in the captured result
* @param
*     [in] CHIMETAHANDLE& pResultMetadata                   result metadata buffer
* @return
*     CDKResult
**************************************************************************************************************************/
CDKResult ChiMetadataUtil::VerifyCaptureResultMetadata(CHIMETAHANDLE pResultMetadata)

{
    CDKResult result = CDKResultSuccess;
    if (m_pMetadataUtilInstance != NULL)
    {
        std::string sfileName = "ResultMetadata.txt" ;
        result = m_pMetadataUtilInstance->DumpMetaHandle(pResultMetadata, sfileName.c_str());
    }
    return result;
}

/**************************************************************************************************************************
* ChiMetadataUtil::CreateChiRationalIdentityMatrix
*
* @brief
*     Creates an identity Matrix which can be used for setting any data of type ChiRational
* @param
*     [out] CHIRATIONAL* matrix             pointer to the matrix that had be filled (1X9 mmatrix whose memory needs to be
                                            allocated by the callee)
* @return
*     VOID
**************************************************************************************************************************/
VOID ChiMetadataUtil::CreateChiRationalIdentityMatrix(CHIRATIONAL* matrix)
{
    CHIRATIONAL zero = { 0, 1 };
    CHIRATIONAL one = { 1, 1 };
    for (int i = 0; i < 9; i++)
    {

        matrix[i] = ((i == 0) || (i == 4) || (i == 8)) ? one : zero;

    }
}

/**************************************************************************************************************************
* ChiMetadataUtil::CheckMetadataOps
*
* @brief
*     Checks if the metadataops struct has no null function pointers
* @param
*     [in] CHIMETADATAOPS* metadataOps          pointer to the metadata ops structure
* @return
*     bool
**************************************************************************************************************************/
bool ChiMetadataUtil::CheckMetadataOps(CHIMETADATAOPS* metadataOps)
{
    bool result = false;
    if (metadataOps->pAddReference != NULL &&
        metadataOps->pCapacity != NULL &&
        metadataOps->pClone != NULL &&
        metadataOps->pCopy != NULL &&
        metadataOps->pCount != NULL &&
        metadataOps->pCreate != NULL &&
        metadataOps->pCreateWithAndroidMetadata != NULL &&
        metadataOps->pCreateWithTagArray != NULL &&
        metadataOps->pDeleteTag != NULL &&
        metadataOps->pDestroy != NULL &&
        metadataOps->pDump != NULL &&
        metadataOps->pFilter != NULL &&
        metadataOps->pGetDefaultAndroidMeta != NULL &&
        metadataOps->pGetDefaultMetadata != NULL &&
        metadataOps->pGetMetadataEntryCount != NULL &&
        metadataOps->pGetMetadataTable != NULL &&
        metadataOps->pGetPrivateData != NULL &&
        metadataOps->pGetTag != NULL &&
        metadataOps->pGetTagEntry != NULL &&
        metadataOps->pGetVendorTag != NULL &&
        metadataOps->pGetVendorTagEntry != NULL &&
        metadataOps->pInvalidate != NULL &&
        metadataOps->pIteratorBegin != NULL &&
        metadataOps->pIteratorCreate != NULL &&
        metadataOps->pIteratorDestroy != NULL &&
        metadataOps->pIteratorGetEntry != NULL &&
        metadataOps->pIteratorNext != NULL &&
        metadataOps->pMerge != NULL &&
        metadataOps->pPrint != NULL &&
        metadataOps->pReferenceCount != NULL &&
        metadataOps->pReleaseReference != NULL &&
        metadataOps->pSetAndroidMetadata != NULL &&
        metadataOps->pSetVendorTag != NULL &&
        metadataOps->pSetTag != NULL &&
        metadataOps->pReleaseAllReferences != NULL
        )
    {
        result = true;
    }
    return result;
}

/**************************************************************************************************************************
* ChiMetadataUtil::DumpMetaHandle
*
* @brief
*    Dumps the handle data into a file
* @param
*     [in] CHIMETAHANDLE metadata               metadata buffer to be dumped
*     [in] const CHAR* fileName                 file name for the dump
* @return
*     CDKResult
**************************************************************************************************************************/
CDKResult ChiMetadataUtil::DumpMetaHandle(CHIMETAHANDLE metadata, const CHAR* fileName)
{
    return m_pMetadataUtilInstance->GetMetadataOps().pDump(metadata, fileName);
}
/**************************************************************************************************************************
* ChiMetadataUtil::PrintMetaHandle
*
* @brief
*    Dumps the handle data into a file
* @param
*     [in] CHIMETAHANDLE metadata               metadata buffer to be printed
* @return
*     CDKResult
**************************************************************************************************************************/
CDKResult ChiMetadataUtil::PrintMetaHandle(CHIMETAHANDLE metadata)
{
    return m_pMetadataUtilInstance->GetMetadataOps().pPrint(metadata);
}
/**************************************************************************************************************************
* ChiMetadataUtil::GetTagCount
*
* @brief
*    Gets the count of tags in a metahandle
* @param
*     [in]  CHIMETAHANDLE metadata                  input metadata buffer
*     [out] UINT32* count                           pointer to the variable that holds count
* @return
*     CDKResult
**************************************************************************************************************************/
CDKResult ChiMetadataUtil::GetTagCount(CHIMETAHANDLE hMetaHandle, UINT32* count)
{
    return m_pMetadataUtilInstance->GetMetadataOps().pCount(hMetaHandle, count);
}
/**************************************************************************************************************************
* ChiMetadataUtil::GetTagEntry
*
* @brief
*    Gets the count of tags in a metahandle
* @param
*     [in]  CHIMETAHANDLE metadata                  input metadata buffer
*     [in]  UINT32 tagID                            tag ID
*     [out] CHIMETADATAENTRY* tagEntry              pointer to the tag Entry
* @return
*     CDKResult
**************************************************************************************************************************/
CDKResult ChiMetadataUtil::GetTagEntry(CHIMETAHANDLE hMetaHandle, UINT32 tagID, CHIMETADATAENTRY* tagEntry)
{
    return m_pMetadataUtilInstance->GetMetadataOps().pGetTagEntry(hMetaHandle, tagID, tagEntry);
}
/**************************************************************************************************************************
* ChiMetadataUtil::GetTagCapacity
*
* @brief
*    Gets the capacity of a metahandle
* @param
*     [in] CHIMETAHANDLE metadata                   input metadata buffer
*     [out] UINT32* pCapacity                       pointer to the variable that holds capacity
* @return
*     CDKResult

**************************************************************************************************************************/
CDKResult ChiMetadataUtil::GetTagCapacity(CHIMETAHANDLE hMetaHandle, UINT32* pCapacity)
{
    return m_pMetadataUtilInstance->GetMetadataOps().pCapacity(hMetaHandle, pCapacity);
}

/**************************************************************************************************************************
* ChiMetadataUtil::CreateIterator
*
* @brief
*    Creates the iterator for a metahandle
* @param
*     [in]  CHIMETAHANDLE metadata                  input metadata buffer
*     [out] CHIMETADATAITERATOR*  iterator          pointer to the iterator variable which is created
* @return
*     CDKResult
**************************************************************************************************************************/
CDKResult ChiMetadataUtil::CreateIterator(CHIMETAHANDLE hMetaHandle, CHIMETADATAITERATOR* iterator)
{
    return m_pMetadataUtilInstance->GetMetadataOps().pIteratorCreate(hMetaHandle, iterator);
}

/**************************************************************************************************************************
* ChiMetadataUtil::BeginIterator
*
* @brief
*    Begins the iterator for a metahandle
* @param
*     [in] CHIMETADATAITERATOR  iterator            iterator
* @return
*     CDKResult
**************************************************************************************************************************/
CDKResult ChiMetadataUtil::BeginIterator(CHIMETADATAITERATOR iterator)
{
    return m_pMetadataUtilInstance->GetMetadataOps().pIteratorBegin(iterator);
}

/**************************************************************************************************************************
* ChiMetadataUtil::IteratorGetEntry
*
* @brief
*    Gets the metadata entry the iterator points to
* @param
*      [in]  CHIMETADATAITERATOR iterator           iterator
*      [out] CHIMETADATAENTRY* metadataEntry        pointer to the metadataentry structure that needs to be filled.
* @return
*     CDKResult
**************************************************************************************************************************/
CDKResult ChiMetadataUtil::IteratorGetEntry(CHIMETADATAITERATOR iterator, CHIMETADATAENTRY* metadataEntry)
{
    return m_pMetadataUtilInstance->GetMetadataOps().pIteratorGetEntry(iterator, metadataEntry);
}

/**************************************************************************************************************************
* ChiMetadataUtil::NextIterator
*
* @brief
*    Points to the next entry in the iterator
* @param
*     [in]  CHIMETADATAITERATOR iterator            iterator
* @return
*     CDKResult
**************************************************************************************************************************/
CDKResult ChiMetadataUtil::NextIterator(CHIMETADATAITERATOR iterator)
{
    return m_pMetadataUtilInstance->GetMetadataOps().pIteratorNext(iterator);
}

/**************************************************************************************************************************
* ChiMetadataUtil::DestroyIterator
*
* @brief
*    Destroys the iterator
* @param
*     [in]  CHIMETADATAITERATOR iterator             iterator
* @return
*     CDKResult
**************************************************************************************************************************/
CDKResult ChiMetadataUtil::DestroyIterator(CHIMETADATAITERATOR iterator)
{
    return m_pMetadataUtilInstance->GetMetadataOps().pIteratorDestroy(iterator);
}

/**************************************************************************************************************************
* ChiMetadataUtil::AddReference
*
* @brief
*    Adds the reference to the metabuffer
* @param
*     [in]  CHIMETAHANDLE hMetahandle               input metadata buffer
*     [in]  CHIMETADATACLIENTID clientId            client Id associated with it
*     [out] UINT32* pCount                          pointer to the variable for reference count
* @return
*     CDKResult
**************************************************************************************************************************/
CDKResult ChiMetadataUtil::AddReference(CHIMETAHANDLE hMetahandle, CHIMETADATACLIENTID clientId, UINT32* pCount)
{
    return m_pMetadataUtilInstance->GetMetadataOps().pAddReference(hMetahandle, clientId, pCount);

}

/**************************************************************************************************************************
* ChiMetadataUtil::ReleaseReference
*
* @brief
*    Releases the reference to the metabuffer
* @param
*     [in]  CHIMETAHANDLE hMetahandle               input metadata buffer
*     [in]  CHIMETADATACLIENTID clientId            client Id associated with it
*     [out] UINT32* pCount                          pointer to the variable for reference count
* @return
*     CDKResult
**************************************************************************************************************************/
CDKResult ChiMetadataUtil::ReleaseReference(CHIMETAHANDLE hMetahandle, CHIMETADATACLIENTID clientId, UINT32* pCount)
{
    return m_pMetadataUtilInstance->GetMetadataOps().pReleaseReference(hMetahandle, clientId, pCount);
}

/**************************************************************************************************************************
* ChiMetadataUtil::ReleaseAllReferences
*
* @brief
*    Releases all the references from the metabuffer
* @param
*     [in]  CHIMETAHANDLE hMetahandle               input metadata buffer
*     [in]  bool          bCHIAndCAMXReferences     Flag to indicate whether to remove only the CHI references or both CAMX and CHI
*                                                   references. This flag should be set to TRUE only for Flush or teardown cases
* @return
*     CDKResult if successful CDK error values otherwise
**************************************************************************************************************************/
CDKResult ChiMetadataUtil::ReleaseAllReferences(CHIMETAHANDLE hMetahandle, bool bCHIAndCAMXReferences)
{
    return m_pMetadataUtilInstance->GetMetadataOps().pReleaseAllReferences(hMetahandle, bCHIAndCAMXReferences);
}

/**************************************************************************************************************************
* ChiMetadataUtil::GetReferenceCount
*
* @brief
*    Gets the reference count for the metabuffer
* @param
*     [in]  CHIMETAHANDLE hMetahandle               input metadata buffer
*     [out] UINT32*	pCount                          pointer to the variable for reference count
* @return
*     CDKResult
**************************************************************************************************************************/
CDKResult ChiMetadataUtil::GetReferenceCount(CHIMETAHANDLE hMetahandle, UINT32* pCount)
{
    return m_pMetadataUtilInstance->GetMetadataOps().pReferenceCount(hMetahandle, pCount);
}

/**************************************************************************************************************************
* ChiMetadataUtil::GetMetadataTable
*
* @brief
*    Gets the metadata Table
* @param
*   [out] CHIMETADATAENTRY* hMetadataTable            pointer to the array of metadata entry structure that's filled.
* @return
*     CDKResult
**************************************************************************************************************************/
CDKResult ChiMetadataUtil::GetMetadataTable(CHIMETADATAENTRY* hMetadataTable)
{
    return m_pMetadataUtilInstance->GetMetadataOps().pGetMetadataTable(hMetadataTable);
}

/**************************************************************************************************************************
* ChiMetadataUtil::InvalidateTags
*
* @brief
*    Invalidates tags for the metadata
* @param
*    [in] CHIMETAHANDLE hMetaHandle                    input metadata buffer
* @return
*     CDKResult
**************************************************************************************************************************/
CDKResult ChiMetadataUtil::InvalidateTags(CHIMETAHANDLE hMetaHandle)
{
    return m_pMetadataUtilInstance->GetMetadataOps().pInvalidate(hMetaHandle);
}

/**************************************************************************************************************************
* ChiMetadataUtil::CreateRandomMetadata
*
* @brief
*    Creates random values for the metadata
* @param
*    [out] CHIMETAHANDLE*   hMetaHandle                 pointer to the metadata buffer that has to be created
* @return
*     VOID
**************************************************************************************************************************/
CDKResult ChiMetadataUtil::CreateRandomMetadata(CHIMETAHANDLE* hMetaHandle)
{
    CHIMETADATAENTRY* metadataEntry = NULL;
    UINT32* tagValues = NULL;
    UINT32 pCount = 0;
    CDKResult retval = CDKResultSuccess;

    CDKResult result = m_pMetadataUtilInstance->GetMetadataOps().pGetMetadataEntryCount(&pCount);
    if (CDKResultSuccess == result)
    {
        metadataEntry = CF2_NEW CHIMETADATAENTRY[pCount];
        tagValues = CF2_NEW UINT32[pCount];

        result = m_pMetadataUtilInstance->GetMetadataTable(metadataEntry);
        if (CDKResultSuccess == result)
        {
            for (UINT32 i = 0; i < pCount; i++)
            {
                tagValues[i] = metadataEntry[i].tagID;
            }

            m_pMetadataUtilInstance->CreateMetabufferWithTagList(tagValues, pCount, hMetaHandle);

            for (UINT32 i = 0; i < pCount; i++)
            {
                CHIMETADATAENTRY tag = metadataEntry[i];
                UINT8 *tagData = NULL;
                tagData = static_cast<UINT8*>(malloc(tag.size / sizeof(UINT8)));
                if (NULL != tagData)
                {
                    for (UINT32 index = 0; index < tag.size / sizeof(UINT8); index++)
                    {
                        tagData[index] = static_cast<UINT8>(rand() % 256);
                    }
                    result = m_pMetadataUtilInstance->GetMetadataOps().pSetTag(*hMetaHandle, tag.tagID, tagData, tag.count);
                    free(tagData);
                    tagData = NULL;
                }
                else
                {
                    CF2_LOG_ERROR("Fail to alloc %u bytes!", tag.size);
                    result = CDKResultENoMemory;
                }
            }
        }
        else
        {
            CF2_LOG_ERROR("Could not get metadata table!");
            retval = CDKResultEFailed;
        }
    }
    else
    {
        CF2_LOG_ERROR("Could not get metadata entry count!");
        retval = CDKResultEFailed;
    }

    // Cleanup
    if (NULL != tagValues)
    {
        delete[] tagValues;
        tagValues = NULL;
    }
    if (NULL != metadataEntry)
    {
        delete[] metadataEntry;
        metadataEntry = NULL;
    }

    return retval;
}

/**************************************************************************************************************************
* ChiMetadataUtil::GetVendorTag
*
* @brief
*    Gets the vendor tag requested
* @param
*   [in] CHIMETAHANDLE hMetaHandle                       input metadata buffer
*   [in] const CHAR* sectionName                         section name of the vendor tag
*   [in] const CHAR* pTagName                            vendor tag name
*   [out] VOID** ppData                                  pointer to the data that is filled with vendor tag details
* @return
*     CDKResult
**************************************************************************************************************************/
CDKResult ChiMetadataUtil::GetVendorTag(CHIMETAHANDLE hMetaHandle, const CHAR* sectionName, const CHAR* pTagName, VOID** ppData)
{
    return m_pMetadataUtilInstance->GetMetadataOps().pGetVendorTag(hMetaHandle, sectionName, pTagName, ppData);
}

/**************************************************************************************************************************
* ChiMetadataUtil::SetVendorTag
*
* @brief
*    Sets the vendor tag value specified.
* @param
*   [in] CHIMETAHANDLE* hMetaHandle                      input metadata buffer
*   [in] const CHAR* pSectionName                        section name of the vendor tag
*   [in] const CHAR* pTagName                            vendor tag name
*   [in] const VOID* pData                               pointer to the data
*   [in] UINT32 count                                    tag count (see camxtitan17xcontext.cpp for what to use)
* @return
*     CDKResult
**************************************************************************************************************************/
CDKResult ChiMetadataUtil::SetVendorTag(CHIMETAHANDLE hMetaHandle, const CHAR* pSectionName, const CHAR* pTagName, const VOID* pData, UINT32 count)
{
    return m_pMetadataUtilInstance->GetMetadataOps().pSetVendorTag(hMetaHandle, pSectionName, pTagName, pData, count);
}

/**************************************************************************************************************************
* ChiMetadataUtil::GetVendorTagEntry
*
* @brief
*    Sets the vendor tag value specified.
* @param
*   [in] CHIMETAHANDLE hMetaHandle                      input metadata buffer
*   [in] const CHAR* pSectionName                       section name of the vendor tag
*   [in] const CHAR* pTagName                           vendor tag name
*   [out] CHIMETADATAENTRY* pMetadataEntry              pointer to the metadata entry structure that has requested tag details
* @return
*     CDKResult
**************************************************************************************************************************/
CDKResult ChiMetadataUtil::GetVendorTagEntry(CHIMETAHANDLE hMetaHandle, const CHAR* pSectionName, const CHAR*   pTagName, CHIMETADATAENTRY* pMetadataEntry)
{
    return m_pMetadataUtilInstance->GetMetadataOps().pGetVendorTagEntry(hMetaHandle, pSectionName, pTagName, pMetadataEntry);
}

/**************************************************************************************************************************
* ChiMetadataUtil::CompareMetaBuffers
*
* @brief
*   Compares one metadata buffer with another metadata.
* @param
*   [in] CHIMETAHANDLE hMetaHandle                      destination metadata buffer
*   [in] CHIMETAHANDLE hDefaultMetaHandle               source metadata buffer
* @return
*     bool
**************************************************************************************************************************/
bool ChiMetadataUtil::CompareMetaBuffers(CHIMETAHANDLE hDefaultMetaHandle, CHIMETAHANDLE hMetaHandle)
{
    CHIMETADATAITERATOR iterator;

    CDKResult result = CreateIterator(hDefaultMetaHandle, &iterator);

    result|= BeginIterator(iterator);

    while (result == CDKResultSuccess)
    {
        CHIMETADATAENTRY metadataEntryA;
        CHIMETADATAENTRY metadataEntryB;
        result = IteratorGetEntry(iterator, &metadataEntryA);
        result |= GetMetadataOps().pGetTagEntry(hMetaHandle, metadataEntryA.tagID, &metadataEntryB);
        if (result == CDKResultSuccess)
        {
            if ((metadataEntryA.tagID == metadataEntryB.tagID) && (0 != memcmp(metadataEntryA.pTagData, metadataEntryB.pTagData, metadataEntryA.size)))
            {
                return false;
            }
        }
        result = NextIterator(iterator);
    }
    result = DestroyIterator(iterator);
    return result == CDKResultSuccess;
}
