////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2016-2020 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  camximagesensormoduledatamanager.cpp
/// @brief Image sensor module data manager class implementation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "camxincs.h"
#include "camxmem.h"
#include "camxhal3defs.h"
#include "camxhwcontext.h"
#include "camxhwenvironment.h"
#include "camxcslsensordefs.h"
#include "camxpropertyblob.h"
#include "camxmetadatapool.h"
#include "camxsdksensordriver.h"
#include "camxflashdata.h"
#include "imagesensormodulesetmanager.h"
#include "camximagesensordata.h"
#include "camxactuatordata.h"
#include "camxoisdata.h"
#include "camxpdafdata.h"
#include "camxpdafconfig.h"
#include "camximagesensordata.h"
#include "camximagesensormoduledata.h"
#include "camximagesensormoduledatamanager.h"

CAMX_NAMESPACE_BEGIN

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ImageSensorModuleDataManager::Create
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult ImageSensorModuleDataManager::Create(
    ImageSensorModuleDataManager** ppManager,
    HwEnvironment*                 pEnv)
{
    CamxResult result = CamxResultEInvalidArg;
    if (NULL != ppManager)
    {
        ImageSensorModuleDataManager* pManager = CAMX_NEW ImageSensorModuleDataManager();
        if (NULL != pManager)
        {
            pManager->m_pEnv = pEnv;
            result = pManager->Initialize();
            if (CamxResultSuccess == result)
            {
                *ppManager = pManager;
            }
            else
            {
                CAMX_LOG_ERROR(CamxLogGroupSensor, "Failed to initialize ImageSensorModuleDataManager");
                pManager->Destroy();
                pManager = NULL;
            }
        }
    }
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ImageSensorModuleDataManager::Destroy
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID ImageSensorModuleDataManager::Destroy()
{
    Uninitialize();
    CAMX_DELETE this;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ImageSensorModuleDataManager::GetNumberOfImageSensorModuleData
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UINT ImageSensorModuleDataManager::GetNumberOfImageSensorModuleData()
{
    return m_numberOfDataObjs;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ImageSensorModuleDataManager::GetImageSensorModuleData
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
ImageSensorModuleData* ImageSensorModuleDataManager::GetImageSensorModuleData(
    UINT index
    ) const
{
    ImageSensorModuleData* pData = NULL;

    if (index < m_numberOfDataObjs)
    {
        pData = m_ppDataObjs[index];
    }

    return pData;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ImageSensorModuleDataManager::GetSensorModuleManagerObj
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult ImageSensorModuleDataManager::GetSensorModuleManagerObj(
    const CHAR*                   pBinName,
    ImageSensorModuleSetManager** ppSensorModuleSetManager)
{
    CamxResult                   result                  = CamxResultSuccess;
    ImageSensorModuleSetManager* pSensorModuleSetManager = NULL;
    FILE*                        pFile                   = NULL;
    UCHAR*                       pBuffer                 = NULL;

    pFile = OsUtils::FOpen(pBinName, "rb");
    if (pFile != NULL)
    {
        UINT64 fileSizeBytes = OsUtils::GetFileSize(pBinName);
        CAMX_ASSERT(0 != fileSizeBytes);

        pBuffer = static_cast<BYTE*>(CAMX_CALLOC(static_cast<SIZE_T>(fileSizeBytes)));
        if (NULL == pBuffer)
        {
            CAMX_LOG_ERROR(CamxLogGroupSensor, "Cannot allocate buffer of length %d", fileSizeBytes);
            result = CamxResultENoMemory;
        }
        else
        {
            BOOL loadResult         = TRUE;
            pSensorModuleSetManager = CAMX_NEW ImageSensorModuleSetManager();
            UINT64 sizeRead         = OsUtils::FRead(pBuffer,
                                                     static_cast<SIZE_T>(fileSizeBytes),
                                                     1,
                                                     static_cast<SIZE_T>(fileSizeBytes),
                                                     pFile);
            CAMX_ASSERT(fileSizeBytes == sizeRead);

            if (NULL != pSensorModuleSetManager)
            {
                loadResult = pSensorModuleSetManager->LoadBinaryParameters(pBuffer, fileSizeBytes);
                if (FALSE == loadResult)
                {
                    CAMX_LOG_WARN(CamxLogGroupSensor, "error %s, binfile: %s", pSensorModuleSetManager->Error, pBinName);
                    CAMX_DELETE pSensorModuleSetManager;
                    pSensorModuleSetManager = NULL;
                    result = CamxResultEFailed;
                }
            }
            else
            {
                CAMX_LOG_ERROR(CamxLogGroupSensor, "error binfile: %s pSensorModuleSetManager is NULL", pBinName);
                result = CamxResultEInvalidPointer;
            }
        }

        if (NULL != pBuffer)
        {
            // Delete the buffer
            CAMX_FREE(pBuffer);
            pBuffer = NULL;
        }

        // Close the file handle
        OsUtils::FClose(pFile);
    }
    else
    {
        CAMX_LOG_ERROR(CamxLogGroupSensor, "Cannot open the file %s", pBinName);
        result = CamxResultENoSuch;
    }

    *ppSensorModuleSetManager = pSensorModuleSetManager;

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ImageSensorModuleDataManager::CreateAllSensorModuleSetManagers
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult ImageSensorModuleDataManager::CreateAllSensorModuleSetManagers()
{
    CamxResult                   result                  = CamxResultSuccess;
    ImageSensorModuleSetManager* pSensorModuleSetManager = NULL;
    UINT16                       fileCount               = 0;
    UINT16                       fileCountMM             = 0;
    CHAR                         binaryFiles[MaxSensorModules][FILENAME_MAX];

    if (-1 != m_pEnv->m_eebinDeviceIndex)
    {
        CAMX_LOG_INFO(CamxLogGroupSensor, "EEBin device exists : %d", m_pEnv->m_eebinDeviceIndex);
        EEbinData* pEEBinDataObj = CAMX_NEW EEbinData(m_pEnv->m_eebinDeviceIndex, 0);
        if (NULL != pEEBinDataObj)
        {
            result = pEEBinDataObj->ReadEEBin();
            if (CamxResultSuccess != result)
            {
                CAMX_LOG_ERROR(CamxLogGroupSensor, "FATAL!!!: EEBin data read failed. Fail camera open!!!");
                CAMX_DELETE pEEBinDataObj;
                return result;
            }
            else
            {
                fileCountMM = pEEBinDataObj->GetEEBinModules(&binaryFiles[0][0], FILENAME_MAX);
                CAMX_DELETE pEEBinDataObj;
            }
        }
        else
        {
            CAMX_LOG_ERROR(CamxLogGroupSensor, "Failed to create EEBin data");
        }
    }
    else
    {
        CAMX_LOG_INFO(CamxLogGroupSensor, "No EEBin device exists : %d", m_pEnv->m_eebinDeviceIndex);
    }

    if (CamxResultSuccess == result)
    {
        fileCount = OsUtils::GetFilesFromPath(SensorModulesPath,
                                              FILENAME_MAX,
                                              &binaryFiles[fileCountMM][0],
                                              "*",
                                              "sensormodule",
                                              "*",
                                              "*",
                                              "bin");

        CAMX_LOG_INFO(CamxLogGroupSensor, "fileCount: %d, fileCountMM: %d", fileCount, fileCountMM);

        fileCount = fileCount + fileCountMM;

        CAMX_LOG_INFO(CamxLogGroupSensor, "total:%d", fileCount);

        CAMX_ASSERT((fileCount != 0) && (fileCount < MaxSensorModules));

        if ((fileCount == 0) || (fileCount >= MaxSensorModules))
        {
            CAMX_LOG_ERROR(CamxLogGroupSensor, "Invalid fileCount", fileCount);
            result = CamxResultEFailed;
        }
        else
        {
            for (UINT i = 0; i < fileCount; i++)
            {
                CAMX_LOG_VERBOSE(CamxLogGroupSensor, "%d = %s", i, &binaryFiles[i][0]);
                result = GetSensorModuleManagerObj(&binaryFiles[i][0], &pSensorModuleSetManager);
                if (CamxResultSuccess == result)
                {
                    m_pSensorModuleManagers[m_numSensorModuleManagers++] = pSensorModuleSetManager;
                }
                else
                {
                    CAMX_LOG_WARN(CamxLogGroupSensor,
                                   "GetSensorModuleManagerObj failed i: %d binFile: %s",
                                   i,
                                   &binaryFiles[i][0]);
                }

                CAMX_ASSERT(m_numSensorModuleManagers > 0);
                if (0 == m_numSensorModuleManagers)
                {
                    CAMX_LOG_ERROR(CamxLogGroupSensor, "Invalid number of sensor module managers");
                    result = CamxResultEFailed;
                }
            }
        }
    }

    return result;
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ImageSensorModuleDataManager::SortSensorDataObjects
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID ImageSensorModuleDataManager::SortSensorDataObjects()
{
    UINT tmpPosition0;
    UINT tmpPosition1;
    ImageSensorModuleData* pTmpImageSensorModuleData = NULL;

    CAMX_LOG_VERBOSE(CamxLogGroupSensor, "number of sensor data objects:%d", m_numberOfDataObjs);

    for (UINT i = 0; i < m_numberOfDataObjs; i++)
    {
        for (UINT j = 0; j< m_numberOfDataObjs - i - 1; j++)
        {
            m_ppDataObjs[j]->GetCameraPosition(&tmpPosition0);
            m_ppDataObjs[j+1]->GetCameraPosition(&tmpPosition1);
            if (tmpPosition0 > tmpPosition1)
            {
                pTmpImageSensorModuleData = m_ppDataObjs[j];
                m_ppDataObjs[j]           = m_ppDataObjs[j + 1];
                m_ppDataObjs[j + 1]       = pTmpImageSensorModuleData;
            }
        }
    }
    for (UINT i = 0; i < m_numberOfDataObjs; i++)
    {
        m_ppDataObjs[i]->GetCameraPosition(&tmpPosition0);
        CAMX_LOG_VERBOSE(CamxLogGroupSensor, "index:%d, position:%d", i, tmpPosition0);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ImageSensorModuleDataManager::Initialize
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult ImageSensorModuleDataManager::Initialize()
{
    CamxResult result = CamxResultEFailed;

    result = CreateAllSensorModuleSetManagers();

    if (CamxResultSuccess == result)
    {
        CAMX_LOG_INFO(CamxLogGroupSensor, "Image sensor moduleCount = %d", m_numSensorModuleManagers);

        m_ppDataObjs = static_cast<ImageSensorModuleData**>(
            CAMX_CALLOC(sizeof(ImageSensorModuleData*) * m_numSensorModuleManagers));

        if (NULL != m_ppDataObjs)
        {
            for (UINT index = 0; index < m_numSensorModuleManagers; index++)
            {
                ImageSensorModuleDataCreateData createData = {0};

                createData.pSensorModuleManagerObj = m_pSensorModuleManagers[index];
                createData.pDataManager            = this;
                result                             = ImageSensorModuleData::Create(&createData);

                if (CamxResultSuccess == result)
                {
                    CAMX_LOG_VERBOSE(CamxLogGroupSensor, "ImageSensorModuleData created");

                    m_ppDataObjs[index] = createData.pImageSensorModuleData;
                    m_numberOfDataObjs++;
                }
            }

            SortSensorDataObjects();
        }
        else
        {
            result = CamxResultENoMemory;
        }
    }
    else
    {
        CAMX_LOG_ERROR(CamxLogGroupSensor, "CreateAllSensorModuleSetManagers failed");
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ImageSensorModuleDataManager::DeleteEEBinFiles
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult ImageSensorModuleDataManager::DeleteEEBinFiles()
{
    CamxResult result       = CamxResultSuccess;
    UINT16     fileCountMM  = 0;
    CHAR       files[MaxSensorModules][FILENAME_MAX];

    fileCountMM = OsUtils::GetFilesFromPath(MmSensorModulesPath,
                                            FILENAME_MAX,
                                            &files[0][0],
                                            "*",
                                            "sensormodule",
                                            "*",
                                            "*",
                                            "bin");

    for (UINT16 count = 0; count < fileCountMM; count++)
    {
        result = OsUtils::FDelete(&files[count][0]);
        if (CamxResultSuccess == result)
        {
            CAMX_LOG_VERBOSE(CamxLogGroupSensor, "Deleted %s", &files[count][0]);
        }
        else
        {
            CAMX_LOG_ERROR(CamxLogGroupSensor, "Failed to Delete %s", &files[count][0]);
            break;
        }
    }

    fileCountMM = OsUtils::GetFilesFromPath(MmSensorModulesPath,
                                            FILENAME_MAX,
                                            &files[0][0],
                                            "*",
                                            "sensor",
                                            "*",
                                            "*",
                                            SharedLibraryExtension);

    for (UINT16 count = 0; count < fileCountMM; count++)
    {
        result = OsUtils::FDelete(&files[count][0]);
        if (CamxResultSuccess == result)
        {
            CAMX_LOG_VERBOSE(CamxLogGroupSensor, "Deleted %s", &files[count][0]);
        }
        else
        {
            CAMX_LOG_ERROR(CamxLogGroupSensor, "Failed to Delete %s", &files[count][0]);
            break;
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ImageSensorModuleDataManager::Uninitialize
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID ImageSensorModuleDataManager::Uninitialize()
{
    if (NULL != m_ppDataObjs)
    {
        for (UINT i = 0; i < m_numberOfDataObjs; i++)
        {
            m_ppDataObjs[i]->Destroy();
            m_ppDataObjs[i] = NULL;
        }

        m_numberOfDataObjs = 0;

        CAMX_FREE(m_ppDataObjs);

        m_ppDataObjs = NULL;
    }

    // Destroy the sensor module manager objects
    for (UINT8 i = 0; i < m_numSensorModuleManagers; i++)
    {
        CAMX_DELETE m_pSensorModuleManagers[i];
        m_pSensorModuleManagers[i] = NULL;
    }
}

CAMX_NAMESPACE_END
