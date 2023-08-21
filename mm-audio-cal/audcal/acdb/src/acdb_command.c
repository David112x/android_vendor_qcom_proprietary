/*===========================================================================
FILE:           acdb_command.c

OVERVIEW:       This file contains the implementaion of the helper methods
used to service ACDB ioctls.

DEPENDENCIES:   None

Copyright (c) 2010-2020 Qualcomm Technologies, Inc.
All Rights Reserved.
Confidential and Proprietary - Qualcomm Technologies, Inc.
========================================================================== */

/*===========================================================================
EDIT HISTORY FOR MODULE

This section contains comments describing changes made to the module.
Notice that changes are listed in reverse chronological order. Please
use ISO format for dates.

$Header: //source/qcom/qct/multimedia2/Audio/audcal4/acdb_sw/dev/fluence.x/acdb/src/acdb_command.c#6 $

when        who     what, where, why
----------  ---     -----------------------------------------------------
2019-09-13 mahadevk Fixed sub-overflow failure for voice usecases
2019-08-12 Ramesh   Fixed the warnings and  sub-overflow failure
2018-05-08 mahadevk Updating ACDB SW version to 10.0.8, includes fix for heap issue
2018-05-01 Ramesh   Updating ACDB SW version to 10.0.7 and added support to query new device property
2018-04-26 mahadevk Updating ACDB software version to 10.0.6
2018-04-17 mahadevk Fixed KW errors
2018-01-25 Ramesh   WDSP API bug fixes and updated the ACDB software version to 10.0.1
2017-12-28 Ramesh   Added support for instanceID and updated ACDB software version to 10.0.0
2017-08-07 san      Updated ACDB software version to 9.0.8.
2017-02-28 mahadevk Fixed WP compilation errors and warnings observed in 9.0.5 release
2017-02-03 san      Updating ACDB software version to 9.0.4
2017-01-24 mahadevk Updating ACDB software version to 9.0.3
2016-07-12 mahadevk Updating ACDB software version to 8.0.4
2016-06-09 mahadevk Updating ACDB software version to 8.0.3
2016-06-09 mahadevk Fixed warnings messages shown in Nexus ACDB S/W compilation.
2016-06-09 mahadevk Added fix to avoid 64bit ACDB S/W crash observed using CLANG compiler
2015-09-11 NVD      Updating ACDB software version to 7.0.7
2015-08-05 san      Updating ACDB software version to 7.0.6
2015-06-29 mahadevk Updating ACDB software version to 7.0.4
2015-05-22 mahadevk Updating ACDB software version to 7.0.3
2015-05-14  san     Updating ACDB software version to 7.0.2
2014-06-09  mh      Updating ACDB software version to 4.0.5
2014-06-07  mh      Added fix to avoid compilation issue with packed data structures
2014-05-28  mh      SW migration from 32-bit to 64-bit architecture
2014-02-14  avi     Support ACDB persistence and SET APIs for AudProc, AudStrm TABLE.
2013-08-07  avi     Support Fluence VP3 interfaces
2013-05-23  mh      Updated software version minor from 5 to 6 (i.e. v2.6)
2013-07-18  mh      Added new funstion to get device property and its size
2013-06-07  avi     Support Voice Volume boost feature
2013-06-07  mh      Corrected checkpatch errors
2013-06-03  mh      Updated software version minor from 1 to 2 (i.e. v2.2)
2013-05-23  mh      Updated software version minor from 0 to 1
2010-11-16  ernanl  1. Added get/set calibration data of Afe table API
2010-09-21  ernanl  1. Device Info Data Structure changes, update ACDB to main
backward comptible with new changes.
2. ADIE calibration support.
2010-07-23  ernanl  Complete get/set function and introduce new
functions into command specific functions
2010-07-01  vmn     Split apart the global functions into command
specific functions.
2010-06-16  aas     Fixed the get table so that it returns module id,
param id and data length of each calibration unit
2010-04-26  aas     Initial implementation of the acdb_ioctl API and
associated helper methods.
2012-05-15 kpavan   1. New interface for APQ MDM device Mapping.
2. New interface to get TX RX Device pair for Recording

========================================================================== */

/* ---------------------------------------------------------------------------
* Include Files
*--------------------------------------------------------------------------- */

#include "acdb_command.h"
#include "acdb_file_mgr.h"
#include "acdb_delta_file_mgr.h"
#include "acdb_init.h"
#include "acdb_utility.h"
#include "acdb_data_mgr.h"
#include "acdb_translation.h"
#include "acdb_init_utility.h"
#include "acdb_linked_list.h"
/* ---------------------------------------------------------------------------
* Preprocessor Definitions and Constants
*--------------------------------------------------------------------------- */
#define ACDB_SOFTWARE_VERSION_MAJOR 0x0000000A
#define ACDB_SOFTWARE_VERSION_MINOR 0x00000000
#define ACDB_SOFTWARE_VERSION_REVISION 0x00000011
#define ACDB_SOFTWARE_VERSION_CPLINFO 0x00000000
#define MAX_BUFFER_LENGTH                                0x1000
#define INF 4294967295U
#define ACDB_WDSP_FWK_HEADER_SIZE 4* sizeof(uint32_t) //MID_FWK(4) + IID_FWK(2) + reserved (2) + PID_FWK(4) + param_size(2) + reserved(2)

static const uint32_t GROUP_DEVICE_ATTRIBUTE_VALUE = 1;
//static const uint32_t MAX_HEAP_DATA= 2000;

/* ---------------------------------------------------------------------------
* Type Declarations
*--------------------------------------------------------------------------- */

typedef struct _AcdbFileBufType{
   uint32_t nfile_size;
   uint8_t *pFileBuf;
}AcdbFileBufType;

typedef struct _AcdbCmdInitType {
   uint32_t nNoOfFiles;
   AcdbFileBufType acdbFileBufs[20];
} AcdbCmdInitType;

/* ---------------------------------------------------------------------------
* Global Data Definitions
*--------------------------------------------------------------------------- */
// Variable to enable/disable persistence from test framework.
// It is not expected to be used externally.
int32_t g_persistenceStatus = ACDB_UTILITY_INIT_FAILURE;
/* ---------------------------------------------------------------------------
* Static Variable Definitions
*--------------------------------------------------------------------------- */

/* ---------------------------------------------------------------------------
* Static Function Declarations and Definitions
*--------------------------------------------------------------------------- */

/* ---------------------------------------------------------------------------
* Externalized Function Declarations and Definitions
*--------------------------------------------------------------------------- */

int32_t AcdbCmdIsPersistenceSupported(uint32_t *resp)
{
   int32_t result = ACDB_SUCCESS;
   if (resp == NULL)
      return ACDB_BADPARM;
   if (ACDB_INIT_SUCCESS == AcdbIsPersistenceSupported())
   {
      *resp = TRUE;
   }
   else
   {
      *resp = FALSE;
   }

   return result;
}

void AcdbCmdEnablePersistence(void)
{
   g_persistenceStatus = ACDB_UTILITY_INIT_SUCCESS;
}

void AcdbCmdDisablePersistence(void)
{
   g_persistenceStatus = ACDB_UTILITY_INIT_FAILURE;
}

int32_t AcdbCmdInitializeAcdb(AcdbInitCmdType *pInput)
{
   int32_t result = ACDB_SUCCESS;
   int32_t persist_result = ACDB_SUCCESS;
   uint32_t i = 0;
   uint32_t avail_count = 0;
   uint32_t delta_data_count = 0;
   uint32_t delta_result = 0;
   AcdbCmdFileInfo *pCmdFileInfos = NULL;
   AcdbCmdDeltaFileInfo *pCmdDeltaFileInfos = NULL;
   uint32_t persistData = FALSE;

   if (pInput == NULL || pInput->nNoOfFiles > 20)
      return ACDB_BADPARM;

   pCmdFileInfos = (AcdbCmdFileInfo *)ACDB_MALLOC(sizeof(AcdbCmdFileInfo) * pInput->nNoOfFiles);
   if (NULL == pCmdFileInfos)
      return ACDB_INSUFFICIENTMEMORY;

   pCmdDeltaFileInfos = (AcdbCmdDeltaFileInfo *)ACDB_MALLOC(sizeof(AcdbCmdDeltaFileInfo) * pInput->nNoOfFiles);
   if (NULL == pCmdDeltaFileInfos)
   {
      ACDB_MEM_FREE(pCmdFileInfos);
      return ACDB_INSUFFICIENTMEMORY;
   }

   persist_result = AcdbCmdIsPersistenceSupported(&persistData);
   if (persist_result != ACDB_SUCCESS)
   {
      persistData = FALSE;
   }

   for (i = 0; i < pInput->nNoOfFiles; i++)
   {
      uint32_t maj=0;
      uint32_t min=0;
      uint32_t revision=0;
      uint32_t cplInfo = INF;
      uint32_t deltamaj=0;
      uint32_t deltamin=0;
      uint32_t deltaCplInfo=0;
      uint32_t deltarevision=0;
      int32_t deltainitresult = ACDB_ERROR;
      uint32_t removeDeltaFileFromMemory = FALSE;
      uint32_t deltaFileExists = FALSE;
      uint32_t nFileSize = 0;
      uint8_t *pFileBuf = NULL;
	  int32_t initresult = acdb_init(pInput->acdbFiles[i].fileName, (uint8_t **)&pCmdFileInfos[i].pFileBuf, (uint32_t *)&pCmdFileInfos[i].nFileSize, (uint32_t *)&maj, (uint32_t *)&min, (uint32_t *)&revision, (uint32_t *)&cplInfo);
      pCmdDeltaFileInfos[i].pFileBuf = NULL;
	  
      if (initresult == ACDB_INIT_SUCCESS)
      {
         ACDB_DEBUG_LOG("[ACDB Command]->SW Minor/Major/Revision version info for %s\n", pInput->acdbFiles[i].fileName);
         ACDB_DEBUG_LOG("[ACDB Command]->ACDB Sw Major = %d, ACDB Sw Minor = %d, ACDB Sw Revision = %d, ACDB Sw Cpl Number = %d\n", ACDB_SOFTWARE_VERSION_MAJOR, ACDB_SOFTWARE_VERSION_MINOR, ACDB_SOFTWARE_VERSION_REVISION, ACDB_SOFTWARE_VERSION_CPLINFO);
         if (cplInfo != INF)
         {
            ACDB_DEBUG_LOG("[ACDB Command]->ACDB File Major = %u, ACDB File Minor = %u, ACDB File Revision = %u, ACDB File Cpl Number = %u\n",maj,min,revision,cplInfo);
         }
         else
         {
            ACDB_DEBUG_LOG("[ACDB Command]->ACDB File Major = %u, ACDB File Minor = %u, ACDB File Revision = %u\n",maj,min,revision);
         }
      }

      if (ACDB_INIT_SUCCESS != initresult)
      {
         // If even one file is not valid the acdb_init should return failure.
         // So we should free the memory allocated for any of the files;
         uint8_t idx = 0;
         for (idx = 0; idx < i; idx++)
         {
            AcdbFreeFileData((void *)pCmdFileInfos[idx].pFileBuf);

            if (NULL != pCmdDeltaFileInfos[idx].pFileBuf)
            {
               ACDB_MEM_FREE((void *)pCmdDeltaFileInfos[idx].pFileBuf);
               pCmdDeltaFileInfos[idx].pFileBuf = NULL;
            }
         }
         ACDB_MEM_FREE(pCmdFileInfos);
         ACDB_MEM_FREE(pCmdDeltaFileInfos);
         return ACDB_ERROR;
      }


         //Load delta file if present.
         deltainitresult = acdb_delta_init(pInput->acdbFiles[i].fileName, pInput->acdbFiles[i].fileNameLen, &deltaFileExists, &pFileBuf, &nFileSize, &deltamaj, &deltamin, &deltarevision, &deltaCplInfo);

         pCmdDeltaFileInfos[i].deltaFileExists = deltaFileExists;
         pCmdDeltaFileInfos[i].nFileSize = nFileSize;
         pCmdDeltaFileInfos[i].pFileBuf = pFileBuf;

         removeDeltaFileFromMemory = FALSE;
         if (ACDB_INIT_SUCCESS != deltainitresult)
         {
            removeDeltaFileFromMemory = TRUE;
         }

         if (pCmdDeltaFileInfos[i].deltaFileExists == TRUE &&
            !(maj == deltamaj && min == deltamin && revision == deltarevision && (cplInfo == INF || cplInfo == deltaCplInfo)))
         {
            ACDB_DEBUG_LOG("[ACDB Command]->SW Minor/Major/Revision delta version mismatch warning %s\n", pInput->acdbFiles[i].fileName);
            ACDB_DEBUG_LOG("[ACDB Command]->ACDB File Major = %u, ACDB File Minor = %u, ACDB File Revision = %u\n", maj, min, revision);
            if (cplInfo == INF)
            {
               ACDB_DEBUG_LOG("[ACDB Command]->ACDB Delta File Major = %u, ACDB Delta File Minor = %u, ACDB Delta File Revision = %u\n", deltamaj, deltamin, deltarevision);
            }
            else
            {
               ACDB_DEBUG_LOG("[ACDB Command]->ACDB Delta File Major = %u, ACDB Delta File Minor = %u, ACDB Delta File Revision = %u, ACDB Delta File Cpl Number = %u\n", deltamaj, deltamin, deltarevision, deltaCplInfo);
            }
            removeDeltaFileFromMemory = TRUE;
         }

         if (removeDeltaFileFromMemory == TRUE)
         {
            // ignore loading this delta acdb file
            if (pCmdDeltaFileInfos[i].pFileBuf != NULL)
            {
               ACDB_MEM_FREE((void *)pCmdDeltaFileInfos[i].pFileBuf);
               pCmdDeltaFileInfos[i].pFileBuf = NULL;
            }

            pCmdDeltaFileInfos[i].deltaFileExists = FALSE;
            pCmdDeltaFileInfos[i].nFileSize = 0;
            pCmdDeltaFileInfos[i].isFileUpdated = FALSE;
         }
   }

   // Now that all the acbd files requested for initialization
   // are valid, check if acdb sw can host these man acdb files first
   // and then set the files info to the file mgr
   result = acdbdata_ioctl(ACDBDATACMD_GET_AVAILABLE_FILES_SLOT_COUNT, NULL, 0, (uint8_t *)&avail_count, sizeof(uint32_t));
   if (ACDB_SUCCESS != result || avail_count < pInput->nNoOfFiles)
   {
      uint8_t idx = 0;
      for (idx = 0; idx < pInput->nNoOfFiles; idx++)
      {
         AcdbFreeFileData((void *)pCmdFileInfos[idx].pFileBuf);
         if (pCmdDeltaFileInfos[idx].pFileBuf != NULL)
         {
            ACDB_MEM_FREE((void *)pCmdDeltaFileInfos[idx].pFileBuf);
            pCmdDeltaFileInfos[idx].pFileBuf = NULL;
         }
      }
      ACDB_MEM_FREE(pCmdFileInfos);
      ACDB_MEM_FREE(pCmdDeltaFileInfos);
      return ACDB_ERROR;
   }

   for (i = 0; i < pInput->nNoOfFiles; i++)
   {
      ACDB_MEM_CPY(&pCmdFileInfos[i].chFileName[0], sizeof(pCmdFileInfos[i].chFileName), &pInput->acdbFiles[i].fileName[0], sizeof(pInput->acdbFiles[i].fileName));
      pCmdFileInfos[i].nFileNameLen = pInput->acdbFiles[i].fileNameLen;
      if (ACDB_SUCCESS != acdbdata_ioctl(ACDBDATACMD_SET_ACDB_DATA, (uint8_t *)&pCmdFileInfos[i], sizeof(AcdbCmdFileInfo), NULL, 0))
      {
         // This failure should never occur,
         // if so free up the rest of the acdb files memory which was created for safety
         while (i < pInput->nNoOfFiles)
         {
            AcdbFreeFileData((void *)pCmdFileInfos[i].pFileBuf);
            if (pCmdDeltaFileInfos[i].pFileBuf != NULL)
            {
               ACDB_MEM_FREE((void *)pCmdDeltaFileInfos[i].pFileBuf);
               pCmdDeltaFileInfos[i].pFileBuf = NULL;
            }
            i++;
         }
         ACDB_MEM_FREE(pCmdFileInfos);
         ACDB_MEM_FREE(pCmdDeltaFileInfos);
         return ACDB_ERROR;
      }
   }

      for (i = 0; i < pInput->nNoOfFiles; i++)
      {
         AcdbDeltaFileVersion deltaVersion = { 0, 0 };
         pCmdDeltaFileInfos[i].fileIndex = i;
         pCmdDeltaFileInfos[i].nFileInfo.pDevList = NULL;
         pCmdDeltaFileInfos[i].nFileInfo.noOfDevs = 0;

         if (ACDB_SUCCESS != acdbdata_ioctl(ACDBDATACMD_GET_FILE_TYPE_INFO, (uint8_t *)&pCmdFileInfos[i].chFileName, pCmdFileInfos[i].nFileNameLen, (uint8_t *)&pCmdDeltaFileInfos[i].nFileInfo, sizeof(AcdbFileInfo)))
         {
            ACDB_DEBUG_LOG("[ACDB Command]->Could not initialize delta acdb file correctly!\n");
            ACDB_MEM_FREE(pCmdDeltaFileInfos);
            return ACDB_ERROR;
         }

         if (ACDB_SUCCESS != acdb_delta_data_ioctl(ACDBDELTADATACMD_INIT_DELTA_ACDB_DATA, (uint8_t *)&pCmdDeltaFileInfos[i], sizeof(AcdbCmdDeltaFileInfo), NULL, 0))
         {
            // remove from memory.
            if (NULL != pCmdDeltaFileInfos[i].pFileBuf)
            {
               ACDB_MEM_FREE((void *)pCmdDeltaFileInfos[i].pFileBuf);
               pCmdDeltaFileInfos[i].pFileBuf = NULL;
            }
         }

         if (ACDB_SUCCESS != acdb_delta_data_ioctl(ACDBDELTADATCMD_GET_DELTA_FILE_VERSION, (uint8_t *)&i, sizeof(uint32_t), (uint8_t *)&deltaVersion, sizeof(AcdbDeltaFileVersion)))
         {
            ACDB_DEBUG_LOG("[ACDB Command]->Could not get delta file version correctly!\n");
            return ACDB_ERROR;
         }
         if (ACDB_SUCCESS == acdb_delta_data_ioctl(ACDBDELTADATACMD_GET_DELTA_ACDB_DATA_COUNT_FOR_ONE_FILE, (uint8_t *)&i, sizeof(uint32_t), (uint8_t *)&delta_data_count, sizeof(uint32_t)))
         {
            if (delta_data_count > 0)
            {
               if (deltaVersion.majorVersion == 1)
               {
                  AcdbCmdDeltaFileDataInstanceV2 *deltaFileInstance = (AcdbCmdDeltaFileDataInstanceV2 *)ACDB_MALLOC(sizeof(AcdbCmdDeltaFileDataInstanceV2)*delta_data_count);
                  if (deltaFileInstance == NULL)
                  {
                     ACDB_DEBUG_LOG("ACDB_COMMAND: Unable to allocate memory for AcdbCmdInstanceDeltaFileData\n");
                     ACDB_MEM_FREE(pCmdDeltaFileInfos);
                     ACDB_MEM_FREE(pCmdFileInfos);
                     return ACDB_INSUFFICIENTMEMORY;
                  }
                  if (ACDB_SUCCESS == acdb_delta_data_ioctl((uint32_t)ACDBDELTADATACMD_GET_DELTA_ACDB_DATA_FOR_ONE_FILE_V1, (uint8_t *)&i, (uint32_t)sizeof(uint32_t), (uint8_t *)deltaFileInstance, (uint32_t)sizeof(AcdbCmdDeltaFileDataInstanceV2)*delta_data_count))
                  {
                     uint32_t idxVal = 0;
                     for (idxVal = 0; idxVal < delta_data_count; idxVal++)
                     {
                        uint32_t tblId = *deltaFileInstance[idxVal].pTblId;
                        uint32_t numCdftIndices = 0;
						uint32_t numofTblIndices = 0;
						uint32_t nonModuleTblFOund = 0;
						uint32_t numOfCmdIndices = 0;
                        if (ACDB_SUCCESS == Get_table_indices_count(tblId,&numofTblIndices, &nonModuleTblFOund, &numCdftIndices, &numOfCmdIndices))
						{
							int32_t ret = ACDB_SUCCESS;
							ret = AcdbCmdSetOnlineDataV2(persistData, tblId, deltaFileInstance[idxVal].pIndices, *deltaFileInstance[idxVal].pIndicesCount - numCdftIndices,
								numCdftIndices, deltaFileInstance[idxVal].pData, *deltaFileInstance[idxVal].pDataLen);
							if (ret != ACDB_SUCCESS)
							{
								ACDB_DEBUG_LOG("ACDB_COMMAND: Unable to save unused delta file chunk data: tblID: %u \n", *deltaFileInstance[idxVal].pTblId);
								continue;
							}
						}
                     }
                  }
                  ACDB_MEM_FREE(deltaFileInstance);
               }
               else
               {
                  AcdbCmdDeltaFileDataInstance *deltaFileInstance = (AcdbCmdDeltaFileDataInstance *)ACDB_MALLOC(sizeof(AcdbCmdDeltaFileDataInstance)*delta_data_count);

                  if (deltaFileInstance == NULL)
                  {
                     ACDB_DEBUG_LOG("ACDB_COMMAND: Unable to allocate memory for AcdbCmdDeltaFileDataInstance\n");
                     ACDB_MEM_FREE(pCmdDeltaFileInfos);
                     ACDB_MEM_FREE(pCmdFileInfos);
                     return ACDB_INSUFFICIENTMEMORY;
                  }
                  if (ACDB_SUCCESS == acdb_delta_data_ioctl((uint32_t)ACDBDELTADATACMD_GET_DELTA_ACDB_DATA_FOR_ONE_FILE_V0, (uint8_t *)&i, (uint32_t)sizeof(uint32_t), (uint8_t *)deltaFileInstance, (uint32_t)sizeof(AcdbCmdDeltaFileDataInstance)*delta_data_count))
                  {
                     uint32_t idxVal = 0;
                     for (idxVal = 0; idxVal < delta_data_count; idxVal++)
                     {
                        const uint32_t tblId = *deltaFileInstance[idxVal].pTblId;
						uint32_t tempOffset = 0, length = (*deltaFileInstance[idxVal].pIndicesCount + (uint32_t)NON_INSTANCE_CDFT_INDICES_COUNT)*(uint32_t)sizeof(uint32_t);						
						uint32_t mid = *deltaFileInstance[idxVal].pMid;
						uint32_t pid = *deltaFileInstance[idxVal].pPid;
						uint8_t *buffer = (uint8_t *)malloc(length);
						if(buffer == NULL)
						{
							 ACDB_DEBUG_LOG("ACDB_COMMAND: Unable to allocate memory for AcdbCmdDeltaFileDataInstance - temporary buffer\n");
							 return ACDB_INSUFFICIENTMEMORY;
						}
						ACDB_MEM_CPY(buffer, length, deltaFileInstance[idxVal].pIndices, (*deltaFileInstance[idxVal].pIndicesCount*sizeof(uint32_t)));
						tempOffset = (uint32_t)((*deltaFileInstance[idxVal].pIndicesCount*sizeof(uint32_t)));
                        ACDB_MEM_CPY(buffer + tempOffset, sizeof(uint32_t), &mid, sizeof(uint32_t));
						tempOffset += (uint32_t)sizeof(uint32_t);
						ACDB_MEM_CPY(buffer + tempOffset, sizeof(uint32_t), &pid, sizeof(uint32_t));
						tempOffset += (uint32_t)sizeof(uint32_t);
			int32_t ret = ACDB_SUCCESS;
                        ret = AcdbCmdSetOnlineDataV2(persistData, tblId, buffer, *deltaFileInstance[idxVal].pIndicesCount,
                           NON_INSTANCE_CDFT_INDICES_COUNT, deltaFileInstance[idxVal].pData, *deltaFileInstance[idxVal].pDataLen); //No of CDFT indices is 2 for older versions of delta files.

                        if (ret != ACDB_SUCCESS)
                        {
                           ACDB_DEBUG_LOG("ACDB_COMMAND: Unable to save unused delta file chunk data: tblID: %u, Mid: %u, Pid: %u \n", *deltaFileInstance[idxVal].pTblId, *deltaFileInstance[idxVal].pMid, *deltaFileInstance[idxVal].pPid);
                           continue;
                        }
                     }
                  }
                  ACDB_MEM_FREE(deltaFileInstance);
               }
            }
         }
      }

      delta_result = acdb_delta_data_ioctl(ACDBDELTADATACMD_FREE_DELTA_ACDB_BUF, NULL, 0, NULL, 0);
      if (delta_result != ACDB_SUCCESS)
      {
         ACDB_DEBUG_LOG("ACDB_COMMAND: Unable to free delta file chunk\n");
      }

   ACDB_MEM_FREE(pCmdFileInfos);
   ACDB_MEM_FREE(pCmdDeltaFileInfos);
   return result;
}

int32_t AcdbCmdSystemReset(void)
{
   int32_t result = ACDB_SUCCESS;
   result = acdbdata_ioctl(ACDBDATACMD_RESET, NULL, 0, NULL, 0);
   if (ACDB_UTILITY_INIT_SUCCESS == AcdbIsPersistenceSupported())
   {
      result = acdb_delta_data_ioctl(ACDBDELTADATACMD_DELTA_RESET, NULL, 0, NULL, 0);
   }
   result = ACDBHeapReset();

   return result;
}

int32_t AcdbCmdSaveDeltaFileData(void)
{
   int32_t delta_result = ACDB_SUCCESS;
   if (ACDB_UTILITY_INIT_SUCCESS == AcdbIsPersistenceSupported())
   {
      delta_result = acdb_delta_data_ioctl(ACDBDELTADATACMD_SAVE_DELTA_ACDB_DATA, NULL, 0, NULL, 0);
      if (delta_result != ACDB_SUCCESS)
      {
         ACDB_DEBUG_LOG("AcdbCmdSetOnlineData: Unable to set delta file data!\n");
      }
   }

   return delta_result;
}

int32_t AcdbCmdGetAcdbSwVersion(AcdbModuleVersionType *pOutput)
{
   int32_t result = ACDB_SUCCESS;

   uint16_t major = ACDB_SOFTWARE_VERSION_MAJOR;
   uint16_t minor = ACDB_SOFTWARE_VERSION_REVISION;

   if (pOutput != NULL)
   {
      ACDB_MEM_CPY(&pOutput->major, sizeof(pOutput->major), &major, sizeof(major));
      ACDB_MEM_CPY(&pOutput->minor, sizeof(pOutput->minor), &minor, sizeof(minor));
   }
   else
   {
      return ACDB_BADPARM;
   }
   return result;
}

int32_t AcdbCmdGetAcdbSwVersionV2(AcdbModuleVersionTypeV2 *pOutput)
{
   int32_t result = ACDB_SUCCESS;

   uint32_t major = ACDB_SOFTWARE_VERSION_MAJOR;
   uint32_t minor = ACDB_SOFTWARE_VERSION_MINOR;
   uint32_t revision = ACDB_SOFTWARE_VERSION_REVISION;

   if (pOutput != NULL)
   {
      ACDB_MEM_CPY(&pOutput->major, sizeof(pOutput->major), &major, sizeof(major));
      ACDB_MEM_CPY(&pOutput->minor, sizeof(pOutput->minor), &minor, sizeof(minor));
      ACDB_MEM_CPY(&pOutput->revision, sizeof(pOutput->revision), &revision, sizeof(revision));
   }
   else
   {
      return ACDB_BADPARM;
   }
   return result;
}

int32_t AcdbCmdGetCmnDeviceInfo(AcdbDeviceInfoCmnCmdType* pInput,
   AcdbQueryResponseType* pOutput)
{
   int32_t result = ACDB_SUCCESS;

   if (pInput == NULL || pOutput == NULL)
   {
      ACDB_DEBUG_LOG("[ACDB Command]->[AcdbCmdGetCmnDeviceInfo]->System Erorr\n");
      result = ACDB_BADPARM;
   }
   else if (pInput->nBufferPointer == NULL)
   {
      ACDB_DEBUG_LOG("[ACDB Command]->[AcdbCmdGetCmnDeviceInfo]->NULL pointer\n");
      result = ACDB_BADPARM;
   }
   else
   {
	   AcdbDevPropInfo dpInfo = { 0, 0, { 0, NULL} };
      memset(&dpInfo, 0, sizeof(dpInfo));
      dpInfo.devId = pInput->nDeviceId;
      dpInfo.pId = DEVICE_CMN_INFO;
      result = acdbdata_ioctl(ACDBDATACMD_GET_DEVICE_PROP, (uint8_t *)&dpInfo, sizeof(AcdbDevPropInfo),
         (uint8_t *)NULL, 0);
      if (result != ACDB_SUCCESS)
      {
         ACDB_DEBUG_LOG("ACDB_COMMAND: Failed to fetch the common dev info property for devid %08X \n", pInput->nDeviceId);
         return result;
      }
      if (pInput->nBufferLength < dpInfo.dataInfo.nDataLen)
      {
         ACDB_DEBUG_LOG("ACDB_COMMAND: Buffer size insufficient to copy the cmn device info data for devid %08X \n", pInput->nDeviceId);
         return ACDB_INSUFFICIENTMEMORY;
      }
      ACDB_MEM_CPY(pInput->nBufferPointer, pInput->nBufferLength, dpInfo.dataInfo.pData, dpInfo.dataInfo.nDataLen);
      pOutput->nBytesUsedInBuffer = dpInfo.dataInfo.nDataLen;
   }

   return result;
}

int32_t AcdbCmdGetTSDeviceInfo(AcdbDeviceInfoTargetSpecificCmdType* pInput,
   AcdbQueryResponseType* pOutput)
{
   int32_t result = ACDB_SUCCESS;

   if (pInput == NULL || pOutput == NULL)
   {
      ACDB_DEBUG_LOG("[ACDB Command]->[AcdbCmdGetTSDeviceInfo]->System Erorr\n");
      result = ACDB_BADPARM;
   }
   else if (pInput->pBufferPointer == NULL)
   {
      ACDB_DEBUG_LOG("[ACDB Command]->[AcdbCmdGetTSDeviceInfo]->NULL pointer\n");
      result = ACDB_BADPARM;
   }
   else
   {
      AcdbDevPropInfo dpInfo = { 0, 0, { 0, NULL} };
      memset(&dpInfo, 0, sizeof(dpInfo));
      dpInfo.devId = pInput->nDeviceId;
      dpInfo.pId = DEVICE_SPEC_INFO;
      result = acdbdata_ioctl(ACDBDATACMD_GET_DEVICE_PROP, (uint8_t *)&dpInfo, sizeof(AcdbDevPropInfo),
         (uint8_t *)NULL, 0);
      if (result != ACDB_SUCCESS)
      {
         ACDB_DEBUG_LOG("ACDB_COMMAND: Failed to fetch the common dev info property for devid %08X \n", pInput->nDeviceId);
         return result;
      }
      if (pInput->nBufferLength < dpInfo.dataInfo.nDataLen)
      {
         ACDB_DEBUG_LOG("ACDB_COMMAND: Buffer size insufficient to copy the cmn device info data for devid %08X \n", pInput->nDeviceId);
         return ACDB_INSUFFICIENTMEMORY;
      }
      ACDB_MEM_CPY(pInput->pBufferPointer, pInput->nBufferLength, dpInfo.dataInfo.pData, dpInfo.dataInfo.nDataLen);
      pOutput->nBytesUsedInBuffer = dpInfo.dataInfo.nDataLen;
   }

   return result;
}

int32_t AcdbCmdGetDeviceCapabilities(AcdbDeviceCapabilitiesCmdType *pInput,
   AcdbQueryResponseType *pOutput)
{
   int32_t result = ACDB_SUCCESS;

   if (pInput == NULL || pOutput == NULL)
   {
      ACDB_DEBUG_LOG("[ACDB Command]->[AcdbCmdGetDeviceCapabilities]->System Erorr\n");
      result = ACDB_BADPARM;
   }
   else if (pInput->nBufferPointer == NULL)
   {
      ACDB_DEBUG_LOG("[ACDB Command]->[AcdbCmdGetDeviceCapabilities]->NULL pointer\n");
      result = ACDB_BADPARM;
   }
   else
   {
      uint32_t i = 0;
      int32_t dstOffset = 0, srcOffset = 0;
      AcdbDevices *pDevs = (AcdbDevices *)ACDB_MALLOC(sizeof(AcdbDevices));
      if (pDevs == NULL)
      {
         ACDB_DEBUG_LOG("ACDB_COMMAND: Unable to allocate memory for AcdbDevices\n");
         return ACDB_INSUFFICIENTMEMORY;
      }

      memset((void*)pDevs, 0, sizeof(AcdbDevices));

      if (ACDB_DEVICE_CAPABILITY_PARAM != pInput->nParamId)
      {
         ACDB_DEBUG_LOG("ACDB_COMMAND: Invalid param value %08X provided for GetDeviceCapabilities\n", pInput->nParamId);
         ACDB_MEM_FREE(pDevs);
         return ACDB_BADPARM;
      }
      result = acdbdata_ioctl(ACDBDATACMD_GET_DEVICE_LIST, (uint8_t *)pDevs, sizeof(AcdbDevices),
         (uint8_t *)NULL, 0);
      if (result != ACDB_SUCCESS)
      {
         ACDB_DEBUG_LOG("ACDB_COMMAND: Failed to fetch the devicelist to fill the device capabilities info\n");
         ACDB_MEM_FREE(pDevs);
         return result;
      }

      if (pInput->nBufferLength < (sizeof(uint32_t) +
         (pDevs->noOfDevs * sizeof(AcdbDeviceCapabilityType))))
      {
         result = ACDB_INSUFFICIENTMEMORY;
         ACDB_DEBUG_LOG("ACDB_COMMAND: InsufficientMemory to fulfill GetDeviceCapabilities request\n");
         ACDB_MEM_FREE(pDevs);
         return result;
      }
      ACDB_MEM_CPY((void*)(pInput->nBufferPointer + dstOffset), sizeof(uint32_t), (void*)&pDevs->noOfDevs, sizeof(uint32_t));
      dstOffset += (int32_t)sizeof(uint32_t);
      for (i = 0; i < pDevs->noOfDevs; i++)
      {
         uint32_t nDeviceType = 0;
         AcdbDevPropInfo dpCmnInfo = { 0, 0, { 0, NULL} };
         AcdbDevPropInfo dpSpecInfo = { 0, 0, { 0, NULL} };
         memset(&dpCmnInfo, 0, sizeof(dpCmnInfo));
         memset(&dpSpecInfo, 0, sizeof(dpSpecInfo));
         dpCmnInfo.devId = pDevs->devList[i];
         dpCmnInfo.pId = DEVICE_CMN_INFO;
         result = acdbdata_ioctl(ACDBDATACMD_GET_DEVICE_PROP, (uint8_t *)&dpCmnInfo, sizeof(AcdbDevPropInfo),
            (uint8_t *)NULL, 0);
         if (result != ACDB_SUCCESS)
         {
            ACDB_DEBUG_LOG("ACDB_COMMAND: Failed to fetch the device capabilities for devid %08X \n", dpCmnInfo.devId);
            ACDB_MEM_FREE(pDevs);
            return result;
         }

         dpSpecInfo.devId = pDevs->devList[i];
         dpSpecInfo.pId = DEVICE_SPEC_INFO;
         result = acdbdata_ioctl(ACDBDATACMD_GET_DEVICE_PROP, (uint8_t *)&dpSpecInfo, sizeof(AcdbDevPropInfo),
            (uint8_t *)NULL, 0);
         if (result != ACDB_SUCCESS)
         {
            ACDB_DEBUG_LOG("ACDB_COMMAND: Failed to fetch the device capabilities for devid %08X \n", dpSpecInfo.devId);
            ACDB_MEM_FREE(pDevs);
            return result;
         }

         // Get DeviceId
         ACDB_MEM_CPY((void*)(pInput->nBufferPointer + dstOffset), sizeof(uint32_t), (void*)&pDevs->devList[i], sizeof(uint32_t));
         dstOffset += (int32_t)sizeof(uint32_t);

         //Get DeviceType
         ACDB_MEM_CPY((void*)&nDeviceType, sizeof(uint32_t), (void*)(dpCmnInfo.dataInfo.pData + sizeof(uint32_t)), sizeof(uint32_t));

         //Memcopy Sample rate mask
         srcOffset = acdb_devinfo_getSampleMaskOffset(nDeviceType);
         if (!srcOffset)
         {
            ACDB_DEBUG_LOG("ACDB_COMMAND: AcdbCmdGetDeviceCapabilities failed\n");
            ACDB_MEM_FREE(pDevs);
            return ACDB_ERROR;
         }

         ACDB_MEM_CPY((void*)(pInput->nBufferPointer + dstOffset), sizeof(uint32_t), (void*)(dpSpecInfo.dataInfo.pData + srcOffset), sizeof(uint32_t));
         dstOffset += (int32_t)sizeof(uint32_t);

         //Memcopy Byte per sample rate mask
         srcOffset = acdb_devinfo_getBytesPerSampleMaskOffset(nDeviceType);
         if (!srcOffset)
         {
            ACDB_DEBUG_LOG("ACDB_COMMAND: AcdbCmdGetDeviceCapabilities failed\n");
            ACDB_MEM_FREE(pDevs);
            return ACDB_ERROR;
         }
         ACDB_MEM_CPY((void*)(pInput->nBufferPointer + dstOffset), sizeof(uint32_t), (void*)(dpSpecInfo.dataInfo.pData + srcOffset), sizeof(uint32_t));
         dstOffset += (int32_t)sizeof(uint32_t);
      }
      pOutput->nBytesUsedInBuffer = dstOffset;
      ACDB_MEM_FREE(pDevs);
   }

   return result;
}

int32_t AcdbCmdGetDevicePair(AcdbDevicePairType *pInput,
   AcdbDevicePairingResponseType *pOutput)
{
   int32_t result = ACDB_ERROR;

   if (pInput == NULL || pOutput == NULL)
   {
      ACDB_DEBUG_LOG("ACDB_Command: Provided invalid input\n");
      result = ACDB_BADPARM;
   }
   else
   {
      AcdbDevPropInfo devPropInfo = { 0, 0, { 0, NULL} };
      uint32_t i = 0;
      uint32_t nNoOfRxDevs = 0;
      AcdbDevPairInfo *pRxDevs = NULL;
      pOutput->ulIsDevicePairValid = 0;
      memset(&devPropInfo, 0, sizeof(devPropInfo));
      devPropInfo.devId = pInput->nTxDeviceId;
      devPropInfo.pId = DEVPAIR;
      result = acdbdata_ioctl(ACDBDATACMD_GET_DEVICE_PROP, (uint8_t *)&devPropInfo, sizeof(AcdbDevPropInfo),
         (uint8_t *)NULL, 0);
      if (result != ACDB_SUCCESS)
      {
         ACDB_DEBUG_LOG("ACDB_COMMAND: Failed to fetch the device pair info for devid %08X \n", pInput->nTxDeviceId);
         return result;
      }
      nNoOfRxDevs = devPropInfo.dataInfo.nDataLen / ((uint32_t)sizeof(AcdbDevPairInfo));
      pRxDevs = (AcdbDevPairInfo *)devPropInfo.dataInfo.pData;
      if (nNoOfRxDevs == 0)
      {
         ACDB_DEBUG_LOG("ACDB_COMMAND: Contains invalid devpair property for devid %08X \n", pInput->nTxDeviceId);
         return ACDB_ERROR;
      }
      for (i = 0; i < nNoOfRxDevs; i++)
      {
         if (pRxDevs[i].nRxDevId == pInput->nRxDeviceId)
         {
            pOutput->ulIsDevicePairValid = 1;
            return ACDB_SUCCESS;
         }
      }
   }

   return ACDB_SUCCESS;
}

int32_t AcdbCmdGetVolTableStepSize(AcdbVolTblStepSizeRspType *pOutput)
{
   int32_t result = ACDB_SUCCESS;

   if (pOutput == NULL)
   {
      ACDB_DEBUG_LOG("ACDB_COMMAND: Provided invalid param\n");
      result = ACDB_BADPARM;
   }
   else
   {
	   AcdbGlbalPropInfo glbPropInfo = { 0, { 0, NULL } };
      memset(&glbPropInfo, 0, sizeof(glbPropInfo));
      glbPropInfo.pId = AUD_VOL_STEPS;
      result = acdbdata_ioctl(ACDBDATACMD_GET_GLOBAL_PROP, (uint8_t *)&glbPropInfo, sizeof(AcdbGlbalPropInfo),
         (uint8_t *)NULL, 0);
      if (result != ACDB_SUCCESS)
      {
         ACDB_DEBUG_LOG("ACDB_COMMAND: Failed to fetch the property info for pid %08X \n", glbPropInfo.pId);
         return result;
      }
      if (NULL == glbPropInfo.dataInfo.pData)
      {
         ACDB_DEBUG_LOG("ACDB_COMMAND: pData NULL on fetch the property info for pid %08X \n", glbPropInfo.pId);
         return ACDB_ERROR;
      }

      pOutput->AudProcVolTblStepSize = READ_UInt32(glbPropInfo.dataInfo.pData);

      glbPropInfo.pId = VOC_VOL_STEPS;
      result = acdbdata_ioctl(ACDBDATACMD_GET_GLOBAL_PROP, (uint8_t *)&glbPropInfo, sizeof(AcdbGlbalPropInfo),
         (uint8_t *)NULL, 0);
      if (result != ACDB_SUCCESS)
      {
         ACDB_DEBUG_LOG("ACDB_COMMAND: Failed to fetch the property info for pid %08X \n", glbPropInfo.pId);
         return result;
      }
      pOutput->VocProcVolTblStepSize = READ_UInt32(glbPropInfo.dataInfo.pData);
   }
   return result;
}

int32_t AcdbCmdGetANCDevicePair(AcdbAncDevicePairCmdType *pInput,
   AcdbAncDevicePairRspType *pOutput)
{
   int32_t result = ACDB_SUCCESS;

   if (pInput == NULL || pOutput == NULL)
   {
      ACDB_DEBUG_LOG("[ACDB Command]->[AcdbAncDevicePairCmdType]->System Erorr\n");
      result = ACDB_BADPARM;
   }
   else
   {
      AcdbDevPropInfo devPropInfo = { 0, 0, { 0, NULL} };
      uint32_t nNoOfTxDevs = 0;
      AcdbANCPairInfo *pTxDevs = NULL;
      memset(&devPropInfo, 0, sizeof(devPropInfo));
      devPropInfo.devId = pInput->nRxDeviceId;
      devPropInfo.pId = ANCDEVPAIR;
      result = acdbdata_ioctl(ACDBDATACMD_GET_DEVICE_PROP, (uint8_t *)&devPropInfo, sizeof(AcdbDevPropInfo),
         (uint8_t *)NULL, 0);
      if (result != ACDB_SUCCESS)
      {
         ACDB_DEBUG_LOG("ACDB_COMMAND: Failed to fetch the anc device pair info for devid %08X \n", pInput->nRxDeviceId);
         return result;
      }
      nNoOfTxDevs = devPropInfo.dataInfo.nDataLen / ((uint32_t)sizeof(AcdbANCPairInfo));
      pTxDevs = (AcdbANCPairInfo *)devPropInfo.dataInfo.pData;
      if (nNoOfTxDevs == 0)
      {
         ACDB_DEBUG_LOG("ACDB_COMMAND: Contains invalid anc devpair property for devid %08X \n", pInput->nRxDeviceId);
         return ACDB_ERROR;
      }
      pOutput->nTxAncDeviceId = pTxDevs[0].nTxDevId;
   }

   return result;
}

int32_t AcdbCmdGetAudProcCmnTopId(AcdbGetAudProcTopIdCmdType *pInput,
   AcdbGetTopologyIdRspType *pOutput
   )
{
   int32_t result = ACDB_PARMNOTFOUND;

   if (pInput == NULL || pOutput == NULL)
   {
      ACDB_DEBUG_LOG("[ACDB Command]->[AcdbCmdGetAudProcCmnTopId]->System Erorr\n");
      result = ACDB_BADPARM;
   }
   else
   {
      AcdbDevPropInfo devPropInfo = { 0, 0, { 0, NULL} };
      uint32_t i = 0;
      uint32_t nNoOfEntries = 0;
      AcdbAudprocTopInfo *pTopInfo = NULL;
      memset(&devPropInfo, 0, sizeof(devPropInfo));
      devPropInfo.devId = pInput->nDeviceId;
      devPropInfo.pId = AUDPROC_CMN_TOPID;
      result = acdbdata_ioctl(ACDBDATACMD_GET_DEVICE_PROP, (uint8_t *)&devPropInfo, sizeof(AcdbDevPropInfo),
         (uint8_t *)NULL, 0);
      if (result != ACDB_SUCCESS)
      {
         ACDB_DEBUG_LOG("ACDB_COMMAND: Failed to fetch the aud topo info for devid %08X \n", pInput->nDeviceId);
         return ACDB_PARMNOTFOUND;
      }
      nNoOfEntries = devPropInfo.dataInfo.nDataLen / ((uint32_t)sizeof(AcdbAudprocTopInfo));
      pTopInfo = (AcdbAudprocTopInfo *)devPropInfo.dataInfo.pData;
      if (nNoOfEntries == 0)
      {
         ACDB_DEBUG_LOG("ACDB_COMMAND: Contains invalid audtopo property for devid %08X \n", pInput->nDeviceId);
         return ACDB_PARMNOTFOUND;
      }
      //reset the result
      result = ACDB_PARMNOTFOUND;
      for (i = 0; i < nNoOfEntries; i++)
      {
         if (pTopInfo[i].nAppId == pInput->nApplicationType)
         {
            pOutput->nTopologyId = pTopInfo[i].nTopoId;
            return ACDB_SUCCESS;
         }
      }
   }

   return result;
}

int32_t AcdbCmdGetVocProcCmnTopId(AcdbGetVocProcTopIdCmdType *pInput,
   AcdbGetTopologyIdRspType *pOutput
   )
{
   int32_t result = ACDB_SUCCESS;

   if (pInput == NULL || pOutput == NULL)
   {
      ACDB_DEBUG_LOG("[ACDB Command]->[AcdbCmdGetVocProcCmnTopId]->System Erorr\n");
      result = ACDB_BADPARM;
   }
   else
   {
      AcdbDevPropInfo devPropInfo = { 0, 0, { 0, NULL} };
      uint32_t nNoOfEntries = 0;
      AcdbVocprocTopInfo *pTopInfo = NULL;
      memset(&devPropInfo, 0, sizeof(devPropInfo));
      devPropInfo.devId = pInput->nDeviceId;
      devPropInfo.pId = VOCPROC_CMN_TOPID;
      result = acdbdata_ioctl(ACDBDATACMD_GET_DEVICE_PROP, (uint8_t *)&devPropInfo, sizeof(AcdbDevPropInfo),
         (uint8_t *)NULL, 0);
      if (result != ACDB_SUCCESS)
      {
         ACDB_DEBUG_LOG("ACDB_COMMAND: Failed to fetch the voc topo info for devid %08X \n", pInput->nDeviceId);
         return ACDB_PARMNOTFOUND;
      }
      nNoOfEntries = devPropInfo.dataInfo.nDataLen / ((uint32_t)sizeof(AcdbVocprocTopInfo));
      pTopInfo = (AcdbVocprocTopInfo *)devPropInfo.dataInfo.pData;
      if (nNoOfEntries == 0)
      {
         ACDB_DEBUG_LOG("ACDB_COMMAND: Contains invalid voctopo property for devid %08X \n", pInput->nDeviceId);
         return ACDB_PARMNOTFOUND;
      }
      pOutput->nTopologyId = pTopInfo[0].nTopoId;
   }

   return result;
}

int32_t AcdbCmdGetAudProcStrmTopId(AcdbGetAudProcStrmTopIdCmdType *pInput,
   AcdbGetTopologyIdRspType *pOutput
   )
{
   int32_t result = ACDB_ERROR;

   if (pInput == NULL || pOutput == NULL)
   {
      ACDB_DEBUG_LOG("[ACDB Command]->[AcdbCmdGetAudProcStrmTopId]->System Erorr\n");
      result = ACDB_BADPARM;
   }
   else
   {
      uint32_t i = 0;
      uint32_t nNoOfEntries = 0;
      AcdbAudStrmTopInfo *pTopInfo = NULL;
      AcdbGlbalPropInfo glbPropInfo = { 0, { 0, NULL } };
      memset(&glbPropInfo, 0, sizeof(glbPropInfo));
      glbPropInfo.pId = AUDPROC_STRM_TOPID;
      result = acdbdata_ioctl(ACDBDATACMD_GET_GLOBAL_PROP, (uint8_t *)&glbPropInfo, sizeof(AcdbGlbalPropInfo),
         (uint8_t *)NULL, 0);
      if (result != ACDB_SUCCESS)
      {
         ACDB_DEBUG_LOG("ACDB_COMMAND: Failed to fetch the property info for pid %08X \n", glbPropInfo.pId);
         return ACDB_PARMNOTFOUND;
      }
      nNoOfEntries = glbPropInfo.dataInfo.nDataLen / ((uint32_t)sizeof(AcdbAudStrmTopInfo));
      pTopInfo = (AcdbAudStrmTopInfo *)glbPropInfo.dataInfo.pData;
      if (nNoOfEntries == 0)
      {
         ACDB_DEBUG_LOG("ACDB_COMMAND: Contains invalid vocstream property for appid %08X \n", pInput->nApplicationType);
         return ACDB_PARMNOTFOUND;
      }
      for (i = 0; i < nNoOfEntries; i++)
      {
         if (pTopInfo[i].nAppId == pInput->nApplicationType)
         {
            pOutput->nTopologyId = pTopInfo[i].nTopoId;
            return ACDB_SUCCESS;
         }
      }
   }
   return ACDB_PARMNOTFOUND;
}

int32_t AcdbCmdGetAfeTopId(AcdbGetAfeTopIdCmdType *pInput,
   AcdbGetTopologyIdRspType *pOutput
   )
{
   int32_t result = ACDB_PARMNOTFOUND;

   if (pInput == NULL || pOutput == NULL)
   {
      ACDB_DEBUG_LOG("[ACDB Command]->[AcdbCmdGetAfeCmnTopId]->System Erorr\n");
      result = ACDB_BADPARM;
   }
   else
   {
      AcdbDevPropInfo devPropInfo = { 0, 0, { 0, NULL} };
      uint32_t nNoOfEntries = 0;
      AcdbAfeTopInfo *pTopInfo = NULL;
      memset(&devPropInfo, 0, sizeof(devPropInfo));
      devPropInfo.devId = pInput->nDeviceId;
      devPropInfo.pId = AFE_CMN_TOPID_V2;
      result = acdbdata_ioctl(ACDBDATACMD_GET_DEVICE_PROP, (uint8_t *)&devPropInfo, sizeof(AcdbDevPropInfo),
         (uint8_t *)NULL, 0);
      if (result != ACDB_SUCCESS)
      {
         ACDB_DEBUG_LOG("ACDB_COMMAND: Failed to fetch the afe topo info for devid %08X \n", pInput->nDeviceId);
         return ACDB_PARMNOTFOUND;
      }
      nNoOfEntries = devPropInfo.dataInfo.nDataLen / ((uint32_t)sizeof(AcdbAfeTopInfo));
      pTopInfo = (AcdbAfeTopInfo *)devPropInfo.dataInfo.pData;
      if (nNoOfEntries == 0)
      {
         ACDB_DEBUG_LOG("ACDB_COMMAND: Contains invalid afe topo property for devid %08X \n", pInput->nDeviceId);
         return ACDB_PARMNOTFOUND;
      }

      pOutput->nTopologyId = pTopInfo[0].nTopoId;
   }

   return result;
}

int32_t AcdbCmdGetLsmTopId(AcdbGetLsmTopIdCmdType *pInput,
   AcdbGetTopologyIdRspType *pOutput
   )
{
   int32_t result = ACDB_PARMNOTFOUND;

   if (pInput == NULL || pOutput == NULL)
   {
      ACDB_DEBUG_LOG("[ACDB Command]->[AcdbCmdGetLsmTopId]->System Erorr\n");
      result = ACDB_BADPARM;
   }
   else
   {
      AcdbDevPropInfo devPropInfo = { 0, 0, { 0, NULL} };
      uint32_t i = 0;
      uint32_t nNoOfEntries = 0;
      AcdbLsmTopInfo *pTopInfo = NULL;
      memset(&devPropInfo, 0, sizeof(devPropInfo));
      devPropInfo.devId = pInput->nDeviceId;
      devPropInfo.pId = LSM_TOPID_V2;
      result = acdbdata_ioctl(ACDBDATACMD_GET_DEVICE_PROP, (uint8_t *)&devPropInfo, sizeof(AcdbDevPropInfo),
         (uint8_t *)NULL, 0);
      if (result != ACDB_SUCCESS)
      {
         ACDB_DEBUG_LOG("ACDB_COMMAND: Failed to fetch the lsm topo info for devid %08X \n", pInput->nDeviceId);
         return ACDB_PARMNOTFOUND;
      }
      nNoOfEntries = devPropInfo.dataInfo.nDataLen / ((uint32_t)sizeof(AcdbLsmTopInfo));
      pTopInfo = (AcdbLsmTopInfo *)devPropInfo.dataInfo.pData;
      if (nNoOfEntries == 0)
      {
         ACDB_DEBUG_LOG("ACDB_COMMAND: Contains invalid lsm topo property for devid %08X \n", pInput->nDeviceId);
         return ACDB_PARMNOTFOUND;
      }
      //reset the result
      result = ACDB_PARMNOTFOUND;
      for (i = 0; i < nNoOfEntries; i++)
      {
         if (pTopInfo[i].nLsmAppId == pInput->nLsmApplicationType)
         {
            pOutput->nTopologyId = pTopInfo[i].nTopoId;
            return ACDB_SUCCESS;
         }
      }
   }

   return result;
}

int32_t AcdbCmdGetCompRemoteDevId(AcdbGetRmtCompDevIdCmdType *pInput,
   AcdbGetRmtCompDevIdRspType *pOutput
   )
{
   int32_t result = ACDB_SUCCESS;

   if (pInput == NULL || pOutput == NULL)
   {
      ACDB_DEBUG_LOG("[ACDB Command]->[AcdbCmdGetCompRemoteDevId]->System Erorr\n");
      result = ACDB_BADPARM;
   }
   else
   {
      AcdbDevPropInfo devPropInfo = { 0, 0, { 0, NULL} };
      uint32_t nNoOfEntries = 0;
      AcdbCompRemoteDevIdInfo *pRemDevInfo = NULL;
      memset(&devPropInfo, 0, sizeof(devPropInfo));
      devPropInfo.devId = pInput->nNativeDeviceId;
      devPropInfo.pId = APQ_MDM_COMP_DEVID;
      result = acdbdata_ioctl(ACDBDATACMD_GET_DEVICE_PROP, (uint8_t *)&devPropInfo, sizeof(AcdbDevPropInfo),
         (uint8_t *)NULL, 0);
      if (result != ACDB_SUCCESS)
      {
         ACDB_DEBUG_LOG("ACDB_COMMAND: Failed to fetch the compatible remote devid for native devid %08X \n", pInput->nNativeDeviceId);
         return ACDB_PARMNOTFOUND;
      }
      nNoOfEntries = devPropInfo.dataInfo.nDataLen / ((uint32_t)sizeof(AcdbCompRemoteDevIdInfo));
      pRemDevInfo = (AcdbCompRemoteDevIdInfo *)devPropInfo.dataInfo.pData;
      if (nNoOfEntries == 0)
      {
         ACDB_DEBUG_LOG("ACDB_COMMAND: Contains invalid remotedevid property for native devid %08X \n", pInput->nNativeDeviceId);
         return ACDB_PARMNOTFOUND;
      }
      pOutput->nRmtDeviceId = pRemDevInfo[0].nDeviceId;
   }

   return result;
}

int32_t AcdbCmdGetRecordRxDeviceList(AcdbAudioRecRxListCmdType *pInput,
   AcdbAudioRecRxListRspType *pOutput)

{
   int32_t result = ACDB_SUCCESS;

   if (pInput == NULL || pOutput == NULL)
   {
      ACDB_DEBUG_LOG("[ACDB Command]->[AcdbCmdGetRecordRxDeviceList]->System Erorr\n");
      result = ACDB_BADPARM;
   }
   else
   {
      AcdbDevPropInfo devPropInfo = { 0, 0, { 0, NULL} };
      uint32_t i = 0;
      uint32_t offset = 0;
      uint32_t nNoOfRxDevs = 0;
      AcdbECRecDevPairInfo *pRxDevs = NULL;
      memset(&devPropInfo, 0, sizeof(devPropInfo));
      devPropInfo.devId = pInput->nTxDeviceId;
      devPropInfo.pId = RECORDED_DEVICEPAIR;
      result = acdbdata_ioctl(ACDBDATACMD_GET_DEVICE_PROP, (uint8_t *)&devPropInfo, sizeof(AcdbDevPropInfo),
         (uint8_t *)NULL, 0);
      if (result != ACDB_SUCCESS)
      {
         ACDB_DEBUG_LOG("ACDB_COMMAND: Failed to fetch the ec record device pair info for devid %08X \n", pInput->nTxDeviceId);
         return ACDB_PARMNOTFOUND;
      }
      nNoOfRxDevs = devPropInfo.dataInfo.nDataLen / ((uint32_t)sizeof(AcdbECRecDevPairInfo));
      pRxDevs = (AcdbECRecDevPairInfo *)devPropInfo.dataInfo.pData;
      if (nNoOfRxDevs == 0)
      {
         ACDB_DEBUG_LOG("ACDB_COMMAND: Contains invalid ec record devpair property for devid %08X \n", pInput->nTxDeviceId);
         return ACDB_PARMNOTFOUND;
      }
      pOutput->nNoOfRxDevs = nNoOfRxDevs;
      for (i = 0; i < nNoOfRxDevs; i++)
      {
         ACDB_MEM_CPY((uint8_t *)((uint8_t *)pOutput->pRxDevs + offset), sizeof(AcdbECRecDevPairInfo), (uint8_t *)&pRxDevs[i].nRxDeviceId, sizeof(AcdbECRecDevPairInfo));
         offset += (uint32_t)sizeof(AcdbECRecDevPairInfo);
      }
   }
   return result;
}

int32_t AcdbCmdGetAudProcInfo(uint32_t queryType, uint8_t *pIn, uint8_t *pOut)
{
   int32_t result = ACDB_SUCCESS;

   if (pIn == NULL || pOut == NULL)
   {
      ACDB_DEBUG_LOG("[ACDB Command]->[AcdbCmdGetAudProcInfo]->Invalid NULL value parameters are provided\n");
      result = ACDB_BADPARM;
   }
   else
   {
      uint32_t index = 0;
      uint32_t tblId = AUDPROC_GAIN_INDP_TBL;
	  AcdbTableCmd cmd = { 0, 0 };
	  AcdbTableInfo tblInfo = { { 0, NULL }, { 0, NULL }, { 0, NULL } };
	  AudProcCmnDataLookupTblType lutTbl = { 0, NULL};
	  ContentDefTblType cdefTbl = { 0, NULL };
	  ContentDataOffsetsTblType cdotTbl = { 0, NULL };
      uintptr_t nLookupKey = 0;
	  AudProcCmnCmdLookupType audcmd = { 0, 0, 0 };
	  AudProcCmnDataLookupType tempLut = { 0, 0, 0, 0, 0 };

      switch (queryType)
      {
      case TABLE_CMD:
      {
         AcdbAudProcTableCmdType *pInput = (AcdbAudProcTableCmdType *)pIn;
         if (pInput->nBufferPointer == NULL)
         {
            return ACDB_BADPARM;
         }
         audcmd.nDeviceId = pInput->nDeviceId;
         audcmd.nDeviceSampleRateId = pInput->nDeviceSampleRateId;
         audcmd.nApplicationType = pInput->nApplicationType;
      }
      break;
      case TABLE_SIZE_CMD:
      {
         AcdbAudProcTableSizeCmdType *pInput = (AcdbAudProcTableSizeCmdType *)pIn;
         audcmd.nDeviceId = pInput->nDeviceId;
         audcmd.nDeviceSampleRateId = pInput->nDeviceSampleRateId;
         audcmd.nApplicationType = pInput->nApplicationType;
      }
      break;
      case DATA_CMD:
      {
         AcdbAudProcCmdType *pInput = (AcdbAudProcCmdType *)pIn;
         if (pInput->nBufferPointer == NULL)
         {
            return ACDB_BADPARM;
         }
         audcmd.nDeviceId = pInput->nDeviceId;
         audcmd.nDeviceSampleRateId = pInput->nDeviceSampleRateId;
         audcmd.nApplicationType = pInput->nApplicationType;
      }
      break;
      default:
         return  ACDB_ERROR;
      }
      //acdb_translate_sample_rate(audcmd.nDeviceSampleRateId,&audcmd.nDeviceSampleRateId);

      cmd.devId = audcmd.nDeviceId;
      cmd.tblId = tblId;
      result = acdbdata_ioctl(ACDBDATACMD_GET_TABLE_INFO, (uint8_t *)&cmd, sizeof(cmd),
         (uint8_t *)&tblInfo, sizeof(tblInfo));
      if (result != ACDB_SUCCESS)
      {
         ACDB_DEBUG_LOG("Failed to fetch the lookup information of the device %08X \n"
            , audcmd.nDeviceId);
         return result;
      }
      lutTbl.nLen = READ_UInt32(tblInfo.tblLutChnk.pData);
      lutTbl.pLut = (AudProcCmnDataLookupType *)(tblInfo.tblLutChnk.pData + sizeof(lutTbl.nLen));

      result = AcdbDataBinarySearch((void *)lutTbl.pLut, lutTbl.nLen,
         AUDPROC_LUT_INDICES_COUNT, &audcmd, AUDPROC_CMD_INDICES_COUNT, &index);
      if (result != SEARCH_SUCCESS)
      {
         ACDB_DEBUG_LOG("Failed to fetch the lookup information of the device %08X \n"
            , audcmd.nDeviceId);
         return ACDB_INPUT_PARAMS_NOT_FOUND;
      }

      nLookupKey = (uintptr_t)&lutTbl.pLut[index];
      // Now get CDEF info
      ACDB_MEM_CPY(&tempLut, sizeof(tempLut), &lutTbl.pLut[index], sizeof(tempLut));
      ACDB_MEM_CPY(&cdefTbl.nLen, sizeof(uint32_t), (tblInfo.tblCdftChnk.pData + tempLut.nCDEFTblOffset), sizeof(uint32_t));
      cdefTbl.pCntDef = (ContentDefType *)(tblInfo.tblCdftChnk.pData + tempLut.nCDEFTblOffset + sizeof(cdefTbl.nLen));

      // Now get CDOT info
      ACDB_MEM_CPY(&cdotTbl.nLen, sizeof(uint32_t), tblInfo.tblCdotChnk.pData + tempLut.nCDOTTblOffset, sizeof(uint32_t));
      cdotTbl.pDataOffsets = (uint32_t *)(tblInfo.tblCdotChnk.pData + tempLut.nCDOTTblOffset + sizeof(cdotTbl.nLen));

      if (cdefTbl.nLen != cdotTbl.nLen)
      {
         ACDB_DEBUG_LOG("The no of entries in CDEF and CDOT tables for devid %08X not matching\n", audcmd.nDeviceId);
         return ACDB_ERROR;
      }

      switch (queryType)
      {
      case TABLE_CMD:
      {
         AcdbAudProcTableCmdType *pInput = (AcdbAudProcTableCmdType *)pIn;
         AcdbQueryResponseType *pOutput = (AcdbQueryResponseType *)pOut;
         result = GetMidPidCalibTable(tblId, nLookupKey, cdefTbl, cdotTbl, tblInfo.dataPoolChnk,
            pInput->nBufferPointer, pInput->nBufferLength, &pOutput->nBytesUsedInBuffer);
         if (ACDB_SUCCESS != result)
         {
            return result;
         }
      }
      break;
      case TABLE_SIZE_CMD:
      {
         AcdbSizeResponseType *pOutput = (AcdbSizeResponseType *)pOut;
         if (ACDB_SUCCESS != GetMidPidCalibTableSize(tblId, nLookupKey, cdefTbl, cdotTbl, tblInfo.dataPoolChnk, &pOutput->nSize))
         {
            return ACDB_ERROR;
         }
      }
      break;
      case DATA_CMD:
      {
         AcdbAudProcCmdType *pInput = (AcdbAudProcCmdType *)pIn;
         AcdbQueryResponseType *pOutput = (AcdbQueryResponseType *)pOut;
         result = GetMidPidCalibData(tblId, nLookupKey, cdefTbl, cdotTbl, tblInfo.dataPoolChnk, pInput->nModuleId,
            pInput->nParamId, pInput->nBufferPointer, pInput->nBufferLength, &pOutput->nBytesUsedInBuffer);
         if (ACDB_SUCCESS != result)
         {
            return result;
         }
      }
      break;
      default:
         return  ACDB_ERROR;
      }

      result = ACDB_SUCCESS;
   }

   return result;
}

int32_t AcdbCmdSetAudProcInfo(AcdbAudProcTableCmdType *pInput)
{
   //Helper Structure to copy one ParamData entry's header information
   typedef struct _AudProcTableEntryHeader {
      uint32_t nModuleId;
      uint32_t nParamId;
      uint16_t nParamSize; //multiple of 4
      uint16_t nReserved; // Must be 0
   } AudProcTableEntryHeader;

   int32_t result = ACDB_SUCCESS;
   uint32_t remaining_bufLength = 0;
   uint32_t offSet = 0;

   AudProcTableEntryHeader oneEntryHeader = { 0, 0, 0, 0};
   uint8_t  *pOneEntryBuffer = NULL;

   AudProcCmnCmdLookupType audProcCmnLookupCmd = { 0, 0, 0 };
   uint32_t persistData = FALSE;
   int32_t persist_result = 0;

   if (pInput == NULL)
   {
      ACDB_DEBUG_LOG("[ACDB Command]->[AcdbCmdSetAudProcInfo]->Invalid NULL value parameters are provided\n");
      return ACDB_BADPARM;
   }
   else if (pInput->nBufferPointer == NULL)
   {
      return ACDB_BADPARM;
   }

   remaining_bufLength = pInput->nBufferLength;

   persist_result = AcdbCmdIsPersistenceSupported(&persistData);
   if (persist_result != ACDB_SUCCESS)
   {
      persistData = FALSE;
   }

   audProcCmnLookupCmd.nDeviceId = pInput->nDeviceId;
   audProcCmnLookupCmd.nDeviceSampleRateId = pInput->nDeviceSampleRateId;
   audProcCmnLookupCmd.nApplicationType = pInput->nApplicationType;

   //Iterate over each entry
   while (remaining_bufLength > sizeof(AudProcTableEntryHeader))
   {
      //Copy one entry header
      ACDB_MEM_CPY(&oneEntryHeader, sizeof(oneEntryHeader), pInput->nBufferPointer + offSet, sizeof(oneEntryHeader));

      //Update book keeping
      remaining_bufLength = remaining_bufLength - ((uint32_t)sizeof(oneEntryHeader));
      offSet = offSet + ((uint32_t)sizeof(oneEntryHeader));

      if (remaining_bufLength >= oneEntryHeader.nParamSize)
	  {
		  uint32_t offset = 0, size = ((AUDPROCTBL_INDICES_COUNT+ AUDPROCTBL_CDFT_INDICES_COUNT)*sizeof(uint32_t));
		  uint8_t *buffer = (uint8_t *)malloc(size);
		  if(buffer == NULL)
		  {
			  ACDB_DEBUG_LOG("ACDB_COMMAND: Unable to allocate memory for AcdbCmdSetAudProcInfo - temporary buffer\n");
			  return ACDB_INSUFFICIENTMEMORY;
		  }
		  //get pointer to one entry's param data
		  pOneEntryBuffer = &pInput->nBufferPointer[offSet];

		  //Update book keeping
		  remaining_bufLength = remaining_bufLength - oneEntryHeader.nParamSize;
		  offSet = offSet + oneEntryHeader.nParamSize;

		  //Set one ParamData entry
		  ACDB_MEM_CPY(buffer, size, (uint8_t *)&audProcCmnLookupCmd, AUDPROCTBL_INDICES_COUNT *sizeof(uint32_t));
		  offset += (uint32_t)(AUDPROCTBL_INDICES_COUNT *sizeof(uint32_t));
		  ACDB_MEM_CPY(buffer + offset, sizeof(uint32_t), &oneEntryHeader.nModuleId, sizeof(uint32_t));
		  offset += ((uint32_t)sizeof(uint32_t));
		  ACDB_MEM_CPY(buffer + offset, sizeof(uint32_t), &oneEntryHeader.nParamId, sizeof(uint32_t));
		  offset += ((uint32_t)sizeof(uint32_t));
		  result = AcdbCmdSetOnlineDataV2(persistData, AUDPROC_GAIN_INDP_TBL, buffer, AUDPROCTBL_INDICES_COUNT, AUDPROCTBL_CDFT_INDICES_COUNT, pOneEntryBuffer, oneEntryHeader.nParamSize);
		  ACDB_MEM_FREE(buffer);
		  if (result != ACDB_SUCCESS)
		  {
			  ACDB_DEBUG_LOG("[ACDB Command]->[AcdbCmdSetAudProcInfo]->Err:%0x Failed to set AudProcData for...\n", result);
			  ACDB_DEBUG_LOG("ModID:%0x PID:%0x ParamSize:%0x", oneEntryHeader.nModuleId, oneEntryHeader.nParamId, oneEntryHeader.nParamSize);
			  return result;
		  }
	  }
      else
      {
         ACDB_DEBUG_LOG("[ACDB Command]->[AcdbCmdSetAudProcInfo]->Unexpected buffer length. Residual of %u bytes found\n", remaining_bufLength);
         result = ACDB_BADPARM;
         return result;
      }
   }

   if (remaining_bufLength > 0)
   {
      ACDB_DEBUG_LOG("[ACDB Command]->[AcdbCmdSetAudProcInfo]->Unexpected buffer length. Residual of %u bytes found\n", remaining_bufLength);
   }

   if (result == ACDB_SUCCESS &&
      ACDB_INIT_SUCCESS == AcdbIsPersistenceSupported())
   {
      result = AcdbCmdSaveDeltaFileData();
      if (result != ACDB_SUCCESS)
      {
         ACDB_DEBUG_LOG("[ACDB Command]->[AcdbCmdSetAudStreamData]->Unable to save delta file data\n");
      }
   }

   result = ACDB_SUCCESS;

   return result;
}
int32_t AcdbCmdSetAudProcData(AcdbAudProcCmdType *pInput)
{
   int32_t result = ACDB_SUCCESS;
   uint32_t persistData = FALSE;
   int32_t persist_result = AcdbCmdIsPersistenceSupported(&persistData);
   if (persist_result != ACDB_SUCCESS)
   {
      persistData = FALSE;
   }

   if (pInput == NULL)
   {
      ACDB_DEBUG_LOG("[ACDB Command]->[AcdbCmdSetAudProcData]->Invalid NULL value parameters are provided\n");
      result = ACDB_BADPARM;
   }
   else if (pInput->nBufferPointer == NULL)
   {
      result = ACDB_BADPARM;
   }
   else
   {
      const uint32_t tblId = AUDPROC_GAIN_INDP_TBL;
	  uint32_t offset = 0, size = ((AUDPROCTBL_INDICES_COUNT+ AUDPROCTBL_CDFT_INDICES_COUNT)*sizeof(uint32_t));	  
      AudProcCmnCmdLookupType audcmd = { 0, 0, 0 };
	  uint8_t *buffer = (uint8_t *)malloc(size);
	  if(buffer == NULL)
	  {
			ACDB_DEBUG_LOG("ACDB_COMMAND: Unable to allocate memory for AcdbCmdSetAudProcData - temporary buffer\n");
			return ACDB_INSUFFICIENTMEMORY;
	  }
      audcmd.nDeviceId = pInput->nDeviceId;
      audcmd.nDeviceSampleRateId = pInput->nDeviceSampleRateId;
      audcmd.nApplicationType = pInput->nApplicationType;
	  ACDB_MEM_CPY(buffer, size, (uint8_t *)&audcmd, AUDPROCTBL_INDICES_COUNT *sizeof(uint32_t));
	  offset += (uint32_t)(AUDPROCTBL_INDICES_COUNT *sizeof(uint32_t));
	  ACDB_MEM_CPY(buffer + offset, sizeof(uint32_t), &pInput->nModuleId, sizeof(uint32_t));
	  offset += ((uint32_t)sizeof(uint32_t));
	  ACDB_MEM_CPY(buffer + offset, sizeof(uint32_t), &pInput->nParamId, sizeof(uint32_t));
	  offset += ((uint32_t)sizeof(uint32_t));
	  result = AcdbCmdSetOnlineDataV2(persistData, tblId, buffer, AUDPROCTBL_INDICES_COUNT, AUDPROCTBL_CDFT_INDICES_COUNT, pInput->nBufferPointer, pInput->nBufferLength);

      if (result == ACDB_SUCCESS &&
         ACDB_INIT_SUCCESS == AcdbIsPersistenceSupported())
      {
         result = AcdbCmdSaveDeltaFileData();

         if (result != ACDB_SUCCESS)
         {
            ACDB_DEBUG_LOG("[ACDB Command]->[AcdbCmdSetAudProcData]->Unable to save delta file data\n");
         }
      }
   }

   return result;
}

int32_t AcdbCmdGetAudProcGainDepStepInfo(uint32_t queryType, uint8_t *pIn, uint8_t *pOut)
{
   int32_t result = ACDB_SUCCESS;

   if (pIn == NULL || pOut == NULL)
   {
      ACDB_DEBUG_LOG("[ACDB Command]->[AcdbCmdGetAudProcGainDepStepInfo]->Invalid Null input provided\n");
      result = ACDB_BADPARM;
   }
   else
   {
      uint32_t index = 0;
      AcdbTableCmd cmd = { 0, 0 };
      AcdbTableInfo tblInfo = { { 0, NULL }, { 0, NULL }, { 0, NULL } };
	  AudProcGainDepDataLookupTblType lutTbl = { 0, NULL };
      ContentDefTblType cdefTbl = { 0, NULL };
      ContentDataOffsetsTblType cdotTbl = { 0, NULL };
      uintptr_t nLookupKey = 0;
	  AudProcGainDepCmdLookupType audcmd = { 0, 0, 0 };
      uint32_t tblId = AUDPROC_COPP_GAIN_DEP_TBL;
	  AudProcGainDepDataLookupType tempLut = { 0, 0, 0, 0, 0};

      switch (queryType)
      {
      case TABLE_CMD:
      {
         AcdbAudProcGainDepVolTblStepCmdType *pInput = (AcdbAudProcGainDepVolTblStepCmdType *)pIn;
         if (pInput->nBufferPointer == NULL)
         {
            return ACDB_BADPARM;
         }
         audcmd.nDeviceId = pInput->nDeviceId;
         audcmd.nApplicationType = pInput->nApplicationType;
         audcmd.nVolIdx = pInput->nVolumeIndex;
      }
      break;
      case TABLE_SIZE_CMD:
      {
         AcdbAudProcGainDepVolTblStepSizeCmdType *pInput = (AcdbAudProcGainDepVolTblStepSizeCmdType *)pIn;
         audcmd.nDeviceId = pInput->nDeviceId;
         audcmd.nApplicationType = pInput->nApplicationType;
         audcmd.nVolIdx = pInput->nVolumeIndex;
      }
      break;
      default:
         return  ACDB_ERROR;
      }

      cmd.devId = audcmd.nDeviceId;
      cmd.tblId = tblId;
      result = acdbdata_ioctl(ACDBDATACMD_GET_TABLE_INFO, (uint8_t *)&cmd, sizeof(cmd),
         (uint8_t *)&tblInfo, sizeof(tblInfo));
      if (result != ACDB_SUCCESS)
      {
         ACDB_DEBUG_LOG("Failed to fetch the lookup information of the device %08X \n"
            , audcmd.nDeviceId);
         return result;
      }

      lutTbl.nLen = READ_UInt32(tblInfo.tblLutChnk.pData);
      lutTbl.pLut = (AudProcGainDepDataLookupType *)(tblInfo.tblLutChnk.pData + sizeof(lutTbl.nLen));

      result = AcdbDataBinarySearch((void *)lutTbl.pLut, lutTbl.nLen,
         AUDPROC_GAIN_DEP_LUT_INDICES_COUNT, &audcmd, AUDPROC_GAIN_DEP_CMD_INDICES_COUNT, &index);
      if (result != SEARCH_SUCCESS)
      {
         ACDB_DEBUG_LOG("Failed to fetch the lookup information of the device %08X \n"
            , audcmd.nDeviceId);
         return ACDB_INPUT_PARAMS_NOT_FOUND;
      }

      // Now get CDEF info
      nLookupKey = (uintptr_t)&lutTbl.pLut[index];
      ACDB_MEM_CPY(&tempLut, sizeof(tempLut), &lutTbl.pLut[index], sizeof(tempLut));
      ACDB_MEM_CPY(&cdefTbl.nLen, sizeof(uint32_t), (tblInfo.tblCdftChnk.pData + tempLut.nCDEFTblOffset), sizeof(uint32_t));
      cdefTbl.pCntDef = (ContentDefType *)(tblInfo.tblCdftChnk.pData + tempLut.nCDEFTblOffset + sizeof(cdefTbl.nLen));

      // Now get CDOT info
      ACDB_MEM_CPY(&cdotTbl.nLen, sizeof(uint32_t), (tblInfo.tblCdotChnk.pData + tempLut.nCDOTTblOffset), sizeof(uint32_t));

      cdotTbl.pDataOffsets = (uint32_t *)(tblInfo.tblCdotChnk.pData + tempLut.nCDOTTblOffset + sizeof(cdotTbl.nLen));

      if (cdefTbl.nLen != cdotTbl.nLen)
      {
         ACDB_DEBUG_LOG("The no of entries in CDEF and CDOT tables for devid %08X not matching \n", audcmd.nDeviceId);
         return ACDB_ERROR;
      }

      switch (queryType)
      {
      case TABLE_CMD:
      {
         AcdbAudProcGainDepVolTblStepCmdType *pInput = (AcdbAudProcGainDepVolTblStepCmdType *)pIn;
         AcdbQueryResponseType *pOutput = (AcdbQueryResponseType *)pOut;
         result = GetMidPidCalibTable(tblId, nLookupKey, cdefTbl, cdotTbl, tblInfo.dataPoolChnk,
            pInput->nBufferPointer, pInput->nBufferLength, &pOutput->nBytesUsedInBuffer);
         if (ACDB_SUCCESS != result)
         {
            return result;
         }
      }
      break;
      case TABLE_SIZE_CMD:
      {
         AcdbSizeResponseType *pOutput = (AcdbSizeResponseType *)pOut;
         if (ACDB_SUCCESS != GetMidPidCalibTableSize(tblId, nLookupKey, cdefTbl, cdotTbl, tblInfo.dataPoolChnk, &pOutput->nSize))
         {
            return ACDB_ERROR;
         }
      }
      break;
      default:
         return  ACDB_ERROR;
      }

      result = ACDB_SUCCESS;
   }

   return result;
}

int32_t AcdbCmdGetAudProcVolStepInfo(uint32_t queryType, uint8_t *pIn, uint8_t *pOut)
{
   int32_t result = ACDB_SUCCESS;

   if (pIn == NULL || pOut == NULL)
   {
      ACDB_DEBUG_LOG("[ACDB Command]->[AcdbCmdGetAudProcVolStepInfo]->Invalid Null input param provided\n");
      result = ACDB_BADPARM;
   }
   else
   {
      uint32_t index = 0;
      AcdbTableCmd cmd = { 0, 0 };
      AcdbTableInfo tblInfo = { { 0, NULL }, { 0, NULL }, { 0, NULL } };
      AudProcGainDepDataLookupTblType lutTbl = { 0, NULL };
      ContentDefTblType cdefTbl = { 0, NULL };
      ContentDataOffsetsTblType cdotTbl = { 0, NULL };
      uintptr_t nLookupKey = 0;
      AudProcGainDepCmdLookupType audcmd = { 0, 0, 0 };
      uint32_t tblId = AUDPROC_AUD_VOL_TBL;
      AudProcGainDepDataLookupType tempLut = { 0, 0, 0, 0, 0};

      switch (queryType)
      {
      case TABLE_CMD:
      {
         AcdbAudProcGainDepVolTblStepCmdType *pInput = (AcdbAudProcGainDepVolTblStepCmdType *)pIn;
         if (pInput->nBufferPointer == NULL)
         {
            return ACDB_BADPARM;
         }
         audcmd.nDeviceId = pInput->nDeviceId;
         audcmd.nApplicationType = pInput->nApplicationType;
         audcmd.nVolIdx = pInput->nVolumeIndex;
      }
      break;
      case TABLE_SIZE_CMD:
      {
         AcdbAudProcGainDepVolTblStepSizeCmdType *pInput = (AcdbAudProcGainDepVolTblStepSizeCmdType *)pIn;
         audcmd.nDeviceId = pInput->nDeviceId;
         audcmd.nApplicationType = pInput->nApplicationType;
         audcmd.nVolIdx = pInput->nVolumeIndex;
      }
      break;
      default:
         return  ACDB_ERROR;
      }

      cmd.devId = audcmd.nDeviceId;
      cmd.tblId = tblId;
      result = acdbdata_ioctl(ACDBDATACMD_GET_TABLE_INFO, (uint8_t *)&cmd, sizeof(cmd),
         (uint8_t *)&tblInfo, sizeof(tblInfo));
      if (result != ACDB_SUCCESS)
      {
         ACDB_DEBUG_LOG("Failed to fetch the lookup information of the device %08X \n"
            , audcmd.nDeviceId);
         return result;
      }
      lutTbl.nLen = READ_UInt32(tblInfo.tblLutChnk.pData);
      lutTbl.pLut = (AudProcGainDepDataLookupType *)(tblInfo.tblLutChnk.pData + sizeof(lutTbl.nLen));
      result = AcdbDataBinarySearch((void *)lutTbl.pLut, lutTbl.nLen,
         AUDPROC_VOL_LUT_INDICES_COUNT, &audcmd, AUDPROC_VOL_CMD_INDICES_COUNT, &index);
      if (result != SEARCH_SUCCESS)
      {
         ACDB_DEBUG_LOG("Failed to fetch the lookup information of the device %08X \n"
            , audcmd.nDeviceId);
         return ACDB_INPUT_PARAMS_NOT_FOUND;
      }

      nLookupKey = (uintptr_t)&lutTbl.pLut[index];
      // Now get CDEF info
      ACDB_MEM_CPY(&tempLut, sizeof(tempLut), &lutTbl.pLut[index], sizeof(tempLut));
      ACDB_MEM_CPY(&cdefTbl.nLen, sizeof(uint32_t), (tblInfo.tblCdftChnk.pData + tempLut.nCDEFTblOffset), sizeof(uint32_t));
      cdefTbl.pCntDef = (ContentDefType *)(tblInfo.tblCdftChnk.pData + tempLut.nCDEFTblOffset + sizeof(cdefTbl.nLen));

      // Now get CDOT info
      ACDB_MEM_CPY(&cdotTbl.nLen, sizeof(uint32_t), (tblInfo.tblCdotChnk.pData + tempLut.nCDOTTblOffset), sizeof(uint32_t));
      cdotTbl.pDataOffsets = (uint32_t *)(tblInfo.tblCdotChnk.pData + tempLut.nCDOTTblOffset + sizeof(cdotTbl.nLen));

      if (cdefTbl.nLen != cdotTbl.nLen)
      {
         ACDB_DEBUG_LOG("The no of entries in CDEF and CDOT tables for devid %08X not matching\n", audcmd.nDeviceId);
         return ACDB_ERROR;
      }

      switch (queryType)
      {
      case TABLE_CMD:
      {
         AcdbAudProcGainDepVolTblStepCmdType *pInput = (AcdbAudProcGainDepVolTblStepCmdType *)pIn;
         AcdbQueryResponseType *pOutput = (AcdbQueryResponseType *)pOut;
         result = GetMidPidCalibTable(tblId, nLookupKey, cdefTbl, cdotTbl, tblInfo.dataPoolChnk,
            pInput->nBufferPointer, pInput->nBufferLength, &pOutput->nBytesUsedInBuffer);
         if (ACDB_SUCCESS != result)
         {
            return result;
         }
      }
      break;
      case TABLE_SIZE_CMD:
      {
         AcdbSizeResponseType *pOutput = (AcdbSizeResponseType *)pOut;
         if (ACDB_SUCCESS != GetMidPidCalibTableSize(tblId, nLookupKey, cdefTbl, cdotTbl, tblInfo.dataPoolChnk, &pOutput->nSize))
         {
            return ACDB_ERROR;
         }
      }
      break;
      default:
         return  ACDB_ERROR;
      }
      result = ACDB_SUCCESS;
   }

   return result;
}

int32_t AcdbCmdGetAudStreamInfo(uint32_t queryType, uint8_t *pIn, uint8_t *pOut)
{
   int32_t result = ACDB_SUCCESS;

   if (pIn == NULL || pOut == NULL)
   {
      ACDB_DEBUG_LOG("[ACDB Command]->[AcdbCmdGetAudStreamTableSize]->System Erorr\n");
      result = ACDB_BADPARM;
   }
   else
   {
      uint32_t index = 0;
      AcdbTableCmd cmd = { 0, 0 };
      AcdbTableInfo tblInfo = { { 0, NULL }, { 0, NULL }, { 0, NULL } };
	  AudStreamDataLookupTblType lutTbl = { 0, NULL };
      ContentDefTblType cdefTbl = { 0, NULL };
      ContentDataOffsetsTblType cdotTbl = { 0, NULL };
      uintptr_t nLookupKey = 0;
	  AudStreamCmdLookupType audstrmcmd = { 0 };
      uint32_t tblId = AUD_STREAM_TBL;
	  AudStreamDataLookupType tempLut = { 0, 0, 0 };

      memset(&cmd, 0, sizeof(AcdbTableCmd));
      memset(&tblInfo, 0, sizeof(AcdbTableInfo));
      memset(&lutTbl, 0, sizeof(AudStreamDataLookupTblType));
      memset(&cdefTbl, 0, sizeof(ContentDefTblType));
      memset(&cdotTbl, 0, sizeof(ContentDataOffsetsTblType));
      memset(&audstrmcmd, 0, sizeof(AudStreamCmdLookupType));

      switch (queryType)
      {
      case TABLE_CMD:
      {
         AcdbAudStrmTableV2CmdType *pInput = (AcdbAudStrmTableV2CmdType *)pIn;
         if (pInput->nBufferPointer == NULL)
         {
            return ACDB_BADPARM;
         }
         audstrmcmd.nApplicationType = pInput->nApplicationTypeId;
      }
      break;
      case TABLE_SIZE_CMD:
      {
         AcdbAudStrmTblSizeCmdType *pInput = (AcdbAudStrmTblSizeCmdType *)pIn;
         audstrmcmd.nApplicationType = pInput->nApplicationTypeId;
      }
      break;
      case DATA_CMD:
      {
         AcdbAudStrmV2CmdType *pInput = (AcdbAudStrmV2CmdType *)pIn;
         if (pInput->nBufferPointer == NULL)
         {
            return ACDB_BADPARM;
         }
         audstrmcmd.nApplicationType = pInput->nApplicationTypeId;
      }
      break;
      default:
         return  ACDB_ERROR;
      }

      cmd.devId = 0;
      cmd.tblId = tblId;

      result = acdbdata_ioctl(ACDBDATACMD_GET_TABLE_INFO, (uint8_t *)&cmd, sizeof(cmd),
         (uint8_t *)&tblInfo, sizeof(tblInfo));
      if (result != ACDB_SUCCESS)
      {
         ACDB_DEBUG_LOG("Failed to fetch the lookup information of the appid %08X \n"
            , audstrmcmd.nApplicationType);
         return result;
      }

      lutTbl.nLen = READ_UInt32(tblInfo.tblLutChnk.pData);
      lutTbl.pLut = (AudStreamDataLookupType *)(tblInfo.tblLutChnk.pData + sizeof(lutTbl.nLen));
      result = AcdbDataBinarySearch((void *)lutTbl.pLut, lutTbl.nLen,
         AUDSTREAM_LUT_INDICES_COUNT, &audstrmcmd, AUDSTREAM_CMD_INDICES_COUNT, &index);
      if (result != SEARCH_SUCCESS)
      {
         ACDB_DEBUG_LOG("Failed to fetch the lookup information of the appid %08X \n"
            , audstrmcmd.nApplicationType);
         return ACDB_INPUT_PARAMS_NOT_FOUND;
      }

      nLookupKey = (uintptr_t)&lutTbl.pLut[index];
      // Now get CDEF info
      ACDB_MEM_CPY(&tempLut, sizeof(tempLut), &lutTbl.pLut[index], sizeof(tempLut));
      ACDB_MEM_CPY(&cdefTbl.nLen, sizeof(uint32_t), (tblInfo.tblCdftChnk.pData + tempLut.nCDEFTblOffset), sizeof(uint32_t));
      cdefTbl.pCntDef = (ContentDefType *)(tblInfo.tblCdftChnk.pData + tempLut.nCDEFTblOffset + sizeof(cdefTbl.nLen));

      // Now get CDOT info
      ACDB_MEM_CPY(&cdotTbl.nLen, sizeof(uint32_t), (tblInfo.tblCdotChnk.pData + tempLut.nCDOTTblOffset), sizeof(uint32_t));
      cdotTbl.pDataOffsets = (uint32_t *)(tblInfo.tblCdotChnk.pData + tempLut.nCDOTTblOffset + sizeof(cdotTbl.nLen));

      if (cdefTbl.nLen != cdotTbl.nLen)
      {
         ACDB_DEBUG_LOG("The no of entries in CDEF and CDOT tables for appid %08X \n", audstrmcmd.nApplicationType);
         return ACDB_ERROR;
      }

      switch (queryType)
      {
      case TABLE_CMD:
      {
         AcdbAudStrmTableV2CmdType *pInput = (AcdbAudStrmTableV2CmdType *)pIn;
         AcdbQueryResponseType *pOutput = (AcdbQueryResponseType *)pOut;
         result = GetMidPidCalibTable(tblId, nLookupKey, cdefTbl, cdotTbl, tblInfo.dataPoolChnk,
            pInput->nBufferPointer, pInput->nBufferLength, &pOutput->nBytesUsedInBuffer);
         if (ACDB_SUCCESS != result)
         {
            return result;
         }
      }
      break;
      case TABLE_SIZE_CMD:
      {
         AcdbSizeResponseType *pOutput = (AcdbSizeResponseType *)pOut;
         if (ACDB_SUCCESS != GetMidPidCalibTableSize(tblId, nLookupKey, cdefTbl, cdotTbl, tblInfo.dataPoolChnk, &pOutput->nSize))
         {
            return ACDB_ERROR;
         }
      }
      break;
      case DATA_CMD:
      {
         AcdbAudStrmV2CmdType *pInput = (AcdbAudStrmV2CmdType *)pIn;
         AcdbQueryResponseType *pOutput = (AcdbQueryResponseType *)pOut;
         result = GetMidPidCalibData(tblId, nLookupKey, cdefTbl, cdotTbl, tblInfo.dataPoolChnk, pInput->nModuleId,
            pInput->nParamId, pInput->nBufferPointer, pInput->nBufferLength, &pOutput->nBytesUsedInBuffer);
         if (ACDB_SUCCESS != result)
         {
            return result;
         }
      }
      break;
      default:
         return  ACDB_ERROR;
      }
      result = ACDB_SUCCESS;
   }

   return result;
}

int32_t AcdbCmdSetAudStreamInfo(AcdbAudStrmTableV2CmdType *pInput)
{
   //Helper Structure to copy one ParamData entry's header information
   typedef struct _AudStrmTableEntryHeader {
      uint32_t nModuleId;
      uint32_t nParamId;
      uint16_t nParamSize; //multiple of 4
      uint16_t nReserved; // Must be 0
   } AudStrmTableEntryHeader;

   int32_t result = ACDB_SUCCESS;
   uint32_t remaining_bufLength = 0;
   uint32_t offSet = 0;

   AudStrmTableEntryHeader oneEntryHeader = { 0, 0, 0, 0 };
   uint8_t  *pOneEntryBuffer = NULL;

   AudStreamCmdLookupType audStrmCmnLookupCmd = { 0 };
   uint32_t persistData = FALSE;

   int32_t persist_result = 0;

   if (pInput == NULL)
   {
      ACDB_DEBUG_LOG("[ACDB Command]->[AcdbCmdSetAudProcInfo]->Invalid NULL value parameters are provided\n");
      return ACDB_BADPARM;
   }
   else if (pInput->nBufferPointer == NULL)
   {
      return ACDB_BADPARM;
   }

   remaining_bufLength = pInput->nBufferLength;

   persist_result = AcdbCmdIsPersistenceSupported(&persistData);
   if (persist_result != ACDB_SUCCESS)
   {
      persistData = FALSE;
   }
   audStrmCmnLookupCmd.nApplicationType = pInput->nApplicationTypeId;

   //Iterate over each entry
   while (remaining_bufLength > sizeof(AudStrmTableEntryHeader))
   {
      //Copy one entry header
      ACDB_MEM_CPY(&oneEntryHeader, sizeof(oneEntryHeader), pInput->nBufferPointer + offSet, sizeof(oneEntryHeader));

      //Update book keeping
      remaining_bufLength = remaining_bufLength - ((uint32_t)sizeof(oneEntryHeader));
      offSet = offSet + ((uint32_t)sizeof(oneEntryHeader));

      if (remaining_bufLength >= oneEntryHeader.nParamSize)
	  {
		  uint32_t offset = 0, size = ((AUDSTREAMTBL_INDICES_COUNT+ AUDSTREAMTBL_CDFT_INDICES_COUNT)*sizeof(uint32_t));
		  uint8_t *buffer = (uint8_t *)malloc(size);
		  if(buffer == NULL)
	      {
				ACDB_DEBUG_LOG("ACDB_COMMAND: Unable to allocate memory for AcdbCmdSetAudStreamInfo - temporary buffer\n");
				return ACDB_INSUFFICIENTMEMORY;
	      }
		  //get pointer to one entry's param data
		  pOneEntryBuffer = &pInput->nBufferPointer[offSet];

		  //Update book keeping
		  remaining_bufLength = remaining_bufLength - oneEntryHeader.nParamSize;
		  offSet = offSet + oneEntryHeader.nParamSize;

		  ACDB_MEM_CPY(buffer, size, (uint8_t *)&audStrmCmnLookupCmd, AUDSTREAMTBL_INDICES_COUNT *sizeof(uint32_t));
		  offset += (uint32_t)(AUDSTREAMTBL_INDICES_COUNT *sizeof(uint32_t));
		  ACDB_MEM_CPY(buffer + offset, sizeof(uint32_t), &oneEntryHeader.nModuleId, sizeof(uint32_t));
		  offset += ((uint32_t)sizeof(uint32_t));
		  ACDB_MEM_CPY(buffer + offset, sizeof(uint32_t), &oneEntryHeader.nParamId, sizeof(uint32_t));
		  offset += ((uint32_t)sizeof(uint32_t));

		  //Set one ParamData entry
		  result = AcdbCmdSetOnlineDataV2(persistData, AUD_STREAM_TBL, buffer, AUDSTREAMTBL_INDICES_COUNT, AUDSTREAMTBL_CDFT_INDICES_COUNT, pOneEntryBuffer, oneEntryHeader.nParamSize);

		  if (result != ACDB_SUCCESS)
		  {
			  ACDB_DEBUG_LOG("[ACDB Command]->[AcdbCmdSetAudProcInfo]->Err:%0x Failed to set AudProcData for...\n", result);
			  ACDB_DEBUG_LOG("ModID:%0x PID:%0x ParamSize:%0x \n", oneEntryHeader.nModuleId, oneEntryHeader.nParamId, oneEntryHeader.nParamSize);
			  return result;
		  }
	  }
      else
      {
         ACDB_DEBUG_LOG("[ACDB Command]->[AcdbCmdSetAudProcInfo]->Unexpected buffer length. Residual of %u bytes found\n", remaining_bufLength);
         result = ACDB_BADPARM;
         return result;
      }
   }

   if (remaining_bufLength > 0)
   {
      ACDB_DEBUG_LOG("[ACDB Command]->[AcdbCmdSetAudProcInfo]->Unexpected buffer length. Residual of %u bytes found\n", remaining_bufLength);
   }

   if (result == ACDB_SUCCESS &&
      ACDB_INIT_SUCCESS == AcdbIsPersistenceSupported())
   {
      result = AcdbCmdSaveDeltaFileData();
      if (result != ACDB_SUCCESS)
      {
         ACDB_DEBUG_LOG("[ACDB Command]->[AcdbCmdSetAudStreamData]->Unable to save delta file data\n");
      }
   }

   result = ACDB_SUCCESS;

   return result;
}

int32_t AcdbCmdSetAudStreamData(AcdbAudStrmV2CmdType *pInput)
{
   int32_t result = ACDB_SUCCESS;
   uint32_t persistData = FALSE;
   int32_t persist_result = AcdbCmdIsPersistenceSupported(&persistData);
   if (persist_result != ACDB_SUCCESS)
   {
      persistData = FALSE;
   }

   if (pInput == NULL)
   {
      ACDB_DEBUG_LOG("[ACDB Command]->[AcdbCmdSetAudStreamData]->Invalid NULL value parameters are provided\n");
      result = ACDB_BADPARM;
   }
   else if (pInput->nBufferPointer == NULL)
   {
      return ACDB_BADPARM;
   }
   else
   {
      const uint32_t tblId = AUD_STREAM_TBL;
      AudStreamCmdLookupType audstrmcmd = { 0 };
	  uint32_t offset = 0, size = ((AUDSTREAMTBL_INDICES_COUNT+ AUDSTREAMTBL_CDFT_INDICES_COUNT)*sizeof(uint32_t));
	  uint8_t *buffer = (uint8_t *)malloc(size);
	  if(buffer == NULL)
	  {
			ACDB_DEBUG_LOG("ACDB_COMMAND: Unable to allocate memory for AcdbCmdSetAudStreamData - temporary buffer\n");
			return ACDB_INSUFFICIENTMEMORY;
	  }
      audstrmcmd.nApplicationType = pInput->nApplicationTypeId;

	  ACDB_MEM_CPY(buffer, size, (uint8_t *)&audstrmcmd, AUDSTREAMTBL_INDICES_COUNT *sizeof(uint32_t));
	  offset += ((uint32_t)(AUDSTREAMTBL_INDICES_COUNT *sizeof(uint32_t)));
	  ACDB_MEM_CPY(buffer + offset, sizeof(uint32_t), &pInput->nModuleId, sizeof(uint32_t));
	  offset += ((uint32_t)sizeof(uint32_t));
	  ACDB_MEM_CPY(buffer + offset, sizeof(uint32_t), &pInput->nParamId, sizeof(uint32_t));
	  offset += ((uint32_t)sizeof(uint32_t));
	  result = AcdbCmdSetOnlineDataV2(persistData, tblId, buffer, AUDSTREAMTBL_INDICES_COUNT, AUDSTREAMTBL_CDFT_INDICES_COUNT, pInput->nBufferPointer, pInput->nBufferLength);

      if (result == ACDB_SUCCESS &&
         ACDB_INIT_SUCCESS == AcdbIsPersistenceSupported())
      {
         result = AcdbCmdSaveDeltaFileData();

         if (result != ACDB_SUCCESS)
         {
            ACDB_DEBUG_LOG("[ACDB Command]->[AcdbCmdSetAudStreamData]->Unable to save delta file data\n");
         }
      }
   }

   return result;
}

int32_t AcdbCmdGetVocProcStatInfo(uint32_t queryType, uint8_t *pIn, uint8_t *pOut)
{
   int32_t result = ACDB_SUCCESS;
   uintptr_t nLookupKey = 0;
   uintptr_t cdftLookup = 0;
   struct node *lut_cdft = NULL, *lut_cdot = NULL, *lut_cdft_buffer = NULL, *lut_cdot_buffer = NULL, *lut_data_buffer = NULL, *lut_data = NULL, *lut_cdot_new = NULL, *lut_cdft_new = NULL;
   int size_cdft = 0, size_cdot = 0, size_data = 0, lut_cdot_entries = 0, lut_cdft_entries = 0, lut_data_entries = 0;

   if (pIn == NULL || pOut == NULL)
   {
      ACDB_DEBUG_LOG("[ACDB Command]->[AcdbCmdGetVocProcStatInfo]->Invalid Null input param provided\n");
      result = ACDB_BADPARM;
   }
   else
   {
      uint32_t LookUptble_Length = 0, lut_index_count = 0, lut_index = 0, major = 1, minor = 0, ofTbloffset = 0, cvdTbloffset = 0, lutLength = 0;
      int cdft_start = 0, cdot_start = 0, i = 0, j = 0, k = 0, l = 0, m = 0, temp_size = 0;
      int cdft_offset = 0, cdot_offset = 0, datapool_offset = 0, datapool_start = 0;
      uintptr_t data_Seclookup = 0;
      int lut_cdftfound = 0, lut_cdotfound = 0, calc_data = 0, heapsize = 0;
	  VocProcStatCVDTblHdrFmtType temp_lut_node = { 0, 0, 0, 0, 0, 0, 0, 0, 0};
	  AcdbDataType cData = { 0, NULL };
      uint32_t index = 0, Valid_length = 0;
      AcdbTableCmd cmd = { 0, 0 };
	  AcdbTableInfoCVD tblInfo = { { 0, NULL }, { 0, NULL }, { 0, NULL }, { 0, NULL }, { 0, NULL }, { 0, NULL } };
	  VocProcStatDataLookupType_UniqueData tempLut = { 0, 0, 0, 0, 0, 0};
	  VocProcStatDataLookupTblType_UniqueData lutTbl = { 0, NULL };
	  VocProcStatDataLookupTblType_CommonData lutTblCVD = { 0, NULL };
      VocProcStatDataLookupTblType_offsetData lutoffsetTbl = { 0, NULL };
      ContentDefTblType cdefTbl = { 0, NULL };
      ContentDataOffsetsTblType cdotTbl = { 0, NULL };
      uint32_t offset = 0;
      uint32_t nMemBytesLeft = 0, nPaddedBytes = 0;
	  VocProcCmdLookupType voccmd = { 0, 0, 0, 0 };
      uint32_t tblId = VOCPROC_STATIC_TBL;
      int temp_pointer = 0, extraheap_cdot = 0, isheapentryfound = 0;
      switch (queryType)
      {
      case TABLE_CMD:
      {
         AcdbVocProcCmnTblCmdType *pInput = (AcdbVocProcCmnTblCmdType *)pIn;
         if (pInput->nBufferPointer == NULL)
            return ACDB_BADPARM;
         voccmd.nTxDevId = pInput->nTxDeviceId;
         voccmd.nRxDevId = pInput->nRxDeviceId;
         voccmd.nTxAfeSr = pInput->nTxDeviceSampleRateId;
         voccmd.nRxAfeSr = pInput->nRxDeviceSampleRateId;
      }
      break;
      case TABLE_SIZE_CMD:
      {
         AcdbVocProcCmnTblSizeCmdType *pInput = (AcdbVocProcCmnTblSizeCmdType *)pIn;
         voccmd.nTxDevId = pInput->nTxDeviceId;
         voccmd.nRxDevId = pInput->nRxDeviceId;
         voccmd.nTxAfeSr = pInput->nTxDeviceSampleRateId;
         voccmd.nRxAfeSr = pInput->nRxDeviceSampleRateId;
      }
      break;
      default:
         result = ACDB_ERROR;
         goto end;
      }

      cmd.devId = voccmd.nTxDevId;
      cmd.tblId = tblId;
      result = acdbdata_ioctl(ACDBDATACMD_GET_TABLE_INFO, (uint8_t *)&cmd, sizeof(cmd),
         (uint8_t *)&tblInfo, sizeof(tblInfo));
      if (result != ACDB_SUCCESS)
      {
         if (result == ACDB_DATA_INTERFACE_NOT_FOUND)
         {
            ACDB_DEBUG_LOG("Failed to fetch the Table in ACDB files Table-ID %08X \n"
               , tblId);
            goto end;
         }
         ACDB_DEBUG_LOG("Failed to fetch the lookup information of the device %08X \n"
            , voccmd.nTxDevId);
         goto end;
      }
      lutTbl.nLen = READ_UInt32(tblInfo.tblLutChnk.pData);
      lutTbl.pLut = (VocProcStatDataLookupType_UniqueData *)(tblInfo.tblLutChnk.pData + sizeof(lutTbl.nLen));

      result = AcdbDataBinarySearch((void *)lutTbl.pLut, lutTbl.nLen,
         VOCPROCSTAT_LUT_INDICES_COUNT - 7, &voccmd, VOCPROCSTAT_CMD_INDICES_COUNT, &index);

      if (result != SEARCH_SUCCESS)
      {
         ACDB_DEBUG_LOG("Failed to fetch the lookup information of the device %08X \n"
            , voccmd.nTxDevId);
         result = ACDB_INPUT_PARAMS_NOT_FOUND;
         goto end;
      }
      data_Seclookup = (uintptr_t)&lutTbl.pLut[index];
      lutTblCVD.nLen = READ_UInt32(tblInfo.tblLutCVDChnk.pData);
      lutTblCVD.pLut = (VocProcStatDataLookupType_CommonData *)(tblInfo.tblLutCVDChnk.pData + sizeof(lutTbl.nLen));

      lutoffsetTbl.nLen = READ_UInt32(tblInfo.tblLutOffsetChnk.pData);
      lutoffsetTbl.pLut = (VocProcStatDataLookupType_offsetData *)(tblInfo.tblLutOffsetChnk.pData + sizeof(lutTbl.nLen));

      cdefTbl.nLen = READ_UInt32(tblInfo.tblCdftChnk.pData);
      cdefTbl.pCntDef = (ContentDefType *)(tblInfo.tblCdftChnk.pData + sizeof(cdefTbl.nLen));

      if (cdefTbl.nLen <= 0)
      {
         ACDB_DEBUG_LOG("Vocprocstatic cdefTbl empty\n");
         result = ACDB_ERROR;
         goto end;
      }
      cdotTbl.nLen = READ_UInt32(tblInfo.tblCdotChnk.pData);
      cdotTbl.pDataOffsets = (uint32_t *)(tblInfo.tblCdotChnk.pData + sizeof(cdotTbl.nLen));
      if (cdotTbl.nLen <= 0)
      {
         ACDB_DEBUG_LOG("Vocprocstatic cdotTbl empty\n");
         result = ACDB_ERROR;
         goto end;
      }
      ACDB_MEM_CPY(&tempLut, sizeof(tempLut), &lutTbl.pLut[index], sizeof(tempLut));
      // Go to the offset table from luptableunique
	  ofTbloffset = tempLut.nOffTableOffset;
	  cvdTbloffset = tempLut.nCommonTblOffset;
      ACDB_MEM_CPY(&lutLength, sizeof(uint32_t), tblInfo.tblLutOffsetChnk.pData + ofTbloffset, sizeof(uint32_t));
      lutoffsetTbl.pLut = (VocProcStatDataLookupType_offsetData *)(tblInfo.tblLutOffsetChnk.pData + sizeof(uint32_t) + ofTbloffset);
      lutTblCVD.pLut = (VocProcStatDataLookupType_CommonData *)(tblInfo.tblLutCVDChnk.pData + sizeof(uint32_t) + cvdTbloffset);

      if (lutLength == 0)
      {
         lutLength = lutTblCVD.nLen;
      }

      j = 0, k = 0;
      for (lut_index = 0; lut_index < (int)lutLength; lut_index++)
      {
         uint32_t pid = 0;
		 VocProcStatDataLookupType_offsetData offsettbleinstance = { 0, 0 };//=(VocProcStatDataLookupType_offsetData*)(&lutoffsetTbl.pLut[lut_index]);
         int32_t calc_cdft = 0, calc_cdot = 0, lut_datafound = 0, dataofst = 0;
         ACDB_MEM_CPY(&offsettbleinstance, sizeof(VocProcStatDataLookupType_offsetData), &lutoffsetTbl.pLut[lut_index], sizeof(VocProcStatDataLookupType_offsetData));
         //datatable calculation
         cdotTbl.nLen = READ_UInt32(tblInfo.tblCdotChnk.pData + offsettbleinstance.nCDOTTblOffset);
         cdotTbl.pDataOffsets = (uint32_t *)(tblInfo.tblCdotChnk.pData + offsettbleinstance.nCDOTTblOffset + sizeof(tblInfo.tblCdotChnk.nDataLen));
         {
            cdefTbl.nLen = READ_UInt32(tblInfo.tblCdftChnk.pData + offsettbleinstance.nCDEFTblOffset);
            cdefTbl.pCntDef = (ContentDefType *)(tblInfo.tblCdftChnk.pData + offsettbleinstance.nCDEFTblOffset + sizeof(tblInfo.tblCdftChnk.nDataLen));
         }
         if (!update(offsettbleinstance.nCDOTTblOffset, lut_index, &lut_cdot_new))
         {
            result = ACDB_INSUFFICIENTMEMORY;
            goto end;
         }

         if (!update(offsettbleinstance.nCDEFTblOffset, lut_index, &lut_cdft_new))
         {
            result = ACDB_INSUFFICIENTMEMORY;
            goto end;
         }
         if (!update(0, 0, &lut_data_buffer))
         {
            result = ACDB_INSUFFICIENTMEMORY;
            goto end;
         }
         isheapentryfound = 0;
         for (m = 0; m < (int)cdotTbl.nLen; m++)
         {
            ACDB_MEM_CPY(&dataofst, sizeof(dataofst), (void*)(&cdotTbl.pDataOffsets[m]), sizeof(dataofst));
			//mid = cdefTbl.pCntDef[m].nMid;
			pid = cdefTbl.pCntDef[m].nPid;
            nLookupKey = (uintptr_t)&lutTblCVD.pLut[lut_index];
            cdftLookup = (uintptr_t)&cdefTbl.pCntDef[m];
            result = GetMidPidCalibHeapDataEx(tblId, nLookupKey, data_Seclookup, cdftLookup, pid, &cData);//get from heap
            if (result == ACDB_SUCCESS)
            {
               nPaddedBytes = 0;
               if (cData.nLen % 4)
               {
                  nPaddedBytes = 4 - cData.nLen % 4;
               }
               heapsize += cData.nLen + nPaddedBytes;
               heapsize += ((uint32_t)sizeof(cData.nLen));
               isheapentryfound = 1;
            }
            lut_datafound = 0;
            if (Find(dataofst, &lut_data, &i))// if data is already present skip it , fill the buffer with only unique list of cdft,cdot,data pool entires.
            {
               lut_datafound = 1;
            }
            if (!lut_datafound)//fill data pool entries
            {
               if (!update(dataofst, l, &lut_data))
               {
                  result = ACDB_INSUFFICIENTMEMORY;
                  goto end;
               }

               {
                  calc_data = *((int *)(tblInfo.dataPoolChnk.pData + dataofst));
                  nPaddedBytes = 0;
                  if (calc_data % 4)
                  {
                     nPaddedBytes = 4 - calc_data % 4;
                  }
                  if (!update(calc_data + nPaddedBytes + (int32_t)sizeof(uint32_t) + GetAt(l, &lut_data_buffer), l + 1, &lut_data_buffer))
                  {
                     result = ACDB_INSUFFICIENTMEMORY;
                     goto end;
                  }

                  size_data = GetAt(l + 1, &lut_data_buffer);
               }
               l++;
            }
         }
         if (isheapentryfound == 1)
         {
            extraheap_cdot += ((uint32_t)sizeof(cdotTbl.nLen));
            extraheap_cdot += ((uint32_t)(sizeof(cdotTbl.pDataOffsets)*cdotTbl.nLen));
         }
         //cdft table parsing

         lut_cdftfound = 0;
         if (!update(0, 0, &lut_cdft_buffer))
         {
            result = ACDB_INSUFFICIENTMEMORY;
            goto end;
         }

         if (Find(lutoffsetTbl.pLut[lut_index].nCDEFTblOffset, &lut_cdft, &i))
         {
            lut_cdftfound = 1;
         }
         if (!lut_cdftfound)
         {
            if (!update(lutoffsetTbl.pLut[lut_index].nCDEFTblOffset, j, &lut_cdft))
            {
               result = ACDB_INSUFFICIENTMEMORY;
               goto end;
            }
            {
               //calc_cdft= *((int *)(tblInfo.tblCdftChnk.pData + offsettbleinstance.nCDEFTblOffset));
               ACDB_MEM_CPY(&calc_cdft, sizeof(int32_t), tblInfo.tblCdftChnk.pData + offsettbleinstance.nCDEFTblOffset, sizeof(int32_t));

               if (!update((int32_t)(calc_cdft*sizeof(uint32_t) * 2 + sizeof(uint32_t) + GetAt(j, &lut_cdft_buffer)), j + 1, &lut_cdft_buffer))
               {
                  result = ACDB_INSUFFICIENTMEMORY;
                  goto end;
               }
               size_cdft = GetAt(j + 1, &lut_cdft_buffer);
            }
            if (queryType == TABLE_CMD)
               if (!update(GetAt(j, &lut_cdft_buffer), lut_index, &lut_cdft_new))
               {
                  result = ACDB_INSUFFICIENTMEMORY;
                  goto end;
               }
            j++;
         }
         else
            if (queryType == TABLE_CMD)
               if (!update(GetAt(i, &lut_cdft_buffer), lut_index, &lut_cdft_new))
               {
                  result = ACDB_INSUFFICIENTMEMORY;
                  goto end;
               }

         //calculate cdot
         lut_cdotfound = 0;
         if (!update(0, 0, &lut_cdot_buffer))
         {
            result = ACDB_INSUFFICIENTMEMORY;
            goto end;
         }

         if (Find(offsettbleinstance.nCDOTTblOffset, &lut_cdot, &i))
         {
            lut_cdotfound = 1;
         }
         if (!lut_cdotfound)
         {
            if (!update(offsettbleinstance.nCDOTTblOffset, k, &lut_cdot))
            {
               result = ACDB_INSUFFICIENTMEMORY;
               goto end;
            }
                  {
                     ACDB_MEM_CPY(&calc_cdot, sizeof(int32_t), tblInfo.tblCdotChnk.pData + offsettbleinstance.nCDOTTblOffset, sizeof(int32_t));
                     update((int32_t)(calc_cdot*sizeof(uint32_t) + sizeof(uint32_t) + GetAt(k, &lut_cdot_buffer)), k + 1, &lut_cdot_buffer);

                     size_cdot = GetAt(k + 1, &lut_cdot_buffer);
                  }
                  if (queryType == TABLE_CMD)
                     if (!update(GetAt(k, &lut_cdot_buffer), lut_index, &lut_cdot_new))
                     {
                        result = ACDB_INSUFFICIENTMEMORY;
                        goto end;
                     }
                  k++;
         }
         else
            if (queryType == TABLE_CMD)
               if (!update(GetAt(i, &lut_cdot_buffer), lut_index, &lut_cdot_new))
               {
                  result = ACDB_INSUFFICIENTMEMORY;
                  goto end;
               }
      }
      lut_index_count = lut_index;
      lut_cdft_entries = j; lut_cdot_entries = k; lut_data_entries = l;
      lut_index = 0;
      //add the data
      if (queryType == TABLE_CMD)// write lut size for the first time
      {
         AcdbVocProcCmnTblCmdType *pInput = (AcdbVocProcCmnTblCmdType *)pIn;
         AcdbQueryResponseType *pOutput = (AcdbQueryResponseType *)pOut;
         int dataOffset = 0;
         nMemBytesLeft = pInput->nBufferLength;
         LookUptble_Length = ((uint32_t)((lut_index_count*sizeof(uint32_t)*(VOCPROCSTAT_LUT_INDICES_COUNT - 4)) + sizeof(uint32_t)));//lookup table no_of_entries+data
         if (nMemBytesLeft < (uint32_t)LookUptble_Length)
         {
            ACDB_DEBUG_LOG("Insufficient memory to copy the vocproc static table data\n");
            result = ACDB_INSUFFICIENTMEMORY;
            goto end;
         }
         // write major and minor cvd versions.
         ACDB_MEM_CPY(pInput->nBufferPointer + offset, nMemBytesLeft, &major, sizeof(uint32_t));//Major version
         offset += ((uint32_t)sizeof(uint32_t));
         ACDB_MEM_CPY(pInput->nBufferPointer + offset, nMemBytesLeft, &minor, sizeof(uint32_t));//minor version
         offset += ((uint32_t)sizeof(uint32_t));
         ACDB_MEM_CPY(pInput->nBufferPointer + offset, nMemBytesLeft, &LookUptble_Length, sizeof(uint32_t));//length of the table
         offset += ((uint32_t)sizeof(uint32_t));
         ACDB_MEM_CPY(pInput->nBufferPointer + offset, nMemBytesLeft, &lut_index_count, sizeof(uint32_t));//no of entries
         offset += ((uint32_t)sizeof(uint32_t));
         LookUptble_Length += ((uint32_t)(3 * sizeof(uint32_t)));//major and minor versions + table length
         cdft_offset = cdft_start = LookUptble_Length, cdft_offset = LookUptble_Length;
         //cdft table
         temp_size = size_cdft;//+ sizeof(temp_size);
         ACDB_MEM_CPY(pInput->nBufferPointer + cdft_start, sizeof(int32_t), (void*)(&temp_size), sizeof(temp_size));
         cdft_offset += (int32_t)(sizeof(tblInfo.tblCdftChnk.nDataLen));
         for (k = 0; k < lut_cdft_entries; k++)//fill cdft table in buffer
         {
            dataOffset = GetAt(k, &lut_cdft);
            cdefTbl.nLen = READ_UInt32(tblInfo.tblCdftChnk.pData + dataOffset);
            cdefTbl.pCntDef = (ContentDefType *)(tblInfo.tblCdftChnk.pData + dataOffset + sizeof(tblInfo.tblCdftChnk.nDataLen));
            ACDB_MEM_CPY(pInput->nBufferPointer + cdft_offset, sizeof(cdefTbl.nLen), (void*)(&cdefTbl.nLen), sizeof(cdefTbl.nLen));
            cdft_offset += ((int32_t)sizeof(cdefTbl.nLen));
            dataOffset += ((int32_t)sizeof(cdefTbl.nLen));
            for (i = 0; i < (int)cdefTbl.nLen; i++)
            {
               ACDB_MEM_CPY(pInput->nBufferPointer + cdft_offset, sizeof(cdefTbl.pCntDef[i].nMid), (void*)(&cdefTbl.pCntDef[i].nMid), sizeof(cdefTbl.pCntDef[i].nMid));
               cdft_offset += ((int32_t)sizeof(cdefTbl.pCntDef[i].nMid));
               dataOffset += ((int32_t)sizeof(cdefTbl.pCntDef[i].nMid));
               ACDB_MEM_CPY(pInput->nBufferPointer + cdft_offset, sizeof(cdefTbl.pCntDef[i].nPid), (void*)(&cdefTbl.pCntDef[i].nPid), sizeof(cdefTbl.pCntDef[i].nPid));
               cdft_offset += ((int32_t)sizeof(cdefTbl.pCntDef[i].nPid));
               dataOffset += ((int32_t)sizeof(cdefTbl.pCntDef[i].nPid));
            }
         }

         temp_size = cdft_offset - cdft_start;//calculate cdft temp size
         cdot_start = cdot_offset = cdft_offset;
         //write cdot also
         dataOffset = 0;
         temp_size = size_cdot + extraheap_cdot;//+ sizeof(temp_size);

         datapool_start = datapool_offset = (cdot_offset + size_cdot + (int32_t)sizeof(size_cdot) + (int32_t)sizeof(temp_size) + extraheap_cdot); //reserve the space to write data size.//have the
         ACDB_MEM_CPY(pInput->nBufferPointer + cdot_start, sizeof(size_cdot), (void*)(&temp_size), sizeof(size_cdot));
         cdot_offset += (int32_t)sizeof(size_cdot);

         for (k = 0; k < lut_cdot_entries; k++)//fill cdot table in buffer
         {
            int dataoffset_for_actualbuffer = 0;
            dataOffset = GetAt(k, &lut_cdot);
            cdotTbl.nLen = READ_UInt32(tblInfo.tblCdotChnk.pData + dataOffset);
            cdotTbl.pDataOffsets = (uint32_t *)(tblInfo.tblCdotChnk.pData + dataOffset + sizeof(tblInfo.tblCdotChnk.nDataLen));
            ACDB_MEM_CPY(pInput->nBufferPointer + cdot_offset, sizeof(cdotTbl.nLen), (void*)(&cdotTbl.nLen), sizeof(cdotTbl.nLen));
            cdot_offset += (int32_t)sizeof(cdotTbl.nLen);
            dataOffset += (int32_t)sizeof(cdotTbl.nLen);
            for (i = 0; i < (int)cdotTbl.nLen; i++)
            {
               //calculate actual buffer offset
               for (l = 0; l < lut_data_entries; l++)
               {
                  if (cdotTbl.pDataOffsets[i] == (uint32_t)GetAt(l, &lut_data))
                  {
                     dataoffset_for_actualbuffer = GetAt(l, &lut_data_buffer);
                     break;
                  }
               }
               ACDB_MEM_CPY(pInput->nBufferPointer + cdot_offset, sizeof(dataoffset_for_actualbuffer), (void*)(&dataoffset_for_actualbuffer), sizeof(dataoffset_for_actualbuffer));
               cdot_offset += (int32_t)sizeof(cdotTbl.pDataOffsets[i]);
               dataOffset += (int32_t)sizeof(cdotTbl.pDataOffsets[i]);
            }
         }
         temp_pointer = cdot_offset;

         //datapool_offset=temp_pointer;

         for (l = 0; l < lut_data_entries; l++)//fill data pool entries table in buffer
         {
            dataOffset = GetAt(l, &lut_data);
            cData.nLen = READ_UInt32(tblInfo.dataPoolChnk.pData + dataOffset);
            cData.pData = tblInfo.dataPoolChnk.pData + dataOffset + sizeof(cData.nLen);
            nPaddedBytes = 0;
            if (cData.nLen % 4)
            {
               nPaddedBytes = 4 - cData.nLen % 4;
            }
            Valid_length = cData.nLen + nPaddedBytes;
            ACDB_MEM_CPY(pInput->nBufferPointer + datapool_offset, sizeof(cData.nLen), (void*)&(Valid_length), sizeof(cData.nLen));
            datapool_offset += (int32_t)sizeof(cData.nLen);
            memset((pInput->nBufferPointer + datapool_offset), 0, (size_t)(cData.nLen + nPaddedBytes));
            ACDB_MEM_CPY(pInput->nBufferPointer + datapool_offset, cData.nLen, (void*)(cData.pData), cData.nLen);
            datapool_offset += cData.nLen + nPaddedBytes;
         }
         //fill heap entries.
         if (heapsize > 0)
         {
            for (lut_index = 0; lut_index<(int)lutLength; lut_index++)
            {
               int dataofst = 0;
               int mid = 0, pid = 0;
               isheapentryfound = 0;
               //datatable calculation
               cdotTbl.nLen = READ_UInt32(pInput->nBufferPointer + cdot_start + 4 + GetAt(lut_index, &lut_cdot_new));
               cdotTbl.pDataOffsets = (uint32_t *)(pInput->nBufferPointer + cdot_start + 4 + GetAt(lut_index, &lut_cdot_new) + sizeof(tblInfo.tblCdotChnk.nDataLen));

               cdefTbl.nLen = READ_UInt32(pInput->nBufferPointer + cdft_start + 4 + GetAt(lut_index, &lut_cdft_new));
               cdefTbl.pCntDef = (ContentDefType *)(pInput->nBufferPointer + cdft_start + 4 + GetAt(lut_index, &lut_cdft_new)/*offsettbleinstance->nCDEFTblOffset*/ + sizeof(tblInfo.tblCdftChnk.nDataLen));
               //reserve space
               if (temp_pointer + (int)cdotTbl.nLen*(int)sizeof(cdotTbl.pDataOffsets) + 4>(datapool_start - (int)sizeof(uint32_t)))//reached max cdot entries
               {
                  continue;
               }
               ACDB_MEM_CPY(pInput->nBufferPointer + temp_pointer, sizeof(cdotTbl.nLen), (void*)(&cdotTbl.nLen), sizeof(cdotTbl.nLen));

               ACDB_MEM_CPY(pInput->nBufferPointer + temp_pointer + 4, cdotTbl.nLen*sizeof(cdotTbl.pDataOffsets), (void*)cdotTbl.pDataOffsets, cdotTbl.nLen*sizeof(cdotTbl.pDataOffsets));

               cdotTbl.nLen = READ_UInt32(pInput->nBufferPointer + temp_pointer);
               cdotTbl.pDataOffsets = (uint32_t *)(pInput->nBufferPointer + temp_pointer + sizeof(tblInfo.tblCdotChnk.nDataLen));

               //fill heap
               for (m = 0; m < (int)cdotTbl.nLen; m++)
               {
                  ACDB_MEM_CPY(&dataofst, sizeof(dataofst), (void*)(&cdotTbl.pDataOffsets[m]), sizeof(dataofst));
                  ACDB_MEM_CPY(&mid, sizeof(dataofst), (void*)(&cdefTbl.pCntDef[m].nMid), sizeof(dataofst));
                  ACDB_MEM_CPY(&pid, sizeof(dataofst), (void*)(&cdefTbl.pCntDef[m].nPid), sizeof(dataofst));
                  nLookupKey = (uintptr_t)&lutTblCVD.pLut[lut_index];
                  cdftLookup = (uintptr_t)&cdefTbl.pCntDef[m];
                  result = GetMidPidCalibHeapDataEx(tblId, nLookupKey, data_Seclookup, cdftLookup, pid, &cData);//get from heap
                  if (ACDB_SUCCESS == result)
                  {
                     nPaddedBytes = 0;
                     if (cData.nLen % 4)
                     {
                        nPaddedBytes = 4 - cData.nLen % 4;
                     }
                     Valid_length = cData.nLen + nPaddedBytes;
                     cdotTbl.pDataOffsets[m] = datapool_offset - datapool_start;
                     ACDB_MEM_CPY(pInput->nBufferPointer + datapool_offset, sizeof(cData.nLen), (void*)(&Valid_length), sizeof(cData.nLen));
                     datapool_offset += (int32_t)sizeof(cData.nLen);
                     memset((pInput->nBufferPointer + datapool_offset), 0, (size_t)(cData.nLen + nPaddedBytes));
                     ACDB_MEM_CPY(pInput->nBufferPointer + datapool_offset, cData.nLen, (void*)(cData.pData), cData.nLen);
                     datapool_offset += cData.nLen + nPaddedBytes;
                     if (!update(temp_pointer - cdot_start - (int32_t)sizeof(uint32_t), lut_index, &lut_cdot_new))
                     {
                        result = ACDB_INSUFFICIENTMEMORY;
                        goto end;
                     }
                     isheapentryfound = 1;
                  }
               }
               if (isheapentryfound == 1)
               {
                  temp_pointer += (int32_t)sizeof(cdotTbl.nLen);
                  temp_pointer += (int32_t)(cdotTbl.nLen*sizeof(cdotTbl.pDataOffsets));
                  if (temp_pointer >= (datapool_start - (int)sizeof(uint32_t)))//reached max cdot entries
                     break;
               }
            }
         }

         j = datapool_offset - temp_pointer - (int32_t)sizeof(uint32_t);
         ACDB_MEM_CPY(pInput->nBufferPointer + temp_pointer, sizeof(size_cdot), (void*)(&j), sizeof(size_cdot));
         nMemBytesLeft -= datapool_offset;
         pOutput->nBytesUsedInBuffer = datapool_offset;
      }
      else
      {
         AcdbSizeResponseType *pOutput = (AcdbSizeResponseType *)pOut;
         LookUptble_Length = (uint32_t)((lut_index_count*sizeof(uint32_t)*(VOCPROCSTAT_LUT_INDICES_COUNT - 4)) + sizeof(uint32_t));
         LookUptble_Length += (uint32_t)(3 * sizeof(uint32_t));//major and minor versions
         offset += LookUptble_Length;
		 offset += (uint32_t)sizeof(size_cdft);
         offset += size_cdft;
         offset += (uint32_t)sizeof(size_cdot);
         offset += size_cdot;
         offset += (uint32_t)sizeof(uint32_t);//reseve for data size chunk
         offset += size_data;
         offset += heapsize;
         offset += extraheap_cdot;
         pOutput->nSize = offset;
         result = ACDB_SUCCESS;
         goto end;
      }
      for (lut_index = 0; lut_index < lutLength; lut_index++)
      {
         //go the cvd common table to get actual common entry
		  VocProcStatDataLookupType_CommonData commonLUTInstance = { 0, 0, 0, 0, 0, 0, 0 };
         ACDB_MEM_CPY(&commonLUTInstance, sizeof(VocProcStatDataLookupType_CommonData), (tblInfo.tblLutCVDChnk.pData + sizeof(uint32_t) + cvdTbloffset + sizeof(VocProcStatDataLookupType_CommonData)*lut_index), sizeof(VocProcStatDataLookupType_CommonData));

         switch (queryType)
         {
         case TABLE_CMD:
         {
            //fill lut entries in CVD format
            AcdbVocProcCmnTblCmdType *pInput = (AcdbVocProcCmnTblCmdType *)pIn;
            ACDB_MEM_CPY(&temp_lut_node.nNetworkId, sizeof(uint32_t), &commonLUTInstance.nNetwork, sizeof(uint32_t));
            ACDB_MEM_CPY(&temp_lut_node.nRxVocSr, sizeof(uint32_t), &commonLUTInstance.nRxVocSR, sizeof(uint32_t));
            ACDB_MEM_CPY(&temp_lut_node.nTxVocSr, sizeof(uint32_t), &commonLUTInstance.nTxVocSR, sizeof(uint32_t));
            ACDB_MEM_CPY(&temp_lut_node.nVocoder_type, sizeof(uint32_t), &commonLUTInstance.nVocoder_type, sizeof(uint32_t));
            ACDB_MEM_CPY(&temp_lut_node.rx_Voc_mode, sizeof(uint32_t), &commonLUTInstance.rx_Voc_mode, sizeof(uint32_t));
            ACDB_MEM_CPY(&temp_lut_node.tx_Voc_mode, sizeof(uint32_t), &commonLUTInstance.tx_Voc_mode, sizeof(uint32_t));
            ACDB_MEM_CPY(&temp_lut_node.nFeature, sizeof(uint32_t), &commonLUTInstance.nFeature, sizeof(uint32_t));
            temp_lut_node.cdft_offset = GetAt(lut_index, &lut_cdft_new);//lutoffsetTbl.pLut[lut_index].nCDEFTblOffset;
            temp_lut_node.cdot_offset = GetAt(lut_index, &lut_cdot_new);
            ACDB_MEM_CPY(pInput->nBufferPointer + offset, sizeof(temp_lut_node.nNetworkId), (void*)(&temp_lut_node.nNetworkId), sizeof(temp_lut_node.nNetworkId));
            offset += (uint32_t)sizeof(temp_lut_node.nNetworkId);
            ACDB_MEM_CPY(pInput->nBufferPointer + offset, sizeof(temp_lut_node.nTxVocSr), (void*)(&temp_lut_node.nTxVocSr), sizeof(temp_lut_node.nTxVocSr));
            offset += (uint32_t)sizeof(temp_lut_node.nTxVocSr);
            ACDB_MEM_CPY(pInput->nBufferPointer + offset, sizeof(temp_lut_node.nRxVocSr), (void*)(&temp_lut_node.nRxVocSr), sizeof(temp_lut_node.nRxVocSr));
            offset += (uint32_t)sizeof(temp_lut_node.nRxVocSr);

            ACDB_MEM_CPY(pInput->nBufferPointer + offset, sizeof(temp_lut_node.tx_Voc_mode), (void*)(&temp_lut_node.tx_Voc_mode), sizeof(temp_lut_node.tx_Voc_mode));
            offset += (uint32_t)sizeof(temp_lut_node.tx_Voc_mode);
            //one more time for CDOT OFFSET. here both are same.
            ACDB_MEM_CPY(pInput->nBufferPointer + offset, sizeof(temp_lut_node.rx_Voc_mode), (void*)(&temp_lut_node.rx_Voc_mode), sizeof(temp_lut_node.rx_Voc_mode));
            offset += (uint32_t)sizeof(temp_lut_node.rx_Voc_mode);
            ACDB_MEM_CPY(pInput->nBufferPointer + offset, sizeof(temp_lut_node.nVocoder_type), (void*)(&temp_lut_node.nVocoder_type), sizeof(temp_lut_node.nVocoder_type));
            offset += (uint32_t)sizeof(temp_lut_node.nVocoder_type);
            //one more time for CDOT OFFSET. here both are same.
            ACDB_MEM_CPY(pInput->nBufferPointer + offset, sizeof(temp_lut_node.nFeature), (void*)(&temp_lut_node.nFeature), sizeof(temp_lut_node.nFeature));
            offset += (uint32_t)sizeof(temp_lut_node.nFeature);
            ACDB_MEM_CPY(pInput->nBufferPointer + offset, sizeof(temp_lut_node.cdft_offset), (void*)(&temp_lut_node.cdft_offset), sizeof(temp_lut_node.cdft_offset));
            offset += (uint32_t)sizeof(temp_lut_node.cdft_offset);
            //one more time for CDOT OFFSET. here both are same.
            ACDB_MEM_CPY(pInput->nBufferPointer + offset, sizeof(temp_lut_node.cdot_offset), (void*)(&temp_lut_node.cdot_offset), sizeof(temp_lut_node.cdot_offset));
            offset += (uint32_t)sizeof(temp_lut_node.cdot_offset);
         }
         break;
         default:
            result = ACDB_ERROR;
            goto end;
         }
      }
      result = ACDB_SUCCESS;
      goto end;
   }
end:
   if (lut_cdft != NULL)
   {
      deleteList(&lut_cdft);
      lut_cdft = NULL;
   }
   if (lut_cdot != NULL)
   {
      deleteList(&lut_cdot);
      lut_cdot = NULL;
   }
   if (lut_cdft_buffer != NULL)
   {
      deleteList(&lut_cdft_buffer);
      lut_cdft_buffer = NULL;
   }
   if (lut_cdot_buffer != NULL)
   {
      deleteList(&lut_cdot_buffer);
      lut_cdot_buffer = NULL;
   }
   if (lut_data_buffer != NULL)
   {
      deleteList(&lut_data_buffer);
      lut_data_buffer = NULL;
   }
   if (lut_data != NULL)
   {
      deleteList(&lut_data);
      lut_data = NULL;
   }
   if (lut_cdot_new != NULL)
   {
      deleteList(&lut_cdot_new);
      lut_cdot_new = NULL;
   }
   if (lut_cdft_new != NULL)
   {
      deleteList(&lut_cdft_new);
      lut_cdft_new = NULL;
   }

   return result;
}

int32_t AcdbCmdGetVocProcStatWithInstanceInfo(uint32_t queryType, uint8_t *pIn, uint8_t *pOut)
{
   int32_t result = ACDB_SUCCESS;
   uintptr_t nLookupKey = 0, cdftLookup = 0;
   struct node *lut_cdft = NULL, *lut_cdot = NULL, *lut_cdft_buffer = NULL, *lut_cdot_buffer = NULL, *lut_data_buffer = NULL, *lut_data = NULL, *lut_cdot_new = NULL, *lut_cdft_new = NULL;
   int size_cdft = 0, size_cdot = 0, lut_cdot_entries = 0, lut_cdft_entries = 0, lut_data_entries = 0;
    struct node *persistDataList = NULL;
    int persistDataIndex = 0;
    uint32_t size_data = 0;
   if (pIn == NULL || pOut == NULL)
   {
      ACDB_DEBUG_LOG("[ACDB Command]->[AcdbCmdGetVocProcStatInfo]->Invalid Null input param provided\n");
      result = ACDB_BADPARM;
   }
   else
   {
      uint32_t LookUptble_Length = 0, lut_index_count = 0, lut_index = 0, major = 1, minor = 1, ofTbloffset = 0, cvdTbloffset = 0, lutLength = 0;
      int cdft_start = 0, cdot_start = 0, i = 0, j = 0, k = 0, l = 0, m = 0, temp_size = 0;
        int cdft_offset = 0, cdot_offset = 0, datapool_start = 0;
        uint32_t datapool_offset = 0;
      uintptr_t data_Seclookup = 0;
      int lut_cdftfound = 0, lut_cdotfound = 0, calc_data = 0, heapsize = 0;
      VocProcStatCVDTblHdrFmtType temp_lut_node = { 0, 0, 0, 0, 0, 0, 0, 0, 0};
      AcdbDataType cData = { 0, NULL };
      uint32_t index = 0, Valid_length = 0;
      AcdbTableCmd cmd = { 0, 0 };
      AcdbTableInfoCVD tblInfo = { { 0, NULL }, { 0, NULL }, { 0, NULL }, { 0, NULL }, { 0, NULL }, { 0, NULL } };
      VocProcStatDataLookupType_UniqueData tempLut = { 0, 0, 0, 0, 0, 0};
      VocProcStatDataLookupTblType_UniqueData lutTbl = { 0, NULL };
      VocProcStatDataLookupTblType_CommonData lutTblCVD = { 0, NULL };
      VocProcStatDataLookupTblType_offsetData lutoffsetTbl  = { 0, NULL };
	  ContentDefWithInstanceTblType cdefTbl = { 0, NULL };
      ContentDataOffsetsTblType cdotTbl = { 0, NULL };
        uint32_t cvd_blob_offset = 0; 
        int pdi = 0;
      uint32_t offset = 0;
      uint32_t nMemBytesLeft = 0, nPaddedBytes = 0;
      VocProcCmdLookupType voccmd = { 0, 0, 0, 0 };
      uint32_t tblId = VOCPROC_STAT_INST_TBL_ID;
      int temp_pointer = 0, extraheap_cdot = 0, isheapentryfound = 0;
      switch (queryType)
      {
      case TABLE_CMD:
      {
         AcdbVocProcCmnTblCmdType *pInput = (AcdbVocProcCmnTblCmdType *)pIn;
         if (pInput->nBufferPointer == NULL)
            return ACDB_BADPARM;
         voccmd.nTxDevId = pInput->nTxDeviceId;
         voccmd.nRxDevId = pInput->nRxDeviceId;
         voccmd.nTxAfeSr = pInput->nTxDeviceSampleRateId;
         voccmd.nRxAfeSr = pInput->nRxDeviceSampleRateId;
      }
      break;
      case TABLE_SIZE_CMD:
      {
         AcdbVocProcCmnTblSizeCmdType *pInput = (AcdbVocProcCmnTblSizeCmdType *)pIn;
         voccmd.nTxDevId = pInput->nTxDeviceId;
         voccmd.nRxDevId = pInput->nRxDeviceId;
         voccmd.nTxAfeSr = pInput->nTxDeviceSampleRateId;
         voccmd.nRxAfeSr = pInput->nRxDeviceSampleRateId;
      }
      break;
      default:
         result = ACDB_ERROR;
         goto end;
      }

      cmd.devId = voccmd.nTxDevId;
      cmd.tblId = tblId;
      result = acdbdata_ioctl(ACDBDATACMD_GET_TABLE_INFO, (uint8_t *)&cmd, sizeof(cmd),
         (uint8_t *)&tblInfo, sizeof(tblInfo));
      if (result != ACDB_SUCCESS)
      {
         if (result == ACDB_DATA_INTERFACE_NOT_FOUND)
         {
            ACDB_DEBUG_LOG("Failed to fetch the Table in ACDB files Table-ID %08X \n"
               , tblId);
            goto end;
         }
         ACDB_DEBUG_LOG("Failed to fetch the lookup information of the device %08X \n"
            , voccmd.nTxDevId);
         goto end;
      }
      lutTbl.nLen = READ_UInt32(tblInfo.tblLutChnk.pData);
      lutTbl.pLut = (VocProcStatDataLookupType_UniqueData *)(tblInfo.tblLutChnk.pData + sizeof(lutTbl.nLen));

      result = AcdbDataBinarySearch((void *)lutTbl.pLut, lutTbl.nLen,
         VOCPROCSTAT_LUT_INDICES_COUNT - 7, &voccmd, VOCPROCSTAT_CMD_INDICES_COUNT, &index);

      if (result != SEARCH_SUCCESS)
      {
         ACDB_DEBUG_LOG("Failed to fetch the lookup information of the device %08X \n"
            , voccmd.nTxDevId);
         result = ACDB_INPUT_PARAMS_NOT_FOUND;
         goto end;
      }
      data_Seclookup = (uintptr_t)&lutTbl.pLut[index];
      lutTblCVD.nLen = READ_UInt32(tblInfo.tblLutCVDChnk.pData);
      lutTblCVD.pLut = (VocProcStatDataLookupType_CommonData *)(tblInfo.tblLutCVDChnk.pData + sizeof(lutTbl.nLen));

      lutoffsetTbl.nLen = READ_UInt32(tblInfo.tblLutOffsetChnk.pData);
      lutoffsetTbl.pLut = (VocProcStatDataLookupType_offsetData *)(tblInfo.tblLutOffsetChnk.pData + sizeof(lutTbl.nLen));

      cdefTbl.nLen = READ_UInt32(tblInfo.tblCdftChnk.pData);
      cdefTbl.pCntDef = (ContentDefInstanceType *)(tblInfo.tblCdftChnk.pData + sizeof(cdefTbl.nLen));

      if (cdefTbl.nLen <= 0)
      {
         ACDB_DEBUG_LOG("Vocprocstatic cdefTbl empty\n");
         result = ACDB_ERROR;
         goto end;
      }
      cdotTbl.nLen = READ_UInt32(tblInfo.tblCdotChnk.pData);
      cdotTbl.pDataOffsets = (uint32_t *)(tblInfo.tblCdotChnk.pData + sizeof(cdotTbl.nLen));
      if (cdotTbl.nLen <= 0)
      {
         ACDB_DEBUG_LOG("Vocprocstatic cdotTbl empty\n");
         result = ACDB_ERROR;
         goto end;
      }
      ACDB_MEM_CPY(&tempLut, sizeof(tempLut), &lutTbl.pLut[index], sizeof(tempLut));
      // Go to the offset table from luptableunique
      ACDB_MEM_CPY(&ofTbloffset, sizeof(uint32_t), &tempLut.nOffTableOffset, sizeof(uint32_t));// get offset table offset from lut table
      ACDB_MEM_CPY(&cvdTbloffset, sizeof(uint32_t), &tempLut.nCommonTblOffset, sizeof(uint32_t));// get commoncvd table offset from lut table
      ACDB_MEM_CPY(&lutLength, sizeof(uint32_t), tblInfo.tblLutOffsetChnk.pData + ofTbloffset, sizeof(uint32_t));
      lutoffsetTbl.pLut = (VocProcStatDataLookupType_offsetData *)(tblInfo.tblLutOffsetChnk.pData + sizeof(uint32_t) + ofTbloffset);
      lutTblCVD.pLut = (VocProcStatDataLookupType_CommonData *)(tblInfo.tblLutCVDChnk.pData + sizeof(uint32_t) + cvdTbloffset);

      if (lutLength == 0)
      {
         lutLength = lutTblCVD.nLen;
      }

      j = 0, k = 0;
      for (lut_index = 0; lut_index < (int)lutLength; lut_index++)
      {
         uint32_t pid = 0;
         VocProcStatDataLookupType_offsetData offsettbleinstance = { 0, 0 };//=(VocProcStatDataLookupType_offsetData*)(&lutoffsetTbl.pLut[lut_index]);
         int32_t calc_cdft = 0, calc_cdot = 0, lut_datafound = 0, dataofst = 0;
         ACDB_MEM_CPY(&offsettbleinstance, sizeof(VocProcStatDataLookupType_offsetData), &lutoffsetTbl.pLut[lut_index], sizeof(VocProcStatDataLookupType_offsetData));
         //datatable calculation
         cdotTbl.nLen = READ_UInt32(tblInfo.tblCdotChnk.pData + offsettbleinstance.nCDOTTblOffset);
         cdotTbl.pDataOffsets = (uint32_t *)(tblInfo.tblCdotChnk.pData + offsettbleinstance.nCDOTTblOffset + sizeof(tblInfo.tblCdotChnk.nDataLen));
         {
            cdefTbl.nLen = READ_UInt32(tblInfo.tblCdftChnk.pData + offsettbleinstance.nCDEFTblOffset);
            cdefTbl.pCntDef = (ContentDefInstanceType *)(tblInfo.tblCdftChnk.pData + offsettbleinstance.nCDEFTblOffset + sizeof(tblInfo.tblCdftChnk.nDataLen));
         }
         if (!update(offsettbleinstance.nCDOTTblOffset, lut_index, &lut_cdot_new))
         {
            result = ACDB_INSUFFICIENTMEMORY;
            goto end;
         }

         if (!update(offsettbleinstance.nCDEFTblOffset, lut_index, &lut_cdft_new))
         {
            result = ACDB_INSUFFICIENTMEMORY;
            goto end;
         }
         if (!update(0, 0, &lut_data_buffer))
         {
            result = ACDB_INSUFFICIENTMEMORY;
            goto end;
         }
         isheapentryfound = 0;
         for (m = 0; m < (int)cdotTbl.nLen; m++)
         {
                int32_t isPersistPID = 0;
                int unusedIndex = 0;
            ACDB_MEM_CPY(&dataofst, sizeof(dataofst), (void*)(&cdotTbl.pDataOffsets[m]), sizeof(dataofst));
			//mid = cdefTbl.pCntDef[m].nMid;
			//iid = cdefTbl.pCntDef[m].nIid;
			pid = cdefTbl.pCntDef[m].nPid;

                isPersistPID = IsOffloadedParameter(pid);
                if (isPersistPID == ACDB_SUCCESS)
                {
                    // if data is already present skip it
                    if (Find(dataofst, &persistDataList, &unusedIndex))
                    {
                        continue;
                    }

                    //Save Data Pool Offset in persistDataList
                    if (!update(dataofst, persistDataIndex, &persistDataList))
                    {
                        result = ACDB_INSUFFICIENTMEMORY;
                        goto end;
                    }
                    ++persistDataIndex;
                    continue;
                }

            nLookupKey = (uintptr_t)&lutTblCVD.pLut[lut_index];
            cdftLookup = (uintptr_t)&cdefTbl.pCntDef[m];
            result = GetMidIidPidCalibHeapDataEx(tblId, nLookupKey, data_Seclookup, cdftLookup, pid, &cData);//get from heap
            if (result == ACDB_SUCCESS)
            {
               nPaddedBytes = 0;
               if (cData.nLen % 4)
               {
                  nPaddedBytes = 4 - cData.nLen % 4;
               }

               heapsize += cData.nLen + nPaddedBytes;
               heapsize += (int32_t)sizeof(cData.nLen);
               isheapentryfound = 1;
            }
            lut_datafound = 0;
            if (Find(dataofst, &lut_data, &i))// if data is already present skip it , fill the buffer with only unique list of cdft,cdot,data pool entires.
            {
               lut_datafound = 1;
            }
            if (!lut_datafound)//fill data pool entries
            {
               if (!update(dataofst, l, &lut_data))
               {
                  result = ACDB_INSUFFICIENTMEMORY;
                  goto end;
               }

                  calc_data = *((int *)(tblInfo.dataPoolChnk.pData + dataofst));
                  nPaddedBytes = 0;
                  if (calc_data % 4)
                  {
                     nPaddedBytes = 4 - calc_data % 4;
                  }
                  if (!update((calc_data + nPaddedBytes + (int32_t)sizeof(uint32_t) + GetAt(l, &lut_data_buffer)), l + 1, &lut_data_buffer))
                  {
                     result = ACDB_INSUFFICIENTMEMORY;
                     goto end;
                  }

                  size_data = GetAt(l + 1, &lut_data_buffer);

               l++;
            }
         }
         if (isheapentryfound == 1)
         {
            extraheap_cdot += (int32_t)sizeof(cdotTbl.nLen);
            extraheap_cdot += (int32_t)sizeof(cdotTbl.pDataOffsets)*cdotTbl.nLen;
         }
         //cdft table parsing

         lut_cdftfound = 0;
         if (!update(0, 0, &lut_cdft_buffer))
         {
            result = ACDB_INSUFFICIENTMEMORY;
            goto end;
         }

         if (Find(lutoffsetTbl.pLut[lut_index].nCDEFTblOffset, &lut_cdft, &i))
         {
            lut_cdftfound = 1;
         }
         if (!lut_cdftfound)
         {
            if (!update(lutoffsetTbl.pLut[lut_index].nCDEFTblOffset, j, &lut_cdft))
            {
               result = ACDB_INSUFFICIENTMEMORY;
               goto end;
            }
            {
               //calc_cdft= *((int *)(tblInfo.tblCdftChnk.pData + offsettbleinstance.nCDEFTblOffset));
               ACDB_MEM_CPY(&calc_cdft, sizeof(int32_t), tblInfo.tblCdftChnk.pData + offsettbleinstance.nCDEFTblOffset, sizeof(int32_t));

               if (!update((int32_t)(calc_cdft*sizeof(uint32_t) * 3 + sizeof(uint32_t) + GetAt(j, &lut_cdft_buffer)), j + 1, &lut_cdft_buffer))
               {
                  result = ACDB_INSUFFICIENTMEMORY;
                  goto end;
               }
               size_cdft = GetAt(j + 1, &lut_cdft_buffer);
            }
            if (queryType == TABLE_CMD)
               if (!update(GetAt(j, &lut_cdft_buffer), lut_index, &lut_cdft_new))
               {
                  result = ACDB_INSUFFICIENTMEMORY;
                  goto end;
               }
            j++;
         }
         else
            if (queryType == TABLE_CMD)
               if (!update(GetAt(i, &lut_cdft_buffer), lut_index, &lut_cdft_new))
               {
                  result = ACDB_INSUFFICIENTMEMORY;
                  goto end;
               }

         //calculate cdot
         lut_cdotfound = 0;
         if (!update(0, 0, &lut_cdot_buffer))
         {
            result = ACDB_INSUFFICIENTMEMORY;
            goto end;
         }

         if (Find(offsettbleinstance.nCDOTTblOffset, &lut_cdot, &i))
         {
            lut_cdotfound = 1;
         }
         if (!lut_cdotfound)
         {
            if (!update(offsettbleinstance.nCDOTTblOffset, k, &lut_cdot))
            {
               result = ACDB_INSUFFICIENTMEMORY;
               goto end;
            }
                  {
                     ACDB_MEM_CPY(&calc_cdot, sizeof(int32_t), tblInfo.tblCdotChnk.pData + offsettbleinstance.nCDOTTblOffset, sizeof(int32_t));
                     update((int32_t)(calc_cdot*sizeof(uint32_t) + sizeof(uint32_t) + GetAt(k, &lut_cdot_buffer)), k + 1, &lut_cdot_buffer);

                     size_cdot = GetAt(k + 1, &lut_cdot_buffer);
                  }
                  if (queryType == TABLE_CMD)
                     if (!update(GetAt(k, &lut_cdot_buffer), lut_index, &lut_cdot_new))
                     {
                        result = ACDB_INSUFFICIENTMEMORY;
                        goto end;
                     }
                  k++;
         }
         else
            if (queryType == TABLE_CMD)
               if (!update(GetAt(i, &lut_cdot_buffer), lut_index, &lut_cdot_new))
               {
                  result = ACDB_INSUFFICIENTMEMORY;
                  goto end;
               }
      }

        /* Add Offloaded Parameter Data at the end of the Data Pool.
         * Calculate starting offset of data pool to pass to
         * GetOffloadedParameter(...) to calculate the correct
         * offload parameter front padding.
         */
        LookUptble_Length = (uint32_t)
            (
                3 * sizeof(uint32_t) //<Maj, Min, Table Length>
                + lut_index * (uint32_t)sizeof(uint32_t)
                * (VOCPROCSTAT_LUT_INDICES_COUNT - 4)
                + (uint32_t)sizeof(uint32_t)
            );

        cvd_blob_offset = LookUptble_Length
            + (int32_t)sizeof(size_cdft) + size_cdft
            + (int32_t)sizeof(size_cdot) + size_cdot + extraheap_cdot
            + (int32_t)sizeof(size_data);

        for (pdi = 0; pdi < persistDataIndex; pdi++)
        {
            uint32_t pid_size = 0;
            int dataofst = 0;

            cvd_blob_offset += size_data;
            cvd_blob_offset += sizeof(uint32_t); // <data_size, data[data_size]>
            dataofst = GetAt(pdi, &persistDataList);

            if (!update(dataofst, l, &lut_data))
            {
                result = ACDB_INSUFFICIENTMEMORY;
                goto end;
            }

            cData.nLen = READ_UInt32(tblInfo.dataPoolChnk.pData + dataofst);
            cData.pData = tblInfo.dataPoolChnk.pData + dataofst + (int)sizeof(cData.nLen);
            pid_size = cData.nLen;

            result = GetOffloadedParameterData(&cData, &pid_size,
                NULL, 0, &cvd_blob_offset);
            if (ACDB_SUCCESS != result)
            {
                result = ACDB_INSUFFICIENTMEMORY;
                goto end;
            }

            if (!update((pid_size + (int32_t)sizeof(uint32_t) + GetAt(l, &lut_data_buffer)), l + 1, &lut_data_buffer))
            {
                result = ACDB_INSUFFICIENTMEMORY;
                goto end;
            }

            size_data = GetAt(l + 1, &lut_data_buffer);

            l++;
        }

        LookUptble_Length = 0;

      lut_index_count = lut_index;
      lut_cdft_entries = j; lut_cdot_entries = k; lut_data_entries = l;
      lut_index = 0;
      //add the data
      if (queryType == TABLE_CMD)// write lut size for the first time
      {
         AcdbVocProcCmnTblCmdType *pInput = (AcdbVocProcCmnTblCmdType *)pIn;
         AcdbQueryResponseType *pOutput = (AcdbQueryResponseType *)pOut;
         int dataOffset = 0;
         nMemBytesLeft = pInput->nBufferLength;
         LookUptble_Length = (uint32_t)((lut_index_count*sizeof(uint32_t)*(VOCPROCSTAT_LUT_INDICES_COUNT - 4)) + sizeof(uint32_t));//lookup table no_of_entries+data
         if (nMemBytesLeft < (uint32_t)LookUptble_Length)
         {
            ACDB_DEBUG_LOG("Insufficient memory to copy the vocproc static table data\n");
            result = ACDB_INSUFFICIENTMEMORY;
            goto end;
         }
         // write major and minor cvd versions.
         ACDB_MEM_CPY(pInput->nBufferPointer + offset, nMemBytesLeft, &major, sizeof(uint32_t));//Major version
         offset += (uint32_t)sizeof(uint32_t);
         ACDB_MEM_CPY(pInput->nBufferPointer + offset, nMemBytesLeft, &minor, sizeof(uint32_t));//minor version
         offset += (uint32_t)sizeof(uint32_t);
         ACDB_MEM_CPY(pInput->nBufferPointer + offset, nMemBytesLeft, &LookUptble_Length, sizeof(uint32_t));//length of the table
         offset += (uint32_t)sizeof(uint32_t);
         ACDB_MEM_CPY(pInput->nBufferPointer + offset, nMemBytesLeft, &lut_index_count, sizeof(uint32_t));//no of entries
         offset += (uint32_t)sizeof(uint32_t);
         LookUptble_Length += (uint32_t)(3 * sizeof(uint32_t));//major and minor versions + table length
         cdft_offset = cdft_start = LookUptble_Length, cdft_offset = LookUptble_Length;
         //cdft table
         temp_size = size_cdft;//+ sizeof(temp_size);
         ACDB_MEM_CPY(pInput->nBufferPointer + cdft_start, sizeof(temp_size), (void*)(&temp_size), sizeof(temp_size));
         cdft_offset += (int32_t)sizeof(tblInfo.tblCdftChnk.nDataLen);
         for (k = 0; k < lut_cdft_entries; k++)//fill cdft table in buffer
         {
            dataOffset = GetAt(k, &lut_cdft);
            cdefTbl.nLen = READ_UInt32(tblInfo.tblCdftChnk.pData + dataOffset);
            cdefTbl.pCntDef = (ContentDefInstanceType *)(tblInfo.tblCdftChnk.pData + dataOffset + sizeof(tblInfo.tblCdftChnk.nDataLen));
            ACDB_MEM_CPY(pInput->nBufferPointer + cdft_offset, sizeof(cdefTbl.nLen), (void*)(&cdefTbl.nLen), sizeof(cdefTbl.nLen));
            cdft_offset += (int32_t)sizeof(cdefTbl.nLen);
            dataOffset += (int32_t)sizeof(cdefTbl.nLen);
            for (i = 0; i < (int)cdefTbl.nLen; i++)
            {
               uint16_t iid = 0, reserved = 0;
               ACDB_MEM_CPY(pInput->nBufferPointer + cdft_offset, sizeof(cdefTbl.pCntDef[i].nMid), (void*)(&cdefTbl.pCntDef[i].nMid), sizeof(cdefTbl.pCntDef[i].nMid));
               cdft_offset += (int32_t)sizeof(cdefTbl.pCntDef[i].nMid);
               dataOffset += (int32_t)sizeof(cdefTbl.pCntDef[i].nMid);

               iid = GetUint16IID(cdefTbl.pCntDef[i].nIid);
               ACDB_MEM_CPY(pInput->nBufferPointer + cdft_offset, sizeof(iid), (void*)(&iid), sizeof(iid));
               cdft_offset += (int32_t)sizeof(iid);
               dataOffset += (int32_t)sizeof(iid);

               ACDB_MEM_CPY(pInput->nBufferPointer + cdft_offset, sizeof(reserved), (void*)(&reserved), sizeof(reserved));
               cdft_offset += (int32_t)sizeof(reserved);
               dataOffset += (int32_t)sizeof(reserved);

               ACDB_MEM_CPY(pInput->nBufferPointer + cdft_offset, sizeof(cdefTbl.pCntDef[i].nPid), (void*)(&cdefTbl.pCntDef[i].nPid), sizeof(cdefTbl.pCntDef[i].nPid));
               cdft_offset += (int32_t)sizeof(cdefTbl.pCntDef[i].nPid);
               dataOffset += (int32_t)sizeof(cdefTbl.pCntDef[i].nPid);
            }
         }

         temp_size = cdft_offset - cdft_start;//calculate cdft temp size
         cdot_start = cdot_offset = cdft_offset;
         //write cdot also
         dataOffset = 0;
         temp_size = size_cdot + extraheap_cdot;//+ sizeof(temp_size);

            datapool_start = (cdot_offset + size_cdot + (int32_t)sizeof(size_cdot) + (int32_t)sizeof(temp_size) + extraheap_cdot); //reserve the space to write data size.//have the
            datapool_offset = (uint32_t)datapool_start;
         ACDB_MEM_CPY(pInput->nBufferPointer + cdot_start, sizeof(size_cdot), (void*)(&temp_size), sizeof(temp_size));
         cdot_offset += (int32_t)sizeof(size_cdot);

         for (k = 0; k < lut_cdot_entries; k++)//fill cdot table in buffer
         {
            int dataoffset_for_actualbuffer = 0;
            dataOffset = GetAt(k, &lut_cdot);
            cdotTbl.nLen = READ_UInt32(tblInfo.tblCdotChnk.pData + dataOffset);
            cdotTbl.pDataOffsets = (uint32_t *)(tblInfo.tblCdotChnk.pData + dataOffset + sizeof(tblInfo.tblCdotChnk.nDataLen));
            ACDB_MEM_CPY(pInput->nBufferPointer + cdot_offset, sizeof(cdotTbl.nLen), (void*)(&cdotTbl.nLen), sizeof(cdotTbl.nLen));
            cdot_offset += (int32_t)sizeof(cdotTbl.nLen);
            dataOffset += (int32_t)sizeof(cdotTbl.nLen);
            for (i = 0; i < (int)cdotTbl.nLen; i++)
            {
               //calculate actual buffer offset
               for (l = 0; l < lut_data_entries; l++)
               {
                  if (cdotTbl.pDataOffsets[i] == (uint32_t)GetAt(l, &lut_data))
                  {
                     dataoffset_for_actualbuffer = GetAt(l, &lut_data_buffer);
                     break;
                  }
               }
               ACDB_MEM_CPY(pInput->nBufferPointer + cdot_offset, sizeof(dataoffset_for_actualbuffer), (void*)(&dataoffset_for_actualbuffer), sizeof(dataoffset_for_actualbuffer));
               cdot_offset += (int32_t)sizeof(cdotTbl.pDataOffsets[i]);
               dataOffset += (int32_t)sizeof(cdotTbl.pDataOffsets[i]);
            }
         }


            temp_pointer = cdot_offset;
         //datapool_offset=temp_pointer;

         for (l = 0; l < lut_data_entries; l++)//fill data pool entries table in buffer
         {
                int persistIndex = 0;
                uint32_t  tmp_data_pool_offset = 0;
            dataOffset = GetAt(l, &lut_data);
            cData.nLen = READ_UInt32(tblInfo.dataPoolChnk.pData + dataOffset);
            cData.pData = tblInfo.dataPoolChnk.pData + dataOffset + sizeof(cData.nLen);
                if (Find(dataOffset, &persistDataList, &persistIndex))
                {
                    //curDataOfst = GetAt(l, &lut_data_buffer);
                    Valid_length = cData.nLen;
                    tmp_data_pool_offset = datapool_offset + (uint32_t)sizeof(cData.nLen);

                    result = GetOffloadedParameterData(&cData, &Valid_length,
                        NULL, 0, &tmp_data_pool_offset);
                    if (ACDB_SUCCESS != result)
                    {
                        result = ACDB_INSUFFICIENTMEMORY;
                        goto end;
                    }

                    ACDB_MEM_CPY(pInput->nBufferPointer + datapool_offset, sizeof(cData.nLen), (void*)&(Valid_length), sizeof(cData.nLen));
                    datapool_offset += (uint32_t)sizeof(cData.nLen);

                    memset((pInput->nBufferPointer + datapool_offset), 0, (size_t)(Valid_length));
                    result = GetOffloadedParameterData(&cData, &cData.nLen,
                        pInput->nBufferPointer + datapool_offset, Valid_length, &datapool_offset);
                    if (ACDB_SUCCESS != result)
                    {
                        result = ACDB_INSUFFICIENTMEMORY;
                        goto end;
                    }
                }
                else
                {
            nPaddedBytes = 0;
            if (cData.nLen % 4)
            {
               nPaddedBytes = 4 - cData.nLen % 4;
            }
            Valid_length = cData.nLen + nPaddedBytes;
            ACDB_MEM_CPY(pInput->nBufferPointer + datapool_offset, sizeof(cData.nLen), (void*)&(Valid_length), sizeof(cData.nLen));
                    datapool_offset += (uint32_t)sizeof(cData.nLen);
            memset((pInput->nBufferPointer + datapool_offset), 0, (size_t)(cData.nLen + nPaddedBytes));
            ACDB_MEM_CPY(pInput->nBufferPointer + datapool_offset, cData.nLen, (void*)(cData.pData), cData.nLen);
            datapool_offset += cData.nLen + nPaddedBytes;
         }
            }

         //fill heap entries.
         if (heapsize > 0)
         {
            for (lut_index = 0; lut_index<(int)lutLength; lut_index++)
            {
               int dataofst = 0;
               int mid = 0, iid = 0, pid = 0;
               isheapentryfound = 0;
               //datatable calculation
               cdotTbl.nLen = READ_UInt32(pInput->nBufferPointer + cdot_start + 4 + GetAt(lut_index, &lut_cdot_new));
               cdotTbl.pDataOffsets = (uint32_t *)(pInput->nBufferPointer + cdot_start + 4 + GetAt(lut_index, &lut_cdot_new) + sizeof(tblInfo.tblCdotChnk.nDataLen));

               cdefTbl.nLen = READ_UInt32(pInput->nBufferPointer + cdft_start + 4 + GetAt(lut_index, &lut_cdft_new));
               cdefTbl.pCntDef = (ContentDefInstanceType *)(pInput->nBufferPointer + cdft_start + 4 + GetAt(lut_index, &lut_cdft_new)/*offsettbleinstance->nCDEFTblOffset*/ + sizeof(tblInfo.tblCdftChnk.nDataLen));
               //reserve space
               if (temp_pointer + (int)cdotTbl.nLen*(int)sizeof(cdotTbl.pDataOffsets) + 4>(datapool_start - (int)sizeof(uint32_t)))//reached max cdot entries
               {
                  continue;
               }
               ACDB_MEM_CPY(pInput->nBufferPointer + temp_pointer, sizeof(cdotTbl.nLen), (void*)(&cdotTbl.nLen), sizeof(cdotTbl.nLen));

               ACDB_MEM_CPY(pInput->nBufferPointer + temp_pointer + 4, cdotTbl.nLen*sizeof(cdotTbl.pDataOffsets), (void*)cdotTbl.pDataOffsets, cdotTbl.nLen*sizeof(cdotTbl.pDataOffsets));

               cdotTbl.nLen = READ_UInt32(pInput->nBufferPointer + temp_pointer);
               cdotTbl.pDataOffsets = (uint32_t *)(pInput->nBufferPointer + temp_pointer + sizeof(tblInfo.tblCdotChnk.nDataLen));

               //fill heap
               for (m = 0; m < (int)cdotTbl.nLen; m++)
               {
                  ACDB_MEM_CPY(&dataofst, sizeof(dataofst), (void*)(&cdotTbl.pDataOffsets[m]), sizeof(dataofst));
                  ACDB_MEM_CPY(&mid, sizeof(dataofst), (void*)(&cdefTbl.pCntDef[m].nMid), sizeof(dataofst));
                  ACDB_MEM_CPY(&iid, sizeof(dataofst), (void*)(&cdefTbl.pCntDef[m].nIid), sizeof(dataofst));
                  ACDB_MEM_CPY(&pid, sizeof(dataofst), (void*)(&cdefTbl.pCntDef[m].nPid), sizeof(dataofst));
                  nLookupKey = (uintptr_t)&lutTblCVD.pLut[lut_index];
                  cdftLookup = (uintptr_t)&cdefTbl.pCntDef[m];
                  result = GetMidIidPidCalibHeapDataEx(tblId, nLookupKey, data_Seclookup, cdftLookup, pid, &cData);//get from heap
                  if (ACDB_SUCCESS == result)
                  {
                            cdotTbl.pDataOffsets[m] = datapool_offset - datapool_start;
                            if (IsOffloadedParameter(pid) == ACDB_SUCCESS)
                            {
                                uint32_t  tmp_data_pool_offset = datapool_offset + (uint32_t)sizeof(cData.nLen);
                                Valid_length = 0;
                                result = GetOffloadedParameterData(&cData, &Valid_length,
                                    NULL, 0, &tmp_data_pool_offset);
                                if (ACDB_SUCCESS != result)
                                {
                                    result = ACDB_INSUFFICIENTMEMORY;
                                    goto end;
                                }
                                ACDB_MEM_CPY(pInput->nBufferPointer + datapool_offset, sizeof(cData.nLen), (void*)(&Valid_length), sizeof(cData.nLen));
                                datapool_offset += (uint32_t)sizeof(cData.nLen);

                                memset((pInput->nBufferPointer + datapool_offset), 0, Valid_length);

                                result = GetOffloadedParameterData(&cData, &cData.nLen,
                                    pInput->nBufferPointer + datapool_offset, Valid_length, &datapool_offset);
                                if (ACDB_SUCCESS != result)
                                {
                                    result = ACDB_INSUFFICIENTMEMORY;
                                    goto end;
                                }
                            }
                            else
                            {
                     nPaddedBytes = 0;
                     if (cData.nLen % 4)
                     {
                        nPaddedBytes = 4 - cData.nLen % 4;
                     }
                     Valid_length = cData.nLen + nPaddedBytes;
                     ACDB_MEM_CPY(pInput->nBufferPointer + datapool_offset, sizeof(cData.nLen), (void*)(&Valid_length), sizeof(cData.nLen));
                                datapool_offset += (uint32_t)sizeof(cData.nLen);

                     memset((pInput->nBufferPointer + datapool_offset), 0, (size_t)(cData.nLen + nPaddedBytes));

                     ACDB_MEM_CPY(pInput->nBufferPointer + datapool_offset, cData.nLen, (void*)(cData.pData), cData.nLen);
                     datapool_offset += cData.nLen + nPaddedBytes;
                     if (!update((temp_pointer - cdot_start - (int32_t)sizeof(uint32_t)), lut_index, &lut_cdot_new))
                     {
                        result = ACDB_INSUFFICIENTMEMORY;
                        goto end;
                     }
                            }

                     isheapentryfound = 1;
                  }
               }
               if (isheapentryfound == 1)
               {
                  temp_pointer += (int32_t)sizeof(cdotTbl.nLen);
                  temp_pointer += (int32_t)(cdotTbl.nLen*sizeof(cdotTbl.pDataOffsets));
                  if (temp_pointer >= (datapool_start - (int)sizeof(uint32_t)))//reached max cdot entries
                     break;
               }
            }
         }

         j = datapool_offset - temp_pointer - (int32_t)sizeof(uint32_t);
         ACDB_MEM_CPY(pInput->nBufferPointer + temp_pointer, sizeof(size_cdot), (void*)(&j), sizeof(size_cdot));
         nMemBytesLeft -= datapool_offset;
         pOutput->nBytesUsedInBuffer = datapool_offset;
      }
      else
      {
         AcdbSizeResponseType *pOutput = (AcdbSizeResponseType *)pOut;
		 LookUptble_Length = (uint32_t)((lut_index_count*sizeof(uint32_t)*(VOCPROCSTAT_LUT_INDICES_COUNT - 4)) + sizeof(uint32_t));
         LookUptble_Length += (uint32_t)(3 * sizeof(uint32_t));//major and minor versions
         offset += LookUptble_Length;
         offset += (uint32_t)sizeof(size_cdft);
         offset += size_cdft;
         offset += (uint32_t)sizeof(size_cdot);
         offset += size_cdot;
         offset += (uint32_t)sizeof(uint32_t);//reseve for data size chunk
         offset += size_data;
         offset += heapsize;
         offset += extraheap_cdot;
         pOutput->nSize = offset;
         result = ACDB_SUCCESS;
         goto end;
      }
      for (lut_index = 0; lut_index < lutLength; lut_index++)
      {
         //go the cvd common table to get actual common entry
         VocProcStatDataLookupType_CommonData commonLUTInstance = { 0, 0, 0, 0, 0, 0, 0 };
         ACDB_MEM_CPY(&commonLUTInstance, sizeof(VocProcStatDataLookupType_CommonData), (tblInfo.tblLutCVDChnk.pData + sizeof(uint32_t) + cvdTbloffset + sizeof(VocProcStatDataLookupType_CommonData)*lut_index), sizeof(VocProcStatDataLookupType_CommonData));

         switch (queryType)
         {
         case TABLE_CMD:
         {
            //fill lut entries in CVD format
            AcdbVocProcCmnTblCmdType *pInput = (AcdbVocProcCmnTblCmdType *)pIn;
            ACDB_MEM_CPY(&temp_lut_node.nNetworkId, sizeof(uint32_t), &commonLUTInstance.nNetwork, sizeof(uint32_t));
            ACDB_MEM_CPY(&temp_lut_node.nRxVocSr, sizeof(uint32_t), &commonLUTInstance.nRxVocSR, sizeof(uint32_t));
            ACDB_MEM_CPY(&temp_lut_node.nTxVocSr, sizeof(uint32_t), &commonLUTInstance.nTxVocSR, sizeof(uint32_t));
            ACDB_MEM_CPY(&temp_lut_node.nVocoder_type, sizeof(uint32_t), &commonLUTInstance.nVocoder_type, sizeof(uint32_t));
            ACDB_MEM_CPY(&temp_lut_node.rx_Voc_mode, sizeof(uint32_t), &commonLUTInstance.rx_Voc_mode, sizeof(uint32_t));
            ACDB_MEM_CPY(&temp_lut_node.tx_Voc_mode, sizeof(uint32_t), &commonLUTInstance.tx_Voc_mode, sizeof(uint32_t));
            ACDB_MEM_CPY(&temp_lut_node.nFeature, sizeof(uint32_t), &commonLUTInstance.nFeature, sizeof(uint32_t));
            temp_lut_node.cdft_offset = GetAt(lut_index, &lut_cdft_new);//lutoffsetTbl.pLut[lut_index].nCDEFTblOffset;
            temp_lut_node.cdot_offset = GetAt(lut_index, &lut_cdot_new);
            ACDB_MEM_CPY(pInput->nBufferPointer + offset, sizeof(temp_lut_node.nNetworkId), (void*)(&temp_lut_node.nNetworkId), sizeof(temp_lut_node.nNetworkId));
            offset += (uint32_t)sizeof(temp_lut_node.nNetworkId);
            ACDB_MEM_CPY(pInput->nBufferPointer + offset, sizeof(temp_lut_node.nTxVocSr), (void*)(&temp_lut_node.nTxVocSr), sizeof(temp_lut_node.nTxVocSr));
            offset += (uint32_t)sizeof(temp_lut_node.nTxVocSr);
            ACDB_MEM_CPY(pInput->nBufferPointer + offset, sizeof(temp_lut_node.nRxVocSr), (void*)(&temp_lut_node.nRxVocSr), sizeof(temp_lut_node.nRxVocSr));
            offset += (uint32_t)sizeof(temp_lut_node.nRxVocSr);

            ACDB_MEM_CPY(pInput->nBufferPointer + offset, sizeof(temp_lut_node.tx_Voc_mode), (void*)(&temp_lut_node.tx_Voc_mode), sizeof(temp_lut_node.tx_Voc_mode));
            offset += (uint32_t)sizeof(temp_lut_node.tx_Voc_mode);
            //one more time for CDOT OFFSET. here both are same.
            ACDB_MEM_CPY(pInput->nBufferPointer + offset, sizeof(temp_lut_node.rx_Voc_mode), (void*)(&temp_lut_node.rx_Voc_mode), sizeof(temp_lut_node.rx_Voc_mode));
            offset += (uint32_t)sizeof(temp_lut_node.rx_Voc_mode);
            ACDB_MEM_CPY(pInput->nBufferPointer + offset, sizeof(temp_lut_node.nVocoder_type), (void*)(&temp_lut_node.nVocoder_type), sizeof(temp_lut_node.nVocoder_type));
            offset += (uint32_t)sizeof(temp_lut_node.nVocoder_type);
            //one more time for CDOT OFFSET. here both are same.
            ACDB_MEM_CPY(pInput->nBufferPointer + offset, sizeof(temp_lut_node.nFeature), (void*)(&temp_lut_node.nFeature), sizeof(temp_lut_node.nFeature));
            offset += (uint32_t)sizeof(temp_lut_node.nFeature);
            ACDB_MEM_CPY(pInput->nBufferPointer + offset, sizeof(temp_lut_node.cdft_offset), (void*)(&temp_lut_node.cdft_offset), sizeof(temp_lut_node.cdft_offset));
            offset += (uint32_t)sizeof(temp_lut_node.cdft_offset);
            //one more time for CDOT OFFSET. here both are same.
            ACDB_MEM_CPY(pInput->nBufferPointer + offset, sizeof(temp_lut_node.cdot_offset), (void*)(&temp_lut_node.cdot_offset), sizeof(temp_lut_node.cdot_offset));
            offset += (uint32_t)sizeof(temp_lut_node.cdot_offset);
         }
         break;
         default:
            result = ACDB_ERROR;
            goto end;
         }
      }
      result = ACDB_SUCCESS;
      goto end;
   }
end:
   if (lut_cdft != NULL)
   {
      deleteList(&lut_cdft);
      lut_cdft = NULL;
   }
   if (lut_cdot != NULL)
   {
      deleteList(&lut_cdot);
      lut_cdot = NULL;
   }
   if (lut_cdft_buffer != NULL)
   {
      deleteList(&lut_cdft_buffer);
      lut_cdft_buffer = NULL;
   }
   if (lut_cdot_buffer != NULL)
   {
      deleteList(&lut_cdot_buffer);
      lut_cdot_buffer = NULL;
   }
   if (lut_data_buffer != NULL)
   {
      deleteList(&lut_data_buffer);
      lut_data_buffer = NULL;
   }
   if (lut_data != NULL)
   {
      deleteList(&lut_data);
      lut_data = NULL;
   }
   if (lut_cdot_new != NULL)
   {
      deleteList(&lut_cdot_new);
      lut_cdot_new = NULL;
   }
   if (lut_cdft_new != NULL)
   {
      deleteList(&lut_cdft_new);
      lut_cdft_new = NULL;
   }

   return result;
}

int32_t AcdbCmdGetVocProcDynInfo(uint32_t queryType, uint8_t *pIn, uint8_t *pOut)
{
   int32_t result = ACDB_SUCCESS;
   uintptr_t nLookupKey = 0;
   uintptr_t cdftLookup = 0;
   struct node *lut_cdft = NULL, *lut_cdot = NULL, *lut_cdft_buffer = NULL, *lut_cdot_buffer = NULL, *lut_data_buffer = NULL, *lut_data = NULL, *lut_cdot_new = NULL, *lut_cdft_new = NULL;
   int size_cdft = 0, size_cdot = 0, size_data = 0, lut_cdot_entries = 0, lut_cdft_entries = 0, lut_data_entries = 0;

   if (pIn == NULL || pOut == NULL)
   {
      ACDB_DEBUG_LOG("[ACDB Command]->[AcdbCmdGetVocProcDynInfo]->Invalid Null input param provided\n");
      result = ACDB_BADPARM;
   }
   else
   {
      uint32_t LookUptble_Length = 0, lut_index_count = 0, lut_index = 0, major = 1, minor = 0, ofTbloffset = 0, cvdTbloffset = 0, lutLength = 0;
      int cdft_start = 0, cdot_start = 0, i = 0, j = 0, k = 0, l = 0, m = 0, temp_size = 0;
      int cdft_offset = 0, cdot_offset = 0, datapool_offset = 0, datapool_start = 0;
      uintptr_t data_Seclookup = 0;
      int lut_cdftfound = 0, lut_cdotfound = 0, calc_data = 0, heapsize = 0;
	  VocProcDynCVDTblHdrFmtType temp_lut_node = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
      AcdbDataType cData = { 0, NULL };
      uint32_t index = 0, Valid_length = 0;
      AcdbTableCmd cmd = { 0, 0 };
      AcdbTableInfoCVD tblInfo = { { 0, NULL }, { 0, NULL }, { 0, NULL }, { 0, NULL }, { 0, NULL }, { 0, NULL } };
	  VocProcDynDataLookupType_UniqueData tempLut = { 0, 0, 0, 0, 0 };
	  VocProcDynDataLookupTblType_UniqueData lutTbl = { 0, NULL };
      VocProcDynDataLookupTblType_CommonData lutTblCVD = { 0, NULL };
      VocProcDynDataLookupTblType_offsetData lutoffsetTbl = { 0, NULL };
      ContentDefTblType cdefTbl = { 0, NULL };
      ContentDataOffsetsTblType cdotTbl = { 0, NULL };
      uint32_t offset = 0;
      uint32_t nMemBytesLeft = 0, nPaddedBytes = 0;
	  VocProcVolV2CmdLookupType voccmd = { 0, 0, 0 };
      uint32_t tblId = VOCPROC_DYNAMIC_TBL;
      int temp_pointer = 0, extraheap_cdot = 0, isheapentryfound = 0;;

      switch (queryType)
      {
      case TABLE_CMD:
      {
         AcdbVocProcGainDepVolTblV2CmdType *pInput = (AcdbVocProcGainDepVolTblV2CmdType *)pIn;
         if (pInput->nBufferPointer == NULL)
            return ACDB_BADPARM;
         voccmd.nTxDevId = pInput->nTxDeviceId;
         voccmd.nRxDevId = pInput->nRxDeviceId;
         voccmd.nFeatureId = pInput->nFeatureId;
      }
      break;
      case TABLE_SIZE_CMD:
      {
         AcdbVocProcGainDepVolTblSizeV2CmdType *pInput = (AcdbVocProcGainDepVolTblSizeV2CmdType *)pIn;
         voccmd.nTxDevId = pInput->nTxDeviceId;
         voccmd.nRxDevId = pInput->nRxDeviceId;
         voccmd.nFeatureId = pInput->nFeatureId;
      }
      break;
      default:
         result = ACDB_ERROR;
         goto end;
      }

      cmd.devId = voccmd.nTxDevId;
      cmd.tblId = tblId;
      result = acdbdata_ioctl(ACDBDATACMD_GET_TABLE_INFO, (uint8_t *)&cmd, sizeof(cmd),
         (uint8_t *)&tblInfo, sizeof(tblInfo));
      if (result != ACDB_SUCCESS)
      {
         if (result == ACDB_DATA_INTERFACE_NOT_FOUND)
         {
            ACDB_DEBUG_LOG("Failed to fetch the Table in ACDB files Table-ID %08X \n"
               , tblId);
            goto end;
         }
         ACDB_DEBUG_LOG("Failed to fetch the lookup information of the device %08X \n"
            , voccmd.nTxDevId);
         goto end;
      }

      lutTbl.nLen = READ_UInt32(tblInfo.tblLutChnk.pData);
      lutTbl.pLut = (VocProcDynDataLookupType_UniqueData *)(tblInfo.tblLutChnk.pData + sizeof(lutTbl.nLen));

      result = AcdbDataBinarySearch((void *)lutTbl.pLut, lutTbl.nLen,
         VOCPROCDYN_LUT_INDICES_COUNT - 8, &voccmd, VOCPROCDYN_CMD_INDICES_COUNT, &index);
      if (result != SEARCH_SUCCESS)
      {
         ACDB_DEBUG_LOG("Failed to fetch the lookup information of the device %08X \n"
            , voccmd.nTxDevId);
         result = ACDB_INPUT_PARAMS_NOT_FOUND;
         goto end;
      }

      data_Seclookup = (uintptr_t)&lutTbl.pLut[index];
      lutTblCVD.nLen = READ_UInt32(tblInfo.tblLutCVDChnk.pData);

      lutTblCVD.pLut = (VocProcDynDataLookupType_CommonData *)(tblInfo.tblLutCVDChnk.pData + sizeof(lutTbl.nLen));
      if (lutTblCVD.nLen <= 0)
      {
         ACDB_DEBUG_LOG("Vocprocdyn lutcvdTbl empty\n");
         result = ACDB_ERROR;
         goto end;
      }
      lutoffsetTbl.nLen = READ_UInt32(tblInfo.tblLutOffsetChnk.pData);
      lutoffsetTbl.pLut = (VocProcDynDataLookupType_offsetData *)(tblInfo.tblLutOffsetChnk.pData + sizeof(lutTbl.nLen));
      cdefTbl.nLen = READ_UInt32(tblInfo.tblCdftChnk.pData);
      cdefTbl.pCntDef = (ContentDefType *)(tblInfo.tblCdftChnk.pData + sizeof(cdefTbl.nLen));
      if (cdefTbl.nLen <= 0)
      {
         ACDB_DEBUG_LOG("Vocprocdyn cdefTbl empty\n");
         result = ACDB_ERROR;
         goto end;
      }

      cdotTbl.nLen = READ_UInt32(tblInfo.tblCdotChnk.pData);
      cdotTbl.pDataOffsets = (uint32_t *)(tblInfo.tblCdotChnk.pData + sizeof(cdotTbl.nLen));
      if (cdotTbl.nLen <= 0)
      {
         ACDB_DEBUG_LOG("Vocstream cdotTbl empty\n");
         result = ACDB_ERROR;
         goto end;
      }

      ACDB_MEM_CPY(&tempLut, sizeof(tempLut), &lutTbl.pLut[index], sizeof(tempLut));
      // Go to the offset table from luptableunique
	  ofTbloffset = tempLut.nOffTableOffset;
	  cvdTbloffset = tempLut.nCommonTblOffset;
      ACDB_MEM_CPY(&lutLength, sizeof(uint32_t), tblInfo.tblLutCVDChnk.pData + cvdTbloffset, sizeof(uint32_t));

      lutoffsetTbl.pLut = (VocProcDynDataLookupType_offsetData *)(tblInfo.tblLutOffsetChnk.pData + sizeof(uint32_t) + ofTbloffset);
      lutTblCVD.pLut = (VocProcDynDataLookupType_CommonData *)(tblInfo.tblLutCVDChnk.pData + sizeof(uint32_t) + cvdTbloffset);

      if (lutLength == 0)
      {
         ACDB_DEBUG_LOG("[ACDB Command]->[AcdbCmdGetVocProcDynInfo]->inside lutlength zero of lut tables %08X ", lutLength);
         lutLength = lutTblCVD.nLen;
      }

      j = 0, k = 0;
      for (lut_index = 0; lut_index < (int)lutLength; lut_index++)
      {
         uint32_t mid = 0, pid = 0;
		 VocProcDynDataLookupType_offsetData offsettbleinstance = { 0, 0 };
         int32_t calc_cdft = 0, calc_cdot = 0, lut_datafound = 0, dataofst = 0;
         ACDB_MEM_CPY(&offsettbleinstance, sizeof(VocProcDynDataLookupType_offsetData), &lutoffsetTbl.pLut[lut_index], sizeof(VocProcDynDataLookupType_offsetData));

         //datatable calculation
         {
            cdotTbl.nLen = READ_UInt32(tblInfo.tblCdotChnk.pData + offsettbleinstance.nCDOTTblOffset);
            cdotTbl.pDataOffsets = (uint32_t *)(tblInfo.tblCdotChnk.pData + offsettbleinstance.nCDOTTblOffset + sizeof(tblInfo.tblCdotChnk.nDataLen));
         }
         {
            //ACDB_DEBUG_LOG("[ACDB Command]->[AcdbCmdGetVocProcDynInfo]->crash related logs-before crash lutindex %08X cdefoffset %08X \n",lut_index,offsettbleinstance.nCDEFTblOffset);
            cdefTbl.nLen = READ_UInt32(tblInfo.tblCdftChnk.pData + offsettbleinstance.nCDEFTblOffset);
            cdefTbl.pCntDef = (ContentDefType *)(tblInfo.tblCdftChnk.pData + offsettbleinstance.nCDEFTblOffset + sizeof(tblInfo.tblCdftChnk.nDataLen));
         }
         if (!update(offsettbleinstance.nCDOTTblOffset, lut_index, &lut_cdot_new))
         {
            result = ACDB_INSUFFICIENTMEMORY;
            goto end;
         }
         if (!update(offsettbleinstance.nCDEFTblOffset, lut_index, &lut_cdft_new))
         {
            result = ACDB_INSUFFICIENTMEMORY;
            goto end;
         }
         if (!update(0, 0, &lut_data_buffer))
         {
            result = ACDB_INSUFFICIENTMEMORY;
            goto end;
         }
         isheapentryfound = 0;
         for (m = 0; m < (int)cdotTbl.nLen; m++)
         {
            ACDB_MEM_CPY(&dataofst, sizeof(dataofst), (void*)(&cdotTbl.pDataOffsets[m]), sizeof(dataofst));
            ACDB_MEM_CPY(&mid, sizeof(mid), (void*)(&cdefTbl.pCntDef[m].nMid), sizeof(mid));
            ACDB_MEM_CPY(&pid, sizeof(pid), (void*)(&cdefTbl.pCntDef[m].nPid), sizeof(pid));
            nLookupKey = (uintptr_t)&lutTblCVD.pLut[lut_index];
            cdftLookup = (uintptr_t)&cdefTbl.pCntDef[m];
            result = GetMidPidCalibHeapDataEx(tblId, nLookupKey, data_Seclookup, cdftLookup, pid, &cData);//get from heap
            if (result == ACDB_SUCCESS)
            {
               nPaddedBytes = 0;
               if (cData.nLen % 4)
               {
                  nPaddedBytes = 4 - cData.nLen % 4;
               }
               heapsize += cData.nLen + nPaddedBytes;
               heapsize += (int32_t)sizeof(cData.nLen);
               isheapentryfound = 1;
            }
            lut_datafound = 0;
            if (Find(dataofst, &lut_data, &i))// if data is already present skip it , fill the buffer with only unique list of cdft,cdot,data pool entires.
            {
               lut_datafound = 1;
            }
            if (!lut_datafound)//fill data pool entries
            {
               if (!update(dataofst, l, &lut_data))
               {
                  result = ACDB_INSUFFICIENTMEMORY;
                  goto end;
               }

               {
                  calc_data = *((int *)(tblInfo.dataPoolChnk.pData + dataofst));
                  nPaddedBytes = 0;
                  if (calc_data % 4)
                  {
                     nPaddedBytes = 4 - calc_data % 4;
                  }
                  if (!update((calc_data + nPaddedBytes + (int32_t)sizeof(uint32_t) + GetAt(l, &lut_data_buffer)), l + 1, &lut_data_buffer))
                  {
                     result = ACDB_INSUFFICIENTMEMORY;
                     goto end;
                  }
                  size_data = GetAt(l + 1, &lut_data_buffer);
               }
               l++;
            }
         }
         if (isheapentryfound == 1)
         {
            extraheap_cdot += (int32_t)sizeof(cdotTbl.nLen);
            extraheap_cdot += (int32_t)(sizeof(cdotTbl.pDataOffsets)*cdotTbl.nLen);
         }
         //cdft table parsing
         lut_cdftfound = 0;
         if (!update(0, 0, &lut_cdft_buffer))
         {
            result = ACDB_INSUFFICIENTMEMORY;
            goto end;
         }

         if (Find(lutoffsetTbl.pLut[lut_index].nCDEFTblOffset, &lut_cdft, &i))
         {
            lut_cdftfound = 1;
         }
         if (!lut_cdftfound)
         {
            if (!update(lutoffsetTbl.pLut[lut_index].nCDEFTblOffset, j, &lut_cdft))
            {
               result = ACDB_INSUFFICIENTMEMORY;
               goto end;
            }
            {
               ACDB_MEM_CPY(&calc_cdft, sizeof(int32_t), tblInfo.tblCdftChnk.pData + offsettbleinstance.nCDEFTblOffset, sizeof(int32_t));

               if (!update((int32_t)(calc_cdft*sizeof(uint32_t) * 2 + sizeof(uint32_t) + GetAt(j, &lut_cdft_buffer)), j + 1, &lut_cdft_buffer))

               {
                  {
                     result = ACDB_INSUFFICIENTMEMORY;
                     goto end;
                  }
               }
               size_cdft = GetAt(j + 1, &lut_cdft_buffer);
            }
            if (queryType == TABLE_CMD)
               if (!update(GetAt(j, &lut_cdft_buffer), lut_index, &lut_cdft_new))
               {
                  result = ACDB_INSUFFICIENTMEMORY;
                  goto end;
               }
            j++;
         }
         else
            if (queryType == TABLE_CMD)
               if (!update(GetAt(i, &lut_cdft_buffer), lut_index, &lut_cdft_new))
               {
                  result = ACDB_INSUFFICIENTMEMORY;
                  goto end;
               }

         //calculate cdot
         lut_cdotfound = 0;

         if (!update(0, 0, &lut_cdot_buffer))
         {
            result = ACDB_INSUFFICIENTMEMORY;
            goto end;
         }

         if (Find(offsettbleinstance.nCDOTTblOffset, &lut_cdot, &i))
         {
            lut_cdotfound = 1;
         }
         if (!lut_cdotfound)
         {
            if (!update(offsettbleinstance.nCDOTTblOffset, k, &lut_cdot))
            {
               result = ACDB_INSUFFICIENTMEMORY;
               goto end;
            }
                  {
                     ACDB_MEM_CPY(&calc_cdot, sizeof(int32_t), tblInfo.tblCdotChnk.pData + offsettbleinstance.nCDOTTblOffset, sizeof(int32_t));
                     if (!update((int32_t)(calc_cdot*sizeof(uint32_t) + sizeof(uint32_t) + GetAt(k, &lut_cdot_buffer)), k + 1, &lut_cdot_buffer))
                     {
                        result = ACDB_INSUFFICIENTMEMORY;
                        goto end;
                     }

                     size_cdot = GetAt(k + 1, &lut_cdot_buffer);
                  }
                  if (queryType == TABLE_CMD)
                     if (!update(GetAt(k, &lut_cdot_buffer), lut_index, &lut_cdot_new))
                     {
                        result = ACDB_INSUFFICIENTMEMORY;
                        goto end;
                     }
                  k++;
         }
         else
            if (queryType == TABLE_CMD)
               if (!update(GetAt(i, &lut_cdot_buffer), lut_index, &lut_cdot_new))
               {
                  result = ACDB_INSUFFICIENTMEMORY;
                  goto end;
               }
      }
      lut_index_count = lutLength;
      lut_cdft_entries = j; lut_cdot_entries = k; lut_data_entries = l;
      //lut_index=0;
      if (queryType == TABLE_CMD)// write lut size for the first time
      {
         AcdbVocProcGainDepVolTblV2CmdType *pInput = (AcdbVocProcGainDepVolTblV2CmdType *)pIn;
         AcdbQueryResponseType *pOutput = (AcdbQueryResponseType *)pOut;

         int dataOffset = 0;
         nMemBytesLeft = pInput->nBufferLength;
         LookUptble_Length = (uint32_t)((lut_index_count*sizeof(uint32_t)*(VOCPROCDYN_LUT_INDICES_COUNT - 3)) + sizeof(uint32_t));
         if (nMemBytesLeft < (uint32_t)LookUptble_Length)
         {
            ACDB_DEBUG_LOG("Insufficient memory to copy the vocproc static table data\n");
            result = ACDB_INSUFFICIENTMEMORY;
            goto end;
         }
         // write major and minor cvd versions.
         ACDB_MEM_CPY(pInput->nBufferPointer + offset, nMemBytesLeft, &major, sizeof(uint32_t));//Major version
         offset += (uint32_t)sizeof(uint32_t);
         ACDB_MEM_CPY(pInput->nBufferPointer + offset, nMemBytesLeft, &minor, sizeof(uint32_t));//minor version
         offset += (uint32_t)sizeof(uint32_t);
         //
         ACDB_MEM_CPY(pInput->nBufferPointer + offset, nMemBytesLeft, &LookUptble_Length, sizeof(uint32_t));//length of the table
         offset += (uint32_t)sizeof(uint32_t);
         ACDB_MEM_CPY(pInput->nBufferPointer + offset, nMemBytesLeft, &lut_index_count, sizeof(uint32_t));//no of entries
         offset += (uint32_t)sizeof(uint32_t);
         LookUptble_Length += (uint32_t)(3 * sizeof(uint32_t));//major and minor versions +table length

         cdft_offset = cdft_start = LookUptble_Length, cdft_offset = LookUptble_Length;
         //cdft table
         temp_size = size_cdft;//+ sizeof(temp_size);
         ACDB_MEM_CPY(pInput->nBufferPointer + cdft_start, sizeof(temp_size), (void*)(&temp_size), sizeof(temp_size));
         cdft_offset += (int32_t)sizeof(tblInfo.tblCdftChnk.nDataLen);
         for (k = 0; k < lut_cdft_entries; k++)//fill cdft table in buffer
         {
            dataOffset = GetAt(k, &lut_cdft);
            cdefTbl.nLen = READ_UInt32(tblInfo.tblCdftChnk.pData + dataOffset);
            cdefTbl.pCntDef = (ContentDefType *)(tblInfo.tblCdftChnk.pData + dataOffset + sizeof(tblInfo.tblCdftChnk.nDataLen));
            ACDB_MEM_CPY(pInput->nBufferPointer + cdft_offset, sizeof(cdefTbl.nLen), (void*)(&cdefTbl.nLen), sizeof(cdefTbl.nLen));
            cdft_offset += (int32_t)sizeof(cdefTbl.nLen);
            dataOffset += (int32_t)sizeof(cdefTbl.nLen);
            for (i = 0; i < (int)cdefTbl.nLen; i++)
            {
               ACDB_MEM_CPY(pInput->nBufferPointer + cdft_offset, sizeof(cdefTbl.pCntDef[i].nMid), (void*)(&cdefTbl.pCntDef[i].nMid), sizeof(cdefTbl.pCntDef[i].nMid));
               cdft_offset += (int32_t)sizeof(cdefTbl.pCntDef[i].nMid);
               dataOffset += (int32_t)sizeof(cdefTbl.pCntDef[i].nMid);
               ACDB_MEM_CPY(pInput->nBufferPointer + cdft_offset, sizeof(cdefTbl.pCntDef[i].nPid), (void*)(&cdefTbl.pCntDef[i].nPid), sizeof(cdefTbl.pCntDef[i].nPid));
               cdft_offset += (int32_t)sizeof(cdefTbl.pCntDef[i].nPid);
               dataOffset += (int32_t)sizeof(cdefTbl.pCntDef[i].nPid);
            }
         }
         temp_size = cdft_offset - cdft_start;//calculate cdft temp size
         cdot_start = cdot_offset = cdft_offset;
         //write cdot also
         dataOffset = 0;
         temp_size = size_cdot + extraheap_cdot;// sizeof(temp_size);

         datapool_start = datapool_offset = (cdot_offset + size_cdot + (int32_t)sizeof(size_cdot) + (int32_t)sizeof(temp_size) + extraheap_cdot); //reserve the space to write data size.//have the
         ACDB_MEM_CPY(pInput->nBufferPointer + cdot_start, sizeof(size_cdot), (void*)(&temp_size), sizeof(temp_size));
         cdot_offset += (int32_t)sizeof(size_cdot);

         for (k = 0; k < lut_cdot_entries; k++)//fill cdot table in buffer
         {
            int dataoffset_for_actualbuffer = 0;
            dataOffset = GetAt(k, &lut_cdot);
            cdotTbl.nLen = READ_UInt32(tblInfo.tblCdotChnk.pData + dataOffset);
            cdotTbl.pDataOffsets = (uint32_t *)(tblInfo.tblCdotChnk.pData + dataOffset + sizeof(tblInfo.tblCdotChnk.nDataLen));
            ACDB_MEM_CPY(pInput->nBufferPointer + cdot_offset, sizeof(cdotTbl.nLen), (void*)(&cdotTbl.nLen), sizeof(cdotTbl.nLen));
            cdot_offset += (int32_t)sizeof(cdotTbl.nLen);
            dataOffset += (int32_t)sizeof(cdotTbl.nLen);
            for (i = 0; i < (int)cdotTbl.nLen; i++)
            {
               //calculate actual buffer offset
               for (l = 0; l < lut_data_entries; l++)
               {
                  if (cdotTbl.pDataOffsets[i] == (uint32_t)GetAt(l, &lut_data))
                  {
                     dataoffset_for_actualbuffer = GetAt(l, &lut_data_buffer);
                     break;
                  }
               }
               ACDB_MEM_CPY(pInput->nBufferPointer + cdot_offset, sizeof(dataoffset_for_actualbuffer), (void*)(&dataoffset_for_actualbuffer), sizeof(dataoffset_for_actualbuffer));
               cdot_offset += (int32_t)sizeof(cdotTbl.pDataOffsets[i]);
               dataOffset += (int32_t)sizeof(cdotTbl.pDataOffsets[i]);
            }
         }
         temp_pointer = cdot_offset;

         //datapool_offset=temp_pointer;

         for (l = 0; l < lut_data_entries; l++)//fill data pool entries table in buffer
         {
            dataOffset = GetAt(l, &lut_data);
            cData.nLen = READ_UInt32(tblInfo.dataPoolChnk.pData + dataOffset);
            cData.pData = tblInfo.dataPoolChnk.pData + dataOffset + sizeof(cData.nLen);
            nPaddedBytes = 0;
            if (cData.nLen % 4)
            {
               nPaddedBytes = 4 - cData.nLen % 4;
            }
            Valid_length = cData.nLen + nPaddedBytes;
            ACDB_MEM_CPY(pInput->nBufferPointer + datapool_offset, sizeof(cData.nLen), (void*)&(Valid_length), sizeof(cData.nLen));
            datapool_offset += (int32_t)sizeof(cData.nLen);
            memset((pInput->nBufferPointer + datapool_offset), 0, (size_t)(cData.nLen + nPaddedBytes));
            ACDB_MEM_CPY(pInput->nBufferPointer + datapool_offset, cData.nLen, (void*)(cData.pData), cData.nLen);
            datapool_offset += cData.nLen + nPaddedBytes;
         }
         //fill heap entries.
         if (heapsize > 0)
         {
            for (lut_index = 0; lut_index<(int)lutLength; lut_index++)
            {
               // VocProcDynDataLookupType_offsetData *offsettbleinstance=(VocProcDynDataLookupType_offsetData*)(&lutoffsetTbl.pLut[lut_index]);
               int dataofst = 0;
               //ContentDataOffsetsTblType cdotTbl1;
               int mid = 0, pid = 0;
               isheapentryfound = 0;
               //datatable calculation
               cdotTbl.nLen = READ_UInt32(pInput->nBufferPointer + cdot_start + 4 + GetAt(lut_index, &lut_cdot_new));
               cdotTbl.pDataOffsets = (uint32_t *)(pInput->nBufferPointer + cdot_start + 4 + GetAt(lut_index, &lut_cdot_new) + sizeof(tblInfo.tblCdotChnk.nDataLen));

               cdefTbl.nLen = READ_UInt32(pInput->nBufferPointer + cdft_start + 4 + GetAt(lut_index, &lut_cdft_new));
               cdefTbl.pCntDef = (ContentDefType *)(pInput->nBufferPointer + cdft_start + 4 + GetAt(lut_index, &lut_cdft_new)/*offsettbleinstance->nCDEFTblOffset*/ + sizeof(tblInfo.tblCdftChnk.nDataLen));
               //reserve space
               if (temp_pointer + (int)cdotTbl.nLen*(int)sizeof(cdotTbl.pDataOffsets) + 4>(datapool_start - (int)sizeof(uint32_t)))//reached max cdot entries
               {
                  continue;
               }
               ACDB_MEM_CPY(pInput->nBufferPointer + temp_pointer, sizeof(cdotTbl.nLen), (void*)(&cdotTbl.nLen), sizeof(cdotTbl.nLen));

               ACDB_MEM_CPY(pInput->nBufferPointer + temp_pointer + 4, cdotTbl.nLen*sizeof(cdotTbl.pDataOffsets), (void*)cdotTbl.pDataOffsets, cdotTbl.nLen*sizeof(cdotTbl.pDataOffsets));

               cdotTbl.nLen = READ_UInt32(pInput->nBufferPointer + temp_pointer);
               cdotTbl.pDataOffsets = (uint32_t *)(pInput->nBufferPointer + temp_pointer + sizeof(tblInfo.tblCdotChnk.nDataLen));

               //fill heap
               for (m = 0; m < (int)cdotTbl.nLen; m++)
               {
                  ACDB_MEM_CPY(&dataofst, sizeof(dataofst), (void*)(&cdotTbl.pDataOffsets[m]), sizeof(dataofst));
                  ACDB_MEM_CPY(&mid, sizeof(dataofst), (void*)(&cdefTbl.pCntDef[m].nMid), sizeof(dataofst));
                  ACDB_MEM_CPY(&pid, sizeof(dataofst), (void*)(&cdefTbl.pCntDef[m].nPid), sizeof(dataofst));
                  nLookupKey = (uintptr_t)&lutTblCVD.pLut[lut_index];
                  cdftLookup = (uintptr_t)&cdefTbl.pCntDef[m];
                  result = GetMidPidCalibHeapDataEx(tblId, nLookupKey, data_Seclookup, cdftLookup, pid, &cData);//get from heap
                  if (ACDB_SUCCESS == result)
                  {
                     nPaddedBytes = 0;
                     if (cData.nLen % 4)
                     {
                        nPaddedBytes = 4 - cData.nLen % 4;
                     }
                     Valid_length = cData.nLen + nPaddedBytes;
                     cdotTbl.pDataOffsets[m] = datapool_offset - datapool_start;
                     ACDB_MEM_CPY(pInput->nBufferPointer + datapool_offset, sizeof(cData.nLen), (void*)(&Valid_length), sizeof(cData.nLen));
                     datapool_offset += (int32_t)sizeof(cData.nLen);

                     memset((pInput->nBufferPointer + datapool_offset), 0, (size_t)(cData.nLen + nPaddedBytes));

                     ACDB_MEM_CPY(pInput->nBufferPointer + datapool_offset, cData.nLen, (void*)(cData.pData), cData.nLen);
                     datapool_offset += cData.nLen + nPaddedBytes;
                     if (!update((temp_pointer - cdot_start - (int32_t)sizeof(uint32_t)), lut_index, &lut_cdot_new))
                     {
                        result = ACDB_INSUFFICIENTMEMORY;
                        goto end;
                     }
                     isheapentryfound = 1;
                  }
               }
               if (isheapentryfound == 1)
               {
                  temp_pointer += (int32_t)sizeof(cdotTbl.nLen);
                  temp_pointer += (int32_t)(cdotTbl.nLen*sizeof(cdotTbl.pDataOffsets));

                  if (temp_pointer >= (datapool_start - (int)sizeof(uint32_t)))//reached max cdot entries
                     break;
               }
            }
         }

         j = datapool_offset - temp_pointer - (int32_t)sizeof(uint32_t);
         ACDB_MEM_CPY(pInput->nBufferPointer + temp_pointer, sizeof(size_cdot), (void*)(&j), sizeof(size_cdot));
         nMemBytesLeft -= datapool_offset;
         pOutput->nBytesUsedInBuffer = datapool_offset;
      }
      else
      {
         AcdbSizeResponseType *pOutput = (AcdbSizeResponseType *)pOut;
         LookUptble_Length = (uint32_t)((lut_index_count*sizeof(uint32_t)*(VOCPROCDYN_LUT_INDICES_COUNT - 3)) + sizeof(uint32_t));
         LookUptble_Length += (uint32_t)(3 * sizeof(uint32_t));//major and minor versions
         offset += LookUptble_Length;
         offset += (uint32_t)(sizeof(size_cdft));
         offset += size_cdft;
         offset += (uint32_t)(sizeof(size_cdot));
         offset += size_cdot;
         offset += (uint32_t)(sizeof(uint32_t));//reseve for data size chunk
         offset += size_data;
         offset += heapsize;
         offset += extraheap_cdot;
         pOutput->nSize = offset;
         result = ACDB_SUCCESS;
         goto end;
      }
      for (lut_index = 0; lut_index < lutLength; lut_index++)
      {
         //go the cvd common table to get actual common entry
		  VocProcDynDataLookupType_CommonData commonLUTInstance = { 0, 0, 0, 0, 0, 0, 0, 0 };
         ACDB_MEM_CPY(&commonLUTInstance, sizeof(VocProcDynDataLookupType_CommonData), (tblInfo.tblLutCVDChnk.pData + sizeof(uint32_t) + cvdTbloffset + sizeof(VocProcDynDataLookupType_CommonData)*lut_index), sizeof(VocProcDynDataLookupType_CommonData));

         switch (queryType)
         {
         case TABLE_CMD:
         {
            //fill lut entries in CVD format
            AcdbVocProcGainDepVolTblV2CmdType *pInput = (AcdbVocProcGainDepVolTblV2CmdType *)pIn;
            ACDB_MEM_CPY(&temp_lut_node.nNetworkId, sizeof(uint32_t), &commonLUTInstance.nNetwork, sizeof(uint32_t));
            ACDB_MEM_CPY(&temp_lut_node.nRxVocSr, sizeof(uint32_t), &commonLUTInstance.nRxVocSR, sizeof(uint32_t));
            ACDB_MEM_CPY(&temp_lut_node.nTxVocSr, sizeof(uint32_t), &commonLUTInstance.nTxVocSR, sizeof(uint32_t));
            ACDB_MEM_CPY(&temp_lut_node.nVolIdx, sizeof(uint32_t), &commonLUTInstance.nVocVolStep, sizeof(uint32_t));
            ACDB_MEM_CPY(&temp_lut_node.nVocoder_type, sizeof(uint32_t), &commonLUTInstance.nVocoder_type, sizeof(uint32_t));
            ACDB_MEM_CPY(&temp_lut_node.rx_Voc_mode, sizeof(uint32_t), &commonLUTInstance.rx_Voc_mode, sizeof(uint32_t));
            ACDB_MEM_CPY(&temp_lut_node.tx_Voc_mode, sizeof(uint32_t), &commonLUTInstance.tx_Voc_mode, sizeof(uint32_t));
            ACDB_MEM_CPY(&temp_lut_node.nFeature, sizeof(uint32_t), &commonLUTInstance.nFeature, sizeof(uint32_t));

            temp_lut_node.cdft_offset = GetAt(lut_index, &lut_cdft_new);//lutoffsetTbl.pLut[lut_index].nCDEFTblOffset;
            temp_lut_node.cdot_offset = GetAt(lut_index, &lut_cdot_new);
            ACDB_MEM_CPY(pInput->nBufferPointer + offset, sizeof(temp_lut_node.nNetworkId), (void*)(&temp_lut_node.nNetworkId), sizeof(temp_lut_node.nNetworkId));
            offset += (uint32_t)(sizeof(temp_lut_node.nNetworkId));
            ACDB_MEM_CPY(pInput->nBufferPointer + offset, sizeof(temp_lut_node.nTxVocSr), (void*)(&temp_lut_node.nTxVocSr), sizeof(temp_lut_node.nTxVocSr));
            offset += (uint32_t)(sizeof(temp_lut_node.nTxVocSr));
            ACDB_MEM_CPY(pInput->nBufferPointer + offset, sizeof(temp_lut_node.nRxVocSr), (void*)(&temp_lut_node.nRxVocSr), sizeof(temp_lut_node.nRxVocSr));
            offset += (uint32_t)(sizeof(temp_lut_node.nRxVocSr));
            ACDB_MEM_CPY(pInput->nBufferPointer + offset, sizeof(temp_lut_node.nVolIdx), (void*)(&temp_lut_node.nVolIdx), sizeof(temp_lut_node.nVolIdx));
            offset += (uint32_t)(sizeof(temp_lut_node.nVolIdx));
            ACDB_MEM_CPY(pInput->nBufferPointer + offset, sizeof(temp_lut_node.tx_Voc_mode), (void*)(&temp_lut_node.tx_Voc_mode), sizeof(temp_lut_node.tx_Voc_mode));
            offset += (uint32_t)(sizeof(temp_lut_node.tx_Voc_mode));
            ACDB_MEM_CPY(pInput->nBufferPointer + offset, sizeof(temp_lut_node.rx_Voc_mode), (void*)(&temp_lut_node.rx_Voc_mode), sizeof(temp_lut_node.rx_Voc_mode));
            offset += (uint32_t)(sizeof(temp_lut_node.rx_Voc_mode));
            ACDB_MEM_CPY(pInput->nBufferPointer + offset, sizeof(temp_lut_node.nVocoder_type), (void*)(&temp_lut_node.nVocoder_type), sizeof(temp_lut_node.nVocoder_type));
            offset += (uint32_t)(sizeof(temp_lut_node.nVocoder_type));
            ACDB_MEM_CPY(pInput->nBufferPointer + offset, sizeof(temp_lut_node.nFeature), (void*)(&temp_lut_node.nFeature), sizeof(temp_lut_node.nFeature));
            offset += (uint32_t)(sizeof(temp_lut_node.nFeature));
            ACDB_MEM_CPY(pInput->nBufferPointer + offset, sizeof(temp_lut_node.cdft_offset), (void*)(&temp_lut_node.cdft_offset), sizeof(temp_lut_node.cdft_offset));
            offset += (uint32_t)(sizeof(temp_lut_node.cdft_offset));
            ACDB_MEM_CPY(pInput->nBufferPointer + offset, sizeof(temp_lut_node.cdot_offset), (void*)(&temp_lut_node.cdot_offset), sizeof(temp_lut_node.cdot_offset));
            offset += (uint32_t)(sizeof(temp_lut_node.cdot_offset));
         }
         break;

         default:
            result = ACDB_ERROR;
            goto end;
         }
      }
      result = ACDB_SUCCESS;
      goto end;
   }
end:
   if (lut_cdft != NULL)
   {
      deleteList(&lut_cdft);
      lut_cdft = NULL;
   }
   if (lut_cdot != NULL)
   {
      deleteList(&lut_cdot);
      lut_cdot = NULL;
   }
   if (lut_cdft_buffer != NULL)
   {
      deleteList(&lut_cdft_buffer);
      lut_cdft_buffer = NULL;
   }
   if (lut_cdot_buffer != NULL)
   {
      deleteList(&lut_cdot_buffer);
      lut_cdot_buffer = NULL;
   }
   if (lut_data_buffer != NULL)
   {
      deleteList(&lut_data_buffer);
      lut_data_buffer = NULL;
   }
   if (lut_data != NULL)
   {
      deleteList(&lut_data);
      lut_data = NULL;
   }
   if (lut_cdot_new != NULL)
   {
      deleteList(&lut_cdot_new);
      lut_cdot_new = NULL;
   }
   if (lut_cdft_new != NULL)
   {
      deleteList(&lut_cdft_new);
      lut_cdft_new = NULL;
   }

   return result;
}

int32_t AcdbCmdGetVocProcDynWithInstanceInfo(uint32_t queryType, uint8_t *pIn, uint8_t *pOut)
{
   int32_t result = ACDB_SUCCESS;
   uintptr_t nLookupKey = 0;
   uintptr_t cdftLookup = 0;
   struct node *lut_cdft = NULL, *lut_cdot = NULL, *lut_cdft_buffer = NULL, *lut_cdot_buffer = NULL, *lut_data_buffer = NULL, *lut_data = NULL, *lut_cdot_new = NULL, *lut_cdft_new = NULL;
   int size_cdft = 0, size_cdot = 0, size_data = 0, lut_cdot_entries = 0, lut_cdft_entries = 0, lut_data_entries = 0;

   if (pIn == NULL || pOut == NULL)
   {
      ACDB_DEBUG_LOG("[ACDB Command]->[AcdbCmdGetVocProcDynInfo]->Invalid Null input param provided\n");
      result = ACDB_BADPARM;
   }
   else
   {
      uint32_t LookUptble_Length = 0, lut_index_count = 0, lut_index = 0, major = 1, minor = 1, ofTbloffset = 0, cvdTbloffset = 0, lutLength = 0;
      int cdft_start = 0, cdot_start = 0, i = 0, j = 0, k = 0, l = 0, m = 0, temp_size = 0;
      int cdft_offset = 0, cdot_offset = 0, datapool_offset = 0, datapool_start = 0;
      uintptr_t data_Seclookup = 0;
      int lut_cdftfound = 0, lut_cdotfound = 0, calc_data = 0, heapsize = 0;
      VocProcDynCVDTblHdrFmtType temp_lut_node = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
      AcdbDataType cData = { 0, NULL };
      uint32_t index = 0, Valid_length = 0;
      AcdbTableCmd cmd = { 0, 0 };
      AcdbTableInfoCVD tblInfo = { { 0, NULL }, { 0, NULL }, { 0, NULL }, { 0, NULL }, { 0, NULL }, { 0, NULL } };
      VocProcDynDataLookupType_UniqueData tempLut = { 0, 0, 0, 0, 0 };
      VocProcDynDataLookupTblType_UniqueData lutTbl = { 0, NULL };
      VocProcDynDataLookupTblType_CommonData lutTblCVD = { 0, NULL };
      VocProcDynDataLookupTblType_offsetData lutoffsetTbl = { 0, NULL };
      ContentDefWithInstanceTblType cdefTbl = { 0, NULL };
      ContentDataOffsetsTblType cdotTbl = { 0, NULL };
      uint32_t offset = 0;
      uint32_t nMemBytesLeft = 0, nPaddedBytes = 0;
      VocProcVolV2CmdLookupType voccmd = { 0, 0, 0 };
      uint32_t tblId = VOCPROC_DYN_INST_TBL_ID;
      int temp_pointer = 0, extraheap_cdot = 0, isheapentryfound = 0;;

      switch (queryType)
      {
      case TABLE_CMD:
      {
         AcdbVocProcGainDepVolTblV2CmdType *pInput = (AcdbVocProcGainDepVolTblV2CmdType *)pIn;
         if (pInput->nBufferPointer == NULL)
            return ACDB_BADPARM;
         voccmd.nTxDevId = pInput->nTxDeviceId;
         voccmd.nRxDevId = pInput->nRxDeviceId;
         voccmd.nFeatureId = pInput->nFeatureId;
      }
      break;
      case TABLE_SIZE_CMD:
      {
         AcdbVocProcGainDepVolTblSizeV2CmdType *pInput = (AcdbVocProcGainDepVolTblSizeV2CmdType *)pIn;
         voccmd.nTxDevId = pInput->nTxDeviceId;
         voccmd.nRxDevId = pInput->nRxDeviceId;
         voccmd.nFeatureId = pInput->nFeatureId;
      }
      break;
      default:
         result = ACDB_ERROR;
         goto end;
      }

      cmd.devId = voccmd.nTxDevId;
      cmd.tblId = tblId;
      result = acdbdata_ioctl(ACDBDATACMD_GET_TABLE_INFO, (uint8_t *)&cmd, sizeof(cmd),
         (uint8_t *)&tblInfo, sizeof(tblInfo));
      if (result != ACDB_SUCCESS)
      {
         if (result == ACDB_DATA_INTERFACE_NOT_FOUND)
         {
            ACDB_DEBUG_LOG("Failed to fetch the Table in ACDB files Table-ID %08X \n"
               , tblId);
            goto end;
         }
         ACDB_DEBUG_LOG("Failed to fetch the lookup information of the device %08X \n"
            , voccmd.nTxDevId);
         goto end;
      }

      lutTbl.nLen = READ_UInt32(tblInfo.tblLutChnk.pData);
      lutTbl.pLut = (VocProcDynDataLookupType_UniqueData *)(tblInfo.tblLutChnk.pData + sizeof(lutTbl.nLen));

      result = AcdbDataBinarySearch((void *)lutTbl.pLut, lutTbl.nLen,
         VOCPROCDYN_LUT_INDICES_COUNT - 8, &voccmd, VOCPROCDYN_CMD_INDICES_COUNT, &index);
      if (result != SEARCH_SUCCESS)
      {
         ACDB_DEBUG_LOG("Failed to fetch the lookup information of the device %08X \n"
            , voccmd.nTxDevId);
         result = ACDB_INPUT_PARAMS_NOT_FOUND;
         goto end;
      }

      data_Seclookup = (uintptr_t)&lutTbl.pLut[index];
      lutTblCVD.nLen = READ_UInt32(tblInfo.tblLutCVDChnk.pData);

      lutTblCVD.pLut = (VocProcDynDataLookupType_CommonData *)(tblInfo.tblLutCVDChnk.pData + sizeof(lutTbl.nLen));
      if (lutTblCVD.nLen <= 0)
      {
         ACDB_DEBUG_LOG("Vocprocdyn lutcvdTbl empty\n");
         result = ACDB_ERROR;
         goto end;
      }
      lutoffsetTbl.nLen = READ_UInt32(tblInfo.tblLutOffsetChnk.pData);
      lutoffsetTbl.pLut = (VocProcDynDataLookupType_offsetData *)(tblInfo.tblLutOffsetChnk.pData + sizeof(lutTbl.nLen));
      cdefTbl.nLen = READ_UInt32(tblInfo.tblCdftChnk.pData);
      cdefTbl.pCntDef = (ContentDefInstanceType *)(tblInfo.tblCdftChnk.pData + sizeof(cdefTbl.nLen));
      if (cdefTbl.nLen <= 0)
      {
         ACDB_DEBUG_LOG("Vocprocdyn cdefTbl empty\n");
         result = ACDB_ERROR;
         goto end;
      }

      cdotTbl.nLen = READ_UInt32(tblInfo.tblCdotChnk.pData);
      cdotTbl.pDataOffsets = (uint32_t *)(tblInfo.tblCdotChnk.pData + sizeof(cdotTbl.nLen));
      if (cdotTbl.nLen <= 0)
      {
         ACDB_DEBUG_LOG("Vocstream cdotTbl empty\n");
         result = ACDB_ERROR;
         goto end;
      }

      ACDB_MEM_CPY(&tempLut, sizeof(tempLut), &lutTbl.pLut[index], sizeof(tempLut));
      // Go to the offset table from luptableunique
	  ofTbloffset = tempLut.nOffTableOffset;
	  cvdTbloffset = tempLut.nCommonTblOffset;
      ACDB_MEM_CPY(&lutLength, sizeof(uint32_t), tblInfo.tblLutCVDChnk.pData + cvdTbloffset, sizeof(uint32_t));

      lutoffsetTbl.pLut = (VocProcDynDataLookupType_offsetData *)(tblInfo.tblLutOffsetChnk.pData + sizeof(uint32_t) + ofTbloffset);
      lutTblCVD.pLut = (VocProcDynDataLookupType_CommonData *)(tblInfo.tblLutCVDChnk.pData + sizeof(uint32_t) + cvdTbloffset);

      if (lutLength == 0)
      {
         ACDB_DEBUG_LOG("[ACDB Command]->[AcdbCmdGetVocProcDynInfo]->inside lutlength zero of lut tables %08X ", lutLength);
         lutLength = lutTblCVD.nLen;
      }

      j = 0, k = 0;
      for (lut_index = 0; lut_index < (int)lutLength; lut_index++)
      {
         uint32_t mid = 0, pid = 0, iid = 0;
         VocProcDynDataLookupType_offsetData offsettbleinstance = { 0, 0 };
         int32_t calc_cdft = 0, calc_cdot = 0, lut_datafound = 0, dataofst = 0;
         ACDB_MEM_CPY(&offsettbleinstance, sizeof(VocProcDynDataLookupType_offsetData), &lutoffsetTbl.pLut[lut_index], sizeof(VocProcDynDataLookupType_offsetData));

         //datatable calculation
         {
            cdotTbl.nLen = READ_UInt32(tblInfo.tblCdotChnk.pData + offsettbleinstance.nCDOTTblOffset);
            cdotTbl.pDataOffsets = (uint32_t *)(tblInfo.tblCdotChnk.pData + offsettbleinstance.nCDOTTblOffset + sizeof(tblInfo.tblCdotChnk.nDataLen));
         }
         {
            //ACDB_DEBUG_LOG("[ACDB Command]->[AcdbCmdGetVocProcDynInfo]->crash related logs-before crash lutindex %08X cdefoffset %08X \n",lut_index,offsettbleinstance.nCDEFTblOffset);
            cdefTbl.nLen = READ_UInt32(tblInfo.tblCdftChnk.pData + offsettbleinstance.nCDEFTblOffset);
            cdefTbl.pCntDef = (ContentDefInstanceType *)(tblInfo.tblCdftChnk.pData + offsettbleinstance.nCDEFTblOffset + sizeof(tblInfo.tblCdftChnk.nDataLen));
         }
         if (!update(offsettbleinstance.nCDOTTblOffset, lut_index, &lut_cdot_new))
         {
            result = ACDB_INSUFFICIENTMEMORY;
            goto end;
         }
         if (!update(offsettbleinstance.nCDEFTblOffset, lut_index, &lut_cdft_new))
         {
            result = ACDB_INSUFFICIENTMEMORY;
            goto end;
         }
         if (!update(0, 0, &lut_data_buffer))
         {
            result = ACDB_INSUFFICIENTMEMORY;
            goto end;
         }
         isheapentryfound = 0;
         for (m = 0; m < (int)cdotTbl.nLen; m++)
         {
            ACDB_MEM_CPY(&dataofst, sizeof(dataofst), (void*)(&cdotTbl.pDataOffsets[m]), sizeof(dataofst));
            ACDB_MEM_CPY(&mid, sizeof(mid), (void*)(&cdefTbl.pCntDef[m].nMid), sizeof(mid));
            ACDB_MEM_CPY(&iid, sizeof(iid), (void*)(&cdefTbl.pCntDef[m].nIid), sizeof(iid));
            ACDB_MEM_CPY(&pid, sizeof(pid), (void*)(&cdefTbl.pCntDef[m].nPid), sizeof(pid));
            nLookupKey = (uintptr_t)&lutTblCVD.pLut[lut_index];
            cdftLookup = (uintptr_t)&cdefTbl.pCntDef[m];
            result = GetMidIidPidCalibHeapDataEx(tblId, nLookupKey, data_Seclookup, cdftLookup, pid, &cData);//get from heap
            if (result == ACDB_SUCCESS)
            {
               nPaddedBytes = 0;
               if (cData.nLen % 4)
               {
                  nPaddedBytes = 4 - cData.nLen % 4;
               }
               heapsize += cData.nLen + nPaddedBytes;
               heapsize += (int32_t)sizeof(cData.nLen);
               isheapentryfound = 1;
            }
            lut_datafound = 0;
            if (Find(dataofst, &lut_data, &i))// if data is already present skip it , fill the buffer with only unique list of cdft,cdot,data pool entires.
            {
               lut_datafound = 1;
            }
            if (!lut_datafound)//fill data pool entries
            {
               if (!update(dataofst, l, &lut_data))
               {
                  result = ACDB_INSUFFICIENTMEMORY;
                  goto end;
               }

               {
                  calc_data = *((int *)(tblInfo.dataPoolChnk.pData + dataofst));
                  nPaddedBytes = 0;
                  if (calc_data % 4)
                  {
                     nPaddedBytes = 4 - calc_data % 4;
                  }
                  if (!update((calc_data + nPaddedBytes + (int32_t)sizeof(uint32_t) + GetAt(l, &lut_data_buffer)), l + 1, &lut_data_buffer))
                  {
                     result = ACDB_INSUFFICIENTMEMORY;
                     goto end;
                  }
                  size_data = GetAt(l + 1, &lut_data_buffer);
               }
               l++;
            }
         }
         if (isheapentryfound == 1)
         {
            extraheap_cdot += (int32_t)sizeof(cdotTbl.nLen);
            extraheap_cdot += (int32_t)(sizeof(cdotTbl.pDataOffsets)*cdotTbl.nLen);
         }
         //cdft table parsing
         lut_cdftfound = 0;
         if (!update(0, 0, &lut_cdft_buffer))
         {
            result = ACDB_INSUFFICIENTMEMORY;
            goto end;
         }

         if (Find(lutoffsetTbl.pLut[lut_index].nCDEFTblOffset, &lut_cdft, &i))
         {
            lut_cdftfound = 1;
         }
         if (!lut_cdftfound)
         {
            if (!update(lutoffsetTbl.pLut[lut_index].nCDEFTblOffset, j, &lut_cdft))
            {
               result = ACDB_INSUFFICIENTMEMORY;
               goto end;
            }
            {
               ACDB_MEM_CPY(&calc_cdft, sizeof(int32_t), tblInfo.tblCdftChnk.pData + offsettbleinstance.nCDEFTblOffset, sizeof(int32_t));

               if (!update((int32_t)(calc_cdft*sizeof(uint32_t) * 3 + sizeof(uint32_t) + GetAt(j, &lut_cdft_buffer)), j + 1, &lut_cdft_buffer))

               {
                  {
                     result = ACDB_INSUFFICIENTMEMORY;
                     goto end;
                  }
               }
               size_cdft = GetAt(j + 1, &lut_cdft_buffer);
            }
            if (queryType == TABLE_CMD)
               if (!update(GetAt(j, &lut_cdft_buffer), lut_index, &lut_cdft_new))
               {
                  result = ACDB_INSUFFICIENTMEMORY;
                  goto end;
               }
            j++;
         }
         else
            if (queryType == TABLE_CMD)
               if (!update(GetAt(i, &lut_cdft_buffer), lut_index, &lut_cdft_new))
               {
                  result = ACDB_INSUFFICIENTMEMORY;
                  goto end;
               }

         //calculate cdot
         lut_cdotfound = 0;

         if (!update(0, 0, &lut_cdot_buffer))
         {
            result = ACDB_INSUFFICIENTMEMORY;
            goto end;
         }

         if (Find(offsettbleinstance.nCDOTTblOffset, &lut_cdot, &i))
         {
            lut_cdotfound = 1;
         }
         if (!lut_cdotfound)
         {
            if (!update(offsettbleinstance.nCDOTTblOffset, k, &lut_cdot))
            {
               result = ACDB_INSUFFICIENTMEMORY;
               goto end;
            }
                  {
                     ACDB_MEM_CPY(&calc_cdot, sizeof(int32_t), tblInfo.tblCdotChnk.pData + offsettbleinstance.nCDOTTblOffset, sizeof(int32_t));
                     if (!update((int32_t)(calc_cdot*sizeof(uint32_t) + sizeof(uint32_t) + GetAt(k, &lut_cdot_buffer)), k + 1, &lut_cdot_buffer))
                     {
                        result = ACDB_INSUFFICIENTMEMORY;
                        goto end;
                     }

                     size_cdot = GetAt(k + 1, &lut_cdot_buffer);
                  }
                  if (queryType == TABLE_CMD)
                     if (!update(GetAt(k, &lut_cdot_buffer), lut_index, &lut_cdot_new))
                     {
                        result = ACDB_INSUFFICIENTMEMORY;
                        goto end;
                     }
                  k++;
         }
         else
            if (queryType == TABLE_CMD)
               if (!update(GetAt(i, &lut_cdot_buffer), lut_index, &lut_cdot_new))
               {
                  result = ACDB_INSUFFICIENTMEMORY;
                  goto end;
               }
      }
      lut_index_count = lutLength;
      lut_cdft_entries = j; lut_cdot_entries = k; lut_data_entries = l;
      //lut_index=0;
      if (queryType == TABLE_CMD)// write lut size for the first time
      {
         AcdbVocProcGainDepVolTblV2CmdType *pInput = (AcdbVocProcGainDepVolTblV2CmdType *)pIn;
         AcdbQueryResponseType *pOutput = (AcdbQueryResponseType *)pOut;

         int dataOffset = 0;
         nMemBytesLeft = pInput->nBufferLength;
         LookUptble_Length = (uint32_t)((lut_index_count*sizeof(uint32_t)*(VOCPROCDYN_LUT_INDICES_COUNT - 3)) + sizeof(uint32_t));
         if (nMemBytesLeft < (uint32_t)LookUptble_Length)
         {
            ACDB_DEBUG_LOG("Insufficient memory to copy the vocproc static table data\n");
            result = ACDB_INSUFFICIENTMEMORY;
            goto end;
         }
         // write major and minor cvd versions.
         ACDB_MEM_CPY(pInput->nBufferPointer + offset, nMemBytesLeft, &major, sizeof(uint32_t));//Major version
         offset += (uint32_t)sizeof(uint32_t);
         ACDB_MEM_CPY(pInput->nBufferPointer + offset, nMemBytesLeft, &minor, sizeof(uint32_t));//minor version
         offset += (uint32_t)sizeof(uint32_t);
         //
         ACDB_MEM_CPY(pInput->nBufferPointer + offset, nMemBytesLeft, &LookUptble_Length, sizeof(uint32_t));//length of the table
         offset += (uint32_t)sizeof(uint32_t);
         ACDB_MEM_CPY(pInput->nBufferPointer + offset, nMemBytesLeft, &lut_index_count, sizeof(uint32_t));//no of entries
         offset += (uint32_t)sizeof(uint32_t);
         LookUptble_Length += (uint32_t)(3 * sizeof(uint32_t));//major and minor versions +table length

         cdft_offset = cdft_start = LookUptble_Length, cdft_offset = LookUptble_Length;
         //cdft table
         temp_size = size_cdft;//+ sizeof(temp_size);
         ACDB_MEM_CPY(pInput->nBufferPointer + cdft_start, sizeof(temp_size), (void*)(&temp_size), sizeof(temp_size));
         cdft_offset += (int32_t)(sizeof(tblInfo.tblCdftChnk.nDataLen));
         for (k = 0; k < lut_cdft_entries; k++)//fill cdft table in buffer
         {
            dataOffset = GetAt(k, &lut_cdft);
            cdefTbl.nLen = READ_UInt32(tblInfo.tblCdftChnk.pData + dataOffset);
            cdefTbl.pCntDef = (ContentDefInstanceType *)(tblInfo.tblCdftChnk.pData + dataOffset + sizeof(tblInfo.tblCdftChnk.nDataLen));
            ACDB_MEM_CPY(pInput->nBufferPointer + cdft_offset, sizeof(cdefTbl.nLen), (void*)(&cdefTbl.nLen), sizeof(cdefTbl.nLen));
            cdft_offset += (int32_t)sizeof(cdefTbl.nLen);
            dataOffset += (int32_t)sizeof(cdefTbl.nLen);
            for (i = 0; i < (int)cdefTbl.nLen; i++)
            {
               uint16_t iid = 0, reserved = 0;
               ACDB_MEM_CPY(pInput->nBufferPointer + cdft_offset, sizeof(cdefTbl.pCntDef[i].nMid), (void*)(&cdefTbl.pCntDef[i].nMid), sizeof(cdefTbl.pCntDef[i].nMid));
               cdft_offset += (int32_t)sizeof(cdefTbl.pCntDef[i].nMid);
               dataOffset += (int32_t)sizeof(cdefTbl.pCntDef[i].nMid);

               iid = GetUint16IID(cdefTbl.pCntDef[i].nIid);
               ACDB_MEM_CPY(pInput->nBufferPointer + cdft_offset, sizeof(iid), (void*)(&iid), sizeof(iid));
               cdft_offset += (int32_t)sizeof(iid);
               dataOffset += (int32_t)sizeof(iid);

               ACDB_MEM_CPY(pInput->nBufferPointer + cdft_offset, sizeof(reserved), (void*)(&reserved), sizeof(reserved));
               cdft_offset += (int32_t)sizeof(reserved);
               dataOffset += (int32_t)sizeof(reserved);

               ACDB_MEM_CPY(pInput->nBufferPointer + cdft_offset, sizeof(cdefTbl.pCntDef[i].nPid), (void*)(&cdefTbl.pCntDef[i].nPid), sizeof(cdefTbl.pCntDef[i].nPid));
               cdft_offset += (int32_t)sizeof(cdefTbl.pCntDef[i].nPid);
               dataOffset += (int32_t)sizeof(cdefTbl.pCntDef[i].nPid);
            }
         }
         temp_size = cdft_offset - cdft_start;//calculate cdft temp size
         cdot_start = cdot_offset = cdft_offset;
         //write cdot also
         dataOffset = 0;
         temp_size = size_cdot + extraheap_cdot;// sizeof(temp_size);

         datapool_start = datapool_offset = (cdot_offset + size_cdot + (int32_t)sizeof(size_cdot) + (int32_t)sizeof(temp_size) + extraheap_cdot); //reserve the space to write data size.//have the
         ACDB_MEM_CPY(pInput->nBufferPointer + cdot_start, sizeof(size_cdot), (void*)(&temp_size), sizeof(temp_size));
         cdot_offset += (int32_t)sizeof(size_cdot);

         for (k = 0; k < lut_cdot_entries; k++)//fill cdot table in buffer
         {
            int dataoffset_for_actualbuffer = 0;
            dataOffset = GetAt(k, &lut_cdot);
            cdotTbl.nLen = READ_UInt32(tblInfo.tblCdotChnk.pData + dataOffset);
            cdotTbl.pDataOffsets = (uint32_t *)(tblInfo.tblCdotChnk.pData + dataOffset + sizeof(tblInfo.tblCdotChnk.nDataLen));
            ACDB_MEM_CPY(pInput->nBufferPointer + cdot_offset, sizeof(cdotTbl.nLen), (void*)(&cdotTbl.nLen), sizeof(cdotTbl.nLen));
            cdot_offset += (int32_t)sizeof(cdotTbl.nLen);
            dataOffset += (int32_t)sizeof(cdotTbl.nLen);
            for (i = 0; i < (int)cdotTbl.nLen; i++)
            {
               //calculate actual buffer offset
               for (l = 0; l < lut_data_entries; l++)
               {
                  if (cdotTbl.pDataOffsets[i] == (uint32_t)GetAt(l, &lut_data))
                  {
                     dataoffset_for_actualbuffer = GetAt(l, &lut_data_buffer);
                     break;
                  }
               }
               ACDB_MEM_CPY(pInput->nBufferPointer + cdot_offset, sizeof(dataoffset_for_actualbuffer), (void*)(&dataoffset_for_actualbuffer), sizeof(dataoffset_for_actualbuffer));
               cdot_offset += (int32_t)sizeof(cdotTbl.pDataOffsets[i]);
               dataOffset += (int32_t)sizeof(cdotTbl.pDataOffsets[i]);
            }
         }
         temp_pointer = cdot_offset;

         //datapool_offset=temp_pointer;

         for (l = 0; l < lut_data_entries; l++)//fill data pool entries table in buffer
         {
            dataOffset = GetAt(l, &lut_data);
            cData.nLen = READ_UInt32(tblInfo.dataPoolChnk.pData + dataOffset);
            cData.pData = tblInfo.dataPoolChnk.pData + dataOffset + sizeof(cData.nLen);
            nPaddedBytes = 0;
            if (cData.nLen % 4)
            {
               nPaddedBytes = 4 - cData.nLen % 4;
            }
            Valid_length = cData.nLen + nPaddedBytes;
            ACDB_MEM_CPY(pInput->nBufferPointer + datapool_offset, sizeof(cData.nLen), (void*)&(Valid_length), sizeof(cData.nLen));
            datapool_offset += (int32_t)sizeof(cData.nLen);
            memset((pInput->nBufferPointer + datapool_offset), 0, (size_t)(cData.nLen + nPaddedBytes));
            ACDB_MEM_CPY(pInput->nBufferPointer + datapool_offset, cData.nLen, (void*)(cData.pData), cData.nLen);
            datapool_offset += cData.nLen + nPaddedBytes;
         }
         //fill heap entries.
         if (heapsize > 0)
         {
            for (lut_index = 0; lut_index<(int)lutLength; lut_index++)
            {
               // VocProcDynDataLookupType_offsetData *offsettbleinstance=(VocProcDynDataLookupType_offsetData*)(&lutoffsetTbl.pLut[lut_index]);
               int dataofst = 0;
               //ContentDataOffsetsTblType cdotTbl1;
               int mid = 0, pid = 0, iid = 0;
               isheapentryfound = 0;
               //datatable calculation
               cdotTbl.nLen = READ_UInt32(pInput->nBufferPointer + cdot_start + 4 + GetAt(lut_index, &lut_cdot_new));
               cdotTbl.pDataOffsets = (uint32_t *)(pInput->nBufferPointer + cdot_start + 4 + GetAt(lut_index, &lut_cdot_new) + sizeof(tblInfo.tblCdotChnk.nDataLen));

               cdefTbl.nLen = READ_UInt32(pInput->nBufferPointer + cdft_start + 4 + GetAt(lut_index, &lut_cdft_new));
               cdefTbl.pCntDef = (ContentDefInstanceType *)(pInput->nBufferPointer + cdft_start + 4 + GetAt(lut_index, &lut_cdft_new)/*offsettbleinstance->nCDEFTblOffset*/ + sizeof(tblInfo.tblCdftChnk.nDataLen));
               //reserve space
               if (temp_pointer + (int)cdotTbl.nLen*(int)sizeof(cdotTbl.pDataOffsets) + 4>(datapool_start - (int)sizeof(uint32_t)))//reached max cdot entries
               {
                  continue;
               }
               ACDB_MEM_CPY(pInput->nBufferPointer + temp_pointer, sizeof(cdotTbl.nLen), (void*)(&cdotTbl.nLen), sizeof(cdotTbl.nLen));

               ACDB_MEM_CPY(pInput->nBufferPointer + temp_pointer + 4, cdotTbl.nLen*sizeof(cdotTbl.pDataOffsets), (void*)cdotTbl.pDataOffsets, cdotTbl.nLen*sizeof(cdotTbl.pDataOffsets));

               cdotTbl.nLen = READ_UInt32(pInput->nBufferPointer + temp_pointer);
               cdotTbl.pDataOffsets = (uint32_t *)(pInput->nBufferPointer + temp_pointer + sizeof(tblInfo.tblCdotChnk.nDataLen));

               //fill heap
               for (m = 0; m < (int)cdotTbl.nLen; m++)
               {
                  ACDB_MEM_CPY(&dataofst, sizeof(dataofst), (void*)(&cdotTbl.pDataOffsets[m]), sizeof(dataofst));
                  ACDB_MEM_CPY(&mid, sizeof(dataofst), (void*)(&cdefTbl.pCntDef[m].nMid), sizeof(dataofst));
                  ACDB_MEM_CPY(&iid, sizeof(dataofst), (void*)(&cdefTbl.pCntDef[m].nIid), sizeof(dataofst));
                  ACDB_MEM_CPY(&pid, sizeof(dataofst), (void*)(&cdefTbl.pCntDef[m].nPid), sizeof(dataofst));
                  nLookupKey = (uintptr_t)&lutTblCVD.pLut[lut_index];
                  cdftLookup = (uintptr_t)&cdefTbl.pCntDef[m];
                  result = GetMidIidPidCalibHeapDataEx(tblId, nLookupKey, data_Seclookup, cdftLookup, pid, &cData);//get from heap
                  if (ACDB_SUCCESS == result)
                  {
                     nPaddedBytes = 0;
                     if (cData.nLen % 4)
                     {
                        nPaddedBytes = 4 - cData.nLen % 4;
                     }
                     Valid_length = cData.nLen + nPaddedBytes;
                     cdotTbl.pDataOffsets[m] = datapool_offset - datapool_start;
                     ACDB_MEM_CPY(pInput->nBufferPointer + datapool_offset, sizeof(cData.nLen), (void*)(&Valid_length), sizeof(cData.nLen));
                     datapool_offset += (int32_t)sizeof(cData.nLen);

                     memset((pInput->nBufferPointer + datapool_offset), 0, (size_t)(cData.nLen + nPaddedBytes));

                     ACDB_MEM_CPY(pInput->nBufferPointer + datapool_offset, cData.nLen, (void*)(cData.pData), cData.nLen);
                     datapool_offset += cData.nLen + nPaddedBytes;
                     if (!update((temp_pointer - cdot_start - (int32_t)sizeof(uint32_t)), lut_index, &lut_cdot_new))
                     {
                        result = ACDB_INSUFFICIENTMEMORY;
                        goto end;
                     }
                     isheapentryfound = 1;
                  }
               }
               if (isheapentryfound == 1)
               {
                  temp_pointer += (int32_t)sizeof(cdotTbl.nLen);
                  temp_pointer += (int32_t)(cdotTbl.nLen*sizeof(cdotTbl.pDataOffsets));

                  if (temp_pointer >= (datapool_start - (int)sizeof(uint32_t)))//reached max cdot entries
                     break;
               }
            }
         }

         j = (datapool_offset - temp_pointer - (int32_t)sizeof(uint32_t));
         ACDB_MEM_CPY(pInput->nBufferPointer + temp_pointer, sizeof(size_cdot), (void*)(&j), sizeof(size_cdot));
         nMemBytesLeft -= datapool_offset;
         pOutput->nBytesUsedInBuffer = datapool_offset;
      }
      else
      {
         AcdbSizeResponseType *pOutput = (AcdbSizeResponseType *)pOut;
         LookUptble_Length = (uint32_t)((lut_index_count*sizeof(uint32_t)*(VOCPROCDYN_LUT_INDICES_COUNT - 3)) + sizeof(uint32_t));
         LookUptble_Length += (uint32_t)(3 * sizeof(uint32_t));//major and minor versions
         offset += LookUptble_Length;
         offset += (uint32_t)sizeof(size_cdft);
         offset += size_cdft;
         offset += (uint32_t)sizeof(size_cdot);
         offset += size_cdot;
         offset += (uint32_t)sizeof(uint32_t);//reseve for data size chunk
         offset += size_data;
         offset += heapsize;
         offset += extraheap_cdot;
         pOutput->nSize = offset;
         result = ACDB_SUCCESS;
         goto end;
      }
      for (lut_index = 0; lut_index < lutLength; lut_index++)
      {
         //go the cvd common table to get actual common entry
         VocProcDynDataLookupType_CommonData commonLUTInstance = { 0, 0, 0, 0, 0, 0, 0, 0 };
         ACDB_MEM_CPY(&commonLUTInstance, sizeof(VocProcDynDataLookupType_CommonData), (tblInfo.tblLutCVDChnk.pData + sizeof(uint32_t) + cvdTbloffset + sizeof(VocProcDynDataLookupType_CommonData)*lut_index), sizeof(VocProcDynDataLookupType_CommonData));

         switch (queryType)
         {
         case TABLE_CMD:
         {
            //fill lut entries in CVD format
            AcdbVocProcGainDepVolTblV2CmdType *pInput = (AcdbVocProcGainDepVolTblV2CmdType *)pIn;
            ACDB_MEM_CPY(&temp_lut_node.nNetworkId, sizeof(uint32_t), &commonLUTInstance.nNetwork, sizeof(uint32_t));
            ACDB_MEM_CPY(&temp_lut_node.nRxVocSr, sizeof(uint32_t), &commonLUTInstance.nRxVocSR, sizeof(uint32_t));
            ACDB_MEM_CPY(&temp_lut_node.nTxVocSr, sizeof(uint32_t), &commonLUTInstance.nTxVocSR, sizeof(uint32_t));
            ACDB_MEM_CPY(&temp_lut_node.nVolIdx, sizeof(uint32_t), &commonLUTInstance.nVocVolStep, sizeof(uint32_t));
            ACDB_MEM_CPY(&temp_lut_node.nVocoder_type, sizeof(uint32_t), &commonLUTInstance.nVocoder_type, sizeof(uint32_t));
            ACDB_MEM_CPY(&temp_lut_node.rx_Voc_mode, sizeof(uint32_t), &commonLUTInstance.rx_Voc_mode, sizeof(uint32_t));
            ACDB_MEM_CPY(&temp_lut_node.tx_Voc_mode, sizeof(uint32_t), &commonLUTInstance.tx_Voc_mode, sizeof(uint32_t));
            ACDB_MEM_CPY(&temp_lut_node.nFeature, sizeof(uint32_t), &commonLUTInstance.nFeature, sizeof(uint32_t));

            temp_lut_node.cdft_offset = GetAt(lut_index, &lut_cdft_new);//lutoffsetTbl.pLut[lut_index].nCDEFTblOffset;
            temp_lut_node.cdot_offset = GetAt(lut_index, &lut_cdot_new);
            ACDB_MEM_CPY(pInput->nBufferPointer + offset, sizeof(temp_lut_node.nNetworkId), (void*)(&temp_lut_node.nNetworkId), sizeof(temp_lut_node.nNetworkId));
            offset += (uint32_t)sizeof(temp_lut_node.nNetworkId);
            ACDB_MEM_CPY(pInput->nBufferPointer + offset, sizeof(temp_lut_node.nTxVocSr), (void*)(&temp_lut_node.nTxVocSr), sizeof(temp_lut_node.nTxVocSr));
            offset += (uint32_t)sizeof(temp_lut_node.nTxVocSr);
            ACDB_MEM_CPY(pInput->nBufferPointer + offset, sizeof(temp_lut_node.nRxVocSr), (void*)(&temp_lut_node.nRxVocSr), sizeof(temp_lut_node.nRxVocSr));
            offset += (uint32_t)sizeof(temp_lut_node.nRxVocSr);
            ACDB_MEM_CPY(pInput->nBufferPointer + offset, sizeof(temp_lut_node.nVolIdx), (void*)(&temp_lut_node.nVolIdx), sizeof(temp_lut_node.nVolIdx));
            offset += (uint32_t)sizeof(temp_lut_node.nVolIdx);
            ACDB_MEM_CPY(pInput->nBufferPointer + offset, sizeof(temp_lut_node.tx_Voc_mode), (void*)(&temp_lut_node.tx_Voc_mode), sizeof(temp_lut_node.tx_Voc_mode));
            offset += (uint32_t)sizeof(temp_lut_node.tx_Voc_mode);
            ACDB_MEM_CPY(pInput->nBufferPointer + offset, sizeof(temp_lut_node.rx_Voc_mode), (void*)(&temp_lut_node.rx_Voc_mode), sizeof(temp_lut_node.rx_Voc_mode));
            offset += (uint32_t)sizeof(temp_lut_node.rx_Voc_mode);
            ACDB_MEM_CPY(pInput->nBufferPointer + offset, sizeof(temp_lut_node.nVocoder_type), (void*)(&temp_lut_node.nVocoder_type), sizeof(temp_lut_node.nVocoder_type));
            offset += (uint32_t)sizeof(temp_lut_node.nVocoder_type);
            ACDB_MEM_CPY(pInput->nBufferPointer + offset, sizeof(temp_lut_node.nFeature), (void*)(&temp_lut_node.nFeature), sizeof(temp_lut_node.nFeature));
            offset += (uint32_t)sizeof(temp_lut_node.nFeature);
            ACDB_MEM_CPY(pInput->nBufferPointer + offset, sizeof(temp_lut_node.cdft_offset), (void*)(&temp_lut_node.cdft_offset), sizeof(temp_lut_node.cdft_offset));
            offset += (uint32_t)sizeof(temp_lut_node.cdft_offset);
            ACDB_MEM_CPY(pInput->nBufferPointer + offset, sizeof(temp_lut_node.cdot_offset), (void*)(&temp_lut_node.cdot_offset), sizeof(temp_lut_node.cdot_offset));
            offset += (uint32_t)sizeof(temp_lut_node.cdot_offset);
         }
         break;

         default:
            result = ACDB_ERROR;
            goto end;
         }
      }
      result = ACDB_SUCCESS;
      goto end;
   }
end:
   if (lut_cdft != NULL)
   {
      deleteList(&lut_cdft);
      lut_cdft = NULL;
   }
   if (lut_cdot != NULL)
   {
      deleteList(&lut_cdot);
      lut_cdot = NULL;
   }
   if (lut_cdft_buffer != NULL)
   {
      deleteList(&lut_cdft_buffer);
      lut_cdft_buffer = NULL;
   }
   if (lut_cdot_buffer != NULL)
   {
      deleteList(&lut_cdot_buffer);
      lut_cdot_buffer = NULL;
   }
   if (lut_data_buffer != NULL)
   {
      deleteList(&lut_data_buffer);
      lut_data_buffer = NULL;
   }
   if (lut_data != NULL)
   {
      deleteList(&lut_data);
      lut_data = NULL;
   }
   if (lut_cdot_new != NULL)
   {
      deleteList(&lut_cdot_new);
      lut_cdot_new = NULL;
   }
   if (lut_cdft_new != NULL)
   {
      deleteList(&lut_cdft_new);
      lut_cdft_new = NULL;
   }

   return result;
}

int32_t AcdbCmdGetVocColInfo_v2(uint32_t queryType, uint8_t *pIn, uint8_t *pOut)
{
   int32_t result = ACDB_SUCCESS;

   if (pIn == NULL || pOut == NULL)
   {
      ACDB_DEBUG_LOG("[ACDB Command]->[AcdbCmdGetVocColInfo2]->Invalid NULL value parameters are provided\n");
      result = ACDB_BADPARM;
   }
   else
   {
      AcdbVocColumnsInfoCmdType_v2 *pInput = (AcdbVocColumnsInfoCmdType_v2 *)pIn;
      AcdbQueryResponseType *pOutput = (AcdbQueryResponseType *)pOut;

      uint32_t vocColInfo[] = { 3, //No of columns
         ACDB_CAL_COLUMN_NETWORK, ACDB_CAL_COLUMN_TYPE_32_BIT, ACDB_CAL_NETWORK_ID_DUMMY_VALUE, //col id, colidtype, dummyvalue
         ACDB_CAL_COLUMN_TX_SAMPLING_RATE, ACDB_CAL_COLUMN_TYPE_32_BIT, ACDB_CAL_TX_SAMPLING_ID_DUMMY_VALUE, //col id, colidtype, dummyvalue
         ACDB_CAL_COLUMN_RX_SAMPLING_RATE, ACDB_CAL_COLUMN_TYPE_32_BIT, ACDB_CAL_RX_SAMPLING_ID_DUMMY_VALUE, //col id, colidtype, dummyvalue
      };
      uint32_t vocVolColinfo[] = { 4, //No of columns
         ACDB_CAL_COLUMN_NETWORK, ACDB_CAL_COLUMN_TYPE_32_BIT, ACDB_CAL_NETWORK_ID_DUMMY_VALUE, //col id, colidtype, dummyvalue
         ACDB_CAL_COLUMN_TX_SAMPLING_RATE, ACDB_CAL_COLUMN_TYPE_32_BIT, ACDB_CAL_TX_SAMPLING_ID_DUMMY_VALUE, //col id, colidtype, dummyvalue
         ACDB_CAL_COLUMN_RX_SAMPLING_RATE, ACDB_CAL_COLUMN_TYPE_32_BIT, ACDB_CAL_RX_SAMPLING_ID_DUMMY_VALUE, //col id, colidtype, dummyvalue
         ACDB_CAL_COLUMN_VOLUME_INDEX, ACDB_CAL_COLUMN_TYPE_32_BIT, ACDB_CAL_VOLUME_INDEX_ID_DUMMY_VALUE, //col id, colidtype, dummyvalue
      };
      uint32_t vocStatColInfo[] = { 7, //No of columns
         ACDB_CAL_COLUMN_NETWORK, ACDB_CAL_COLUMN_TYPE_32_BIT, ACDB_CAL_NETWORK_ID_DUMMY_VALUE, //col id, colidtype, dummyvalue
         ACDB_CAL_COLUMN_TX_SAMPLING_RATE, ACDB_CAL_COLUMN_TYPE_32_BIT, ACDB_CAL_TX_SAMPLING_ID_DUMMY_VALUE, //col id, colidtype, dummyvalue
         ACDB_CAL_COLUMN_RX_SAMPLING_RATE, ACDB_CAL_COLUMN_TYPE_32_BIT, ACDB_CAL_RX_SAMPLING_ID_DUMMY_VALUE, //col id, colidtype, dummyvalue

         ACDB_CAL_COLUMN_COLUMN_TX_VOC_OPERATING_MODE, ACDB_CAL_COLUMN_TYPE_32_BIT, ACDB_CAL_COLUMN_COLUMN_TX_VOC_OPERATING_MODE_DUMMY_VALUE, //col id, colidtype, dummyvalue
         ACDB_CAL_COLUMN_COLUMN_RX_VOC_OPERATING_MODE, ACDB_CAL_COLUMN_TYPE_32_BIT, ACDB_CAL_COLUMN_COLUMN_RX_VOC_OPERATING_MODE_DUMMY_VALUE, //col id, colidtype, dummyvalue
         ACDB_CAL_COLUMN_COLUMN_MEDIA_ID, ACDB_CAL_COLUMN_TYPE_32_BIT, ACDB_CAL_COLUMN_COLUMN_MEDIA_ID_DUMMY_VALUE, //col id, colidtype, dummyvalue
         ACDB_CAL_COLUMN_COLUMN_FEATURE, ACDB_CAL_COLUMN_TYPE_32_BIT, ACDB_CAL_COLUMN_COLUMN_FEATURE_DUMMY_VALUE, //col id, colidtype, dummyvalue
      };
      uint32_t vocDynColinfo[] = { 8, //No of columns
         ACDB_CAL_COLUMN_NETWORK, ACDB_CAL_COLUMN_TYPE_32_BIT, ACDB_CAL_NETWORK_ID_DUMMY_VALUE, //col id, colidtype, dummyvalue
         ACDB_CAL_COLUMN_TX_SAMPLING_RATE, ACDB_CAL_COLUMN_TYPE_32_BIT, ACDB_CAL_TX_SAMPLING_ID_DUMMY_VALUE, //col id, colidtype, dummyvalue
         ACDB_CAL_COLUMN_RX_SAMPLING_RATE, ACDB_CAL_COLUMN_TYPE_32_BIT, ACDB_CAL_RX_SAMPLING_ID_DUMMY_VALUE, //col id, colidtype, dummyvalue
         ACDB_CAL_COLUMN_VOLUME_INDEX, ACDB_CAL_COLUMN_TYPE_32_BIT, ACDB_CAL_VOLUME_INDEX_ID_DUMMY_VALUE, //col id, colidtype, dummyvalue

         ACDB_CAL_COLUMN_COLUMN_TX_VOC_OPERATING_MODE, ACDB_CAL_COLUMN_TYPE_32_BIT, ACDB_CAL_COLUMN_COLUMN_TX_VOC_OPERATING_MODE_DUMMY_VALUE, //col id, colidtype, dummyvalue
         ACDB_CAL_COLUMN_COLUMN_RX_VOC_OPERATING_MODE, ACDB_CAL_COLUMN_TYPE_32_BIT, ACDB_CAL_COLUMN_COLUMN_RX_VOC_OPERATING_MODE_DUMMY_VALUE, //col id, colidtype, dummyvalue
         ACDB_CAL_COLUMN_COLUMN_MEDIA_ID, ACDB_CAL_COLUMN_TYPE_32_BIT, ACDB_CAL_COLUMN_COLUMN_MEDIA_ID_DUMMY_VALUE, //col id, colidtype, dummyvalue
         ACDB_CAL_COLUMN_COLUMN_FEATURE, ACDB_CAL_COLUMN_TYPE_32_BIT, ACDB_CAL_COLUMN_COLUMN_FEATURE_DUMMY_VALUE, //col id, colidtype, dummyvalue
      };

      switch (queryType)
      {
      case TABLE_CMD:
      {
         if (pInput->pBuff == NULL)
            return ACDB_BADPARM;

         if (pInput->nTableId == ACDB_VOC_PROC_TABLE)
         {
            if (pInput->nBufferLength < sizeof(vocColInfo))
            {
               ACDB_DEBUG_LOG("Memory not sufficeint to copy the vocproc col info\n");
               return ACDB_INSUFFICIENTMEMORY;
            }
            ACDB_MEM_CPY((uint8_t *)pInput->pBuff, pInput->nBufferLength, (uint8_t *)&vocColInfo[0], sizeof(vocColInfo));
            pOutput->nBytesUsedInBuffer = sizeof(vocColInfo);
         }
         else if (pInput->nTableId == ACDB_VOC_PROC_VOL_TABLE)
         {
            if (pInput->nBufferLength < sizeof(vocVolColinfo))
            {
               ACDB_DEBUG_LOG("Memory not sufficeint to copy the vocprocvol col info\n");
               return ACDB_INSUFFICIENTMEMORY;
            }
            ACDB_MEM_CPY((uint8_t *)pInput->pBuff, pInput->nBufferLength, (uint8_t *)&vocVolColinfo[0], sizeof(vocVolColinfo));
            pOutput->nBytesUsedInBuffer = sizeof(vocVolColinfo);
         }
         else if (pInput->nTableId == ACDB_VOC_STREAM_TABLE)
         {
            if (pInput->nBufferLength < sizeof(vocColInfo))
            {
               ACDB_DEBUG_LOG("Memory not sufficeint to copy the vocstream col info\n");
               return ACDB_INSUFFICIENTMEMORY;
            }
            ACDB_MEM_CPY((uint8_t *)pInput->pBuff, pInput->nBufferLength, (uint8_t *)&vocColInfo[0], sizeof(vocColInfo));
            pOutput->nBytesUsedInBuffer = sizeof(vocColInfo);
         }
         else if (pInput->nTableId == ACDB_VOC_PROC_DYN_TABLE_V2 || pInput->nTableId == ACDB_VOC_PROC_DYN_ISNTANCE_TABLE)
         {
            if (pInput->nBufferLength < sizeof(vocDynColinfo))
            {
               ACDB_DEBUG_LOG("Memory not sufficeint to copy the vocDynColinfo col info\n");
               return ACDB_INSUFFICIENTMEMORY;
            }
            ACDB_MEM_CPY((uint8_t *)pInput->pBuff, pInput->nBufferLength, (uint8_t *)&vocDynColinfo[0], sizeof(vocDynColinfo));
            pOutput->nBytesUsedInBuffer = sizeof(vocDynColinfo);
         }
         else if (pInput->nTableId == ACDB_VOC_PROC_STAT_TABLE_V2 || pInput->nTableId == ACDB_VOC_PROC_STAT_INSTANCE_TABLE)
         {
            if (pInput->nBufferLength < sizeof(vocStatColInfo))
            {
               ACDB_DEBUG_LOG("Memory not sufficeint to copy the vocStatColInfo col info\n");
               return ACDB_INSUFFICIENTMEMORY;
            }
            ACDB_MEM_CPY((uint8_t *)pInput->pBuff, pInput->nBufferLength, (uint8_t *)&vocStatColInfo[0], sizeof(vocStatColInfo));
            pOutput->nBytesUsedInBuffer = sizeof(vocStatColInfo);
         }
         else if (pInput->nTableId == ACDB_VOC_STREAM2_TABLE_V2 || pInput->nTableId == ACDB_VOC_PROC_STREAM_INSTANCE_TABLE)
         {
            if (pInput->nBufferLength < sizeof(vocStatColInfo))
            {
               ACDB_DEBUG_LOG("Memory not sufficeint to copy the vocstream2 col info\n");
               return ACDB_INSUFFICIENTMEMORY;
            }
            ACDB_MEM_CPY((uint8_t *)pInput->pBuff, pInput->nBufferLength, (uint8_t *)&vocStatColInfo[0], sizeof(vocStatColInfo));
            pOutput->nBytesUsedInBuffer = sizeof(vocStatColInfo);
         }
         else
         {
            return ACDB_BADPARM;
         }
      }
      break;
      case TABLE_SIZE_CMD:
      {
         AcdbVocColumnsInfoSizeCmdType_v2 *pInput2 = (AcdbVocColumnsInfoSizeCmdType_v2 *)pIn;
         AcdbSizeResponseType *pOutput2 = (AcdbSizeResponseType *)pOut;
         if (pInput2->nTableId == ACDB_VOC_PROC_TABLE_V2)
         {
            pOutput2->nSize = sizeof(vocColInfo);
         }
         else if (pInput2->nTableId == ACDB_VOC_PROC_VOL_TABLE_V2)
         {
            pOutput2->nSize = sizeof(vocVolColinfo);
         }
         else if (pInput2->nTableId == ACDB_VOC_STREAM_TABLE_V2)
         {
            pOutput2->nSize = sizeof(vocColInfo);
         }
         else if (pInput2->nTableId == ACDB_VOC_PROC_DYN_TABLE_V2 || pInput->nTableId == ACDB_VOC_PROC_DYN_ISNTANCE_TABLE)
         {
            pOutput2->nSize = sizeof(vocDynColinfo);
         }
         else if (pInput2->nTableId == ACDB_VOC_PROC_STAT_TABLE_V2 || pInput->nTableId == ACDB_VOC_PROC_STAT_INSTANCE_TABLE)
         {
            pOutput2->nSize = sizeof(vocStatColInfo);
         }
         else if (pInput2->nTableId == ACDB_VOC_STREAM2_TABLE_V2 || pInput->nTableId == ACDB_VOC_PROC_STREAM_INSTANCE_TABLE)
         {
            pOutput2->nSize = sizeof(vocStatColInfo);
         }
         else
         {
            return ACDB_BADPARM;
         }
      }
      break;
      default:
         return ACDB_BADPARM;
      }
      result = ACDB_SUCCESS;
   }
   return result;
}

int32_t IsInstanceIdSupported(void)
{
   AcdbQueryCmdType *acdbQueryCmdType = (AcdbQueryCmdType *)malloc(sizeof(AcdbQueryCmdType));
   AcdbQueryResponseType *acdbQueryRspnseType = (AcdbQueryResponseType *)malloc(sizeof(AcdbQueryResponseType));
   uint32_t isInstanceIdSupported = 0;
   int32_t result = 0;

   if(acdbQueryCmdType != NULL && acdbQueryRspnseType != NULL)
   {
	   acdbQueryCmdType->pBufferPointer = NULL;
	   acdbQueryCmdType->nBufferLength = sizeof(uint32_t);
	   acdbQueryCmdType->pBufferPointer = (uint8_t *)malloc(sizeof(uint32_t));
	   if(acdbQueryCmdType->pBufferPointer == NULL)
	   {
		   ACDB_DEBUG_LOG("Memory not sufficeint to allocate acdbQueryCmdType->pBufferPointer\n");
           result =  ACDB_INSUFFICIENTMEMORY;
		   if(acdbQueryCmdType != NULL)
		   {
			   ACDB_MEM_FREE(acdbQueryCmdType);
		   }

		   if(acdbQueryRspnseType != NULL)
		   {
			   ACDB_MEM_FREE(acdbQueryRspnseType);
		   }
		   return result;
	   }
	   result = AcdbCmdGetAdspInstanceSupportInfo(acdbQueryCmdType, acdbQueryRspnseType);
	   if (result == 0)
	   {
		  isInstanceIdSupported = READ_UInt32(acdbQueryCmdType->pBufferPointer);
       }
   }
   if(acdbQueryCmdType != NULL)
   {
	   ACDB_MEM_FREE(acdbQueryCmdType);
   }

   if(acdbQueryRspnseType != NULL)
   {
	   ACDB_MEM_FREE(acdbQueryRspnseType);
   }

   return isInstanceIdSupported;
}

int32_t AcdbCmdGetVocStream2Info(uint32_t queryType, uint8_t *pIn, uint8_t *pOut)
{
   int32_t result = ACDB_SUCCESS;
   struct node *lut_cdft = NULL, *lut_cdot = NULL, *lut_cdft_buffer = NULL, *lut_cdot_buffer = NULL, *lut_data_buffer = NULL, *lut_data = NULL, *lut_cdot_new = NULL, *lut_cdft_new = NULL;
   int size_cdft = 0, size_cdot = 0, size_data = 0, lut_cdot_entries = 0, lut_cdft_entries = 0, lut_data_entries = 0;

   if ((pOut == NULL) || (queryType == TABLE_CMD && pIn == NULL))
   {
      ACDB_DEBUG_LOG("[ACDB Command]->[AcdbCmdGetVocStream2Info]->Invalid Null params provided\n");
      result = ACDB_BADPARM;
   }
   else
   {
      uint32_t LookUptble_Length = 0, major = 1, minor = 0;//,lutLength=0;
      int cdft_start = 0, cdot_start = 0, lut_index = 0,  i = 0, j = 0, k = 0, l = 0, m = 0, temp_pointer = 0, isheapentryfound = 0, extraheap_cdot = 0, temp_size = 0;
      int cdft_offset = 0, cdot_offset = 0, datapool_offset = 0, datapool_start = 0;
      int lut_cdftfound = 0, lut_cdotfound = 0, calc_data = 0, heapsize = 0;
      VocProcStatCVDTblHdrFmtType temp_lut_node = { 0, 0, 0, 0, 0, 0, 0, 0, 0};
      AcdbDataType cData = { 0, NULL };
      uint32_t index = 0, Valid_length = 0;
      AcdbTableCmd cmd = { 0, 0 };
      AcdbTableInfo tblInfo = { { 0, NULL }, { 0, NULL }, { 0, NULL } };
	  VocStream2DataLookupTblType lutTbl = { 0, NULL };
      ContentDefTblType cdefTbl = { 0, NULL };
      ContentDataOffsetsTblType cdotTbl = { 0, NULL };
      uint32_t offset = 0;
      uint32_t nMemBytesLeft = 0, nPaddedBytes = 0;// Used for Tbl cmd
      uintptr_t nLookupKey = 0, ncdftLookupKey = 0;
      uint32_t tblId = VOC_STREAM2_TBL;
	  VocStream2DataLookupType tempLut = { 0, 0, 0, 0, 0, 0, 0, 0, 0};

      cmd.devId = 0; //Vocstream2 is not device based so make it zero
      cmd.tblId = tblId;
      result = acdbdata_ioctl(ACDBDATACMD_GET_TABLE_INFO, (uint8_t *)&cmd, sizeof(cmd),
         (uint8_t *)&tblInfo, sizeof(tblInfo));
      if (result != ACDB_SUCCESS)
      {
         if (result == ACDB_DATA_INTERFACE_NOT_FOUND)
         {
            ACDB_DEBUG_LOG("Failed to fetch the Table in ACDB files Table-ID %08X \n"
               , tblId);
            goto end;
         }
         ACDB_DEBUG_LOG("Failed to fetch the lookup information of the table %08X \n"
            , tblId);
         goto end;
      }
      lutTbl.nLen = READ_UInt32(tblInfo.tblLutChnk.pData);
      lutTbl.pLut = (VocStream2DataLookupType *)(tblInfo.tblLutChnk.pData + sizeof(lutTbl.nLen));
      if (lutTbl.nLen <= 0)
      {
         ACDB_DEBUG_LOG("Vocstream2 lookuptable empty\n");
         result = ACDB_ERROR;
         goto end;
      }
      cdefTbl.nLen = READ_UInt32(tblInfo.tblCdftChnk.pData);
      cdefTbl.pCntDef = (ContentDefType *)(tblInfo.tblCdftChnk.pData + sizeof(cdefTbl.nLen));
      if (cdefTbl.nLen <= 0)
      {
         ACDB_DEBUG_LOG("Vocstream2 cdefTbl empty\n");
         result = ACDB_ERROR;
         goto end;
      }
      cdotTbl.nLen = READ_UInt32(tblInfo.tblCdotChnk.pData);
      cdotTbl.pDataOffsets = (uint32_t *)(tblInfo.tblCdotChnk.pData + sizeof(cdotTbl.nLen));
      if (cdotTbl.nLen <= 0)
      {
         ACDB_DEBUG_LOG("Vocstream2 cdotTbl empty\n");
         result = ACDB_ERROR;
         goto end;
      }
      // Compress(index,voccmd,lutTbl,tblInfo,queryType,pIn,pOut,&size_cdft,&size_cdot,&size_data);

      j = 0, k = 0;
      index = 0;
      for (lut_index = (int)index; lut_index < (int)lutTbl.nLen; lut_index++)
      {
         int mid = 0, pid = 0, lut_datafound = 0, dataofst = 0;
		 uint32_t calc_cdft = 0, calc_cdot = 0;
         //datatable calculation
         nLookupKey = (uintptr_t)&lutTbl.pLut[lut_index];

         ACDB_MEM_CPY(&tempLut, sizeof(tempLut), &lutTbl.pLut[lut_index], sizeof(tempLut));

         if (!update(tempLut.nCDOTTblOffset, lut_index, &lut_cdot_new))
         {
            result = ACDB_INSUFFICIENTMEMORY;
            goto end;
         }
         if (!update(tempLut.nCDEFTblOffset, lut_index, &lut_cdft_new))
         {
            result = ACDB_INSUFFICIENTMEMORY;
            goto end;
         }

         ACDB_MEM_CPY(&cdotTbl.nLen, sizeof(uint32_t), (tblInfo.tblCdotChnk.pData + tempLut.nCDOTTblOffset), sizeof(uint32_t));
         cdotTbl.pDataOffsets = (uint32_t *)(tblInfo.tblCdotChnk.pData + tempLut.nCDOTTblOffset + sizeof(tblInfo.tblCdotChnk.nDataLen));

         ACDB_MEM_CPY(&cdefTbl.nLen, sizeof(uint32_t), (tblInfo.tblCdftChnk.pData + tempLut.nCDEFTblOffset), sizeof(uint32_t));

         cdefTbl.pCntDef = (ContentDefType *)(tblInfo.tblCdftChnk.pData + tempLut.nCDEFTblOffset + sizeof(tblInfo.tblCdftChnk.nDataLen));
         //lut_data_buffer[0]=0;
         if (!update(0, 0, &lut_data_buffer))
         {
            result = ACDB_INSUFFICIENTMEMORY;
            goto end;
         }
         isheapentryfound = 0;
         for (m = 0; m < (int)cdotTbl.nLen; m++)
         {
            ACDB_MEM_CPY(&dataofst, sizeof(dataofst), (void*)(&cdotTbl.pDataOffsets[m]), sizeof(dataofst));// get offset table offset from lut table
            ACDB_MEM_CPY(&mid, sizeof(dataofst), (void*)(&cdefTbl.pCntDef[m].nMid), sizeof(dataofst));// get commoncvd table offset from lut table
            ACDB_MEM_CPY(&pid, sizeof(dataofst), (void*)(&cdefTbl.pCntDef[m].nPid), sizeof(dataofst));
            ncdftLookupKey = (uintptr_t)&cdefTbl.pCntDef[m];
            result = GetMidPidCalibHeapData(tblId, nLookupKey, ncdftLookupKey, pid, &cData);//get from heap
            if (result == ACDB_SUCCESS)
            {
               nPaddedBytes = 0;
               if (cData.nLen % 4)
               {
                  nPaddedBytes = 4 - cData.nLen % 4;
               }
               heapsize += cData.nLen + nPaddedBytes;
               heapsize += (int32_t)(sizeof(cData.nLen));
               isheapentryfound = 1;
            }
            lut_datafound = 0;

            if (Find(dataofst, &lut_data, &i))// if data is already present skip it , fill the buffer with only unique list of cdft,cdot,data pool entires.
            {
               lut_datafound = 1;
            }

            if (!lut_datafound)//fill data pool entries
            {
               if (!update(dataofst, l, &lut_data))
               {
                  result = ACDB_INSUFFICIENTMEMORY;
                  goto end;
               }

               {
                  calc_data = *((int *)(tblInfo.dataPoolChnk.pData + dataofst));
                  nPaddedBytes = 0;
                  if (calc_data % 4)
                  {
                     nPaddedBytes = 4 - calc_data % 4;
                  }
                  if (!update((calc_data + nPaddedBytes + (int32_t)sizeof(uint32_t) + GetAt(l, &lut_data_buffer)), l + 1, &lut_data_buffer))
                  {
                     result = ACDB_INSUFFICIENTMEMORY;
                     goto end;
                  }
                  size_data = GetAt(l + 1, &lut_data_buffer);
               }
               l++;
            }
         }
         if (isheapentryfound == 1)
         {
            extraheap_cdot += (int32_t)sizeof(cdotTbl.nLen);
            extraheap_cdot += (int32_t)(sizeof(cdotTbl.pDataOffsets)*cdotTbl.nLen);
         }
         //cdft table parsing
         lut_cdftfound = 0;

         if (!update(0, 0, &lut_cdft_buffer))
         {
            result = ACDB_INSUFFICIENTMEMORY;
            goto end;
         }

         if (Find(tempLut.nCDEFTblOffset, &lut_cdft, &i))
         {
            lut_cdftfound = 1;
         }
         if (!lut_cdftfound)
         {
            if (!update(tempLut.nCDEFTblOffset, j, &lut_cdft))
            {
               result = ACDB_INSUFFICIENTMEMORY;
               goto end;
            }
            ACDB_MEM_CPY(&calc_cdft, sizeof(calc_cdft), (tblInfo.tblCdftChnk.pData + tempLut.nCDEFTblOffset), sizeof(calc_cdft));
            if (!update((int32_t)(calc_cdft*sizeof(uint32_t) * 2 + sizeof(uint32_t) + GetAt(j, &lut_cdft_buffer)), j + 1, &lut_cdft_buffer))
            {
               result = ACDB_INSUFFICIENTMEMORY;
               goto end;
            }
            size_cdft = GetAt(j + 1, &lut_cdft_buffer);

            if (queryType == TABLE_CMD)
               if (!update(GetAt(j, &lut_cdft_buffer), lut_index, &lut_cdft_new))
               {
                  result = ACDB_INSUFFICIENTMEMORY;
                  goto end;
               }
            j++;
         }
         else
            if (queryType == TABLE_CMD)

               if (!update(GetAt(i, &lut_cdft_buffer), lut_index, &lut_cdft_new))
               {
                  result = ACDB_INSUFFICIENTMEMORY;
                  goto end;
               }
         //calculate cdot
         lut_cdotfound = 0;
         if (!update(0, 0, &lut_cdot_buffer))
         {
            result = ACDB_INSUFFICIENTMEMORY;
            goto end;
         }

         if (Find(tempLut.nCDOTTblOffset, &lut_cdot, &i))
         {
            lut_cdotfound = 1;
         }
         if (!lut_cdotfound)
         {
            if (!update(tempLut.nCDOTTblOffset, k, &lut_cdot))
            {
               result = ACDB_INSUFFICIENTMEMORY;
               goto end;
            }
                  {
                     ACDB_MEM_CPY(&calc_cdot, sizeof(calc_cdot), (tblInfo.tblCdotChnk.pData + tempLut.nCDOTTblOffset), sizeof(calc_cdot));
                     if (!update((int32_t)(calc_cdot*sizeof(uint32_t) + sizeof(uint32_t) + GetAt(k, &lut_cdot_buffer)), k + 1, &lut_cdot_buffer))
                     {
                        result = ACDB_INSUFFICIENTMEMORY;
                        goto end;
                     }

                     size_cdot = GetAt(k + 1, &lut_cdot_buffer);
                  }
                  if (queryType == TABLE_CMD)
                     if (!update(GetAt(k, &lut_cdot_buffer), lut_index, &lut_cdot_new))
                     {
                        result = ACDB_INSUFFICIENTMEMORY;
                        goto end;
                     }
                  k++;
         }
         else
            if (queryType == TABLE_CMD)
               if (!update(GetAt(i, &lut_cdot_buffer), lut_index, &lut_cdot_new))
               {
                  result = ACDB_INSUFFICIENTMEMORY;
                  goto end;
               }
      }

      lut_cdft_entries = j; lut_cdot_entries = k; lut_data_entries = l;
      lut_index = 0;
      //add the data

      if (queryType == TABLE_CMD)// write lut size for the first time
      {
         AcdbQueryCmdType *pInput = (AcdbQueryCmdType *)pIn;
         AcdbQueryResponseType *pOutput = (AcdbQueryResponseType *)pOut;
         int dataOffset = 0;
         if (pInput->pBufferPointer == NULL)
         {
            return ACDB_BADPARM;
         }
         nMemBytesLeft = pInput->nBufferLength;
         LookUptble_Length = (uint32_t)((lutTbl.nLen*sizeof(uint32_t)*(VOCSTREAM2_LUT_INDICES_COUNT)) + sizeof(uint32_t));
         if (nMemBytesLeft < LookUptble_Length)
         {
            ACDB_DEBUG_LOG("Insufficient memory to copy the vocproc static table data\n");
            result = ACDB_INSUFFICIENTMEMORY;
            goto end;
         }
         // write major and minor cvd versions.
         ACDB_MEM_CPY(pInput->pBufferPointer + offset, nMemBytesLeft, &major, sizeof(uint32_t));//Major version
         offset += (uint32_t)sizeof(uint32_t);
         ACDB_MEM_CPY(pInput->pBufferPointer + offset, nMemBytesLeft, &minor, sizeof(uint32_t));//minor version
         offset += (uint32_t)sizeof(uint32_t);
         ACDB_MEM_CPY(pInput->pBufferPointer + offset, nMemBytesLeft, &LookUptble_Length, sizeof(uint32_t));//length of the table
         offset += (uint32_t)sizeof(uint32_t);
         ACDB_MEM_CPY(pInput->pBufferPointer + offset, nMemBytesLeft, &lutTbl.nLen, sizeof(uint32_t));//no of entries
         offset += (uint32_t)sizeof(uint32_t);
         LookUptble_Length += (uint32_t)(3 * sizeof(uint32_t));//major and minor versions +table length
         cdft_offset = cdft_start = (int32_t)(LookUptble_Length), cdft_offset = (int32_t)(LookUptble_Length);
         //cdft table
         temp_size = size_cdft;//+ sizeof(temp_size);
         ACDB_MEM_CPY(pInput->pBufferPointer + cdft_start, sizeof(temp_size), (void*)(&temp_size), sizeof(temp_size));
         cdft_offset += (int32_t)sizeof(tblInfo.tblCdftChnk.nDataLen);
         for (k = 0; k < lut_cdft_entries; k++)
         {
            dataOffset = GetAt(k, &lut_cdft);
            cdefTbl.nLen = READ_UInt32(tblInfo.tblCdftChnk.pData + dataOffset);
            cdefTbl.pCntDef = (ContentDefType *)(tblInfo.tblCdftChnk.pData + dataOffset + sizeof(tblInfo.tblCdftChnk.nDataLen));
            ACDB_MEM_CPY(pInput->pBufferPointer + cdft_offset, sizeof(cdefTbl.nLen), (void*)(&cdefTbl.nLen), sizeof(cdefTbl.nLen));
            cdft_offset += (int32_t)sizeof(cdefTbl.nLen);
            dataOffset += (int32_t)sizeof(cdefTbl.nLen);
            for (i = 0; i < (int)cdefTbl.nLen; i++)
            {
               ACDB_MEM_CPY(pInput->pBufferPointer + cdft_offset, sizeof(cdefTbl.pCntDef[i].nMid), (void*)(&cdefTbl.pCntDef[i].nMid), sizeof(cdefTbl.pCntDef[i].nMid));
               cdft_offset += (int32_t)sizeof(cdefTbl.pCntDef[i].nMid);
               dataOffset += (int32_t)sizeof(cdefTbl.pCntDef[i].nMid);
               ACDB_MEM_CPY(pInput->pBufferPointer + cdft_offset, sizeof(cdefTbl.pCntDef[i].nPid), (void*)(&cdefTbl.pCntDef[i].nPid), sizeof(cdefTbl.pCntDef[i].nPid));
               cdft_offset += (int32_t)sizeof(cdefTbl.pCntDef[i].nPid);
               dataOffset += (int32_t)sizeof(cdefTbl.pCntDef[i].nPid);
            }
         }
         temp_size = cdft_offset - cdft_start;//calculate cdft temp size
         cdot_start = cdot_offset = cdft_offset;
         //write cdot also
         dataOffset = 0;
         temp_size = size_cdot + extraheap_cdot;//+ sizeof(temp_size);

         datapool_start = datapool_offset = cdot_offset + size_cdot + (int32_t)sizeof(size_cdot) + (int32_t)sizeof(temp_size) + extraheap_cdot; //reserve the space to write data size.//have the
         ACDB_MEM_CPY(pInput->pBufferPointer + cdot_start, sizeof(size_cdot), (void*)(&temp_size), sizeof(temp_size));
         cdot_offset += (int32_t)sizeof(size_cdot);

         for (k = 0; k < lut_cdot_entries; k++)
         {
            int dataoffset_for_actualbuffer = 0;
            dataOffset = GetAt(k, &lut_cdot);
            cdotTbl.nLen = READ_UInt32(tblInfo.tblCdotChnk.pData + dataOffset);
            cdotTbl.pDataOffsets = (uint32_t *)(tblInfo.tblCdotChnk.pData + dataOffset + sizeof(tblInfo.tblCdotChnk.nDataLen));
            ACDB_MEM_CPY(pInput->pBufferPointer + cdot_offset, sizeof(cdotTbl.nLen), (void*)(&cdotTbl.nLen), sizeof(cdotTbl.nLen));
            cdot_offset += (int32_t)sizeof(cdotTbl.nLen);
            dataOffset += (int32_t)sizeof(cdotTbl.nLen);
            for (i = 0; i < (int)cdotTbl.nLen; i++)
            {
               //calculate actual buffer offset
               for (l = 0; l < lut_data_entries; l++)
               {
                  if (cdotTbl.pDataOffsets[i] == (uint32_t)GetAt(l, &lut_data))
                  {
                     dataoffset_for_actualbuffer = GetAt(l, &lut_data_buffer);
                     break;
                  }
               }
               ACDB_MEM_CPY(pInput->pBufferPointer + cdot_offset, sizeof(dataoffset_for_actualbuffer), (void*)(&dataoffset_for_actualbuffer), sizeof(dataoffset_for_actualbuffer));
               cdot_offset += (int32_t)sizeof(cdotTbl.pDataOffsets[i]);
               dataOffset += (int32_t)sizeof(cdotTbl.pDataOffsets[i]);
            }
         }
         temp_pointer = cdot_offset;

         //datapool_offset=temp_pointer;

         for (l = 0; l < lut_data_entries; l++)//fill data pool entries table in buffer
         {
            dataOffset = GetAt(l, &lut_data);
            cData.nLen = READ_UInt32(tblInfo.dataPoolChnk.pData + dataOffset);
            cData.pData = tblInfo.dataPoolChnk.pData + dataOffset + sizeof(cData.nLen);
            nPaddedBytes = 0;
            if (cData.nLen % 4)
            {
               nPaddedBytes = 4 - cData.nLen % 4;
            }
            Valid_length = cData.nLen + nPaddedBytes;
            ACDB_MEM_CPY(pInput->pBufferPointer + datapool_offset, sizeof(cData.nLen), (void*)&(Valid_length), sizeof(cData.nLen));
            datapool_offset += (int32_t)sizeof(cData.nLen);
            memset((pInput->pBufferPointer + datapool_offset), 0, (size_t)(cData.nLen + nPaddedBytes));
            ACDB_MEM_CPY(pInput->pBufferPointer + datapool_offset, cData.nLen, (void*)(cData.pData), cData.nLen);
            datapool_offset += cData.nLen + nPaddedBytes;
         }
         //fill heap entries.
         if (/*isheapentryfound==1 &&*/ heapsize > 0)
         {
            for (lut_index = 0; lut_index<(int)lutTbl.nLen; lut_index++)
            {
               int dataofst = 0;

               int mid = 0, pid = 0;
               isheapentryfound = 0;
               //datatable calculation
               cdotTbl.nLen = READ_UInt32(pInput->pBufferPointer + cdot_start + 4 + GetAt(lut_index, &lut_cdot_new));
               cdotTbl.pDataOffsets = (uint32_t *)(pInput->pBufferPointer + cdot_start + 4 + GetAt(lut_index, &lut_cdot_new) + sizeof(tblInfo.tblCdotChnk.nDataLen));
               cdefTbl.nLen = READ_UInt32(pInput->pBufferPointer + cdft_start + 4 + GetAt(lut_index, &lut_cdft_new));

               cdefTbl.pCntDef = (ContentDefType *)(pInput->pBufferPointer + cdft_start + 4 + GetAt(lut_index, &lut_cdft_new)/*offsettbleinstance->nCDEFTblOffset*/ + sizeof(tblInfo.tblCdftChnk.nDataLen));
               //reserve space
               if (temp_pointer + (int)cdotTbl.nLen*(int)sizeof(cdotTbl.pDataOffsets) + 4>(datapool_start - (int)sizeof(uint32_t)))//reached max cdot entries
               {
                  continue;
               }
               ACDB_MEM_CPY(pInput->pBufferPointer + temp_pointer, sizeof(cdotTbl.nLen), (void*)(&cdotTbl.nLen), sizeof(cdotTbl.nLen));

               ACDB_MEM_CPY(pInput->pBufferPointer + temp_pointer + 4, cdotTbl.nLen*sizeof(cdotTbl.pDataOffsets), (void*)cdotTbl.pDataOffsets, cdotTbl.nLen*sizeof(cdotTbl.pDataOffsets));

               cdotTbl.nLen = READ_UInt32(pInput->pBufferPointer + temp_pointer);
               cdotTbl.pDataOffsets = (uint32_t *)(pInput->pBufferPointer + temp_pointer + sizeof(tblInfo.tblCdotChnk.nDataLen));

               //lut_data_buffer[0]=0;
               //fill heap
               for (m = 0; m < (int)cdotTbl.nLen; m++)
               {
                  ACDB_MEM_CPY(&dataofst, sizeof(dataofst), (void*)(&cdotTbl.pDataOffsets[m]), sizeof(dataofst));
                  ACDB_MEM_CPY(&mid, sizeof(dataofst), (void*)(&cdefTbl.pCntDef[m].nMid), sizeof(dataofst));
                  ACDB_MEM_CPY(&pid, sizeof(dataofst), (void*)(&cdefTbl.pCntDef[m].nPid), sizeof(dataofst));
                  nLookupKey = (uintptr_t)&lutTbl.pLut[lut_index];
                  ncdftLookupKey = (uintptr_t)&cdefTbl.pCntDef[m];
                  result = GetMidPidCalibHeapData(tblId, nLookupKey, ncdftLookupKey, pid, &cData);//get from heap
                  if (ACDB_SUCCESS == result)

                  {
                     nPaddedBytes = 0;
                     if (cData.nLen % 4)
                     {
                        nPaddedBytes = 4 - cData.nLen % 4;
                     }
                     Valid_length = cData.nLen + nPaddedBytes;
                     cdotTbl.pDataOffsets[m] = datapool_offset - datapool_start;
                     ACDB_MEM_CPY(pInput->pBufferPointer + datapool_offset, sizeof(cData.nLen), (void*)(&Valid_length), sizeof(cData.nLen));
                     datapool_offset += (int32_t)sizeof(cData.nLen);

                     memset((pInput->pBufferPointer + datapool_offset), 0, (size_t)(cData.nLen + nPaddedBytes));

                     ACDB_MEM_CPY(pInput->pBufferPointer + datapool_offset, cData.nLen, (void*)(cData.pData), cData.nLen);
                     datapool_offset += cData.nLen + nPaddedBytes;
                     if (!update((temp_pointer - cdot_start - (int32_t)sizeof(uint32_t)), lut_index, &lut_cdot_new))
                     {
                        result = ACDB_INSUFFICIENTMEMORY;
                        goto end;
                     }
                     isheapentryfound = 1;
                  }
               }
               if (isheapentryfound == 1)
               {
                  temp_pointer += (int32_t)sizeof(cdotTbl.nLen);
                  temp_pointer += (int32_t)(cdotTbl.nLen*sizeof(cdotTbl.pDataOffsets));
                  if (temp_pointer >= (datapool_start - (int)sizeof(uint32_t)))//reached max cdot entries
                     break;
               }
            }
         }

         j = (datapool_offset - temp_pointer - (int32_t)sizeof(uint32_t));
         ACDB_MEM_CPY(pInput->pBufferPointer + temp_pointer, sizeof(size_cdot), (void*)(&j), sizeof(size_cdot));
         nMemBytesLeft -= datapool_offset;
         pOutput->nBytesUsedInBuffer = datapool_offset;
      }
      else
      {
         AcdbSizeResponseType *pOutput = (AcdbSizeResponseType *)pOut;
         LookUptble_Length = (uint32_t)((lutTbl.nLen*sizeof(uint32_t)*(VOCSTREAM2_LUT_INDICES_COUNT)) + sizeof(uint32_t));
         LookUptble_Length += (uint32_t)(3 * sizeof(uint32_t));//major and minor versions
         offset += LookUptble_Length;
         offset += (uint32_t)sizeof(size_cdft);
         offset += size_cdft;
         offset += (uint32_t)sizeof(size_cdot);
         offset += size_cdot;
         offset += (uint32_t)sizeof(uint32_t);//reseve for data size chunk
         offset += size_data;
         offset += heapsize;
         offset += extraheap_cdot;
         pOutput->nSize = offset;
         result = ACDB_SUCCESS;
         goto end;
      }
      do
      {
         switch (queryType)
         {
         case TABLE_CMD:
         {
            //fill lut entries in CVD format
            AcdbQueryCmdType *pInput = (AcdbQueryCmdType *)pIn;
            ACDB_MEM_CPY(&tempLut, sizeof(tempLut), &lutTbl.pLut[index], sizeof(tempLut));

            ACDB_MEM_CPY(&temp_lut_node.nNetworkId, sizeof(uint32_t), &tempLut.nNetwork, sizeof(uint32_t));
            ACDB_MEM_CPY(&temp_lut_node.nRxVocSr, sizeof(uint32_t), &tempLut.nRxVocSR, sizeof(uint32_t));
            ACDB_MEM_CPY(&temp_lut_node.nTxVocSr, sizeof(uint32_t), &tempLut.nTxVocSR, sizeof(uint32_t));
            ACDB_MEM_CPY(&temp_lut_node.nVocoder_type, sizeof(uint32_t), &tempLut.nVocoder_type, sizeof(uint32_t));
            ACDB_MEM_CPY(&temp_lut_node.rx_Voc_mode, sizeof(uint32_t), &tempLut.rx_Voc_mode, sizeof(uint32_t));
            ACDB_MEM_CPY(&temp_lut_node.tx_Voc_mode, sizeof(uint32_t), &tempLut.tx_Voc_mode, sizeof(uint32_t));
            ACDB_MEM_CPY(&temp_lut_node.nFeature, sizeof(uint32_t), &tempLut.nFeature, sizeof(uint32_t));
            temp_lut_node.cdft_offset = GetAt(index, &lut_cdft_new);//lutoffsetTbl.pLut[lut_index].nCDEFTblOffset;
            temp_lut_node.cdot_offset = GetAt(index, &lut_cdot_new);

            ACDB_MEM_CPY(pInput->pBufferPointer + offset, sizeof(temp_lut_node.nNetworkId), (void*)(&temp_lut_node.nNetworkId), sizeof(temp_lut_node.nNetworkId));
            offset += (uint32_t)sizeof(temp_lut_node.nNetworkId);
            ACDB_MEM_CPY(pInput->pBufferPointer + offset, sizeof(temp_lut_node.nTxVocSr), (void*)(&temp_lut_node.nTxVocSr), sizeof(temp_lut_node.nTxVocSr));
            offset += (uint32_t)sizeof(temp_lut_node.nTxVocSr);
            ACDB_MEM_CPY(pInput->pBufferPointer + offset, sizeof(temp_lut_node.nRxVocSr), (void*)(&temp_lut_node.nRxVocSr), sizeof(temp_lut_node.nRxVocSr));
            offset += (uint32_t)sizeof(temp_lut_node.nRxVocSr);
            ACDB_MEM_CPY(pInput->pBufferPointer + offset, sizeof(temp_lut_node.tx_Voc_mode), (void*)(&temp_lut_node.tx_Voc_mode), sizeof(temp_lut_node.tx_Voc_mode));
            offset += (uint32_t)sizeof(temp_lut_node.tx_Voc_mode);
            ACDB_MEM_CPY(pInput->pBufferPointer + offset, sizeof(temp_lut_node.rx_Voc_mode), (void*)(&temp_lut_node.rx_Voc_mode), sizeof(temp_lut_node.rx_Voc_mode));
            offset += (uint32_t)sizeof(temp_lut_node.rx_Voc_mode);
            ACDB_MEM_CPY(pInput->pBufferPointer + offset, sizeof(temp_lut_node.nVocoder_type), (void*)(&temp_lut_node.nVocoder_type), sizeof(temp_lut_node.nVocoder_type));
            offset += (uint32_t)sizeof(temp_lut_node.nVocoder_type);
            ACDB_MEM_CPY(pInput->pBufferPointer + offset, sizeof(temp_lut_node.nFeature), (void*)(&temp_lut_node.nFeature), sizeof(temp_lut_node.nFeature));
            offset += (uint32_t)sizeof(temp_lut_node.nFeature);
            ACDB_MEM_CPY(pInput->pBufferPointer + offset, sizeof(temp_lut_node.cdft_offset), (void*)(&temp_lut_node.cdft_offset), sizeof(temp_lut_node.cdft_offset));
            offset += (uint32_t)sizeof(temp_lut_node.cdft_offset);
            ACDB_MEM_CPY(pInput->pBufferPointer + offset, sizeof(temp_lut_node.cdot_offset), (void*)(&temp_lut_node.cdot_offset), sizeof(temp_lut_node.cdot_offset));
            offset += (uint32_t)sizeof(temp_lut_node.cdot_offset);
         }
         break;
         default:
            result = ACDB_ERROR;
            goto end;
         }

         ++index;
      } while (index < lutTbl.nLen);

      result = ACDB_SUCCESS;
      goto end;
   }
end:
   if (lut_cdft != NULL)
   {
      deleteList(&lut_cdft);
      lut_cdft = NULL;
   }
   if (lut_cdot != NULL)
   {
      deleteList(&lut_cdot);
      lut_cdot = NULL;
   }
   if (lut_cdft_buffer != NULL)
   {
      deleteList(&lut_cdft_buffer);
      lut_cdft_buffer = NULL;
   }
   if (lut_cdot_buffer != NULL)
   {
      deleteList(&lut_cdot_buffer);
      lut_cdot_buffer = NULL;
   }
   if (lut_data_buffer != NULL)
   {
      deleteList(&lut_data_buffer);
      lut_data_buffer = NULL;
   }
   if (lut_data != NULL)
   {
      deleteList(&lut_data);
      lut_data = NULL;
   }
   if (lut_cdot_new != NULL)
   {
      deleteList(&lut_cdot_new);
      lut_cdot_new = NULL;
   }
   if (lut_cdft_new != NULL)
   {
      deleteList(&lut_cdft_new);
      lut_cdft_new = NULL;
   }

   return result;
}

int32_t AcdbCmdGetVocStreamWithInstanceInfo(uint32_t queryType, uint8_t *pIn, uint8_t *pOut)
{
   int32_t result = ACDB_SUCCESS;
   struct node *lut_cdft = NULL, *lut_cdot = NULL, *lut_cdft_buffer = NULL, *lut_cdot_buffer = NULL, *lut_data_buffer = NULL, *lut_data = NULL, *lut_cdot_new = NULL, *lut_cdft_new = NULL;
   int size_cdft = 0, size_cdot = 0, size_data = 0, lut_cdot_entries = 0, lut_cdft_entries = 0, lut_data_entries = 0;

   if ((pOut == NULL) || (queryType == TABLE_CMD && pIn == NULL))
   {
      ACDB_DEBUG_LOG("[ACDB Command]->[AcdbCmdGetVocStreamWithInstanceInfo]->Invalid Null params provided\n");
      result = ACDB_BADPARM;
   }
   else
   {
      uint32_t LookUptble_Length = 0,  major = 1, minor = 1;//,lutLength=0;
      int cdft_start = 0, cdot_start = 0, i = 0, j = 0, k = 0, l = 0, m = 0, temp_pointer = 0, isheapentryfound = 0, extraheap_cdot = 0, temp_size = 0, lut_index = 0;
      int cdft_offset = 0, cdot_offset = 0, datapool_offset = 0, datapool_start = 0;
      int lut_cdftfound = 0, lut_cdotfound = 0, calc_data = 0, heapsize = 0;
      VocProcStatCVDTblHdrFmtType temp_lut_node = { 0, 0, 0, 0, 0, 0, 0, 0, 0};
      AcdbDataType cData = { 0, NULL };
      uint32_t index = 0, Valid_length = 0;
      AcdbTableCmd cmd = { 0, 0 };
      AcdbTableInfo tblInfo = { { 0, NULL }, { 0, NULL }, { 0, NULL } };
      VocStream2DataLookupTblType lutTbl = { 0, NULL };
      ContentDefWithInstanceTblType cdefTbl = { 0, NULL };
      ContentDataOffsetsTblType cdotTbl = { 0, NULL };
      uint32_t offset = 0;
      uint32_t nMemBytesLeft = 0, nPaddedBytes = 0;// Used for Tbl cmd
      uintptr_t nLookupKey = 0, ncdftLookupKey = 0;
      uint32_t tblId = VOCSTRM_INST_TBL_ID;
      VocStream2DataLookupType tempLut = { 0, 0, 0, 0, 0, 0, 0, 0, 0};

      cmd.devId = 0; //Vocstream2 is not device based so make it zero
      cmd.tblId = tblId;
      result = acdbdata_ioctl(ACDBDATACMD_GET_TABLE_INFO, (uint8_t *)&cmd, sizeof(cmd),
         (uint8_t *)&tblInfo, sizeof(tblInfo));
      if (result != ACDB_SUCCESS)
      {
         if (result == ACDB_DATA_INTERFACE_NOT_FOUND)
         {
            ACDB_DEBUG_LOG("Failed to fetch the Table in ACDB files Table-ID %08X \n"
               , tblId);
            goto end;
         }
         ACDB_DEBUG_LOG("Failed to fetch the lookup information of the table %08X \n"
            , tblId);
         goto end;
      }
      lutTbl.nLen = READ_UInt32(tblInfo.tblLutChnk.pData);
      lutTbl.pLut = (VocStream2DataLookupType *)(tblInfo.tblLutChnk.pData + sizeof(lutTbl.nLen));
      if (lutTbl.nLen <= 0)
      {
         ACDB_DEBUG_LOG("Vocstream instance lookuptable empty\n");
         result = ACDB_ERROR;
         goto end;
      }
      cdefTbl.nLen = READ_UInt32(tblInfo.tblCdftChnk.pData);
      cdefTbl.pCntDef = (ContentDefInstanceType *)(tblInfo.tblCdftChnk.pData + sizeof(cdefTbl.nLen));
      if (cdefTbl.nLen <= 0)
      {
         ACDB_DEBUG_LOG("Vocstream instance cdefTbl empty\n");
         result = ACDB_ERROR;
         goto end;
      }
      cdotTbl.nLen = READ_UInt32(tblInfo.tblCdotChnk.pData);
      cdotTbl.pDataOffsets = (uint32_t *)(tblInfo.tblCdotChnk.pData + sizeof(cdotTbl.nLen));
      if (cdotTbl.nLen <= 0)
      {
         ACDB_DEBUG_LOG("Vocstream instance cdotTbl empty\n");
         result = ACDB_ERROR;
         goto end;
      }
      // Compress(index,voccmd,lutTbl,tblInfo,queryType,pIn,pOut,&size_cdft,&size_cdot,&size_data);

      j = 0, k = 0;
      index = 0;
      for (lut_index = (int)index; lut_index < (int)lutTbl.nLen; lut_index++)
      {
         int mid = 0, pid = 0, iid = 0, lut_datafound = 0, dataofst = 0;
         uint32_t calc_cdft = 0, calc_cdot = 0;
         //datatable calculation
         nLookupKey = (uintptr_t)&lutTbl.pLut[lut_index];

         ACDB_MEM_CPY(&tempLut, sizeof(tempLut), &lutTbl.pLut[lut_index], sizeof(tempLut));

         if (!update(tempLut.nCDOTTblOffset, lut_index, &lut_cdot_new))
         {
            result = ACDB_INSUFFICIENTMEMORY;
            goto end;
         }
         if (!update(tempLut.nCDEFTblOffset, lut_index, &lut_cdft_new))
         {
            result = ACDB_INSUFFICIENTMEMORY;
            goto end;
         }

         ACDB_MEM_CPY(&cdotTbl.nLen, sizeof(uint32_t), (tblInfo.tblCdotChnk.pData + tempLut.nCDOTTblOffset), sizeof(uint32_t));
         cdotTbl.pDataOffsets = (uint32_t *)(tblInfo.tblCdotChnk.pData + tempLut.nCDOTTblOffset + sizeof(tblInfo.tblCdotChnk.nDataLen));

         ACDB_MEM_CPY(&cdefTbl.nLen, sizeof(uint32_t), (tblInfo.tblCdftChnk.pData + tempLut.nCDEFTblOffset), sizeof(uint32_t));

         cdefTbl.pCntDef = (ContentDefInstanceType *)(tblInfo.tblCdftChnk.pData + tempLut.nCDEFTblOffset + sizeof(tblInfo.tblCdftChnk.nDataLen));
         //lut_data_buffer[0]=0;
         if (!update(0, 0, &lut_data_buffer))
         {
            result = ACDB_INSUFFICIENTMEMORY;
            goto end;
         }
         isheapentryfound = 0;
         for (m = 0; m < (int)cdotTbl.nLen; m++)
         {
            ACDB_MEM_CPY(&dataofst, sizeof(dataofst), (void*)(&cdotTbl.pDataOffsets[m]), sizeof(dataofst));// get offset table offset from lut table
            ACDB_MEM_CPY(&mid, sizeof(dataofst), (void*)(&cdefTbl.pCntDef[m].nMid), sizeof(dataofst));// get commoncvd table offset from lut table
            ACDB_MEM_CPY(&iid, sizeof(dataofst), (void*)(&cdefTbl.pCntDef[m].nIid), sizeof(dataofst));
            ACDB_MEM_CPY(&pid, sizeof(dataofst), (void*)(&cdefTbl.pCntDef[m].nPid), sizeof(dataofst));
            ncdftLookupKey = (uintptr_t)&cdefTbl.pCntDef[m];
            result = GetMidIidPidCalibHeapData(tblId, nLookupKey, ncdftLookupKey, pid, &cData);//get from heap
            if (result == ACDB_SUCCESS)
            {
               nPaddedBytes = 0;
               if (cData.nLen % 4)
               {
                  nPaddedBytes = 4 - cData.nLen % 4;
               }
               heapsize += cData.nLen + nPaddedBytes;
               heapsize += (int32_t)sizeof(cData.nLen);
               isheapentryfound = 1;
            }
            lut_datafound = 0;

            if (Find(dataofst, &lut_data, &i))// if data is already present skip it , fill the buffer with only unique list of cdft,cdot,data pool entires.
            {
               lut_datafound = 1;
            }

            if (!lut_datafound)//fill data pool entries
            {
               if (!update(dataofst, l, &lut_data))
               {
                  result = ACDB_INSUFFICIENTMEMORY;
                  goto end;
               }

               {
                  calc_data = *((int *)(tblInfo.dataPoolChnk.pData + dataofst));
                  nPaddedBytes = 0;
                  if (calc_data % 4)
                  {
                     nPaddedBytes = 4 - calc_data % 4;
                  }
                  if (!update(calc_data + nPaddedBytes + (int32_t)sizeof(uint32_t) + GetAt(l, &lut_data_buffer), l + 1, &lut_data_buffer))
                  {
                     result = ACDB_INSUFFICIENTMEMORY;
                     goto end;
                  }
                  size_data = GetAt(l + 1, &lut_data_buffer);
               }
               l++;
            }
         }
         if (isheapentryfound == 1)
         {
            extraheap_cdot += (int32_t)sizeof(cdotTbl.nLen);
            extraheap_cdot += (int32_t)(sizeof(cdotTbl.pDataOffsets)*cdotTbl.nLen);
         }
         //cdft table parsing
         lut_cdftfound = 0;

         if (!update(0, 0, &lut_cdft_buffer))
         {
            result = ACDB_INSUFFICIENTMEMORY;
            goto end;
         }

         if (Find(tempLut.nCDEFTblOffset, &lut_cdft, &i))
         {
            lut_cdftfound = 1;
         }
         if (!lut_cdftfound)
         {
            if (!update(tempLut.nCDEFTblOffset, j, &lut_cdft))
            {
               result = ACDB_INSUFFICIENTMEMORY;
               goto end;
            }
            ACDB_MEM_CPY(&calc_cdft, sizeof(calc_cdft), (tblInfo.tblCdftChnk.pData + tempLut.nCDEFTblOffset), sizeof(uint32_t));
            if (!update((int32_t)(calc_cdft*sizeof(uint32_t) * 3 + sizeof(uint32_t) + GetAt(j, &lut_cdft_buffer)), j + 1, &lut_cdft_buffer))
            {
               result = ACDB_INSUFFICIENTMEMORY;
               goto end;
            }
            size_cdft = GetAt(j + 1, &lut_cdft_buffer);

            if (queryType == TABLE_CMD)
               if (!update(GetAt(j, &lut_cdft_buffer), lut_index, &lut_cdft_new))
               {
                  result = ACDB_INSUFFICIENTMEMORY;
                  goto end;
               }
            j++;
         }
         else
            if (queryType == TABLE_CMD)

               if (!update(GetAt(i, &lut_cdft_buffer), lut_index, &lut_cdft_new))
               {
                  result = ACDB_INSUFFICIENTMEMORY;
                  goto end;
               }
         //calculate cdot
         lut_cdotfound = 0;
         if (!update(0, 0, &lut_cdot_buffer))
         {
            result = ACDB_INSUFFICIENTMEMORY;
            goto end;
         }

         if (Find(tempLut.nCDOTTblOffset, &lut_cdot, &i))
         {
            lut_cdotfound = 1;
         }
         if (!lut_cdotfound)
         {
            if (!update(tempLut.nCDOTTblOffset, k, &lut_cdot))
            {
               result = ACDB_INSUFFICIENTMEMORY;
               goto end;
            }
                  {
                     ACDB_MEM_CPY(&calc_cdot, sizeof(calc_cdot), (tblInfo.tblCdotChnk.pData + tempLut.nCDOTTblOffset), sizeof(uint32_t));
                     if (!update((int32_t)(calc_cdot*sizeof(uint32_t) + sizeof(uint32_t) + GetAt(k, &lut_cdot_buffer)), k + 1, &lut_cdot_buffer))
                     {
                        result = ACDB_INSUFFICIENTMEMORY;
                        goto end;
                     }

                     size_cdot = GetAt(k + 1, &lut_cdot_buffer);
                  }
                  if (queryType == TABLE_CMD)
                     if (!update(GetAt(k, &lut_cdot_buffer), lut_index, &lut_cdot_new))
                     {
                        result = ACDB_INSUFFICIENTMEMORY;
                        goto end;
                     }
                  k++;
         }
         else
            if (queryType == TABLE_CMD)
               if (!update(GetAt(i, &lut_cdot_buffer), lut_index, &lut_cdot_new))
               {
                  result = ACDB_INSUFFICIENTMEMORY;
                  goto end;
               }
      }

      lut_cdft_entries = j; lut_cdot_entries = k; lut_data_entries = l;
      lut_index = 0;
      //add the data

      if (queryType == TABLE_CMD)// write lut size for the first time
      {
         AcdbQueryCmdType *pInput = (AcdbQueryCmdType *)pIn;
         AcdbQueryResponseType *pOutput = (AcdbQueryResponseType *)pOut;
         int dataOffset = 0;
         if (pInput->pBufferPointer == NULL)
         {
            result =  ACDB_BADPARM;
			goto end;
         }
         nMemBytesLeft = pInput->nBufferLength;
         LookUptble_Length = (uint32_t)((lutTbl.nLen*sizeof(uint32_t)*(VOCSTREAM2_LUT_INDICES_COUNT)) + sizeof(uint32_t));
         if (nMemBytesLeft < (uint32_t)LookUptble_Length)
         {
            ACDB_DEBUG_LOG("Insufficient memory to copy the vocproc static table data\n");
            result = ACDB_INSUFFICIENTMEMORY;
            goto end;
         }
         // write major and minor cvd versions.
         ACDB_MEM_CPY(pInput->pBufferPointer + offset, nMemBytesLeft, &major, sizeof(uint32_t));//Major version
         offset += (uint32_t)sizeof(uint32_t);
         ACDB_MEM_CPY(pInput->pBufferPointer + offset, nMemBytesLeft, &minor, sizeof(uint32_t));//minor version
         offset += (uint32_t)sizeof(uint32_t);
         ACDB_MEM_CPY(pInput->pBufferPointer + offset, nMemBytesLeft, &LookUptble_Length, sizeof(uint32_t));//length of the table
         offset += (uint32_t)sizeof(uint32_t);
         ACDB_MEM_CPY(pInput->pBufferPointer + offset, nMemBytesLeft, &lutTbl.nLen, sizeof(uint32_t));//no of entries
         offset += (uint32_t)sizeof(uint32_t);
         LookUptble_Length += (uint32_t)(3 * sizeof(uint32_t));//major and minor versions +table length
         cdft_offset = cdft_start = LookUptble_Length, cdft_offset = LookUptble_Length;
         //cdft table
         temp_size = size_cdft;//+ sizeof(temp_size);
         ACDB_MEM_CPY(pInput->pBufferPointer + cdft_start, sizeof(int32_t), (void*)(&temp_size), sizeof(temp_size));
         cdft_offset += (uint32_t)sizeof(tblInfo.tblCdftChnk.nDataLen);
         for (k = 0; k < lut_cdft_entries; k++)
         {
            dataOffset = GetAt(k, &lut_cdft);
            cdefTbl.nLen = READ_UInt32(tblInfo.tblCdftChnk.pData + dataOffset);
            cdefTbl.pCntDef = (ContentDefInstanceType *)(tblInfo.tblCdftChnk.pData + dataOffset + sizeof(tblInfo.tblCdftChnk.nDataLen));
            ACDB_MEM_CPY(pInput->pBufferPointer + cdft_offset, sizeof(cdefTbl.nLen), (void*)(&cdefTbl.nLen), sizeof(cdefTbl.nLen));
            cdft_offset += (int32_t)sizeof(cdefTbl.nLen);
            dataOffset += (int32_t)sizeof(cdefTbl.nLen);
            for (i = 0; i < (int)cdefTbl.nLen; i++)
            {
               uint16_t iid = 0;
               uint16_t reserved = 0;
               ACDB_MEM_CPY(pInput->pBufferPointer + cdft_offset, sizeof(cdefTbl.pCntDef[i].nMid), (void*)(&cdefTbl.pCntDef[i].nMid), sizeof(cdefTbl.pCntDef[i].nMid));
               cdft_offset += (int32_t)sizeof(cdefTbl.pCntDef[i].nMid);
               dataOffset += (int32_t)sizeof(cdefTbl.pCntDef[i].nMid);

               iid = GetUint16IID(cdefTbl.pCntDef[i].nIid);
               ACDB_MEM_CPY(pInput->pBufferPointer + cdft_offset, sizeof(iid), (void*)(&iid), sizeof(iid));
               cdft_offset += (int32_t)sizeof(iid);
               dataOffset += (int32_t)sizeof(iid);

               ACDB_MEM_CPY(pInput->pBufferPointer + cdft_offset, sizeof(reserved), (void*)(&reserved), sizeof(reserved));
               cdft_offset += (int32_t)sizeof(reserved);
               dataOffset += (int32_t)sizeof(reserved);
               ACDB_MEM_CPY(pInput->pBufferPointer + cdft_offset, sizeof(cdefTbl.pCntDef[i].nPid), (void*)(&cdefTbl.pCntDef[i].nPid), sizeof(cdefTbl.pCntDef[i].nPid));
               cdft_offset += (int32_t)sizeof(cdefTbl.pCntDef[i].nPid);
               dataOffset += (int32_t)sizeof(cdefTbl.pCntDef[i].nPid);
            }
         }
         temp_size = cdft_offset - cdft_start;//calculate cdft temp size
         cdot_start = cdot_offset = cdft_offset;
         //write cdot also
         dataOffset = 0;
         temp_size = size_cdot + extraheap_cdot;//+ sizeof(temp_size);

         datapool_start = datapool_offset = cdot_offset + size_cdot + (int32_t)sizeof(size_cdot) + (int32_t)sizeof(temp_size) + extraheap_cdot; //reserve the space to write data size.//have the
         ACDB_MEM_CPY(pInput->pBufferPointer + cdot_start, sizeof(size_cdot), (void*)(&temp_size), sizeof(temp_size));
         cdot_offset += (int32_t)sizeof(size_cdot);

         for (k = 0; k < lut_cdot_entries; k++)
         {
            int dataoffset_for_actualbuffer = 0;
            dataOffset = GetAt(k, &lut_cdot);
            cdotTbl.nLen = READ_UInt32(tblInfo.tblCdotChnk.pData + dataOffset);
            cdotTbl.pDataOffsets = (uint32_t *)(tblInfo.tblCdotChnk.pData + dataOffset + sizeof(tblInfo.tblCdotChnk.nDataLen));
            ACDB_MEM_CPY(pInput->pBufferPointer + cdot_offset, sizeof(cdotTbl.nLen), (void*)(&cdotTbl.nLen), sizeof(cdotTbl.nLen));
            cdot_offset += (int32_t)sizeof(cdotTbl.nLen);
            dataOffset += (int32_t)sizeof(cdotTbl.nLen);
            for (i = 0; i < (int)cdotTbl.nLen; i++)
            {
               //calculate actual buffer offset
               for (l = 0; l < lut_data_entries; l++)
               {
                  if (cdotTbl.pDataOffsets[i] == (uint32_t)GetAt(l, &lut_data))
                  {
                     dataoffset_for_actualbuffer = GetAt(l, &lut_data_buffer);
                     break;
                  }
               }
               ACDB_MEM_CPY(pInput->pBufferPointer + cdot_offset, sizeof(dataoffset_for_actualbuffer), (void*)(&dataoffset_for_actualbuffer), sizeof(dataoffset_for_actualbuffer));
               cdot_offset += (int32_t)sizeof(cdotTbl.pDataOffsets[i]);
               dataOffset += (int32_t)sizeof(cdotTbl.pDataOffsets[i]);
            }
         }
         temp_pointer = cdot_offset;

         //datapool_offset=temp_pointer;

         for (l = 0; l < lut_data_entries; l++)//fill data pool entries table in buffer
         {
            dataOffset = GetAt(l, &lut_data);
            cData.nLen = READ_UInt32(tblInfo.dataPoolChnk.pData + dataOffset);
            cData.pData = tblInfo.dataPoolChnk.pData + dataOffset + sizeof(cData.nLen);
            nPaddedBytes = 0;
            if (cData.nLen % 4)
            {
               nPaddedBytes = 4 - cData.nLen % 4;
            }
            Valid_length = cData.nLen + nPaddedBytes;
            ACDB_MEM_CPY(pInput->pBufferPointer + datapool_offset, sizeof(cData.nLen), (void*)&(Valid_length), sizeof(cData.nLen));
            datapool_offset += (int32_t)sizeof(cData.nLen);
            memset((pInput->pBufferPointer + datapool_offset), 0, (size_t)(cData.nLen + nPaddedBytes));
            ACDB_MEM_CPY(pInput->pBufferPointer + datapool_offset, cData.nLen, cData.pData, cData.nLen);
            datapool_offset += cData.nLen + nPaddedBytes;
         }
         //fill heap entries.
         if (/*isheapentryfound==1 &&*/ heapsize > 0)
         {
            for (lut_index = 0; lut_index<(int)lutTbl.nLen; lut_index++)
            {
               int dataofst = 0;

               int mid = 0, pid = 0;
               isheapentryfound = 0;
               //datatable calculation
               cdotTbl.nLen = READ_UInt32(pInput->pBufferPointer + cdot_start + 4 + GetAt(lut_index, &lut_cdot_new));
               cdotTbl.pDataOffsets = (uint32_t *)(pInput->pBufferPointer + cdot_start + 4 + GetAt(lut_index, &lut_cdot_new) + sizeof(tblInfo.tblCdotChnk.nDataLen));
               cdefTbl.nLen = READ_UInt32(pInput->pBufferPointer + cdft_start + 4 + GetAt(lut_index, &lut_cdft_new));

               cdefTbl.pCntDef = (ContentDefInstanceType *)(pInput->pBufferPointer + cdft_start + 4 + GetAt(lut_index, &lut_cdft_new)/*offsettbleinstance->nCDEFTblOffset*/ + sizeof(tblInfo.tblCdftChnk.nDataLen));
               //reserve space
               if (temp_pointer + (int)cdotTbl.nLen*(int)sizeof(cdotTbl.pDataOffsets) + 4>(datapool_start - (int)sizeof(uint32_t)))//reached max cdot entries
               {
                  continue;
               }
               ACDB_MEM_CPY(pInput->pBufferPointer + temp_pointer, sizeof(cdotTbl.nLen), (void*)(&cdotTbl.nLen), sizeof(cdotTbl.nLen));

               ACDB_MEM_CPY(pInput->pBufferPointer + temp_pointer + 4, cdotTbl.nLen*sizeof(cdotTbl.pDataOffsets), (void*)cdotTbl.pDataOffsets, cdotTbl.nLen*sizeof(cdotTbl.pDataOffsets));

               cdotTbl.nLen = READ_UInt32(pInput->pBufferPointer + temp_pointer);
               cdotTbl.pDataOffsets = (uint32_t *)(pInput->pBufferPointer + temp_pointer + sizeof(tblInfo.tblCdotChnk.nDataLen));

               //lut_data_buffer[0]=0;
               //fill heap
               for (m = 0; m < (int)cdotTbl.nLen; m++)
               {
                  ACDB_MEM_CPY(&dataofst, sizeof(dataofst), (void*)(&cdotTbl.pDataOffsets[m]), sizeof(dataofst));
                  ACDB_MEM_CPY(&mid, sizeof(dataofst), (void*)(&cdefTbl.pCntDef[m].nMid), sizeof(dataofst));
                  ACDB_MEM_CPY(&pid, sizeof(dataofst), (void*)(&cdefTbl.pCntDef[m].nPid), sizeof(dataofst));
                  nLookupKey = (uintptr_t)&lutTbl.pLut[lut_index];
                  ncdftLookupKey = (uintptr_t)&cdefTbl.pCntDef[m];
                  result = GetMidPidCalibHeapData(tblId, nLookupKey, ncdftLookupKey, pid, &cData);//get from heap
                  if (ACDB_SUCCESS == result)

                  {
                     nPaddedBytes = 0;
                     if (cData.nLen % 4)
                     {
                        nPaddedBytes = 4 - cData.nLen % 4;
                     }
                     Valid_length = cData.nLen + nPaddedBytes;
                     cdotTbl.pDataOffsets[m] = datapool_offset - datapool_start;
                     ACDB_MEM_CPY(pInput->pBufferPointer + datapool_offset, sizeof(cData.nLen), (void*)(&Valid_length), sizeof(cData.nLen));
                     datapool_offset += (int32_t)sizeof(cData.nLen);

                     memset(pInput->pBufferPointer + datapool_offset, 0, (size_t)(cData.nLen + nPaddedBytes));

                     ACDB_MEM_CPY(pInput->pBufferPointer + datapool_offset, cData.nLen, (void*)(cData.pData), cData.nLen);
                     datapool_offset += cData.nLen + nPaddedBytes;
                     if (!update((temp_pointer - cdot_start - (int32_t)sizeof(uint32_t)), lut_index, &lut_cdot_new))
                     {
                        result = ACDB_INSUFFICIENTMEMORY;
                        goto end;
                     }
                     isheapentryfound = 1;
                  }
               }
               if (isheapentryfound == 1)
               {
                  temp_pointer += (int32_t)sizeof(cdotTbl.nLen);
                  temp_pointer += (int32_t)(cdotTbl.nLen*sizeof(cdotTbl.pDataOffsets));
                  if (temp_pointer >= (datapool_start - (int)sizeof(uint32_t)))//reached max cdot entries
                     break;
               }
            }
         }

         j = (datapool_offset - temp_pointer - (int32_t)sizeof(uint32_t));
         ACDB_MEM_CPY(pInput->pBufferPointer + temp_pointer, sizeof(size_cdot), (void*)(&j), sizeof(size_cdot));
         nMemBytesLeft -= datapool_offset;
         pOutput->nBytesUsedInBuffer = datapool_offset;
      }
      else
      {
         AcdbSizeResponseType *pOutput = (AcdbSizeResponseType *)pOut;
         LookUptble_Length = (uint32_t)((lutTbl.nLen*sizeof(uint32_t)*(VOCSTREAM2_LUT_INDICES_COUNT)) + sizeof(uint32_t));
         LookUptble_Length += (uint32_t)(3 * sizeof(uint32_t));//major and minor versions
         offset += LookUptble_Length;
         offset += (uint32_t)sizeof(size_cdft);
         offset += size_cdft;
         offset += (uint32_t)sizeof(size_cdot);
         offset += size_cdot;
         offset += (uint32_t)sizeof(uint32_t);//reseve for data size chunk
         offset += size_data;
         offset += heapsize;
         offset += extraheap_cdot;
         pOutput->nSize = offset;
         result = ACDB_SUCCESS;
         goto end;
      }
      do
      {
         switch (queryType)
         {
         case TABLE_CMD:
         {
            //fill lut entries in CVD format
            AcdbQueryCmdType *pInput = (AcdbQueryCmdType *)pIn;
            ACDB_MEM_CPY(&tempLut, sizeof(tempLut), &lutTbl.pLut[index], sizeof(tempLut));

            ACDB_MEM_CPY(&temp_lut_node.nNetworkId, sizeof(uint32_t), &tempLut.nNetwork, sizeof(uint32_t));
            ACDB_MEM_CPY(&temp_lut_node.nRxVocSr, sizeof(uint32_t), &tempLut.nRxVocSR, sizeof(uint32_t));
            ACDB_MEM_CPY(&temp_lut_node.nTxVocSr, sizeof(uint32_t), &tempLut.nTxVocSR, sizeof(uint32_t));
            ACDB_MEM_CPY(&temp_lut_node.nVocoder_type, sizeof(uint32_t), &tempLut.nVocoder_type, sizeof(uint32_t));
            ACDB_MEM_CPY(&temp_lut_node.rx_Voc_mode, sizeof(uint32_t), &tempLut.rx_Voc_mode, sizeof(uint32_t));
            ACDB_MEM_CPY(&temp_lut_node.tx_Voc_mode, sizeof(uint32_t), &tempLut.tx_Voc_mode, sizeof(uint32_t));
            ACDB_MEM_CPY(&temp_lut_node.nFeature, sizeof(uint32_t), &tempLut.nFeature, sizeof(uint32_t));
            temp_lut_node.cdft_offset = GetAt(index, &lut_cdft_new);//lutoffsetTbl.pLut[lut_index].nCDEFTblOffset;
            temp_lut_node.cdot_offset = GetAt(index, &lut_cdot_new);

            ACDB_MEM_CPY(pInput->pBufferPointer + offset, sizeof(temp_lut_node.nNetworkId), (void*)(&temp_lut_node.nNetworkId), sizeof(temp_lut_node.nNetworkId));
            offset += (uint32_t)sizeof(temp_lut_node.nNetworkId);
            ACDB_MEM_CPY(pInput->pBufferPointer + offset, sizeof(temp_lut_node.nTxVocSr), (void*)(&temp_lut_node.nTxVocSr), sizeof(temp_lut_node.nTxVocSr));
            offset += (uint32_t)sizeof(temp_lut_node.nTxVocSr);
            ACDB_MEM_CPY(pInput->pBufferPointer + offset, sizeof(temp_lut_node.nRxVocSr), (void*)(&temp_lut_node.nRxVocSr), sizeof(temp_lut_node.nRxVocSr));
            offset += (uint32_t)sizeof(temp_lut_node.nRxVocSr);
            ACDB_MEM_CPY(pInput->pBufferPointer + offset, sizeof(temp_lut_node.tx_Voc_mode), (void*)(&temp_lut_node.tx_Voc_mode), sizeof(temp_lut_node.tx_Voc_mode));
            offset += (uint32_t)sizeof(temp_lut_node.tx_Voc_mode);
            ACDB_MEM_CPY(pInput->pBufferPointer + offset, sizeof(temp_lut_node.rx_Voc_mode), (void*)(&temp_lut_node.rx_Voc_mode), sizeof(temp_lut_node.rx_Voc_mode));
            offset += (uint32_t)sizeof(temp_lut_node.rx_Voc_mode);
            ACDB_MEM_CPY(pInput->pBufferPointer + offset, sizeof(temp_lut_node.nVocoder_type), (void*)(&temp_lut_node.nVocoder_type), sizeof(temp_lut_node.nVocoder_type));
            offset += (uint32_t)sizeof(temp_lut_node.nVocoder_type);
            ACDB_MEM_CPY(pInput->pBufferPointer + offset, sizeof(temp_lut_node.nFeature), (void*)(&temp_lut_node.nFeature), sizeof(temp_lut_node.nFeature));
            offset += (uint32_t)sizeof(temp_lut_node.nFeature);
            ACDB_MEM_CPY(pInput->pBufferPointer + offset, sizeof(temp_lut_node.cdft_offset), (void*)(&temp_lut_node.cdft_offset), sizeof(temp_lut_node.cdft_offset));
            offset += (uint32_t)sizeof(temp_lut_node.cdft_offset);
            ACDB_MEM_CPY(pInput->pBufferPointer + offset, sizeof(temp_lut_node.cdot_offset), (void*)(&temp_lut_node.cdot_offset), sizeof(temp_lut_node.cdot_offset));
            offset += (uint32_t)sizeof(temp_lut_node.cdot_offset);
         }
         break;
         default:
            result = ACDB_ERROR;
            goto end;
         }

         ++index;
      } while (index < lutTbl.nLen);

      result = ACDB_SUCCESS;
      goto end;
   }
end:
   if (lut_cdft != NULL)
   {
      deleteList(&lut_cdft);
      lut_cdft = NULL;
   }
   if (lut_cdot != NULL)
   {
      deleteList(&lut_cdot);
      lut_cdot = NULL;
   }
   if (lut_cdft_buffer != NULL)
   {
      deleteList(&lut_cdft_buffer);
      lut_cdft_buffer = NULL;
   }
   if (lut_cdot_buffer != NULL)
   {
      deleteList(&lut_cdot_buffer);
      lut_cdot_buffer = NULL;
   }
   if (lut_data_buffer != NULL)
   {
      deleteList(&lut_data_buffer);
      lut_data_buffer = NULL;
   }
   if (lut_data != NULL)
   {
      deleteList(&lut_data);
      lut_data = NULL;
   }
   if (lut_cdot_new != NULL)
   {
      deleteList(&lut_cdot_new);
      lut_cdot_new = NULL;
   }
   if (lut_cdft_new != NULL)
   {
      deleteList(&lut_cdft_new);
      lut_cdft_new = NULL;
   }

   return result;
}

int32_t AcdbCmdGetAdieANCDataTable(AcdbCodecANCSettingCmdType *pInput,
   AcdbQueryResponseType *pOutput)
{
   int32_t result = ACDB_SUCCESS;

   if (pInput == NULL || pOutput == NULL)
   {
      ACDB_DEBUG_LOG("[ACDB Command]->[AcdbCmdGetAdieANCDataTable]->System Erorr\n");
      result = ACDB_BADPARM;
   }
   else if (pInput->nBufferPointer == NULL)
   {
      ACDB_DEBUG_LOG("[ACDB Command]->[AcdbCmdGetAdieANCDataTable]->NULL pointer\n");
      result = ACDB_BADPARM;
   }
   else
   {
      uint32_t index = 0;
      AcdbTableCmd cmd = { 0, 0 };
      AcdbTableInfo tblInfo = { { 0, NULL }, { 0, NULL }, { 0, NULL } };
	  ANCDataLookupTblType lutTbl = { 0, NULL };
      uintptr_t nLookupKey = 0;
	  ANCCmdLookupType anccmd = { 0 , 0};
      uint32_t tblId = ADIE_ANC_TBL;

      anccmd.nDeviceId = pInput->nRxDeviceId;
      anccmd.nPID = pInput->nParamId;

      cmd.devId = anccmd.nDeviceId;
      cmd.tblId = tblId;
      result = acdbdata_ioctl(ACDBDATACMD_GET_TABLE_INFO, (uint8_t *)&cmd, sizeof(cmd),
         (uint8_t *)&tblInfo, sizeof(tblInfo));
      if (result != ACDB_SUCCESS)
      {
         ACDB_DEBUG_LOG("Failed to fetch the lookup information of the device %08X \n"
            , cmd.devId);
         return result;
      }
      lutTbl.nLen = READ_UInt32(tblInfo.tblLutChnk.pData);
      lutTbl.pLut = (ANCDataLookupType *)(tblInfo.tblLutChnk.pData + sizeof(lutTbl.nLen));
      result = AcdbDataBinarySearch((void *)lutTbl.pLut, lutTbl.nLen,
         ANC_LUT_INDICES_COUNT, &anccmd, ANC_CMD_INDICES_COUNT, &index);
      if (result != SEARCH_SUCCESS)
      {
         ACDB_DEBUG_LOG("Failed to fetch the lookup information of the device %08X \n"
            , cmd.devId);
         return ACDB_INPUT_PARAMS_NOT_FOUND;
      }

      nLookupKey = (uintptr_t)&lutTbl.pLut[index];
      result = GetCalibData(tblId, nLookupKey, lutTbl.pLut[index].nDataOffset, tblInfo.dataPoolChnk,
         pInput->nBufferPointer, pInput->nBufferLength, &pOutput->nBytesUsedInBuffer);
      if (ACDB_SUCCESS != result)
      {
         return result;
      }
      result = ACDB_SUCCESS;
   }

   return result;
}

int32_t AcdbCmdGetAdieProfileTable(AcdbAdiePathProfileV2CmdType *pInput,
   AcdbQueryResponseType *pOutput)
{
   int32_t result = ACDB_SUCCESS;

   if (pInput == NULL || pOutput == NULL)
   {
      ACDB_DEBUG_LOG("[ACDB Command]->[AcdbCmdGetAdieProfileTable]->System Erorr\n");
      result = ACDB_BADPARM;
   }
   else if (pInput->nBufferPointer == NULL)
   {
      ACDB_DEBUG_LOG("[ACDB Command]->[AcdbCmdGetAdieProfileTable]->NULL pointer\n");
      result = ACDB_BADPARM;
   }
   else
   {
      uint32_t index = 0;
      AcdbTableCmd cmd = { 0, 0 };
      AcdbTableInfo tblInfo = { { 0, NULL }, { 0, NULL }, { 0, NULL } };
	  ADIEDataLookupTblType lutTbl = { 0, NULL };
      uintptr_t nLookupKey = 0;
	  ADIECmdLookupType adiecmd = { 0, 0 };
      uint32_t tblId = ADIE_CODEC_TBL;

      adiecmd.nCodecId = pInput->ulCodecPathId;
      adiecmd.nPID = pInput->nParamId;

      cmd.devId = 0;  //adie calibration is not device based, so make it zero
      cmd.tblId = tblId;
      result = acdbdata_ioctl(ACDBDATACMD_GET_TABLE_INFO, (uint8_t *)&cmd, sizeof(cmd),
         (uint8_t *)&tblInfo, sizeof(tblInfo));
      if (result != ACDB_SUCCESS)
      {
         ACDB_DEBUG_LOG("Failed to fetch the lookup information of the codecpathid %08X \n"
            , adiecmd.nCodecId);
         return result;
      }
      lutTbl.nLen = READ_UInt32(tblInfo.tblLutChnk.pData);
      lutTbl.pLut = (ADIEDataLookupType *)(tblInfo.tblLutChnk.pData + sizeof(lutTbl.nLen));
      result = AcdbDataBinarySearch((void *)lutTbl.pLut, lutTbl.nLen,
         ADIE_LUT_INDICES_COUNT, &adiecmd, ADIE_CMD_INDICES_COUNT, &index);
      if (result != SEARCH_SUCCESS)
      {
         ACDB_DEBUG_LOG("Failed to fetch the lookup information of the codecid %08X \n"
            , adiecmd.nCodecId);
         return ACDB_INPUT_PARAMS_NOT_FOUND;
      }

      nLookupKey = (uintptr_t)&lutTbl.pLut[index];
      result = GetCalibData(tblId, nLookupKey, lutTbl.pLut[index].nDataOffset, tblInfo.dataPoolChnk,
         pInput->nBufferPointer, pInput->nBufferLength, &pOutput->nBytesUsedInBuffer);
      if (ACDB_SUCCESS != result)
      {
         return result;
      }
      result = ACDB_SUCCESS;
   }

   return result;
}

int32_t AcdbCmdGetAdieProfileTableSize(AcdbAdiePathProfileV2SizeCmdType *pInput,
   AcdbQueryResponseType *pOutput)
{
   int32_t result = ACDB_SUCCESS;

   if (pInput == NULL || pOutput == NULL)
   {
      ACDB_DEBUG_LOG("[ACDB Command]->[AcdbCmdGetAdieProfileTableSize]->System Erorr\n");
      result = ACDB_BADPARM;
   }
   else
   {
      uint32_t index = 0;
      AcdbTableCmd cmd = { 0, 0 };
      AcdbTableInfo tblInfo = { { 0, NULL }, { 0, NULL }, { 0, NULL } };
      ADIEDataLookupTblType lutTbl = { 0, NULL };
      uintptr_t nLookupKey = 0;
      ADIECmdLookupType adiecmd = { 0, 0 };
      uint32_t tblId = ADIE_CODEC_TBL;

      adiecmd.nCodecId = pInput->codecPathId;
      adiecmd.nPID = pInput->nParamId;

      cmd.devId = 0;  //adie calibration is not device based, so make it zero
      cmd.tblId = tblId;
      result = acdbdata_ioctl(ACDBDATACMD_GET_TABLE_INFO, (uint8_t *)&cmd, sizeof(cmd),
         (uint8_t *)&tblInfo, sizeof(tblInfo));
      if (result != ACDB_SUCCESS)
      {
         ACDB_DEBUG_LOG("Failed to fetch the lookup information of the codecpathid %08X \n"
            , adiecmd.nCodecId);
         return result;
      }

      lutTbl.nLen = READ_UInt32(tblInfo.tblLutChnk.pData);
      lutTbl.pLut = (ADIEDataLookupType *)(tblInfo.tblLutChnk.pData + sizeof(lutTbl.nLen));
      result = AcdbDataBinarySearch((void *)lutTbl.pLut, lutTbl.nLen,
         ADIE_LUT_INDICES_COUNT, &adiecmd, ADIE_CMD_INDICES_COUNT, &index);
      if (result != SEARCH_SUCCESS)
      {
         ACDB_DEBUG_LOG("Failed to fetch the lookup information of the codecid %08X \n"
            , adiecmd.nCodecId);
         return ACDB_INPUT_PARAMS_NOT_FOUND;
      }

      nLookupKey = (uintptr_t)&lutTbl.pLut[index];
      result = GetADIEprofileSize(tblId, nLookupKey, lutTbl.pLut[index].nDataOffset, tblInfo.dataPoolChnk,
         &pOutput->nBytesUsedInBuffer);
   }

   return result;
}

int32_t AcdbCmdGetGlobalTable(AcdbGblTblCmdType *pInput,
   AcdbQueryResponseType *pOutput)
{
   int32_t result = ACDB_SUCCESS;

   if (pInput == NULL || pOutput == NULL)
   {
      ACDB_DEBUG_LOG("[ACDB Command]->[AcdbCmdGetGlobalTable]->System Erorr\n");
      result = ACDB_BADPARM;
   }
   else if (pInput->nBufferPointer == NULL)
   {
      ACDB_DEBUG_LOG("[ACDB Command]->[AcdbCmdGetGlobalTable]->NULL pointer\n");
      result = ACDB_BADPARM;
   }
   else
   {
      uint32_t index = 0;
      AcdbTableCmd cmd = { 0, 0 };
      AcdbTableInfo tblInfo = { { 0, NULL }, { 0, NULL }, { 0, NULL } };
	  GlobalDataLookupTblType lutTbl = { 0, NULL };
      uintptr_t nLookupKey = 0;
	  GlobalCmdLookupType gcmd = { 0, 0 };
      uint32_t tblId = GLOBAL_DATA_TBL;

      gcmd.nMid = pInput->nModuleId;
      gcmd.nPid = pInput->nParamId;

      cmd.devId = 0;  //adie calibration is not device based, so make it zero
      cmd.tblId = tblId;
      result = acdbdata_ioctl(ACDBDATACMD_GET_TABLE_INFO, (uint8_t *)&cmd, sizeof(cmd),
         (uint8_t *)&tblInfo, sizeof(tblInfo));
      if (result != ACDB_SUCCESS)
      {
         ACDB_DEBUG_LOG("Failed to fetch the lookup information of the globaldata table\n");
         return result;
      }
      lutTbl.nLen = READ_UInt32(tblInfo.tblLutChnk.pData);
      lutTbl.pLut = (GlobalDataLookupType *)(tblInfo.tblLutChnk.pData + sizeof(lutTbl.nLen));
      result = AcdbDataBinarySearch((void *)lutTbl.pLut, lutTbl.nLen,
         GLOBAL_LUT_INDICES_COUNT, &gcmd, GLOBAL_CMD_INDICES_COUNT, &index);
      if (result != SEARCH_SUCCESS)
      {
         ACDB_DEBUG_LOG("Couldnt find the mid %08X and pid %08X \n", gcmd.nMid, gcmd.nPid);
         return ACDB_INPUT_PARAMS_NOT_FOUND;
      }

      nLookupKey = (uintptr_t)&lutTbl.pLut[index];
      result = GetCalibData(tblId, nLookupKey, lutTbl.pLut[index].nDataOffset, tblInfo.dataPoolChnk,
         pInput->nBufferPointer, pInput->nBufferLength, &pOutput->nBytesUsedInBuffer);
      if (ACDB_SUCCESS != result)
      {
         return result;
      }
      result = ACDB_SUCCESS;
   }

   return result;
}

int32_t AcdbCmdGetAfeData(AcdbAfeDataCmdType *pInput,
   AcdbQueryResponseType *pOutput)
{
   int32_t result = ACDB_SUCCESS;

   if (pInput == NULL || pOutput == NULL)
   {
      ACDB_DEBUG_LOG("[ACDB Command]->[AcdbCmdGetAfeData]->System Erorr\n");
      result = ACDB_BADPARM;
   }
   else if (pInput->nBufferPointer == NULL)
   {
      ACDB_DEBUG_LOG("[ACDB Command]->[AcdbCmdGetAfeData]->NULL pointer\n");
      result = ACDB_BADPARM;
   }
   else
   {
      uint32_t index = 0;
      AcdbTableCmd cmd = { 0, 0 };
      AcdbTableInfo tblInfo = { { 0, NULL }, { 0, NULL }, { 0, NULL } };
	  AfeDataLookupTblType lutTbl = { 0, NULL };
      ContentDefTblType cdefTbl = { 0, NULL };
      ContentDataOffsetsTblType cdotTbl = { 0, NULL };
      uintptr_t nLookupKey = 0;
	  AfeCmdLookupType afecmd = { 0, 0 };
      uint32_t tblId = AFE_TBL;
	  AfeDataLookupType tempLut = { 0, 0, 0, 0};

      afecmd.nTxDevId = pInput->nTxDeviceId;
      afecmd.nRxDevId = pInput->nRxDeviceId;

      cmd.devId = afecmd.nTxDevId;
      cmd.tblId = tblId;
      result = acdbdata_ioctl(ACDBDATACMD_GET_TABLE_INFO, (uint8_t *)&cmd, sizeof(cmd),
         (uint8_t *)&tblInfo, sizeof(tblInfo));
      if (result != ACDB_SUCCESS)
      {
         ACDB_DEBUG_LOG("Failed to fetch the lookup information of the device %08X \n"
            , afecmd.nTxDevId);
         return result;
      }
      lutTbl.nLen = READ_UInt32(tblInfo.tblLutChnk.pData);
      lutTbl.pLut = (AfeDataLookupType *)(tblInfo.tblLutChnk.pData + sizeof(lutTbl.nLen));

      result = AcdbDataBinarySearch((void *)lutTbl.pLut, lutTbl.nLen,
         AFE_LUT_INDICES_COUNT, &afecmd, AFE_CMD_INDICES_COUNT, &index);
      if (result != SEARCH_SUCCESS)
      {
         ACDB_DEBUG_LOG("Failed to fetch the lookup information of the device %08X \n"
            , afecmd.nTxDevId);
         return ACDB_INPUT_PARAMS_NOT_FOUND;
      }

      nLookupKey = (uintptr_t)&lutTbl.pLut[index];
      // Now get CDEF info
      ACDB_MEM_CPY(&tempLut, sizeof(tempLut), &lutTbl.pLut[index], sizeof(tempLut));
      ACDB_MEM_CPY(&cdefTbl.nLen, sizeof(uint32_t), (tblInfo.tblCdftChnk.pData + tempLut.nCDEFTblOffset), sizeof(uint32_t));
      cdefTbl.pCntDef = (ContentDefType *)(tblInfo.tblCdftChnk.pData + tempLut.nCDEFTblOffset + sizeof(cdefTbl.nLen));

      // Now get CDOT info
      ACDB_MEM_CPY(&cdotTbl.nLen, sizeof(uint32_t), (tblInfo.tblCdotChnk.pData + tempLut.nCDOTTblOffset), sizeof(uint32_t));
      cdotTbl.pDataOffsets = (uint32_t *)(tblInfo.tblCdotChnk.pData + tempLut.nCDOTTblOffset + sizeof(cdotTbl.nLen));

      if (cdefTbl.nLen != cdotTbl.nLen)
      {
         ACDB_DEBUG_LOG("The no of entries in CDEF and CDOT tables for devid %08X not matching\n", afecmd.nTxDevId);
         return ACDB_ERROR;
      }

      result = GetMidPidCalibData(tblId, nLookupKey, cdefTbl, cdotTbl, tblInfo.dataPoolChnk, pInput->nModuleId,
         pInput->nParamId, pInput->nBufferPointer, pInput->nBufferLength, &pOutput->nBytesUsedInBuffer);
      if (ACDB_SUCCESS != result)
      {
         return result;
      }

      result = ACDB_SUCCESS;
   }

   return result;
}

int32_t AcdbCmdGetAfeCmnInfo(uint32_t queryType, uint8_t *pIn, uint8_t *pOut)
{
   int32_t result = ACDB_SUCCESS;

   if (pIn == NULL || pOut == NULL)
   {
      ACDB_DEBUG_LOG("[ACDB Command]->[AcdbCmdGetAfeCmnInfo]->Invalid NULL value parameters are provided\n");
      result = ACDB_BADPARM;
   }
   else
   {
      uint32_t index = 0;
      AcdbTableCmd cmd = { 0, 0 };
      AcdbTableInfo tblInfo = { { 0, NULL }, { 0, NULL }, { 0, NULL } };
	  AfeCmnDataLookupTblType lutTbl = { 0, NULL };
      ContentDefTblType cdefTbl = { 0, NULL };
      ContentDataOffsetsTblType cdotTbl = { 0, NULL };
      uintptr_t nLookupKey = 0;
	  AfeCmnCmdLookupType afecmncmd = { 0, 0 };
      uint32_t tblId = AFE_CMN_TBL;

	  AfeCmnDataLookupType tempLut = { 0, 0, 0, 0};
      switch (queryType)
      {
      case TABLE_CMD:
      {
         AcdbAfeCommonTableCmdType *pInput = (AcdbAfeCommonTableCmdType *)pIn;
         if (pInput->nBufferPointer == NULL)
         {
            return ACDB_BADPARM;
         }
         afecmncmd.nDeviceId = pInput->nDeviceId;
         afecmncmd.nDeviceSampleRateId = pInput->nSampleRateId;
      }
      break;
      case TABLE_SIZE_CMD:
      {
         AcdbAfeCommonTableSizeCmdType *pInput = (AcdbAfeCommonTableSizeCmdType *)pIn;
         afecmncmd.nDeviceId = pInput->nDeviceId;
         afecmncmd.nDeviceSampleRateId = pInput->nSampleRateId;
      }
      break;
      case DATA_CMD:
      {
         AcdbAfeCmnDataCmdType *pInput = (AcdbAfeCmnDataCmdType *)pIn;
         if (pInput->nBufferPointer == NULL)
         {
            return ACDB_BADPARM;
         }
         afecmncmd.nDeviceId = pInput->nDeviceId;
         afecmncmd.nDeviceSampleRateId = pInput->nAfeSampleRateId;
      }
      break;
      default:
         return  ACDB_ERROR;
      }
      //acdb_translate_sample_rate(afecmncmd.nDeviceSampleRateId,&afecmncmd.nDeviceSampleRateId);

      cmd.devId = afecmncmd.nDeviceId;
      cmd.tblId = tblId;
      result = acdbdata_ioctl(ACDBDATACMD_GET_TABLE_INFO, (uint8_t *)&cmd, sizeof(cmd),
         (uint8_t *)&tblInfo, sizeof(tblInfo));

      if (result != ACDB_SUCCESS)
      {
         ACDB_DEBUG_LOG("Failed to fetch the lookup information of the device %08X \n"
            , afecmncmd.nDeviceId);
         return result;
      }

      lutTbl.nLen = READ_UInt32(tblInfo.tblLutChnk.pData);
      lutTbl.pLut = (AfeCmnDataLookupType *)(tblInfo.tblLutChnk.pData + sizeof(lutTbl.nLen));

      result = AcdbDataBinarySearch((void *)lutTbl.pLut, lutTbl.nLen,
         AFECMN_LUT_INDICES_COUNT, &afecmncmd, AFECMN_CMD_INDICES_COUNT, &index);
      if (result != SEARCH_SUCCESS)
      {
         ACDB_DEBUG_LOG("Failed to fetch the lookup information of the device %08X \n"
            , afecmncmd.nDeviceId);
         return ACDB_INPUT_PARAMS_NOT_FOUND;
      }

      nLookupKey = (uintptr_t)&lutTbl.pLut[index];
      // Now get CDEF info
      ACDB_MEM_CPY(&tempLut, sizeof(tempLut), &lutTbl.pLut[index], sizeof(tempLut));
      ACDB_MEM_CPY(&cdefTbl.nLen, sizeof(uint32_t), (tblInfo.tblCdftChnk.pData + tempLut.nCDEFTblOffset), sizeof(uint32_t));
      cdefTbl.pCntDef = (ContentDefType *)(tblInfo.tblCdftChnk.pData + tempLut.nCDEFTblOffset + sizeof(cdefTbl.nLen));

      // Now get CDOT info
      ACDB_MEM_CPY(&cdotTbl.nLen, sizeof(uint32_t), (tblInfo.tblCdotChnk.pData + tempLut.nCDOTTblOffset), sizeof(uint32_t));
      cdotTbl.pDataOffsets = (uint32_t *)(tblInfo.tblCdotChnk.pData + tempLut.nCDOTTblOffset + sizeof(cdotTbl.nLen));

      if (cdefTbl.nLen != cdotTbl.nLen)
      {
         ACDB_DEBUG_LOG("The no of entries in CDEF and CDOT tables for devid %08X not matching\n", afecmncmd.nDeviceId);
         return ACDB_ERROR;
      }

      switch (queryType)
      {
      case TABLE_CMD:
      {
         AcdbAfeCommonTableCmdType *pInput = (AcdbAfeCommonTableCmdType *)pIn;
         AcdbQueryResponseType *pOutput = (AcdbQueryResponseType *)pOut;
         result = GetMidPidCalibTable(tblId, nLookupKey, cdefTbl, cdotTbl, tblInfo.dataPoolChnk,
            pInput->nBufferPointer, pInput->nBufferLength, &pOutput->nBytesUsedInBuffer);
         if (ACDB_SUCCESS != result)
         {
            return result;
         }
      }
      break;
      case TABLE_SIZE_CMD:
      {
         AcdbSizeResponseType *pOutput = (AcdbSizeResponseType *)pOut;
         if (ACDB_SUCCESS != GetMidPidCalibTableSize(tblId, nLookupKey, cdefTbl, cdotTbl, tblInfo.dataPoolChnk, &pOutput->nSize))
         {
            return ACDB_ERROR;
         }
      }
      break;
      case DATA_CMD:
      {
         AcdbAfeCmnDataCmdType *pInput = (AcdbAfeCmnDataCmdType *)pIn;
         AcdbQueryResponseType *pOutput = (AcdbQueryResponseType *)pOut;
         result = GetMidPidCalibData(tblId, nLookupKey, cdefTbl, cdotTbl, tblInfo.dataPoolChnk, pInput->nModuleId,
            pInput->nParamId, pInput->nBufferPointer, pInput->nBufferLength, &pOutput->nBytesUsedInBuffer);
         if (ACDB_SUCCESS != result)
         {
            return result;
         }
      }
      break;
      default:
         return  ACDB_ERROR;
      }

      result = ACDB_SUCCESS;
   }

   return result;
}

int32_t AcdbCmdGetVocprocDevCfgInfo(uint32_t queryType, uint8_t *pIn, uint8_t *pOut)
{
   int32_t result = ACDB_SUCCESS;

   if (pIn == NULL || pOut == NULL)
   {
      ACDB_DEBUG_LOG("[ACDB Command]->[AcdbCmdGetVocprocDevCfgInfo]->Invalid NULL value parameters are provided\n");
      result = ACDB_BADPARM;
   }
   else
   {
      uint32_t index = 0;
      AcdbTableCmd cmd = { 0, 0 };
      AcdbTableInfo tblInfo = { { 0, NULL }, { 0, NULL }, { 0, NULL } };
	  VocprocDevCfgDataLookupTblType lutTbl = { 0, NULL };
      ContentDefTblType cdefTbl = { 0, NULL };
      ContentDataOffsetsTblType cdotTbl = { 0, NULL };
      uintptr_t nLookupKey = 0;
	  VocprocDevCfgCmdLookupType vpcfgcmd = { 0, 0 };
	  AcdbVP3Info txvp3cmd = { 0, 0, 0, 0}, rxvp3cmd = { 0, 0, 0, 0};
      uint32_t tblId = VOCPROC_DEV_CFG_TBL;
	  VocprocDevCfgDataLookupType tempLut = { 0, 0, 0, 0};

      switch (queryType)
      {
      case TABLE_CMD:
      {
         AcdbVocProcDevCfgCmdType *pInput = (AcdbVocProcDevCfgCmdType *)pIn;
         if (pInput->pBuff == NULL)
            return ACDB_BADPARM;
         vpcfgcmd.nTxDevId = pInput->nTxDeviceId;
         vpcfgcmd.nRxDevId = pInput->nRxDeviceId;

         txvp3cmd.ModuleID = VSS_MODULE_SOUND_DEVICE;
         txvp3cmd.ParamID = VSS_PARAM_TX_SOUND_DEVICE_ID;
         txvp3cmd.Paramsize = 4;
         txvp3cmd.DevId = pInput->nTxDeviceId;

         rxvp3cmd.ModuleID = VSS_MODULE_SOUND_DEVICE;
         rxvp3cmd.ParamID = VSS_PARAM_RX_SOUND_DEVICE_ID;
         rxvp3cmd.Paramsize = 4;
         rxvp3cmd.DevId = pInput->nRxDeviceId;
      }
      break;
      case TABLE_SIZE_CMD:
      {
         AcdbVocProcDevCfgSizeCmdType *pInput = (AcdbVocProcDevCfgSizeCmdType *)pIn;
         vpcfgcmd.nTxDevId = pInput->nTxDeviceId;
         vpcfgcmd.nRxDevId = pInput->nRxDeviceId;
      }
      break;
      default:
         return  ACDB_ERROR;
      }

      cmd.devId = vpcfgcmd.nTxDevId;
      cmd.tblId = tblId;
      result = acdbdata_ioctl(ACDBDATACMD_GET_TABLE_INFO, (uint8_t *)&cmd, sizeof(cmd),
         (uint8_t *)&tblInfo, sizeof(tblInfo));
      if (result != ACDB_SUCCESS)
      {
         ACDB_DEBUG_LOG("Failed to fetch the lookup information of the device %08X \n"
            , vpcfgcmd.nTxDevId);
         return result;
      }
      lutTbl.nLen = READ_UInt32(tblInfo.tblLutChnk.pData);
      lutTbl.pLut = (VocprocDevCfgDataLookupType *)(tblInfo.tblLutChnk.pData + sizeof(lutTbl.nLen));

      result = AcdbDataBinarySearch((void *)lutTbl.pLut, lutTbl.nLen,
         VOCPROCDEVCFG_LUT_INDICES_COUNT, &vpcfgcmd, VOCPROCDEVCFG_CMD_INDICES_COUNT, &index);
      if (result != SEARCH_SUCCESS)
      {
         ACDB_DEBUG_LOG("Failed to fetch the lookup information of the device %08X \n"
            , vpcfgcmd.nTxDevId);
         return ACDB_INPUT_PARAMS_NOT_FOUND;
      }

      nLookupKey = (uintptr_t)&lutTbl.pLut[index];
      // Now get CDEF info
      ACDB_MEM_CPY(&tempLut, sizeof(tempLut), &lutTbl.pLut[index], sizeof(tempLut));
      ACDB_MEM_CPY(&cdefTbl.nLen, sizeof(uint32_t), (tblInfo.tblCdftChnk.pData + tempLut.nCDEFTblOffset), sizeof(uint32_t));
      cdefTbl.pCntDef = (ContentDefType *)(tblInfo.tblCdftChnk.pData + tempLut.nCDEFTblOffset + sizeof(cdefTbl.nLen));

      // Now get CDOT info
      ACDB_MEM_CPY(&cdotTbl.nLen, sizeof(uint32_t), (tblInfo.tblCdotChnk.pData + tempLut.nCDOTTblOffset), sizeof(uint32_t));
      cdotTbl.pDataOffsets = (uint32_t *)(tblInfo.tblCdotChnk.pData + tempLut.nCDOTTblOffset + sizeof(cdotTbl.nLen));

      if (cdefTbl.nLen != cdotTbl.nLen)
      {
         ACDB_DEBUG_LOG("The no of entries in CDEF and CDOT tables for devid %08X not matching\n", vpcfgcmd.nTxDevId);
         return ACDB_ERROR;
      }

      switch (queryType)
      {
      case TABLE_CMD:
      {
         AcdbVocProcDevCfgCmdType *pInput = (AcdbVocProcDevCfgCmdType *)pIn;
         AcdbQueryResponseType *pOutput = (AcdbQueryResponseType *)pOut;
         result = GetMidPidCalibTable(tblId, nLookupKey, cdefTbl, cdotTbl, tblInfo.dataPoolChnk,
            pInput->pBuff, pInput->nBufferLength, &pOutput->nBytesUsedInBuffer);
         if (ACDB_SUCCESS != result)
         {
            return result;
         }

         //Add VP3 support
         ACDB_MEM_CPY((uint8_t *)pInput->pBuff + pOutput->nBytesUsedInBuffer, pInput->nBufferLength, (uint8_t *)&txvp3cmd, sizeof(txvp3cmd));
         ACDB_MEM_CPY((uint8_t *)pInput->pBuff + pOutput->nBytesUsedInBuffer + sizeof(txvp3cmd), pInput->nBufferLength, (uint8_t *)&rxvp3cmd, sizeof(rxvp3cmd));
         pOutput->nBytesUsedInBuffer += (uint32_t)(sizeof(txvp3cmd) + sizeof(rxvp3cmd));
      }
      break;
      case TABLE_SIZE_CMD:
      {
         AcdbSizeResponseType *pOutput = (AcdbSizeResponseType *)pOut;
         if (ACDB_SUCCESS != GetMidPidCalibTableSize(tblId, nLookupKey, cdefTbl, cdotTbl, tblInfo.dataPoolChnk, &pOutput->nSize))
         {
            return ACDB_ERROR;
         }
         //Add VP3 support
         pOutput->nSize += (uint32_t)(sizeof(txvp3cmd) + sizeof(rxvp3cmd));
      }
      break;
      default:
         return  ACDB_ERROR;
      }

      result = ACDB_SUCCESS;
   }

   return result;
}

int32_t AcdbCmdGetFilesInfo(AcdbQueryCmdType *pReq, AcdbQueryResponseType *pRsp)
{
   int32_t result = ACDB_SUCCESS;
   if (pReq == NULL || pRsp == NULL)
   {
      ACDB_DEBUG_LOG("Invalid input provided to fetch the files info\n");
      return ACDB_ERROR;
   }
   if (pReq->pBufferPointer == NULL || pReq->nBufferLength == 0)
   {
      ACDB_DEBUG_LOG("Insufficient memory provided to fetch the files info\n");
      return ACDB_INSUFFICIENTMEMORY;
   }
   result = acdbdata_ioctl(ACDBDATACMD_GET_LOADED_FILES_INFO, pReq->pBufferPointer,
      pReq->nBufferLength, (uint8_t*)&pRsp->nBytesUsedInBuffer, sizeof(pRsp->nBytesUsedInBuffer));
   return result;
}

int32_t AcdbCmdGetFileData(AcdbCmdGetFileDataReq *pReq, AcdbCmdResp *pRsp)
{
   int32_t result = ACDB_SUCCESS;
   AcdbFileMgrGetFileDataReq *pFmReq = (AcdbFileMgrGetFileDataReq *)pReq;
   AcdbFileMgrResp *pFmResp = (AcdbFileMgrResp *)pRsp;
   if (pReq == NULL || pRsp == NULL)
   {
      ACDB_DEBUG_LOG("Invalid input provided to fetch the file data\n");
      return ACDB_ERROR;
   }
   if (pReq->pFileName == NULL || pReq->nfileNameLen == 0)
   {
      ACDB_DEBUG_LOG("Invalid filename or filename len provided to fetch the file data\n");
      return ACDB_ERROR;
   }
   if (pRsp->pRespBuff == NULL || pRsp->nresp_buff_len == 0)
   {
      ACDB_DEBUG_LOG("Insufficient memory provided to fetch the file data\n");
      return ACDB_INSUFFICIENTMEMORY;
   }
   result = acdbdata_ioctl(ACDBDATACMD_GET_FILE_DATA, (uint8_t*)pFmReq,
      sizeof(AcdbFileMgrGetFileDataReq), (uint8_t*)pFmResp, sizeof(AcdbFileMgrResp));
   return result;
}

int32_t AcdbCmdGetOnlineDataV2(uint32_t tblId, uint8_t *pIndices, uint32_t nIdxCount, uint32_t noOfCdftIndices,
   uint8_t *pBuff, uint32_t nBuffLen, uint32_t *pBuffBytesFilled)
{
   int32_t result = ACDB_SUCCESS;

   if (pBuff == NULL || nBuffLen == 0)
   {
      ACDB_DEBUG_LOG("[ACDB Command]->Invalid NULL value parameters are provided\n");
      result = ACDB_BADPARM;
   }
   else
   {
      uint32_t index = 0;
      VocProcVolV2CmdLookupType voccmd = { 0, 0, 0 };
      AcdbTableCmd cmd = { 0, 0 };
      AcdbTableInfo tblInfo = { { 0, NULL }, { 0, NULL }, { 0, NULL } };
      AcdbTableInfoCVD cvdTblInfo = { { 0, NULL }, { 0, NULL }, { 0, NULL }, { 0, NULL }, { 0, NULL }, { 0, NULL } };
      uint32_t devId = 0;
      uint32_t noOfLUTEntries = 0;
      uint8_t *pLut = NULL;
      uint32_t noOfLutIndices = 0;
      uint32_t noOfTableIndices = 0;
      VocProcDynDataLookupTblType_UniqueData lutTbl = { 0, NULL };
      VocProcDynDataLookupTblType_CommonData lutTblCVD = { 0, NULL };
      VocProcDynDataLookupTblType_offsetData lutoffsetTbl = { 0, NULL };
      VocProcStatDataLookupTblType_UniqueData lutStatTbl = { 0, NULL };
      VocProcStatDataLookupTblType_CommonData lutStatTblCVD = { 0, NULL };
      VocProcStatDataLookupTblType_offsetData lutStatoffsetTbl = { 0, NULL };

      ContentDefTblType cdefTbl = { 0, NULL };
      ContentDefWithInstanceTblType cdefInstanceTbl = { 0, NULL };
      ContentDataOffsetsTblType cdotTbl = { 0, NULL };
      uintptr_t nLookupKey = 0, nSecLookupKey = 0;
      uint32_t cdefoffset = 0, cdotoffset = 0;
      uint32_t cdefoffsetval = 0, cdotoffsetval = 0;
      uint8_t nonModuleTblFound = 0;
      uint32_t ofTbloffset = 0, cvdtableoffset = 0;
      uint32_t mid = 0, iid = 0, pid = 0;
      uint32_t offset = (uint32_t)(nIdxCount*sizeof(uint32_t));
      mid = READ_UInt32(pIndices + offset);
      offset += (uint32_t)sizeof(uint32_t);
      if (noOfCdftIndices == INSTANCE_CDFT_INDICES_COUNT)
      {
         iid = READ_UInt32(pIndices + offset);
         offset += (uint32_t)sizeof(uint32_t);
      }
      pid = READ_UInt32(pIndices + offset);
      offset += (uint32_t)sizeof(uint32_t);
      switch (tblId)
      {
      case AUDPROC_GAIN_INDP_TBL:
         ACDB_MEM_CPY((void *)&devId, sizeof(uint32_t), (void *)pIndices, sizeof(uint32_t));
         noOfLutIndices = AUDPROC_LUT_INDICES_COUNT;
         noOfTableIndices = AUDPROCTBL_INDICES_COUNT;
         break;
      case AUDPROC_COPP_GAIN_DEP_TBL:
         ACDB_MEM_CPY((void *)&devId, sizeof(uint32_t), (void *)pIndices, sizeof(uint32_t));
         noOfLutIndices = AUDPROC_GAIN_DEP_LUT_INDICES_COUNT;
         noOfTableIndices = AUDPROCTBL_GAIN_DEP_INDICES_COUNT;
         break;
      case AUDPROC_AUD_VOL_TBL:
         ACDB_MEM_CPY((void *)&devId, sizeof(uint32_t), (void *)pIndices, sizeof(uint32_t));
         noOfLutIndices = AUDPROC_VOL_LUT_INDICES_COUNT;
         noOfTableIndices = AUDPROCTBL_VOL_INDICES_COUNT;
         break;
      case AUD_STREAM_TBL:
         devId = 0;
         noOfLutIndices = AUDSTREAM_LUT_INDICES_COUNT;
         noOfTableIndices = AUDSTREAMTBL_INDICES_COUNT;
         break;
      case VOCPROC_GAIN_INDP_TBL:
         ACDB_MEM_CPY((void *)&devId, sizeof(uint32_t), (void *)pIndices, sizeof(uint32_t));
         noOfLutIndices = VOCPROC_LUT_INDICES_COUNT;
         noOfTableIndices = VOCPROCTBL_INDICES_COUNT;
         break;
      case VOCPROC_COPP_GAIN_DEP_TBL:
         ACDB_MEM_CPY((void *)&devId, sizeof(uint32_t), (void *)pIndices, sizeof(uint32_t));
         noOfLutIndices = VOCPROC_VOL_LUT_INDICES_COUNT;
         noOfTableIndices = VOCPROCTBL_VOL_INDICES_COUNT;
         break;
      case VOC_STREAM_TBL:
         devId = 0;
         noOfLutIndices = VOCSTREAM_LUT_INDICES_COUNT;
         noOfTableIndices = VOCSTREAMTBL_INDICES_COUNT;
         break;
      case AFE_TBL:
         ACDB_MEM_CPY((void *)&devId, sizeof(uint32_t), (void *)pIndices, sizeof(uint32_t));
         noOfLutIndices = AFE_LUT_INDICES_COUNT;
         noOfTableIndices = AFETBL_INDICES_COUNT;
         break;
      case AFE_CMN_TBL:
         ACDB_MEM_CPY((void *)&devId, sizeof(uint32_t), (void *)pIndices, sizeof(uint32_t));
         noOfLutIndices = AFECMN_LUT_INDICES_COUNT;
         noOfTableIndices = AFECMNTBL_INDICES_COUNT;
         break;
      case VOCPROC_DEV_CFG_TBL:
         ACDB_MEM_CPY((void *)&devId, sizeof(uint32_t), (void *)pIndices, sizeof(uint32_t));
         noOfLutIndices = VOCPROCDEVCFG_LUT_INDICES_COUNT;
         noOfTableIndices = VOCPROCDEVCFGTBL_INDICES_COUNT;
         break;
      case LSM_TBL:
         ACDB_MEM_CPY((void *)&devId, sizeof(uint32_t), (void *)pIndices, sizeof(uint32_t));
         noOfLutIndices = LSM_LUT_INDICES_COUNT;
         noOfTableIndices = LSM_INDICES_COUNT;
         break;
      case ADIE_SIDETONE_TBL:
         ACDB_MEM_CPY((void *)&devId, sizeof(uint32_t), (void *)pIndices, sizeof(uint32_t));
         noOfLutIndices = ADST_LUT_INDICES_COUNT;
         noOfTableIndices = ADST_INDICES_COUNT;
         break;
      case AANC_CFG_TBL:
         ACDB_MEM_CPY((void *)&devId, sizeof(uint32_t), (void *)pIndices, sizeof(uint32_t));
         noOfLutIndices = AANC_CFG_LUT_INDICES_COUNT;
         noOfTableIndices = AANC_CFG_TBL_INDICES_COUNT;
         break;
      case ADIE_ANC_TBL:
         ACDB_MEM_CPY((void *)&devId, sizeof(uint32_t), (void *)pIndices, sizeof(uint32_t));
         noOfLutIndices = ANC_LUT_INDICES_COUNT;
         noOfTableIndices = ANCTBL_INDICES_COUNT;
         nonModuleTblFound = 1;
         break;
      case ADIE_CODEC_TBL:
         devId = 0;
         noOfLutIndices = ANC_LUT_INDICES_COUNT;
         noOfTableIndices = ANCTBL_INDICES_COUNT;
         nonModuleTblFound = 1;
         break;
      case GLOBAL_DATA_TBL:
         devId = 0;
         noOfLutIndices = GLOBAL_LUT_INDICES_COUNT;
         noOfTableIndices = GLOBALTBL_INDICES_COUNT;
         nonModuleTblFound = 1;
         break;
      case CDC_FEATURES_TBL:
         ACDB_MEM_CPY((void *)&devId, sizeof(uint32_t), (void *)pIndices, sizeof(uint32_t));
         noOfLutIndices = CDC_FEATURES_DATA_LUT_INDICES_COUNT;
         noOfTableIndices = CDC_FEATURES_DATA_INDICES_COUNT;
         nonModuleTblFound = 1;
         break;
      case VOCPROC_COPP_GAIN_DEP_V2_TBL:
         ACDB_MEM_CPY((void *)&devId, sizeof(uint32_t), (void *)pIndices, sizeof(uint32_t));
         noOfLutIndices = VOCPROC_VOL_V2_LUT_INDICES_COUNT;
         noOfTableIndices = VOCPROCTBL_VOL_V2_INDICES_COUNT;
         break;
      case VOICE_VP3_TBL:
         ACDB_MEM_CPY((void *)&devId, sizeof(uint32_t), (void *)pIndices, sizeof(uint32_t));
         noOfLutIndices = VOICE_VP3_LUT_INDICES_COUNT;
         noOfTableIndices = VOICE_VP3TBL_INDICES_COUNT;
         break;
      case AUDIO_REC_VP3_TBL:
         ACDB_MEM_CPY((void *)&devId, sizeof(uint32_t), (void *)pIndices, sizeof(uint32_t));
         noOfLutIndices = AUDREC_VP3_LUT_INDICES_COUNT;
         noOfTableIndices = AUDREC_VP3TBL_INDICES_COUNT;
         break;
      case AUDIO_REC_EC_VP3_TBL:
         ACDB_MEM_CPY((void *)&devId, sizeof(uint32_t), (void *)pIndices, sizeof(uint32_t));
         noOfLutIndices = AUDREC_EC_VP3_LUT_INDICES_COUNT;
         noOfTableIndices = AUDREC_EC_VP3TBL_INDICES_COUNT;
         break;
      case METAINFO_LOOKUP_TBL:
         devId = 0;
         noOfLutIndices = MINFO_LUT_INDICES_COUNT;
         noOfTableIndices = MINFOTBL_INDICES_COUNT;
         nonModuleTblFound = 1;
         break;
      case VOCPROC_DYNAMIC_TBL:
         ACDB_MEM_CPY((void *)&devId, sizeof(uint32_t), (void *)pIndices, sizeof(uint32_t));
         noOfLutIndices = VOCPROCDYN_LUT_INDICES_COUNT;
         noOfTableIndices = VOCPROCDYNTBL_INDICES_COUNT;
         break;
      case VOCPROC_STATIC_TBL:
         ACDB_MEM_CPY((void *)&devId, sizeof(uint32_t), (void *)pIndices, sizeof(uint32_t));
         noOfLutIndices = VOCPROCSTAT_LUT_INDICES_COUNT;
         noOfTableIndices = VOCPROCSTATTBL_INDICES_COUNT;
         break;
      case VOC_STREAM2_TBL:
         devId = 0;
         noOfLutIndices = VOCSTREAM2_LUT_INDICES_COUNT;
         noOfTableIndices = VOCSTREAM2TBL_INDICES_COUNT;
         break;
      case VOCPROC_DYN_INST_TBL_ID:
         ACDB_MEM_CPY((void *)&devId, sizeof(uint32_t), (void *)pIndices, sizeof(uint32_t));
         noOfLutIndices = VOCPROCDYN_INSTANCE_LUT_INDICES_COUNT;
         noOfTableIndices = VOCPROCDYNTBL_INSTANCE_INDICES_COUNT;
         break;
      case VOCPROC_STAT_INST_TBL_ID:
         ACDB_MEM_CPY((void *)&devId, sizeof(uint32_t), (void *)pIndices, sizeof(uint32_t));
         noOfLutIndices = VOCPROCSTAT_INSTANCE_LUT_INDICES_COUNT;
         noOfTableIndices = VOCPROCSTATTBL_INSTANCE_INDICES_COUNT;
         break;
      case VOCSTRM_INST_TBL_ID:
         devId = 0;
         noOfLutIndices = VOCSTREAM2_INST_LUT_INDICES_COUNT;
         noOfTableIndices = VOCSTREAM2TBL_INST_INDICES_COUNT;
         break;
      case AUDPROC_INST_TBL_ID:ACDB_MEM_CPY((void *)&devId, sizeof(uint32_t), (void *)pIndices, sizeof(uint32_t));
         noOfLutIndices = AUDPROC_INST_LUT_INDICES_COUNT;
         noOfTableIndices = AUDPROCTBL_INST_INDICES_COUNT;
         break;
      case AUDPROCVOL_INST_TBL_ID:
         ACDB_MEM_CPY((void *)&devId, sizeof(uint32_t), (void *)pIndices, sizeof(uint32_t));
         noOfLutIndices = AUDPROC_INST_GAIN_DEP_LUT_INDICES_COUNT;
         noOfTableIndices = AUDPROCTBL_INST__GAIN_DEP_INDICES_COUNT;
         break;
      case AFECMN_INST_TBL_ID:
         ACDB_MEM_CPY((void *)&devId, sizeof(uint32_t), (void *)pIndices, sizeof(uint32_t));
         noOfLutIndices = AFECMN_INST_LUT_INDICES_COUNT;
         noOfTableIndices = AFECMNTBL_INST_INDICES_COUNT;
         break;
      case AUDSTRM_INST_TBL_ID:
         devId = 0;
         noOfLutIndices = AUDSTREAM_INST_LUT_INDICES_COUNT;
         noOfTableIndices = AUDSTREAMTBL_INST_INDICES_COUNT;
         break;
      case LSM_INST_TBL_ID:
         ACDB_MEM_CPY((void *)&devId, sizeof(uint32_t), (void *)pIndices, sizeof(uint32_t));
         noOfLutIndices = LSM_INST_LUT_INDICES_COUNT;
         noOfTableIndices = LSM_INST_INDICES_COUNT;
         break;
      case CODEC_COMMON_TBL:
         ACDB_MEM_CPY((void *)&devId, sizeof(uint32_t), (void *)pIndices, sizeof(uint32_t));
         noOfLutIndices = CODEC_PP_COMMON_LUT_INDICES_COUNT;
         noOfTableIndices = CODEC_PP_COMMON_TABLE_INDICES_COUNT;
         break;
      case CODEC_GAIN_TBL:
         ACDB_MEM_CPY((void *)&devId, sizeof(uint32_t), (void *)pIndices, sizeof(uint32_t));
         noOfLutIndices = CODEC_PP_GAIN_LUT_INDICES_COUNT;
         noOfTableIndices = CODEC_PP_GAIN_TABLE_INDICES_COUNT;
         break;
      default:
         ACDB_DEBUG_LOG("Invalid table passed to get the acdb data\n");
         return ACDB_ERROR;
      }
      if (nIdxCount != noOfTableIndices)
      {
         ACDB_DEBUG_LOG("Invalid indices passed to get the acdb data\n");
         return ACDB_ERROR;
      }
      cmd.devId = devId;
      cmd.tblId = tblId;

      if (tblId == VOCPROC_DYNAMIC_TBL || tblId == VOCPROC_STATIC_TBL)
      {
         result = acdbdata_ioctl(ACDBDATACMD_GET_TABLE_INFO, (uint8_t *)&cmd, sizeof(cmd),
            (uint8_t *)&cvdTblInfo, sizeof(cvdTblInfo));

         if (result != ACDB_SUCCESS)
         {
            ACDB_DEBUG_LOG("Failed to fetch the lookup information of the device %08X \n"
               , devId);
            return result;
         }
         if (tblId == VOCPROC_DYNAMIC_TBL)
         {
            // treate dynamic and static tables seperatly as this will have different tables configuration than usual
            //VocProcVolV2CmdLookupType voccmd;
			 VocProcDynCmdLookupType pInputcvd = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
            VocProcDynDataLookupType_CommonData cvdcmd = { 0, 0, 0, 0, 0, 0, 0, 0 };
            VocProcDynDataLookupType_offsetData offsettbleinstance = { 0, 0 };
            //AcdbVocProcGainDepVolTblV2CmdType *pInput = (AcdbVocProcGainDepVolTblV2CmdType *)pIndices;
            if (pIndices != NULL)
               ACDB_MEM_CPY(&voccmd, sizeof(voccmd), (void*)pIndices, sizeof(voccmd));
            else
               return ACDB_ERROR;
            /*voccmd.nTxDevId = pInput->nTxDeviceId;
            voccmd.nRxDevId = pInput->nRxDeviceId;
            voccmd.nFeatureId = pInput->nFeatureId;*/
            lutTbl.nLen = READ_UInt32(cvdTblInfo.tblLutChnk.pData);
            lutTbl.pLut = (VocProcDynDataLookupType_UniqueData *)(cvdTblInfo.tblLutChnk.pData + sizeof(lutTbl.nLen));
            result = AcdbDataBinarySearch((void *)lutTbl.pLut, lutTbl.nLen,
               VOCPROCDYN_LUT_INDICES_COUNT - 8, &voccmd, VOCPROCDYN_CMD_INDICES_COUNT, &index);
            if (result != SEARCH_SUCCESS)
            {
               ACDB_DEBUG_LOG("Failed to fetch the lookup information of the device %08X \n"
                  , voccmd.nTxDevId);
               return ACDB_INPUT_PARAMS_NOT_FOUND;
            }
            nSecLookupKey = (uintptr_t)&lutTbl.pLut[index];
			ofTbloffset = lutTbl.pLut[index].nOffTableOffset;
			cvdtableoffset = lutTbl.pLut[index].nCommonTblOffset;
            lutTblCVD.nLen = READ_UInt32(cvdTblInfo.tblLutCVDChnk.pData + cvdtableoffset);
            lutTblCVD.pLut = (VocProcDynDataLookupType_CommonData *)(cvdTblInfo.tblLutCVDChnk.pData + cvdtableoffset + sizeof(lutTbl.nLen));

            if (lutTblCVD.nLen <= 0)
            {
               ACDB_DEBUG_LOG("%08X lutcvdTbl empty\n", tblId);
               return ACDB_ERROR;
            }
            ACDB_MEM_CPY(&pInputcvd, sizeof(VocProcDynCmdLookupType), (void *)pIndices, sizeof(VocProcDynCmdLookupType));
            /*if(pIndices!=NULL)
            ACDB_MEM_CPY(&pInputcvd,(void*)pIndices,sizeof(pInputcvd));
            else
            return ACDB_ERROR;*/
            cvdcmd.nNetwork = pInputcvd.nNetworkId;
            cvdcmd.nRxVocSR = pInputcvd.nRxDeviceSampleRateId;
            cvdcmd.nTxVocSR = pInputcvd.nTxDeviceSampleRateId;
            cvdcmd.nVocoder_type = pInputcvd.nVocoder_type;
            cvdcmd.nVocVolStep = pInputcvd.nVoiceVolumeStep;
            cvdcmd.rx_Voc_mode = pInputcvd.rx_Voc_mode;
            cvdcmd.tx_Voc_mode = pInputcvd.tx_Voc_mode;
            cvdcmd.nFeature = pInputcvd.nFeature;
            result = AcdbDataBinarySearch((void *)lutTblCVD.pLut, lutTblCVD.nLen, VOCPROCDYN_LUT_INDICES_COUNT - 5,
               &cvdcmd, VOCPROCDYN_LUT_INDICES_COUNT - 5, &index);//perform lookup in cvdcommon table
            if (result != SEARCH_SUCCESS)
            {
               ACDB_DEBUG_LOG("Failed to fetch the cvd lookup information of the device %08X \n"
                  , voccmd.nTxDevId);
               return ACDB_INPUT_PARAMS_NOT_FOUND;
            }
            lutoffsetTbl.nLen = READ_UInt32(cvdTblInfo.tblLutOffsetChnk.pData);
            lutoffsetTbl.pLut = (VocProcDynDataLookupType_offsetData *)(cvdTblInfo.tblLutOffsetChnk.pData + sizeof(lutTbl.nLen));
            ACDB_MEM_CPY(&noOfLUTEntries, sizeof(uint32_t), cvdTblInfo.tblLutChnk.pData, sizeof(uint32_t));
            lutoffsetTbl.pLut = (VocProcDynDataLookupType_offsetData *)(cvdTblInfo.tblLutOffsetChnk.pData + sizeof(uint32_t) + ofTbloffset);
            ACDB_MEM_CPY(&offsettbleinstance, sizeof(VocProcDynDataLookupType_offsetData), &lutoffsetTbl.pLut[index], sizeof(VocProcDynDataLookupType_offsetData));

            nLookupKey = (uintptr_t)&(lutTblCVD.pLut[index]);// calculate the lookup key based on the offset table
            cdefoffsetval = offsettbleinstance.nCDEFTblOffset;
            cdefTbl.nLen = READ_UInt32(cvdTblInfo.tblCdftChnk.pData + cdefoffsetval);
            cdefTbl.pCntDef = (ContentDefType *)(cvdTblInfo.tblCdftChnk.pData + cdefoffsetval + sizeof(cdefTbl.nLen));

            cdotoffsetval = offsettbleinstance.nCDOTTblOffset;
            cdotTbl.nLen = READ_UInt32(cvdTblInfo.tblCdotChnk.pData + cdotoffsetval);
            cdotTbl.pDataOffsets = (uint32_t *)(cvdTblInfo.tblCdotChnk.pData + cdotoffsetval + sizeof(cdotTbl.nLen));

            if (cdefTbl.nLen != cdotTbl.nLen)
            {
               ACDB_DEBUG_LOG("The no of entries in CDEF and CDOT tables for devid %08X not matching\n", devId);
               return ACDB_ERROR;
            }

            if (ACDB_SUCCESS != GetMidPidCalibCVDData(tblId, nLookupKey, nSecLookupKey, cdefTbl, cdotTbl, cvdTblInfo.dataPoolChnk, mid,
               pid, pBuff, nBuffLen, pBuffBytesFilled))
            {
               ACDB_DEBUG_LOG("ACDB-ACDBCMDGETOnlinedata- failed GetMidPidCalibdata for lookupkey %016lx", (unsigned long)nLookupKey);
               return ACDB_ERROR;
            }
         }
         else if (tblId == VOCPROC_STATIC_TBL)
         {
            VocProcCmdLookupType voccmd2 = { 0, 0, 0, 0 };
			VocProcStatCmdLookupType pInputcvd = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
            VocProcStatDataLookupType_CommonData cvdcmd = { 0, 0, 0, 0, 0, 0, 0 };
            VocProcStatDataLookupType_offsetData offsettbleinstance = { 0, 0 };
            //AcdbVocProcCmnTblCmdType *pInput = (AcdbVocProcCmnTblCmdType *)pIndices;
            if (pIndices != NULL)
               ACDB_MEM_CPY(&voccmd2, sizeof(voccmd2), (void*)pIndices, sizeof(voccmd2));
            else
               return ACDB_ERROR;
            /*voccmd2.nTxDevId = pInput->nTxDeviceId;
            voccmd2.nRxDevId = pInput->nRxDeviceId;
            voccmd2.nRxAfeSr = pInput->nRxDeviceSampleRateId;
            voccmd2.nTxAfeSr = pInput->nTxDeviceSampleRateId;*/
            lutStatTbl.nLen = READ_UInt32(cvdTblInfo.tblLutChnk.pData);
            //lutStatTbl.nLen = *((uint32_t *)cvdTblInfo.tblLutChnk.pData);
            lutStatTbl.pLut = (VocProcStatDataLookupType_UniqueData *)(cvdTblInfo.tblLutChnk.pData + sizeof(lutStatTbl.nLen));
            result = AcdbDataBinarySearch((void *)lutStatTbl.pLut, lutStatTbl.nLen,
               VOCPROCSTAT_LUT_INDICES_COUNT - 7, &voccmd2, VOCPROCSTAT_CMD_INDICES_COUNT, &index);
            if (result != SEARCH_SUCCESS)
            {
               ACDB_DEBUG_LOG("Failed to fetch the lookup information of the device %08X \n"
                  , voccmd2.nTxDevId);
               return ACDB_INPUT_PARAMS_NOT_FOUND;
            }
            nSecLookupKey = (uintptr_t)&lutStatTbl.pLut[index];
            ofTbloffset = lutStatTbl.pLut[index].nOffTableOffset;
			cvdtableoffset = lutStatTbl.pLut[index].nCommonTblOffset;
            lutStatTblCVD.nLen = READ_UInt32(cvdTblInfo.tblLutCVDChnk.pData + cvdtableoffset);
            lutStatTblCVD.pLut = (VocProcStatDataLookupType_CommonData *)(cvdTblInfo.tblLutCVDChnk.pData + cvdtableoffset + sizeof(lutTbl.nLen));
            if (lutStatTblCVD.nLen <= 0)
            {
               ACDB_DEBUG_LOG("%08X lutcvdTbl empty\n", tblId);
               return ACDB_ERROR;
            }
            ACDB_MEM_CPY(&pInputcvd, sizeof(VocProcStatCmdLookupType), (void *)pIndices, sizeof(VocProcStatCmdLookupType));
            /*if(pIndices!=NULL)
            ACDB_MEM_CPY(&pInputcvd,(void*)pIndices,sizeof(pInputcvd));
            else
            return ACDB_ERROR;
            ACDB_DEBUG_LOG("* nNetworkId: %p, nRxDeviceSampleRateId: %p, nTxDeviceSampleRateId: %p, nVocoder_type: %p, rx_Voc_mode: %p, tx_Voc_mode: %p, nFeature: %p\n",
            pInputcvd->nNetworkId, pInputcvd->nRxDeviceSampleRateId, pInputcvd->nTxDeviceSampleRateId, pInputcvd->nVocoder_type, pInputcvd->rx_Voc_mode,
            pInputcvd->tx_Voc_mode, pInputcvd->nFeature);*/
            cvdcmd.nNetwork = pInputcvd.nNetworkId;
            cvdcmd.nRxVocSR = pInputcvd.nRxDeviceSampleRateId;
            cvdcmd.nTxVocSR = pInputcvd.nTxDeviceSampleRateId;
            cvdcmd.nVocoder_type = pInputcvd.nVocoder_type;
            cvdcmd.rx_Voc_mode = pInputcvd.rx_Voc_mode;
            cvdcmd.tx_Voc_mode = pInputcvd.tx_Voc_mode;
            cvdcmd.nFeature = pInputcvd.nFeature;

            result = AcdbDataBinarySearch((void *)lutStatTblCVD.pLut, lutStatTblCVD.nLen, VOCPROCSTAT_LUT_INDICES_COUNT - 6,
               &cvdcmd, VOCPROCSTAT_LUT_INDICES_COUNT - 6, &index);//perform lookup in cvdcommon table
            if (result != SEARCH_SUCCESS)
            {
               ACDB_DEBUG_LOG("Failed to fetch the cvd lookup information of the device %08X \n"
                  , voccmd2.nTxDevId);
               return ACDB_INPUT_PARAMS_NOT_FOUND;
            }
            lutStatoffsetTbl.nLen = READ_UInt32(cvdTblInfo.tblLutOffsetChnk.pData);
            lutStatoffsetTbl.pLut = (VocProcStatDataLookupType_offsetData *)(cvdTblInfo.tblLutOffsetChnk.pData + sizeof(lutTbl.nLen));
            ACDB_MEM_CPY(&noOfLUTEntries, sizeof(uint32_t), cvdTblInfo.tblLutChnk.pData, sizeof(uint32_t));
            lutStatoffsetTbl.pLut = (VocProcStatDataLookupType_offsetData *)(cvdTblInfo.tblLutOffsetChnk.pData + sizeof(uint32_t) + ofTbloffset);
            ACDB_MEM_CPY(&offsettbleinstance, sizeof(VocProcStatDataLookupType_offsetData), &lutStatoffsetTbl.pLut[index], sizeof(VocProcStatDataLookupType_offsetData));
            nLookupKey = (uintptr_t)&lutStatTblCVD.pLut[index];// calculate the lookup key based on the offset table
            cdefoffsetval = offsettbleinstance.nCDEFTblOffset;
            cdefTbl.nLen = READ_UInt32(cvdTblInfo.tblCdftChnk.pData + cdefoffsetval);
            cdefTbl.pCntDef = (ContentDefType *)(cvdTblInfo.tblCdftChnk.pData + cdefoffsetval + sizeof(cdefTbl.nLen));
            cdotoffsetval = offsettbleinstance.nCDOTTblOffset;
            cdotTbl.nLen = READ_UInt32(cvdTblInfo.tblCdotChnk.pData + cdotoffsetval);

            cdotTbl.pDataOffsets = (uint32_t *)(cvdTblInfo.tblCdotChnk.pData + cdotoffsetval + sizeof(cdotTbl.nLen));
            if (cdefTbl.nLen != cdotTbl.nLen)
            {
               ACDB_DEBUG_LOG("The no of entries in CDEF and CDOT tables for devid %08X not matching\n", devId);
               return ACDB_ERROR;
            }
            if (ACDB_SUCCESS != GetMidPidCalibCVDData(tblId, nLookupKey, nSecLookupKey, cdefTbl, cdotTbl, cvdTblInfo.dataPoolChnk, mid,
               pid, pBuff, nBuffLen, pBuffBytesFilled))
            {
               ACDB_DEBUG_LOG("[ACDB static Getonline]->GetMidPidCalibData failed %016lx \n", (unsigned long)nLookupKey);
               return ACDB_ERROR;
            }
         }
      }
      else if (tblId == VOCPROC_DYN_INST_TBL_ID || tblId == VOCPROC_STAT_INST_TBL_ID)
      {
         result = acdbdata_ioctl(ACDBDATACMD_GET_TABLE_INFO, (uint8_t *)&cmd, sizeof(cmd),
            (uint8_t *)&cvdTblInfo, sizeof(cvdTblInfo));

         if (result != ACDB_SUCCESS)
         {
            ACDB_DEBUG_LOG("Failed to fetch the lookup information of the device %08X \n"
               , devId);
            return result;
         }
         if (tblId == VOCPROC_DYN_INST_TBL_ID)
         {
            // treate dynamic and static tables seperatly as this will have different tables configuration than usual
            //VocProcVolV2CmdLookupType voccmd;
            VocProcDynCmdLookupType *pInputcvd = NULL;
            VocProcDynDataLookupType_CommonData cvdcmd = { 0, 0, 0, 0, 0, 0, 0, 0 };
            VocProcDynDataLookupType_offsetData offsettbleinstance = { 0, 0 };
            //AcdbVocProcGainDepVolTblV2CmdType *pInput = (AcdbVocProcGainDepVolTblV2CmdType *)pIndices;
            if (pIndices != NULL)
               ACDB_MEM_CPY(&voccmd, sizeof(voccmd), (void*)pIndices, sizeof(voccmd));
            else
               return ACDB_ERROR;
            /*voccmd.nTxDevId = pInput->nTxDeviceId;
            voccmd.nRxDevId = pInput->nRxDeviceId;
            voccmd.nFeatureId = pInput->nFeatureId;*/
            lutTbl.nLen = READ_UInt32(cvdTblInfo.tblLutChnk.pData);
            lutTbl.pLut = (VocProcDynDataLookupType_UniqueData *)(cvdTblInfo.tblLutChnk.pData + sizeof(lutTbl.nLen));
            result = AcdbDataBinarySearch((void *)lutTbl.pLut, lutTbl.nLen,
               VOCPROCDYN_INSTANCE_LUT_INDICES_COUNT - 8, &voccmd, VOCPROCDYN_INSTANCE_CMD_INDICES_COUNT, &index);
            if (result != SEARCH_SUCCESS)
            {
               ACDB_DEBUG_LOG("Failed to fetch the lookup information of the device %08X \n"
                  , voccmd.nTxDevId);
               return ACDB_INPUT_PARAMS_NOT_FOUND;
            }
            nSecLookupKey = (uintptr_t)&lutTbl.pLut[index];
            ofTbloffset = lutTbl.pLut[index].nOffTableOffset;
			cvdtableoffset = lutTbl.pLut[index].nCommonTblOffset;
            lutTblCVD.nLen = READ_UInt32(cvdTblInfo.tblLutCVDChnk.pData + cvdtableoffset);
            lutTblCVD.pLut = (VocProcDynDataLookupType_CommonData *)(cvdTblInfo.tblLutCVDChnk.pData + cvdtableoffset + sizeof(lutTbl.nLen));

            if (lutTblCVD.nLen <= 0)
            {
               ACDB_DEBUG_LOG("%08X lutcvdTbl empty\n", tblId);
               return ACDB_ERROR;
            }
            pInputcvd = (VocProcDynCmdLookupType *)pIndices;
            /*if(pIndices!=NULL)
            ACDB_MEM_CPY(&pInputcvd,(void*)pIndices,sizeof(pInputcvd));
            else
            return ACDB_ERROR;*/
            cvdcmd.nNetwork = pInputcvd->nNetworkId;
            cvdcmd.nRxVocSR = pInputcvd->nRxDeviceSampleRateId;
            cvdcmd.nTxVocSR = pInputcvd->nTxDeviceSampleRateId;
            cvdcmd.nVocoder_type = pInputcvd->nVocoder_type;
            cvdcmd.nVocVolStep = pInputcvd->nVoiceVolumeStep;
            cvdcmd.rx_Voc_mode = pInputcvd->rx_Voc_mode;
            cvdcmd.tx_Voc_mode = pInputcvd->tx_Voc_mode;
            cvdcmd.nFeature = pInputcvd->nFeature;
            result = AcdbDataBinarySearch((void *)lutTblCVD.pLut, lutTblCVD.nLen, VOCPROCDYN_INSTANCE_LUT_INDICES_COUNT - 5,
               &cvdcmd, VOCPROCDYN_INSTANCE_LUT_INDICES_COUNT - 5, &index);//perform lookup in cvdcommon table
            if (result != SEARCH_SUCCESS)
            {
               ACDB_DEBUG_LOG("Failed to fetch the cvd lookup information of the device %08X \n"
                  , voccmd.nTxDevId);
               return ACDB_INPUT_PARAMS_NOT_FOUND;
            }
            lutoffsetTbl.nLen = READ_UInt32(cvdTblInfo.tblLutOffsetChnk.pData);
            lutoffsetTbl.pLut = (VocProcDynDataLookupType_offsetData *)(cvdTblInfo.tblLutOffsetChnk.pData + sizeof(lutTbl.nLen));
            ACDB_MEM_CPY(&noOfLUTEntries, sizeof(uint32_t), cvdTblInfo.tblLutChnk.pData, sizeof(uint32_t));
            lutoffsetTbl.pLut = (VocProcDynDataLookupType_offsetData *)(cvdTblInfo.tblLutOffsetChnk.pData + sizeof(uint32_t) + ofTbloffset);
            ACDB_MEM_CPY(&offsettbleinstance, sizeof(VocProcDynDataLookupType_offsetData), &lutoffsetTbl.pLut[index], sizeof(VocProcDynDataLookupType_offsetData));

            nLookupKey = (uintptr_t)&(lutTblCVD.pLut[index]);// calculate the lookup key based on the offset table
            cdefoffsetval = offsettbleinstance.nCDEFTblOffset;
            cdefInstanceTbl.nLen = READ_UInt32(cvdTblInfo.tblCdftChnk.pData + cdefoffsetval);
            cdefInstanceTbl.pCntDef = (ContentDefInstanceType *)(cvdTblInfo.tblCdftChnk.pData + cdefoffsetval + sizeof(cdefInstanceTbl.nLen));

            cdotoffsetval = offsettbleinstance.nCDOTTblOffset;
            cdotTbl.nLen = READ_UInt32(cvdTblInfo.tblCdotChnk.pData + cdotoffsetval);
            cdotTbl.pDataOffsets = (uint32_t *)(cvdTblInfo.tblCdotChnk.pData + cdotoffsetval + sizeof(cdotTbl.nLen));

            if (cdefInstanceTbl.nLen != cdotTbl.nLen)
            {
               ACDB_DEBUG_LOG("The no of entries in CDEF and CDOT tables for devid %08X not matching\n", devId);
               return ACDB_ERROR;
            }

            if (ACDB_SUCCESS != GetMidIidPidCalibCVDData(tblId, nLookupKey, nSecLookupKey, cdefInstanceTbl, cdotTbl, cvdTblInfo.dataPoolChnk, mid, iid,
               pid, pBuff, nBuffLen, pBuffBytesFilled))
            {
               ACDB_DEBUG_LOG("ACDB-ACDBCMDGETOnlinedata- failed GetMidPidCalibdata for lookupkey %016lx", (unsigned long)nLookupKey);
               return ACDB_ERROR;
            }
         }
         else if (tblId == VOCPROC_STAT_INST_TBL_ID)
         {
            VocProcCmdLookupType voccmd2 = { 0, 0, 0, 0 };
            VocProcStatCmdLookupType *pInputcvd = NULL;
            VocProcStatDataLookupType_CommonData cvdcmd = { 0, 0, 0, 0, 0, 0, 0 };
            VocProcStatDataLookupType_offsetData offsettbleinstance = { 0, 0 };
            //AcdbVocProcCmnTblCmdType *pInput = (AcdbVocProcCmnTblCmdType *)pIndices;
            if (pIndices != NULL)
               ACDB_MEM_CPY(&voccmd2, sizeof(voccmd2), (void*)pIndices, sizeof(voccmd2));
            else
               return ACDB_ERROR;
            /*voccmd2.nTxDevId = pInput->nTxDeviceId;
            voccmd2.nRxDevId = pInput->nRxDeviceId;
            voccmd2.nRxAfeSr = pInput->nRxDeviceSampleRateId;
            voccmd2.nTxAfeSr = pInput->nTxDeviceSampleRateId;*/
            lutStatTbl.nLen = READ_UInt32(cvdTblInfo.tblLutChnk.pData);
            //lutStatTbl.nLen = *((uint32_t *)cvdTblInfo.tblLutChnk.pData);
            lutStatTbl.pLut = (VocProcStatDataLookupType_UniqueData *)(cvdTblInfo.tblLutChnk.pData + sizeof(lutStatTbl.nLen));
            result = AcdbDataBinarySearch((void *)lutStatTbl.pLut, lutStatTbl.nLen,
               VOCPROCSTAT_INSTANCE_LUT_INDICES_COUNT - 7, &voccmd2, VOCPROCSTAT_INSTANCE_CMD_INDICES_COUNT, &index);
            if (result != SEARCH_SUCCESS)
            {
               ACDB_DEBUG_LOG("Failed to fetch the lookup information of the device %08X \n"
                  , voccmd2.nTxDevId);
               return ACDB_INPUT_PARAMS_NOT_FOUND;
            }
            nSecLookupKey = (uintptr_t)&lutStatTbl.pLut[index];
            ofTbloffset = lutStatTbl.pLut[index].nOffTableOffset;
			cvdtableoffset = lutStatTbl.pLut[index].nCommonTblOffset;
            lutStatTblCVD.nLen = READ_UInt32(cvdTblInfo.tblLutCVDChnk.pData + cvdtableoffset);
            lutStatTblCVD.pLut = (VocProcStatDataLookupType_CommonData *)(cvdTblInfo.tblLutCVDChnk.pData + cvdtableoffset + sizeof(lutTbl.nLen));
            if (lutStatTblCVD.nLen <= 0)
            {
               ACDB_DEBUG_LOG("%08X lutcvdTbl empty\n", tblId);
               return ACDB_ERROR;
            }
            pInputcvd = (VocProcStatCmdLookupType *)pIndices;
            /*if(pIndices!=NULL)
            ACDB_MEM_CPY(&pInputcvd,(void*)pIndices,sizeof(pInputcvd));
            else
            return ACDB_ERROR;*/
            cvdcmd.nNetwork = pInputcvd->nNetworkId;
            cvdcmd.nRxVocSR = pInputcvd->nRxDeviceSampleRateId;
            cvdcmd.nTxVocSR = pInputcvd->nTxDeviceSampleRateId;
            cvdcmd.nVocoder_type = pInputcvd->nVocoder_type;
            cvdcmd.rx_Voc_mode = pInputcvd->rx_Voc_mode;
            cvdcmd.tx_Voc_mode = pInputcvd->tx_Voc_mode;
            cvdcmd.nFeature = pInputcvd->nFeature;

            result = AcdbDataBinarySearch((void *)lutStatTblCVD.pLut, lutStatTblCVD.nLen, VOCPROCSTAT_INSTANCE_LUT_INDICES_COUNT - 6,
               &cvdcmd, VOCPROCSTAT_INSTANCE_LUT_INDICES_COUNT - 6, &index);//perform lookup in cvdcommon table
            if (result != SEARCH_SUCCESS)
            {
               ACDB_DEBUG_LOG("Failed to fetch the cvd lookup information of the device %08X \n"
                  , voccmd2.nTxDevId);
               return ACDB_INPUT_PARAMS_NOT_FOUND;
            }
            lutStatoffsetTbl.nLen = READ_UInt32(cvdTblInfo.tblLutOffsetChnk.pData);
            lutStatoffsetTbl.pLut = (VocProcStatDataLookupType_offsetData *)(cvdTblInfo.tblLutOffsetChnk.pData + sizeof(lutTbl.nLen));
            ACDB_MEM_CPY(&noOfLUTEntries, sizeof(uint32_t), cvdTblInfo.tblLutChnk.pData, sizeof(uint32_t));
            lutStatoffsetTbl.pLut = (VocProcStatDataLookupType_offsetData *)(cvdTblInfo.tblLutOffsetChnk.pData + sizeof(uint32_t) + ofTbloffset);
            ACDB_MEM_CPY(&offsettbleinstance, sizeof(VocProcStatDataLookupType_offsetData), &lutStatoffsetTbl.pLut[index], sizeof(VocProcStatDataLookupType_offsetData));
            nLookupKey = (uintptr_t)&lutStatTblCVD.pLut[index];// calculate the lookup key based on the offset table
            cdefoffsetval = offsettbleinstance.nCDEFTblOffset;
            cdefInstanceTbl.nLen = READ_UInt32(cvdTblInfo.tblCdftChnk.pData + cdefoffsetval);
            cdefInstanceTbl.pCntDef = (ContentDefInstanceType *)(cvdTblInfo.tblCdftChnk.pData + cdefoffsetval + sizeof(cdefInstanceTbl.nLen));
            cdotoffsetval = offsettbleinstance.nCDOTTblOffset;
            cdotTbl.nLen = READ_UInt32(cvdTblInfo.tblCdotChnk.pData + cdotoffsetval);

            cdotTbl.pDataOffsets = (uint32_t *)(cvdTblInfo.tblCdotChnk.pData + cdotoffsetval + sizeof(cdotTbl.nLen));
            if (cdefInstanceTbl.nLen != cdotTbl.nLen)
            {
               ACDB_DEBUG_LOG("The no of entries in CDEF and CDOT tables for devid %08X not matching\n", devId);
               return ACDB_ERROR;
            }
            if (ACDB_SUCCESS != GetMidIidPidCalibCVDData(tblId, nLookupKey, nSecLookupKey, cdefInstanceTbl, cdotTbl, cvdTblInfo.dataPoolChnk, mid, iid,
               pid, pBuff, nBuffLen, pBuffBytesFilled))
            {
               ACDB_DEBUG_LOG("[ACDB static Getonline]->GetMidPidCalibData failed %016lx \n", (unsigned long)nLookupKey);
               return ACDB_ERROR;
            }
         }
      }
      else
      {
         result = acdbdata_ioctl(ACDBDATACMD_GET_TABLE_INFO, (uint8_t *)&cmd, sizeof(cmd),
            (uint8_t *)&tblInfo, sizeof(tblInfo));
         if (result != ACDB_SUCCESS)
         {
            ACDB_DEBUG_LOG("Failed to fetch the lookup information of the device %08X \n"
               , devId);
            return result;
         }
         ACDB_MEM_CPY(&noOfLUTEntries, sizeof(uint32_t), tblInfo.tblLutChnk.pData, sizeof(uint32_t));
         pLut = (uint8_t *)(tblInfo.tblLutChnk.pData + sizeof(noOfLUTEntries));

         result = AcdbDataBinarySearch((void *)pLut, noOfLUTEntries,
            noOfLutIndices, pIndices, noOfTableIndices, &index);
         if (result != SEARCH_SUCCESS)
         {
            ACDB_DEBUG_LOG("Failed to fetch the lookup information of the device %08X \n"
               , devId);
            return ACDB_ERROR;
         }

         nLookupKey = (uintptr_t)(pLut + (noOfLutIndices*index*(uint32_t)sizeof(uint32_t)));

         if (nonModuleTblFound == 0)
         {
            // Now get CDEF info
            cdefoffset = ((noOfLutIndices*index*(uint32_t)sizeof(uint32_t)) + (noOfTableIndices*(uint32_t)sizeof(uint32_t)));
            cdefoffsetval = READ_UInt32(pLut + cdefoffset);

            // Now get CDOT info
            cdotoffset = (uint32_t)(cdefoffset + sizeof(uint32_t));
            cdotoffsetval = READ_UInt32(pLut + cdotoffset);
            cdotTbl.nLen = READ_UInt32(tblInfo.tblCdotChnk.pData + cdotoffsetval);

            cdotTbl.pDataOffsets = (uint32_t *)(tblInfo.tblCdotChnk.pData + cdotoffsetval + sizeof(cdotTbl.nLen));

            if (noOfCdftIndices == NON_INSTANCE_CDFT_INDICES_COUNT)
            {
               cdefTbl.nLen = READ_UInt32(tblInfo.tblCdftChnk.pData + cdefoffsetval);
               cdefTbl.pCntDef = (ContentDefType *)(tblInfo.tblCdftChnk.pData + cdefoffsetval + sizeof(cdefTbl.nLen));
               if (cdefTbl.nLen != cdotTbl.nLen)
               {
                  ACDB_DEBUG_LOG("The no of entries in CDEF and CDOT tables for devid %08X not matching\n", devId);
                  return ACDB_ERROR;
               }
               if (ACDB_SUCCESS != GetMidPidCalibData(tblId, nLookupKey, cdefTbl, cdotTbl, tblInfo.dataPoolChnk, mid,
                  pid, pBuff, nBuffLen, pBuffBytesFilled))
               {
                  return ACDB_ERROR;
               }
            }
            else if (noOfCdftIndices == INSTANCE_CDFT_INDICES_COUNT)
            {
               cdefInstanceTbl.nLen = READ_UInt32(tblInfo.tblCdftChnk.pData + cdefoffsetval);
               cdefInstanceTbl.pCntDef = (ContentDefInstanceType *)(tblInfo.tblCdftChnk.pData + cdefoffsetval + sizeof(cdefInstanceTbl.nLen));
               if (cdefInstanceTbl.nLen != cdotTbl.nLen)
               {
                  ACDB_DEBUG_LOG("The no of entries in CDEF and CDOT tables for devid %08X not matching\n", devId);
                  return ACDB_ERROR;
               }
               if (ACDB_SUCCESS != GetMidIidPidCalibData(tblId, nLookupKey, cdefInstanceTbl, cdotTbl, tblInfo.dataPoolChnk, mid, iid,
                  pid, pBuff, nBuffLen, pBuffBytesFilled))
               {
                  return ACDB_ERROR;
               }
            }
         }
         else
         {
            uint32_t dataoffset = READ_UInt32(pLut + (noOfLutIndices*index*(uint32_t)sizeof(uint32_t)) + (noOfTableIndices*(uint32_t)sizeof(uint32_t)));

            if (ACDB_SUCCESS != GetCalibData(tblId, nLookupKey, dataoffset, tblInfo.dataPoolChnk,
               pBuff, nBuffLen, pBuffBytesFilled))
            {
               return ACDB_ERROR;
            }
         }
      }

      result = ACDB_SUCCESS;
   }

   return result;
}

int32_t AcdbCmdSetOnlineDataV2(uint32_t persistData, const uint32_t tblId, uint8_t *pIndices, uint32_t nIdxCount, uint32_t noOfCdftIndices,
   uint8_t *pInBuff, uint32_t nInBuffLen)
{
   int32_t result = ACDB_SUCCESS;

   if (pInBuff == NULL || nInBuffLen == 0)
   {
      ACDB_DEBUG_LOG("[ACDB Command]->Invalid NULL value parameters are provided\n");
      result = ACDB_BADPARM;
   }
   else
   {
      uint32_t index = 0;
      AcdbTableCmd cmd = { 0, 0 };
      AcdbTableInfo tblInfo = { { 0, NULL }, { 0, NULL }, { 0, NULL } };
      uint32_t devId = 0;
      uint32_t noOfLUTEntries = 0;
      uint8_t *pLut = NULL;
      uint32_t noOfLutIndices = 0;
      uint32_t noOfTableIndices = 0;
      ContentDefTblType cdefTbl = { 0, NULL };
      ContentDefWithInstanceTblType cdeftInstanceTbl = { 0, NULL };
      ContentDataOffsetsTblType cdotTbl = { 0, NULL };
      uintptr_t nLookupKey = 0, nSecLookupKey = 0;
      uint32_t cdefoffset = 0, cdotoffset = 0;
      uint32_t cdefoffsetval = 0, cdotoffsetval = 0;
      uint8_t nonModuleTblFound = 0;

      uint32_t deltaFileIndex = 0;
	  
	  VocProcDynDataLookupType_UniqueData tempLutDyn = { 0, 0, 0, 0, 0 };
      VocProcDynDataLookupTblType_UniqueData lutTbl = { 0, NULL };
      VocProcDynDataLookupTblType_CommonData lutTblCVD = { 0, NULL };
      VocProcDynDataLookupTblType_offsetData lutoffsetTbl = { 0, NULL };
	  VocProcStatDataLookupType_UniqueData tempLutStat  = { 0, 0, 0, 0, 0, 0};
      VocProcStatDataLookupTblType_UniqueData lutStatTbl = { 0, NULL };
      VocProcStatDataLookupTblType_CommonData lutStatTblCVD = { 0, NULL };
      VocProcStatDataLookupTblType_offsetData lutStatoffsetTbl = { 0, NULL };
      AcdbTableInfoCVD cvdTblInfo = { { 0, NULL }, { 0, NULL }, { 0, NULL }, { 0, NULL }, { 0, NULL }, { 0, NULL } };
      uint32_t ofTbloffset = 0, cvdtableoffset = 0, mid = 0, iid = 0, pid = 0;
      uint32_t offset = (uint32_t)(nIdxCount*sizeof(uint32_t));
      mid = READ_UInt32(pIndices + offset);
      offset += (uint32_t)sizeof(uint32_t);
      if (noOfCdftIndices == INSTANCE_CDFT_INDICES_COUNT)
      {
         iid = READ_UInt32(pIndices + offset);
         offset += (uint32_t)sizeof(uint32_t);
      }
      pid = READ_UInt32(pIndices + offset);
      offset += (uint32_t)sizeof(uint32_t);

      switch (tblId)
      {
      case AUDPROC_GAIN_INDP_TBL:
         ACDB_MEM_CPY((void *)&devId, sizeof(uint32_t), (void *)pIndices, sizeof(uint32_t));
         noOfLutIndices = AUDPROC_LUT_INDICES_COUNT;
         noOfTableIndices = AUDPROCTBL_INDICES_COUNT;
         break;
      case AUDPROC_COPP_GAIN_DEP_TBL:
         ACDB_MEM_CPY((void *)&devId, sizeof(uint32_t), (void *)pIndices, sizeof(uint32_t));
         noOfLutIndices = AUDPROC_GAIN_DEP_LUT_INDICES_COUNT;
         noOfTableIndices = AUDPROCTBL_GAIN_DEP_INDICES_COUNT;
         break;
      case AUDPROC_AUD_VOL_TBL:
         ACDB_MEM_CPY((void *)&devId, sizeof(uint32_t), (void *)pIndices, sizeof(uint32_t));
         noOfLutIndices = AUDPROC_VOL_LUT_INDICES_COUNT;
         noOfTableIndices = AUDPROCTBL_VOL_INDICES_COUNT;
         break;
      case AUD_STREAM_TBL:
         devId = 0;
         noOfLutIndices = AUDSTREAM_LUT_INDICES_COUNT;
         noOfTableIndices = AUDSTREAMTBL_INDICES_COUNT;
         break;
      case VOCPROC_GAIN_INDP_TBL:
         ACDB_MEM_CPY((void *)&devId, sizeof(uint32_t), (void *)pIndices, sizeof(uint32_t));
         noOfLutIndices = VOCPROC_LUT_INDICES_COUNT;
         noOfTableIndices = VOCPROCTBL_INDICES_COUNT;
         break;
      case VOCPROC_COPP_GAIN_DEP_TBL:
         ACDB_MEM_CPY((void *)&devId, sizeof(uint32_t), (void *)pIndices, sizeof(uint32_t));
         noOfLutIndices = VOCPROC_VOL_LUT_INDICES_COUNT;
         noOfTableIndices = VOCPROCTBL_VOL_INDICES_COUNT;
         break;
      case VOC_STREAM_TBL:
         devId = 0;
         noOfLutIndices = VOCSTREAM_LUT_INDICES_COUNT;
         noOfTableIndices = VOCSTREAMTBL_INDICES_COUNT;
         break;
      case AFE_TBL:
         ACDB_MEM_CPY((void *)&devId, sizeof(uint32_t), (void *)pIndices, sizeof(uint32_t));
         noOfLutIndices = AFE_LUT_INDICES_COUNT;
         noOfTableIndices = AFETBL_INDICES_COUNT;
         break;
      case AFE_CMN_TBL:
         ACDB_MEM_CPY((void *)&devId, sizeof(uint32_t), (void *)pIndices, sizeof(uint32_t));
         noOfLutIndices = AFECMN_LUT_INDICES_COUNT;
         noOfTableIndices = AFECMNTBL_INDICES_COUNT;
         break;
      case VOCPROC_DEV_CFG_TBL:
         ACDB_MEM_CPY((void *)&devId, sizeof(uint32_t), (void *)pIndices, sizeof(uint32_t));
         noOfLutIndices = VOCPROCDEVCFG_LUT_INDICES_COUNT;
         noOfTableIndices = VOCPROCDEVCFGTBL_INDICES_COUNT;
         break;
      case LSM_TBL:
         ACDB_MEM_CPY((void *)&devId, sizeof(uint32_t), (void *)pIndices, sizeof(uint32_t));
         noOfLutIndices = LSM_LUT_INDICES_COUNT;
         noOfTableIndices = LSM_INDICES_COUNT;
         break;
      case ADIE_SIDETONE_TBL:
         ACDB_MEM_CPY((void *)&devId, sizeof(uint32_t), (void *)pIndices, sizeof(uint32_t));
         noOfLutIndices = ADST_LUT_INDICES_COUNT;
         noOfTableIndices = ADST_INDICES_COUNT;
         break;
      case AANC_CFG_TBL:
         ACDB_MEM_CPY((void *)&devId, sizeof(uint32_t), (void *)pIndices, sizeof(uint32_t));
         noOfLutIndices = AANC_CFG_LUT_INDICES_COUNT;
         noOfTableIndices = AANC_CFG_TBL_INDICES_COUNT;
         break;
      case ADIE_ANC_TBL:
         ACDB_MEM_CPY((void *)&devId, sizeof(uint32_t), (void *)pIndices, sizeof(uint32_t));
         noOfLutIndices = ANC_LUT_INDICES_COUNT;
         noOfTableIndices = ANCTBL_INDICES_COUNT;
         nonModuleTblFound = 1;
         break;
      case ADIE_CODEC_TBL:
         devId = 0;
         noOfLutIndices = ANC_LUT_INDICES_COUNT;
         noOfTableIndices = ANCTBL_INDICES_COUNT;
         nonModuleTblFound = 1;
         break;
      case GLOBAL_DATA_TBL:
         devId = 0;
         noOfLutIndices = GLOBAL_LUT_INDICES_COUNT;
         noOfTableIndices = GLOBALTBL_INDICES_COUNT;
         nonModuleTblFound = 1;
         break;
      case CDC_FEATURES_TBL:
         ACDB_MEM_CPY((void *)&devId, sizeof(uint32_t), (void *)pIndices, sizeof(uint32_t));
         noOfLutIndices = CDC_FEATURES_DATA_LUT_INDICES_COUNT;
         noOfTableIndices = CDC_FEATURES_DATA_INDICES_COUNT;
         nonModuleTblFound = 1;
         break;
      case VOCPROC_COPP_GAIN_DEP_V2_TBL:
         ACDB_MEM_CPY((void *)&devId, sizeof(uint32_t), (void *)pIndices, sizeof(uint32_t));
         noOfLutIndices = VOCPROC_VOL_V2_LUT_INDICES_COUNT;
         noOfTableIndices = VOCPROCTBL_VOL_V2_INDICES_COUNT;

         break;
      case VOICE_VP3_TBL:
         ACDB_MEM_CPY((void *)&devId, sizeof(uint32_t), (void *)pIndices, sizeof(uint32_t));
         noOfLutIndices = VOICE_VP3_LUT_INDICES_COUNT;
         noOfTableIndices = VOICE_VP3TBL_INDICES_COUNT;
         break;
      case AUDIO_REC_VP3_TBL:
         ACDB_MEM_CPY((void *)&devId, sizeof(uint32_t), (void *)pIndices, sizeof(uint32_t));
         noOfLutIndices = AUDREC_VP3_LUT_INDICES_COUNT;
         noOfTableIndices = AUDREC_VP3TBL_INDICES_COUNT;
         break;
      case AUDIO_REC_EC_VP3_TBL:
         ACDB_MEM_CPY((void *)&devId, sizeof(uint32_t), (void *)pIndices, sizeof(uint32_t));
         noOfLutIndices = AUDREC_EC_VP3_LUT_INDICES_COUNT;
         noOfTableIndices = AUDREC_EC_VP3TBL_INDICES_COUNT;
         break;
      case METAINFO_LOOKUP_TBL:
         devId = 0;
         noOfLutIndices = MINFO_LUT_INDICES_COUNT;
         noOfTableIndices = MINFOTBL_INDICES_COUNT;
         nonModuleTblFound = 1;
         break;
      case VOCPROC_DYNAMIC_TBL:
         ACDB_MEM_CPY((void *)&devId, sizeof(uint32_t), (void *)pIndices, sizeof(uint32_t));
         noOfLutIndices = VOCPROCDYN_LUT_INDICES_COUNT;
         noOfTableIndices = VOCPROCDYNTBL_INDICES_COUNT;
         break;
      case VOCPROC_STATIC_TBL:
         ACDB_MEM_CPY((void *)&devId, sizeof(uint32_t), (void *)pIndices, sizeof(uint32_t));
         noOfLutIndices = VOCPROCSTAT_LUT_INDICES_COUNT;
         noOfTableIndices = VOCPROCSTATTBL_INDICES_COUNT;
         break;
      case VOC_STREAM2_TBL:
         devId = 0;
         noOfLutIndices = VOCSTREAM2_LUT_INDICES_COUNT;
         noOfTableIndices = VOCSTREAM2TBL_INDICES_COUNT;
         break;
      case VOCPROC_DYN_INST_TBL_ID:
         ACDB_MEM_CPY((void *)&devId, sizeof(uint32_t), (void *)pIndices, sizeof(uint32_t));
         noOfLutIndices = VOCPROCDYN_INSTANCE_LUT_INDICES_COUNT;
         noOfTableIndices = VOCPROCDYNTBL_INSTANCE_INDICES_COUNT;
         break;
      case VOCPROC_STAT_INST_TBL_ID:
         ACDB_MEM_CPY((void *)&devId, sizeof(uint32_t), (void *)pIndices, sizeof(uint32_t));
         noOfLutIndices = VOCPROCSTAT_INSTANCE_LUT_INDICES_COUNT;
         noOfTableIndices = VOCPROCSTATTBL_INSTANCE_INDICES_COUNT;
         break;
      case VOCSTRM_INST_TBL_ID:
         devId = 0;
         noOfLutIndices = VOCSTREAM2_INST_LUT_INDICES_COUNT;
         noOfTableIndices = VOCSTREAM2TBL_INST_INDICES_COUNT;
         break;
      case AUDPROC_INST_TBL_ID:ACDB_MEM_CPY((void *)&devId, sizeof(uint32_t), (void *)pIndices, sizeof(uint32_t));
         noOfLutIndices = AUDPROC_INST_LUT_INDICES_COUNT;
         noOfTableIndices = AUDPROCTBL_INST_INDICES_COUNT;
         break;
      case AUDPROCVOL_INST_TBL_ID:
         ACDB_MEM_CPY((void *)&devId, sizeof(uint32_t), (void *)pIndices, sizeof(uint32_t));
         noOfLutIndices = AUDPROC_INST_GAIN_DEP_LUT_INDICES_COUNT;
         noOfTableIndices = AUDPROCTBL_INST__GAIN_DEP_INDICES_COUNT;
         break;
      case AFECMN_INST_TBL_ID:
         ACDB_MEM_CPY((void *)&devId, sizeof(uint32_t), (void *)pIndices, sizeof(uint32_t));
         noOfLutIndices = AFECMN_INST_LUT_INDICES_COUNT;
         noOfTableIndices = AFECMNTBL_INST_INDICES_COUNT;
         break;
      case AUDSTRM_INST_TBL_ID:
         devId = 0;
         noOfLutIndices = AUDSTREAM_INST_LUT_INDICES_COUNT;
         noOfTableIndices = AUDSTREAMTBL_INST_INDICES_COUNT;
         break;
      case LSM_INST_TBL_ID:
         ACDB_MEM_CPY((void *)&devId, sizeof(uint32_t), (void *)pIndices, sizeof(uint32_t));
         noOfLutIndices = LSM_INST_LUT_INDICES_COUNT;
         noOfTableIndices = LSM_INST_INDICES_COUNT;
         break;
      case CODEC_COMMON_TBL:
         ACDB_MEM_CPY((void *)&devId, sizeof(uint32_t), (void *)pIndices, sizeof(uint32_t));
         noOfLutIndices = CODEC_PP_COMMON_LUT_INDICES_COUNT;
         noOfTableIndices = CODEC_PP_COMMON_TABLE_INDICES_COUNT;
         break;
      case CODEC_GAIN_TBL:
         ACDB_MEM_CPY((void *)&devId, sizeof(uint32_t), (void *)pIndices, sizeof(uint32_t));
         noOfLutIndices = CODEC_PP_GAIN_LUT_INDICES_COUNT;
         noOfTableIndices = CODEC_PP_GAIN_TABLE_INDICES_COUNT;
         break;
      default:
         ACDB_DEBUG_LOG("Invalid table passed to get the acdb data\n");
         return ACDB_ERROR;
      }
      if (nIdxCount != noOfTableIndices)
      {
         ACDB_DEBUG_LOG("Invalid indices passed to get the acdb data\n");
         return ACDB_ERROR;
      }
      cmd.devId = devId;
      cmd.tblId = tblId;

      if (result != ACDB_SUCCESS)
      {
         ACDB_DEBUG_LOG("Failed to fetch the lookup information of the device %08X \n"
            , devId);
         return result;
      }
      if (tblId == VOCPROC_DYNAMIC_TBL)
      {
         VocProcVolV2CmdLookupType voccmd = { 0, 0, 0 };
		 VocProcDynCmdLookupType pInputcvd = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
         VocProcDynDataLookupType_CommonData cvdcmd = { 0, 0, 0, 0, 0, 0, 0, 0 };
         VocProcDynDataLookupType_offsetData offsettbleinstance = { 0, 0 };
         //AcdbVocProcGainDepVolTblV2CmdType *pInput = (AcdbVocProcGainDepVolTblV2CmdType *)pIndices;
         if (pIndices != NULL)
            ACDB_MEM_CPY(&voccmd, sizeof(voccmd), (void*)pIndices, sizeof(voccmd));
         else
            return ACDB_ERROR;
         /*voccmd.nTxDevId = pInput->nTxDeviceId;
         voccmd.nRxDevId = pInput->nRxDeviceId;
         voccmd.nFeatureId = pInput->nFeatureId;*/
         result = acdbdata_ioctl(ACDBDATACMD_GET_TABLE_INFO, (uint8_t *)&cmd, sizeof(cmd),
            (uint8_t *)&cvdTblInfo, sizeof(cvdTblInfo));
         lutTbl.nLen = READ_UInt32(cvdTblInfo.tblLutChnk.pData);
         lutTbl.pLut = (VocProcDynDataLookupType_UniqueData *)(cvdTblInfo.tblLutChnk.pData + sizeof(lutTbl.nLen));
         result = AcdbDataBinarySearch((void *)lutTbl.pLut, lutTbl.nLen,
            VOCPROCDYN_LUT_INDICES_COUNT - 8, &voccmd, VOCPROCDYN_CMD_INDICES_COUNT, &index);
         if (result != SEARCH_SUCCESS)
         {
            ACDB_DEBUG_LOG("Failed to fetch the lookup information of the device %08X \n"
               , voccmd.nTxDevId);
            return ACDB_INPUT_PARAMS_NOT_FOUND;
         }
         nSecLookupKey = (uintptr_t)&lutTbl.pLut[index];
		 ACDB_MEM_CPY(&tempLutDyn, sizeof(tempLutDyn), &lutTbl.pLut[index], sizeof(tempLutDyn));
		 
		 ofTbloffset = tempLutDyn.nOffTableOffset;
         cvdtableoffset = tempLutDyn.nCommonTblOffset;
         lutTblCVD.nLen = READ_UInt32(cvdTblInfo.tblLutCVDChnk.pData + cvdtableoffset);
         lutTblCVD.pLut = (VocProcDynDataLookupType_CommonData *)(cvdTblInfo.tblLutCVDChnk.pData + cvdtableoffset + sizeof(lutTbl.nLen));
         if (lutTblCVD.nLen <= 0)
         {
            ACDB_DEBUG_LOG("%08X lutcvdTbl empty\n", tblId);
            return ACDB_ERROR;
         }
         ACDB_MEM_CPY(&pInputcvd, sizeof(VocProcDynCmdLookupType), (void *)pIndices, sizeof(VocProcDynCmdLookupType));
         /*if(pIndices!=NULL)
         ACDB_MEM_CPY(&pInputcvd,(void*)pIndices,sizeof(pInputcvd));
         else
         return ACDB_ERROR;*/
         cvdcmd.nNetwork = pInputcvd.nNetworkId;
         cvdcmd.nRxVocSR = pInputcvd.nRxDeviceSampleRateId;
         cvdcmd.nTxVocSR = pInputcvd.nTxDeviceSampleRateId;
         cvdcmd.nVocoder_type = pInputcvd.nVocoder_type;
         cvdcmd.nVocVolStep = pInputcvd.nVoiceVolumeStep;
         cvdcmd.rx_Voc_mode = pInputcvd.rx_Voc_mode;
         cvdcmd.tx_Voc_mode = pInputcvd.tx_Voc_mode;
         cvdcmd.nFeature = pInputcvd.nFeature;

         result = AcdbDataBinarySearch((void *)lutTblCVD.pLut, lutTblCVD.nLen, VOCPROCDYN_LUT_INDICES_COUNT - 5,
            &cvdcmd, VOCPROCDYN_LUT_INDICES_COUNT - 5, &index);// perform binary search for common LUT item in CVD table
         if (result != SEARCH_SUCCESS)
         {
            ACDB_DEBUG_LOG("Failed to fetch the cvd lookup information of the device %08X \n"
               , voccmd.nTxDevId);
            return ACDB_INPUT_PARAMS_NOT_FOUND;
         }
         lutoffsetTbl.nLen = READ_UInt32(cvdTblInfo.tblLutOffsetChnk.pData);
         lutoffsetTbl.pLut = (VocProcDynDataLookupType_offsetData *)(cvdTblInfo.tblLutOffsetChnk.pData + sizeof(lutTbl.nLen));

         ACDB_MEM_CPY(&noOfLUTEntries, sizeof(uint32_t), cvdTblInfo.tblLutChnk.pData, sizeof(uint32_t));

         if (persistData == TRUE &&
            ACDB_UTILITY_INIT_SUCCESS == AcdbIsPersistenceSupported())
         {
            int32_t delta_result = ACDB_SUCCESS;
			AcdbCmdDeltaFileIndexCmdType deltaFileIndexType = { NULL, NULL, NULL };
            uint32_t deltaTblId = tblId;

            deltaFileIndexType.pTblId = &deltaTblId;
            deltaFileIndexType.pIndicesCount = &nIdxCount;
            deltaFileIndexType.pIndices = pIndices;

            delta_result = acdb_delta_data_ioctl(ACDBDELTADATACMD_GET_DELTA_ACDB_FILE_INDEX, (uint8_t *)&deltaFileIndexType, sizeof(AcdbCmdDeltaFileIndexCmdType), (uint8_t *)&deltaFileIndex, sizeof(deltaFileIndex));
            if (delta_result != ACDB_SUCCESS)
            {
               ACDB_DEBUG_LOG("AcdbCmdSetOnlineData: Unable to get delta file index!\n");
            }
         }

         lutoffsetTbl.pLut = (VocProcDynDataLookupType_offsetData *)(cvdTblInfo.tblLutOffsetChnk.pData + sizeof(uint32_t) + ofTbloffset);
         ACDB_MEM_CPY(&offsettbleinstance, sizeof(VocProcDynDataLookupType_offsetData), &lutoffsetTbl.pLut[index], sizeof(VocProcDynDataLookupType_offsetData));

         nLookupKey = (uintptr_t)&lutTblCVD.pLut[index];
         cdefoffsetval = offsettbleinstance.nCDEFTblOffset;
         cdefTbl.nLen = READ_UInt32(cvdTblInfo.tblCdftChnk.pData + cdefoffsetval);
         cdefTbl.pCntDef = (ContentDefType *)(cvdTblInfo.tblCdftChnk.pData + cdefoffsetval + sizeof(cdefTbl.nLen));

         cdotoffsetval = offsettbleinstance.nCDOTTblOffset;
         cdotTbl.nLen = READ_UInt32(cvdTblInfo.tblCdotChnk.pData + cdotoffsetval);
         cdotTbl.pDataOffsets = (uint32_t *)(cvdTblInfo.tblCdotChnk.pData + cdotoffsetval + sizeof(cdotTbl.nLen));

         if (cdefTbl.nLen != cdotTbl.nLen)
         {
            ACDB_DEBUG_LOG("The no of entries in CDEF and CDOT tables for devid %08X not matching\n", devId);
            return ACDB_ERROR;
         }
         if (ACDB_SUCCESS != SetMidPidCalibCVDData(persistData, deltaFileIndex, tblId, nLookupKey, nSecLookupKey, pIndices, nIdxCount, cdefTbl, cdotTbl, cvdTblInfo.dataPoolChnk, mid,
            pid, pInBuff, nInBuffLen))
         {
            return ACDB_ERROR;
         }
      }
      else if (tblId == VOCPROC_STATIC_TBL)
      {
         VocProcCmdLookupType voccmd = { 0, 0, 0, 0 };
         VocProcStatCmdLookupType pInputcvd = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
         VocProcStatDataLookupType_CommonData cvdcmd = { 0, 0, 0, 0, 0, 0, 0 };
         VocProcStatDataLookupType_offsetData offsettbleinstance = { 0, 0 };
         //AcdbVocProcCmnTblCmdType *pInput = (AcdbVocProcCmnTblCmdType *)pIndices;
         if (pIndices != NULL)
            ACDB_MEM_CPY(&voccmd, sizeof(voccmd), (void*)pIndices, sizeof(voccmd));
         else
            return ACDB_ERROR;
         /*voccmd.nTxDevId = pInput->nTxDeviceId;
         voccmd.nRxDevId = pInput->nRxDeviceId;
         voccmd.nRxAfeSr = pInput->nRxDeviceSampleRateId;
         voccmd.nTxAfeSr = pInput->nTxDeviceSampleRateId;*/
         result = acdbdata_ioctl(ACDBDATACMD_GET_TABLE_INFO, (uint8_t *)&cmd, sizeof(cmd),
            (uint8_t *)&cvdTblInfo, sizeof(cvdTblInfo));
         lutStatTbl.nLen = READ_UInt32(cvdTblInfo.tblLutChnk.pData);
         lutStatTbl.pLut = (VocProcStatDataLookupType_UniqueData *)(cvdTblInfo.tblLutChnk.pData + sizeof(lutTbl.nLen));
         result = AcdbDataBinarySearch((void *)lutStatTbl.pLut, lutStatTbl.nLen,
            VOCPROCSTAT_LUT_INDICES_COUNT - 7, &voccmd, VOCPROCSTAT_CMD_INDICES_COUNT, &index);
         if (result != SEARCH_SUCCESS)
         {
            ACDB_DEBUG_LOG("Failed to fetch the lookup information of the device %08X \n"
               , voccmd.nTxDevId);
            return ACDB_INPUT_PARAMS_NOT_FOUND;
         }
         nSecLookupKey = (uintptr_t)&lutStatTbl.pLut[index];
		 ACDB_MEM_CPY(&tempLutStat, sizeof(tempLutStat), &lutStatTbl.pLut[index], sizeof(tempLutStat));
		  
         ofTbloffset = tempLutStat.nOffTableOffset;
		 cvdtableoffset = tempLutStat.nCommonTblOffset;
         lutStatTblCVD.nLen = READ_UInt32(cvdTblInfo.tblLutCVDChnk.pData + cvdtableoffset);
         lutStatTblCVD.pLut = (VocProcStatDataLookupType_CommonData *)(cvdTblInfo.tblLutCVDChnk.pData + cvdtableoffset + sizeof(lutTbl.nLen));
         if (lutStatTblCVD.nLen <= 0)
         {
            ACDB_DEBUG_LOG("%08X lutcvdTbl empty\n", tblId);
            return ACDB_ERROR;
         }
         ACDB_MEM_CPY(&pInputcvd, sizeof(VocProcStatCmdLookupType), (void *)pIndices, sizeof(VocProcStatCmdLookupType));
         /*if(pIndices!=NULL)
         ACDB_MEM_CPY(&pInputcvd,(void*)pIndices,sizeof(pInputcvd));
         else
         return ACDB_ERROR;*/

         cvdcmd.nNetwork = pInputcvd.nNetworkId;
         cvdcmd.nRxVocSR = pInputcvd.nRxDeviceSampleRateId;
         cvdcmd.nTxVocSR = pInputcvd.nTxDeviceSampleRateId;
         cvdcmd.nVocoder_type = pInputcvd.nVocoder_type;
         cvdcmd.rx_Voc_mode = pInputcvd.rx_Voc_mode;
         cvdcmd.tx_Voc_mode = pInputcvd.tx_Voc_mode;
         cvdcmd.nFeature = pInputcvd.nFeature;

         result = AcdbDataBinarySearch((void *)lutStatTblCVD.pLut, lutStatTblCVD.nLen, VOCPROCSTAT_LUT_INDICES_COUNT - 6,
            &cvdcmd, VOCPROCSTAT_LUT_INDICES_COUNT - 6, &index);// perform binary search for common LUT item in CVD table
         if (result != SEARCH_SUCCESS)
         {
            ACDB_DEBUG_LOG("Failed to fetch the cvd lookup information of the device %08X \n"
               , voccmd.nTxDevId);
            return ACDB_INPUT_PARAMS_NOT_FOUND;
         }
         lutStatoffsetTbl.nLen = READ_UInt32(cvdTblInfo.tblLutOffsetChnk.pData);
         lutStatoffsetTbl.pLut = (VocProcStatDataLookupType_offsetData *)(cvdTblInfo.tblLutOffsetChnk.pData + sizeof(lutTbl.nLen));
         ACDB_MEM_CPY(&noOfLUTEntries, sizeof(uint32_t), cvdTblInfo.tblLutChnk.pData, sizeof(uint32_t));

         // get file index which will contain this data.
         if (persistData == TRUE &&
            ACDB_UTILITY_INIT_SUCCESS == AcdbIsPersistenceSupported())
         {
            int32_t delta_result = ACDB_SUCCESS;
            AcdbCmdDeltaFileIndexCmdType deltaFileIndexType = { NULL, NULL, NULL };
            uint32_t deltaTblId = tblId;
            deltaFileIndexType.pTblId = &deltaTblId;
            deltaFileIndexType.pIndicesCount = &nIdxCount;
            deltaFileIndexType.pIndices = pIndices;
            delta_result = acdb_delta_data_ioctl(ACDBDELTADATACMD_GET_DELTA_ACDB_FILE_INDEX, (uint8_t *)&deltaFileIndexType, sizeof(AcdbCmdDeltaFileIndexCmdType), (uint8_t *)&deltaFileIndex, sizeof(deltaFileIndex));
            if (delta_result != ACDB_SUCCESS)
            {
               ACDB_DEBUG_LOG("AcdbCmdSetOnlineData: Unable to get delta file index!\n");
            }
         }

         lutStatoffsetTbl.pLut = (VocProcStatDataLookupType_offsetData *)(cvdTblInfo.tblLutOffsetChnk.pData + sizeof(uint32_t) + ofTbloffset);
         ACDB_MEM_CPY(&offsettbleinstance, sizeof(VocProcStatDataLookupType_offsetData), &lutStatoffsetTbl.pLut[index], sizeof(VocProcStatDataLookupType_offsetData));
         nLookupKey = (uintptr_t)&lutStatTblCVD.pLut[index]; //calculate lookup key based on offsettable becasue it is unique
         cdefoffsetval = offsettbleinstance.nCDEFTblOffset;
         cdefTbl.nLen = READ_UInt32(cvdTblInfo.tblCdftChnk.pData + cdefoffsetval);
         cdefTbl.pCntDef = (ContentDefType *)(cvdTblInfo.tblCdftChnk.pData + cdefoffsetval + sizeof(cdefTbl.nLen));
         cdotoffsetval = offsettbleinstance.nCDOTTblOffset;
         cdotTbl.nLen = READ_UInt32(cvdTblInfo.tblCdotChnk.pData + cdotoffsetval);
         cdotTbl.pDataOffsets = (uint32_t *)(cvdTblInfo.tblCdotChnk.pData + cdotoffsetval + sizeof(cdotTbl.nLen));

         if (cdefTbl.nLen != cdotTbl.nLen)
         {
            ACDB_DEBUG_LOG("The no of entries in CDEF and CDOT tables for devid %08X not matching\n", devId);
            return ACDB_ERROR;
         }

         if (ACDB_SUCCESS != SetMidPidCalibCVDData(persistData, deltaFileIndex, tblId, nLookupKey, nSecLookupKey, pIndices, nIdxCount, cdefTbl, cdotTbl, cvdTblInfo.dataPoolChnk, mid,
            pid, pInBuff, nInBuffLen))
         {
            return ACDB_ERROR;
         }
      }
      else if (tblId == VOCPROC_DYN_INST_TBL_ID)
      {
         VocProcVolV2CmdLookupType voccmd = { 0, 0, 0 };
         VocProcDynCmdLookupType *pInputcvd = NULL;
         VocProcDynDataLookupType_CommonData cvdcmd = { 0, 0, 0, 0, 0, 0, 0, 0 };
         VocProcDynDataLookupType_offsetData offsettbleinstance = { 0, 0 };
         //AcdbVocProcGainDepVolTblV2CmdType *pInput = (AcdbVocProcGainDepVolTblV2CmdType *)pIndices;
         if (pIndices != NULL)
            ACDB_MEM_CPY(&voccmd, sizeof(voccmd), (void*)pIndices, sizeof(voccmd));
         else
            return ACDB_ERROR;
         /*voccmd.nTxDevId = pInput->nTxDeviceId;
         voccmd.nRxDevId = pInput->nRxDeviceId;
         voccmd.nFeatureId = pInput->nFeatureId;*/
         result = acdbdata_ioctl(ACDBDATACMD_GET_TABLE_INFO, (uint8_t *)&cmd, sizeof(cmd),
            (uint8_t *)&cvdTblInfo, sizeof(cvdTblInfo));
         lutTbl.nLen = READ_UInt32(cvdTblInfo.tblLutChnk.pData);
         lutTbl.pLut = (VocProcDynDataLookupType_UniqueData *)(cvdTblInfo.tblLutChnk.pData + sizeof(lutTbl.nLen));
         result = AcdbDataBinarySearch((void *)lutTbl.pLut, lutTbl.nLen,
            VOCPROCDYN_INSTANCE_LUT_INDICES_COUNT - 8, &voccmd, VOCPROCDYN_INSTANCE_CMD_INDICES_COUNT, &index);
         if (result != SEARCH_SUCCESS)
         {
            ACDB_DEBUG_LOG("Failed to fetch the lookup information of the device %08X \n"
               , voccmd.nTxDevId);
            return ACDB_INPUT_PARAMS_NOT_FOUND;
         }
         nSecLookupKey = (uintptr_t)&lutTbl.pLut[index];
		 ACDB_MEM_CPY(&tempLutDyn, sizeof(tempLutDyn), &lutTbl.pLut[index], sizeof(tempLutDyn));
         ofTbloffset = tempLutDyn.nOffTableOffset;
		 cvdtableoffset = tempLutDyn.nCommonTblOffset;
         lutTblCVD.nLen = READ_UInt32(cvdTblInfo.tblLutCVDChnk.pData + cvdtableoffset);
         lutTblCVD.pLut = (VocProcDynDataLookupType_CommonData *)(cvdTblInfo.tblLutCVDChnk.pData + cvdtableoffset + sizeof(lutTbl.nLen));
         if (lutTblCVD.nLen <= 0)
         {
            ACDB_DEBUG_LOG("%08X lutcvdTbl empty\n", tblId);
            return ACDB_ERROR;
         }
         pInputcvd = (VocProcDynCmdLookupType *)pIndices;
         /*if(pIndices!=NULL)
         ACDB_MEM_CPY(&pInputcvd,(void*)pIndices,sizeof(pInputcvd));
         else
         return ACDB_ERROR;*/
         cvdcmd.nNetwork = pInputcvd->nNetworkId;
         cvdcmd.nRxVocSR = pInputcvd->nRxDeviceSampleRateId;
         cvdcmd.nTxVocSR = pInputcvd->nTxDeviceSampleRateId;
         cvdcmd.nVocoder_type = pInputcvd->nVocoder_type;
         cvdcmd.nVocVolStep = pInputcvd->nVoiceVolumeStep;
         cvdcmd.rx_Voc_mode = pInputcvd->rx_Voc_mode;
         cvdcmd.tx_Voc_mode = pInputcvd->tx_Voc_mode;
         cvdcmd.nFeature = pInputcvd->nFeature;

         result = AcdbDataBinarySearch((void *)lutTblCVD.pLut, lutTblCVD.nLen, VOCPROCDYN_INSTANCE_LUT_INDICES_COUNT - 5,
            &cvdcmd, VOCPROCDYN_INSTANCE_LUT_INDICES_COUNT - 5, &index);// perform binary search for common LUT item in CVD table
         if (result != SEARCH_SUCCESS)
         {
            ACDB_DEBUG_LOG("Failed to fetch the cvd lookup information of the device %08X \n"
               , voccmd.nTxDevId);
            return ACDB_INPUT_PARAMS_NOT_FOUND;
         }
         lutoffsetTbl.nLen = READ_UInt32(cvdTblInfo.tblLutOffsetChnk.pData);
         lutoffsetTbl.pLut = (VocProcDynDataLookupType_offsetData *)(cvdTblInfo.tblLutOffsetChnk.pData + sizeof(lutTbl.nLen));

         ACDB_MEM_CPY(&noOfLUTEntries, sizeof(uint32_t), cvdTblInfo.tblLutChnk.pData, sizeof(uint32_t));

         /*cdftcmd.nMid =
         result = AcdbDataBinarySearch((void *)lutoffsetTbl.pLut, lutoffsetTbl.nLen, VOCPROCDYN_INSTANCE_CDFT_INDICES_COUNT,*/

         if (persistData == TRUE &&
            ACDB_UTILITY_INIT_SUCCESS == AcdbIsPersistenceSupported())
         {
            int32_t delta_result = ACDB_SUCCESS;
            AcdbCmdDeltaFileIndexCmdType deltaFileIndexType = { NULL, NULL, NULL };
            uint32_t deltaTblId = tblId;

            deltaFileIndexType.pTblId = &deltaTblId;
            deltaFileIndexType.pIndicesCount = &nIdxCount;
            deltaFileIndexType.pIndices = pIndices;

            delta_result = acdb_delta_data_ioctl(ACDBDELTADATACMD_GET_DELTA_ACDB_FILE_INDEX, (uint8_t *)&deltaFileIndexType, sizeof(AcdbCmdDeltaFileIndexCmdType), (uint8_t *)&deltaFileIndex, sizeof(deltaFileIndex));
            if (delta_result != ACDB_SUCCESS)
            {
               ACDB_DEBUG_LOG("AcdbCmdSetOnlineData: Unable to get delta file index!\n");
            }
         }

         lutoffsetTbl.pLut = (VocProcDynDataLookupType_offsetData *)(cvdTblInfo.tblLutOffsetChnk.pData + sizeof(uint32_t) + ofTbloffset);
         ACDB_MEM_CPY(&offsettbleinstance, sizeof(VocProcDynDataLookupType_offsetData), &lutoffsetTbl.pLut[index], sizeof(VocProcDynDataLookupType_offsetData));

         nLookupKey = (uintptr_t)&lutTblCVD.pLut[index];
         cdefoffsetval = offsettbleinstance.nCDEFTblOffset;
         cdeftInstanceTbl.nLen = READ_UInt32(cvdTblInfo.tblCdftChnk.pData + cdefoffsetval);
         cdeftInstanceTbl.pCntDef = (ContentDefInstanceType *)(cvdTblInfo.tblCdftChnk.pData + cdefoffsetval + sizeof(cdeftInstanceTbl.nLen));

         cdotoffsetval = offsettbleinstance.nCDOTTblOffset;
         cdotTbl.nLen = READ_UInt32(cvdTblInfo.tblCdotChnk.pData + cdotoffsetval);
         cdotTbl.pDataOffsets = (uint32_t *)(cvdTblInfo.tblCdotChnk.pData + cdotoffsetval + sizeof(cdotTbl.nLen));

         if (cdeftInstanceTbl.nLen != cdotTbl.nLen)
         {
            ACDB_DEBUG_LOG("The no of entries in CDEF and CDOT tables for devid %08X not matching\n", devId);
            return ACDB_ERROR;
         }
         if (ACDB_SUCCESS != SetMidIidPidCalibCVDData(persistData, deltaFileIndex, tblId, nLookupKey, nSecLookupKey, pIndices, nIdxCount, cdeftInstanceTbl, cdotTbl, cvdTblInfo.dataPoolChnk, mid,
            pid, iid, pInBuff, nInBuffLen))
         {
            return ACDB_ERROR;
         }
      }
      else if (tblId == VOCPROC_STAT_INST_TBL_ID)
      {
         VocProcCmdLookupType voccmd = { 0, 0, 0, 0 };
         VocProcStatCmdLookupType *pInputcvd = NULL;
         VocProcStatDataLookupType_CommonData cvdcmd = { 0, 0, 0, 0, 0, 0, 0 };
         VocProcStatDataLookupType_offsetData offsettbleinstance = { 0, 0 };
         //AcdbVocProcCmnTblCmdType *pInput = (AcdbVocProcCmnTblCmdType *)pIndices;
         if (pIndices != NULL)
            ACDB_MEM_CPY(&voccmd, sizeof(voccmd), (void*)pIndices, sizeof(voccmd));
         else
            return ACDB_ERROR;
         /*voccmd.nTxDevId = pInput->nTxDeviceId;
         voccmd.nRxDevId = pInput->nRxDeviceId;
         voccmd.nRxAfeSr = pInput->nRxDeviceSampleRateId;
         voccmd.nTxAfeSr = pInput->nTxDeviceSampleRateId;*/
         result = acdbdata_ioctl(ACDBDATACMD_GET_TABLE_INFO, (uint8_t *)&cmd, sizeof(cmd),
            (uint8_t *)&cvdTblInfo, sizeof(cvdTblInfo));
         lutStatTbl.nLen = READ_UInt32(cvdTblInfo.tblLutChnk.pData);
         lutStatTbl.pLut = (VocProcStatDataLookupType_UniqueData *)(cvdTblInfo.tblLutChnk.pData + sizeof(lutTbl.nLen));
         result = AcdbDataBinarySearch((void *)lutStatTbl.pLut, lutStatTbl.nLen,
            VOCPROCSTAT_LUT_INDICES_COUNT - 7, &voccmd, VOCPROCSTAT_CMD_INDICES_COUNT, &index);
         if (result != SEARCH_SUCCESS)
         {
            ACDB_DEBUG_LOG("Failed to fetch the lookup information of the device %08X \n"
               , voccmd.nTxDevId);
            return ACDB_INPUT_PARAMS_NOT_FOUND;
         }
         nSecLookupKey = (uintptr_t)&lutStatTbl.pLut[index];
		 ACDB_MEM_CPY(&tempLutStat, sizeof(tempLutStat), &lutStatTbl.pLut[index], sizeof(tempLutStat));
         ofTbloffset = tempLutStat.nOffTableOffset;
		 cvdtableoffset = tempLutStat.nCommonTblOffset;
         lutStatTblCVD.nLen = READ_UInt32(cvdTblInfo.tblLutCVDChnk.pData + cvdtableoffset);
         lutStatTblCVD.pLut = (VocProcStatDataLookupType_CommonData *)(cvdTblInfo.tblLutCVDChnk.pData + cvdtableoffset + sizeof(lutTbl.nLen));
         if (lutStatTblCVD.nLen <= 0)
         {
            ACDB_DEBUG_LOG("%08X lutcvdTbl empty\n", tblId);
            return ACDB_ERROR;
         }
         pInputcvd = (VocProcStatCmdLookupType *)pIndices;
         /*if(pIndices!=NULL)
         ACDB_MEM_CPY(&pInputcvd,(void*)pIndices,sizeof(pInputcvd));
         else
         return ACDB_ERROR;*/

         cvdcmd.nNetwork = pInputcvd->nNetworkId;
         cvdcmd.nRxVocSR = pInputcvd->nRxDeviceSampleRateId;
         cvdcmd.nTxVocSR = pInputcvd->nTxDeviceSampleRateId;
         cvdcmd.nVocoder_type = pInputcvd->nVocoder_type;
         cvdcmd.rx_Voc_mode = pInputcvd->rx_Voc_mode;
         cvdcmd.tx_Voc_mode = pInputcvd->tx_Voc_mode;
         cvdcmd.nFeature = pInputcvd->nFeature;

         result = AcdbDataBinarySearch((void *)lutStatTblCVD.pLut, lutStatTblCVD.nLen, VOCPROCSTAT_LUT_INDICES_COUNT - 6,
            &cvdcmd, VOCPROCSTAT_LUT_INDICES_COUNT - 6, &index);// perform binary search for common LUT item in CVD table
         if (result != SEARCH_SUCCESS)
         {
            ACDB_DEBUG_LOG("Failed to fetch the cvd lookup information of the device %08X \n"
               , voccmd.nTxDevId);
            return ACDB_INPUT_PARAMS_NOT_FOUND;
         }
         lutStatoffsetTbl.nLen = READ_UInt32(cvdTblInfo.tblLutOffsetChnk.pData);
         lutStatoffsetTbl.pLut = (VocProcStatDataLookupType_offsetData *)(cvdTblInfo.tblLutOffsetChnk.pData + sizeof(lutTbl.nLen));
         ACDB_MEM_CPY(&noOfLUTEntries, sizeof(uint32_t), cvdTblInfo.tblLutChnk.pData, sizeof(uint32_t));

         // get file index which will contain this data.
         if (persistData == TRUE &&
            ACDB_UTILITY_INIT_SUCCESS == AcdbIsPersistenceSupported())
         {
            int32_t delta_result = ACDB_SUCCESS;
            AcdbCmdDeltaFileIndexCmdType deltaFileIndexType = { NULL, NULL, NULL };
            uint32_t deltaTblId = tblId;
            deltaFileIndexType.pTblId = &deltaTblId;
            deltaFileIndexType.pIndicesCount = &nIdxCount;
            deltaFileIndexType.pIndices = pIndices;
            delta_result = acdb_delta_data_ioctl(ACDBDELTADATACMD_GET_DELTA_ACDB_FILE_INDEX, (uint8_t *)&deltaFileIndexType, sizeof(AcdbCmdDeltaFileIndexCmdType), (uint8_t *)&deltaFileIndex, sizeof(deltaFileIndex));
            if (delta_result != ACDB_SUCCESS)
            {
               ACDB_DEBUG_LOG("AcdbCmdSetOnlineData: Unable to get delta file index!\n");
            }
         }

         lutStatoffsetTbl.pLut = (VocProcStatDataLookupType_offsetData *)(cvdTblInfo.tblLutOffsetChnk.pData + sizeof(uint32_t) + ofTbloffset);
         ACDB_MEM_CPY(&offsettbleinstance, sizeof(VocProcStatDataLookupType_offsetData), &lutStatoffsetTbl.pLut[index], sizeof(VocProcStatDataLookupType_offsetData));
         nLookupKey = (uintptr_t)&lutStatTblCVD.pLut[index]; //calculate lookup key based on offsettable becasue it is unique
         cdefoffsetval = offsettbleinstance.nCDEFTblOffset;
         cdeftInstanceTbl.nLen = READ_UInt32(cvdTblInfo.tblCdftChnk.pData + cdefoffsetval);
         cdeftInstanceTbl.pCntDef = (ContentDefInstanceType *)(cvdTblInfo.tblCdftChnk.pData + cdefoffsetval + sizeof(cdeftInstanceTbl.nLen));
         cdotoffsetval = offsettbleinstance.nCDOTTblOffset;
         cdotTbl.nLen = READ_UInt32(cvdTblInfo.tblCdotChnk.pData + cdotoffsetval);
         cdotTbl.pDataOffsets = (uint32_t *)(cvdTblInfo.tblCdotChnk.pData + cdotoffsetval + sizeof(cdotTbl.nLen));

         if (cdeftInstanceTbl.nLen != cdotTbl.nLen)
         {
            ACDB_DEBUG_LOG("The no of entries in CDEF and CDOT tables for devid %08X not matching\n", devId);
            return ACDB_ERROR;
         }

         if (ACDB_SUCCESS != SetMidIidPidCalibCVDData(persistData, deltaFileIndex, tblId, nLookupKey, nSecLookupKey, pIndices, nIdxCount, cdeftInstanceTbl, cdotTbl, cvdTblInfo.dataPoolChnk, mid,
            pid, iid, pInBuff, nInBuffLen))
         {
            return ACDB_ERROR;
         }

         // able to set persistence data, notify delta mgr that file has been updated.
         if (persistData == TRUE &&
            ACDB_UTILITY_INIT_SUCCESS == AcdbIsPersistenceSupported())
         {
            int32_t delta_result = ACDB_SUCCESS;

            delta_result = acdb_delta_data_ioctl(ACDBDELTADATACMD_SET_DELTA_ACDB_FILE_UPDATED, (uint8_t *)&deltaFileIndex, sizeof(deltaFileIndex), NULL, 0);
            if (delta_result != ACDB_SUCCESS)
            {
               ACDB_DEBUG_LOG("AcdbCmdSetOnlineData: Unable to set delta file updated!\n");
            }
         }
      }
      else
      {
          result = acdbdata_ioctl(ACDBDATACMD_GET_TABLE_INFO, (uint8_t *)&cmd, sizeof(cmd),
            (uint8_t *)&tblInfo, sizeof(tblInfo));
          if (result != ACDB_SUCCESS)
          {
              ACDB_DEBUG_LOG("Failed to fetch the lookup information of the device %08X and table %08X \n", cmd.devId, cmd.tblId);
              return result;
          }

         ACDB_MEM_CPY(&noOfLUTEntries, sizeof(uint32_t), tblInfo.tblLutChnk.pData, sizeof(uint32_t));
         pLut = (uint8_t *)(tblInfo.tblLutChnk.pData + sizeof(noOfLUTEntries));

         result = AcdbDataBinarySearch((void *)pLut, noOfLUTEntries,
            noOfLutIndices, pIndices, noOfTableIndices, &index);
         if (result != SEARCH_SUCCESS)
         {
            ACDB_DEBUG_LOG("Failed to fetch the lookup information of the device %08X \n"
               , devId);
            return ACDB_ERROR;
         }

         nLookupKey = (uintptr_t)(pLut + (noOfLutIndices*index*(uint32_t)sizeof(uint32_t)));

         // get file index which will contain this data.
         if (persistData == TRUE &&
            ACDB_UTILITY_INIT_SUCCESS == AcdbIsPersistenceSupported())
         {
            int32_t delta_result = ACDB_SUCCESS;
            AcdbCmdDeltaFileIndexCmdType deltaFileIndexType = { NULL, NULL, NULL };
            uint32_t deltaTblId = tblId;

            deltaFileIndexType.pTblId = &deltaTblId;
            deltaFileIndexType.pIndicesCount = &nIdxCount;
            deltaFileIndexType.pIndices = pIndices;

            delta_result = acdb_delta_data_ioctl(ACDBDELTADATACMD_GET_DELTA_ACDB_FILE_INDEX, (uint8_t *)&deltaFileIndexType, sizeof(AcdbCmdDeltaFileIndexCmdType), (uint8_t *)&deltaFileIndex, sizeof(deltaFileIndex));
            if (delta_result != ACDB_SUCCESS)
            {
               ACDB_DEBUG_LOG("AcdbCmdSetOnlineData: Unable to get delta file index!\n");
            }
         }

         if (nonModuleTblFound == 0)
         {
            // Now get CDEF info
            cdefoffset = ((noOfLutIndices*index*(uint32_t)sizeof(uint32_t)) + (noOfTableIndices*(uint32_t)sizeof(uint32_t)));
            cdefoffsetval = READ_UInt32(pLut + cdefoffset);

            // Now get CDOT info
            cdotoffset = (uint32_t)(cdefoffset + sizeof(uint32_t));
            cdotoffsetval = READ_UInt32(pLut + cdotoffset);
            cdotTbl.nLen = READ_UInt32(tblInfo.tblCdotChnk.pData + cdotoffsetval);
            cdotTbl.pDataOffsets = (uint32_t *)(tblInfo.tblCdotChnk.pData + cdotoffsetval + sizeof(cdotTbl.nLen));

            if (noOfCdftIndices == NON_INSTANCE_CDFT_INDICES_COUNT)
            {
               cdefTbl.nLen = READ_UInt32(tblInfo.tblCdftChnk.pData + cdefoffsetval);
               cdefTbl.pCntDef = (ContentDefType *)(tblInfo.tblCdftChnk.pData + cdefoffsetval + sizeof(cdefTbl.nLen));
               if (cdefTbl.nLen != cdotTbl.nLen)
               {
                  ACDB_DEBUG_LOG("The no of entries in CDEF and CDOT tables for devid %08X not matching\n", devId);
                  return ACDB_ERROR;
               }
               if (ACDB_SUCCESS != SetMidPidCalibData(persistData, deltaFileIndex, tblId, nLookupKey, pIndices, nIdxCount, cdefTbl, cdotTbl, tblInfo.dataPoolChnk, mid,
                  pid, pInBuff, nInBuffLen))
               {
                  return ACDB_ERROR;
               }
            }
            else if (noOfCdftIndices == INSTANCE_CDFT_INDICES_COUNT)
            {
               cdeftInstanceTbl.nLen = READ_UInt32(tblInfo.tblCdftChnk.pData + cdefoffsetval);
               cdeftInstanceTbl.pCntDef = (ContentDefInstanceType *)(tblInfo.tblCdftChnk.pData + cdefoffsetval + sizeof(cdeftInstanceTbl.nLen));
               if (cdeftInstanceTbl.nLen != cdotTbl.nLen)
               {
                  ACDB_DEBUG_LOG("The no of entries in CDEF and CDOT tables for devid %08X not matching\n", devId);
                  return ACDB_ERROR;
               }
               if (ACDB_SUCCESS != SetMidIidPidCalibData(persistData, deltaFileIndex, tblId, nLookupKey, pIndices, nIdxCount, cdeftInstanceTbl, cdotTbl, tblInfo.dataPoolChnk, mid, iid,
                  pid, pInBuff, nInBuffLen))
               {
                  return ACDB_ERROR;
               }
            }
         }
         else
         {
            uint32_t dataoffset = READ_UInt32(pLut + (noOfLutIndices*index*(uint32_t)sizeof(uint32_t)) + (noOfTableIndices*(uint32_t)sizeof(uint32_t)));
            if (ACDB_SUCCESS != SetCalibData(persistData, deltaFileIndex, tblId, nLookupKey, pIndices, nIdxCount, dataoffset, tblInfo.dataPoolChnk,
               pInBuff, nInBuffLen))
            {
               return ACDB_ERROR;
            }
         }
      }
      // able to set persistence data, notify delta mgr that file has been updated.
      if (persistData == TRUE &&
         ACDB_UTILITY_INIT_SUCCESS == AcdbIsPersistenceSupported())
      {
         int32_t delta_result = ACDB_SUCCESS;

         delta_result = acdb_delta_data_ioctl(ACDBDELTADATACMD_SET_DELTA_ACDB_FILE_UPDATED, (uint8_t *)&deltaFileIndex, sizeof(deltaFileIndex), NULL, 0);
         if (delta_result != ACDB_SUCCESS)
         {
            ACDB_DEBUG_LOG("AcdbCmdSetOnlineData: Unable to set delta file updated!\n");
         }
      }

      result = ACDB_SUCCESS;
   }

   return result;
}

int32_t AcdbCmdGetNoOfTblEntriesOnHeap(uint8_t *pCmd, uint32_t nCmdSize, uint8_t *pRsp, uint32_t nRspSizse)
{
   int32_t result = ACDB_SUCCESS;
   if (pCmd == NULL || pRsp == NULL || nCmdSize == 0 || nRspSizse == 0)
   {
      ACDB_DEBUG_LOG("[ACDB Command]->Invalid NULL value parameters are provided to get no of tbl entries\n");
      return ACDB_ERROR;
   }
   result = GetNoOfTblEntriesOnHeap(pCmd, nCmdSize, pRsp, nRspSizse);

   return result;
}

int32_t AcdbCmdGetTblEntriesOnHeap(uint8_t *pCmd, uint32_t nCmdSize, uint8_t *pRsp, uint32_t nRspSizse)
{
   int32_t result = ACDB_SUCCESS;
   if (pCmd == NULL || pRsp == NULL || nCmdSize == 0 || nRspSizse == 0)
   {
      ACDB_DEBUG_LOG("[ACDB Command]->Invalid NULL value parameters are provided to get tbl entries\n");
      return ACDB_ERROR;
   }
   result = GetTblEntriesOnHeap(pCmd, nCmdSize, pRsp, nRspSizse);

   return result;
}

int32_t AcdbCmdGetFeatureSupportedDevList(AcdbFeatureSupportedDevListCmdType *pCmd, AcdbQueryResponseType *pResp)
{
   int32_t result = ACDB_SUCCESS;
   uint32_t i = 0;
   int32_t offset = 0;
   uint32_t noOfDevs = 0;
   uint32_t noOfBytesRemaining = 0;
   uint32_t nParamId = 0;
   AcdbDevices *pDevs = NULL;
   if (pCmd == NULL || pResp == NULL)
   {
      ACDB_DEBUG_LOG("[ACDB Command]->Invalid NULL value parameters are provided to get supported device list for a given feature\n");
      return ACDB_BADPARM;
   }
   else if (pCmd->pBufferPointer == NULL)
   {
      return ACDB_BADPARM;
   }

   noOfBytesRemaining = pCmd->nBufferLength;

   switch (pCmd->nFeatureID)
   {
   case ACDB_FEEDBACK_SPEAKERPROTECTION_RX:
      nParamId = IS_RX_DEV_FB_SPEAKER_PROT_ENABLED;
      break;
   case ACDB_FEEDBACK_SPEAKERPROTECTION_TX:
      nParamId = IS_TX_DEV_FB_SPEAKER_PROT_ENABLED;
      break;
   case ACDB_ADAPTIVE_ANC_RX:
      nParamId = IS_RX_DEV_ADAPTIVE_ANC_ENABLED;
      break;
   case ACDB_LOW_POWER_LISTEN:
      nParamId = IS_LOW_POWER_LISTEN_ENABLED;
      break;
   case ACDB_HIGH_POWER_LISTEN:
      nParamId = IS_HIGH_POWER_LISTEN_ENABLED;
      break;
   default:
      ACDB_DEBUG_LOG("ACDB_COMMAND: Provided invalid feature id to get the feature supported device list\n");
      return ACDB_BADPARM;
   }

   pDevs = (AcdbDevices *)ACDB_MALLOC(sizeof(AcdbDevices));
   if (pDevs == NULL)
   {
      ACDB_DEBUG_LOG("ACDB_COMMAND: Unable to allocate memory for AcdbDevices\n");
      return ACDB_INSUFFICIENTMEMORY;
   }
   // Minimum provided buffer size should be 4 to atleast holds the no of devices value as zero
   // if none of the device support the given feature.
   if (noOfBytesRemaining < sizeof(noOfDevs))
   {
      ACDB_DEBUG_LOG("ACDB_COMMAND: Insufficient memory provided to get the list of supported devices for given feature\n");
      ACDB_MEM_FREE(pDevs);
      return ACDB_INSUFFICIENTMEMORY;
   }

   memset((void*)pDevs, 0, sizeof(AcdbDevices));

   result = acdbdata_ioctl(ACDBDATACMD_GET_DEVICE_LIST, (uint8_t *)pDevs, sizeof(AcdbDevices),
      (uint8_t *)NULL, 0);
   if (result != ACDB_SUCCESS)
   {
      ACDB_DEBUG_LOG("ACDB_COMMAND: No devices found.Please check if the correct set of acdb files are loaded.\n");
      ACDB_MEM_FREE(pDevs);
      return result;
   }
   offset += (int32_t)sizeof(noOfDevs);
   noOfBytesRemaining -= (uint32_t)sizeof(noOfDevs);

   for (i = 0; i < pDevs->noOfDevs; i++)
   {
      AcdbDevPropInfo devPropInfo = { 0, 0, { 0, NULL} };
      uint32_t *pIsValid = NULL;
      memset(&devPropInfo, 0, sizeof(devPropInfo));
      devPropInfo.devId = pDevs->devList[i];
      devPropInfo.pId = nParamId;
      result = acdbdata_ioctl(ACDBDATACMD_GET_DEVICE_PROP, (uint8_t *)&devPropInfo, sizeof(AcdbDevPropInfo),
         (uint8_t *)NULL, 0);
      if (result != ACDB_SUCCESS)
      {
         continue;
      }
      if (devPropInfo.dataInfo.nDataLen != sizeof(uint32_t))
      {
         ACDB_DEBUG_LOG("The property %08X contains invalid data len for device %08X", nParamId, pDevs->devList[i]);
         ACDB_MEM_FREE(pDevs);
         return ACDB_BADPARM;
      }
      pIsValid = (uint32_t *)devPropInfo.dataInfo.pData;
      if (*pIsValid != TRUE)
      {
         continue;
      }
      if (noOfBytesRemaining < sizeof(pDevs->devList[i]))
      {
         ACDB_DEBUG_LOG("ACDB_COMMAND: Insufficient memory provided to get the list of supported devices for given feature\n");
         ACDB_MEM_FREE(pDevs);
         return ACDB_INSUFFICIENTMEMORY;
      }
      noOfDevs += 1;
      ACDB_MEM_CPY((void *)(pCmd->pBufferPointer + offset), noOfBytesRemaining, (void *)(&pDevs->devList[i]), sizeof(pDevs->devList[i]));
      offset += (int32_t)sizeof(pDevs->devList[i]);
      noOfBytesRemaining -= (uint32_t)sizeof(pDevs->devList[i]);
   }
   if (noOfDevs == 0)
   {
      ACDB_DEBUG_LOG("ACDB_COMMAND: No devices found with the requested feature enabled\n");
      ACDB_MEM_FREE(pDevs);
      return ACDB_ERROR;
   }
   else
   {
      result = ACDB_SUCCESS;
   }
   ACDB_MEM_CPY((void *)(pCmd->pBufferPointer), sizeof(noOfDevs), (void *)(&noOfDevs), sizeof(noOfDevs));
   pResp->nBytesUsedInBuffer = offset;

   ACDB_MEM_FREE(pDevs);
   return result;
}

int32_t AcdbCmdGetDevPairList(AcdbDevicePairListCmdType *pCmd, AcdbQueryResponseType *pResp)
{
   int32_t result = ACDB_SUCCESS;
   int32_t offset = 0;
   uint32_t noOfDevs = 0;
   uint32_t noOfBytesRemaining = 0;
   uint32_t nParamId = 0;
   AcdbDevPropInfo devPropInfo = { 0, 0, { 0, NULL} };
   uint32_t *pDevList = NULL;
   memset(&devPropInfo, 0, sizeof(devPropInfo));
   if (pCmd == NULL || pResp == NULL)
   {
      ACDB_DEBUG_LOG("[ACDB Command]->Invalid NULL value parameters are provided to get supported device list for a given feature\n");
      return ACDB_BADPARM;
   }
   else if (pCmd->pBufferPointer == NULL)
   {
      return ACDB_BADPARM;
   }

   noOfBytesRemaining = pCmd->nBufferLength;

   switch (pCmd->nDevPairType)
   {
   case ACDB_FEEDBACK_SPEAKERPROTECTION_RX2TX_LIST:
      nParamId = FB_SPEAKER_PROT_RX_TX_DEV_PAIR;
      break;
   case ACDB_VOICE_TX2RX_LIST:
      nParamId = DEVPAIR;
      break;
   case ACDB_ANC_RX2TX_LIST:
      nParamId = ANCDEVPAIR;
      break;
   case ACDB_NATIVE_REMOTE_MAP_LIST:
      nParamId = APQ_MDM_COMP_DEVID;
      break;
   case ACDB_AUDIO_RECORD_TX2RX_LIST:
      nParamId = RECORDED_DEVICEPAIR;
      break;
   case ACDB_ADAPTIVE_ANC_RX2TX_LIST:
      nParamId = ADAPTIVE_ANCDEVPAIR;
      break;
   case ACDB_AUDIOEC_TX2ANY_LIST:
      nParamId = TXTOANY_RECORDED_DEVICEPAIR;
      break;
   default:
      ACDB_DEBUG_LOG("ACDB_COMMAND: Provided invalid devpair type to get the device pair list\n");
      return ACDB_BADPARM;
   }

   // Minimum provided buffer size should be 4 to atleast holds the no of devices value as zero
   // if none of the device support the given feature.
   if (noOfBytesRemaining < sizeof(noOfDevs))
   {
      ACDB_DEBUG_LOG("ACDB_COMMAND: Insufficient memory provided to get the list of supported devices for given feature\n");
      return ACDB_INSUFFICIENTMEMORY;
   }

   devPropInfo.devId = pCmd->nDeviceID;
   devPropInfo.pId = nParamId;
   result = acdbdata_ioctl(ACDBDATACMD_GET_DEVICE_PROP, (uint8_t *)&devPropInfo, sizeof(AcdbDevPropInfo),
      (uint8_t *)NULL, 0);
   if (result != ACDB_SUCCESS)
   {
      ACDB_DEBUG_LOG("Requested Device Pair not found");
      return ACDB_ERROR;
   }
   if ((devPropInfo.dataInfo.nDataLen == 0) || ((devPropInfo.dataInfo.nDataLen % 4) != 0))
   {
      ACDB_DEBUG_LOG("Contains Invalid data for the device pair for the given device id %08X", pCmd->nDeviceID);
      return ACDB_ERROR;
   }
   noOfDevs = devPropInfo.dataInfo.nDataLen / 4;
   pDevList = (uint32_t *)devPropInfo.dataInfo.pData;

   if (noOfBytesRemaining < (devPropInfo.dataInfo.nDataLen + sizeof(noOfDevs)))
   {
      ACDB_DEBUG_LOG("ACDB_COMMAND: Insufficient memory provided to get the dev pair data\n");
      return ACDB_INSUFFICIENTMEMORY;
   }

   ACDB_MEM_CPY((void *)(pCmd->pBufferPointer + offset), noOfBytesRemaining, (void *)(&noOfDevs), sizeof(noOfDevs));
   offset += (int32_t)sizeof(noOfDevs);
   ACDB_MEM_CPY((void *)(pCmd->pBufferPointer + offset), (noOfBytesRemaining - sizeof(noOfDevs)), (void *)(pDevList), devPropInfo.dataInfo.nDataLen);
   offset += devPropInfo.dataInfo.nDataLen;

   pResp->nBytesUsedInBuffer = offset;

   return result;
}

int32_t AcdbCmdGetLSMInstanceTblInfo(uint32_t queryType, uint8_t *pIn, uint8_t *pOut)
{
   int32_t result = ACDB_SUCCESS;

   if (pIn == NULL || pOut == NULL)
   {
      ACDB_DEBUG_LOG("[ACDB Command]->[AcdbCmdGetLSMTblInfo]->Invalid NULL value parameters are provided\n");
      result = ACDB_BADPARM;
   }
   else
   {
      uint32_t index = 0;
      AcdbTableCmd cmd = { 0, 0 };
      AcdbTableInfo tblInfo = { { 0, NULL }, { 0, NULL }, { 0, NULL } };
	  LSMTblLookupTblType lutTbl = { 0, NULL };
      ContentDefWithInstanceTblType cdefTbl = { 0, NULL };
      ContentDataOffsetsTblType cdotTbl = { 0, NULL };
      uintptr_t nLookupKey = 0;
	  LSMTblCmdLookupType lsmtblcmd = { 0, 0 };
      uint32_t tblId = LSM_INST_TBL_ID;
	  LSMTblLookupType tempLut = { 0, 0, 0, 0 };

      switch (queryType)
      {
      case TABLE_CMD:
      {
         AcdbLsmTableCmdType *pInput = (AcdbLsmTableCmdType *)pIn;
         if (pInput->nBufferPointer == NULL)
         {
            return ACDB_BADPARM;
         }
         lsmtblcmd.nDevId = pInput->nDeviceId;
         lsmtblcmd.nLSMAppTypeId = pInput->nMadApplicationType;
      }
      break;
      case TABLE_SIZE_CMD:
      {
         AcdbLsmTableSizeCmdType *pInput = (AcdbLsmTableSizeCmdType *)pIn;
         lsmtblcmd.nDevId = pInput->nDeviceId;
         lsmtblcmd.nLSMAppTypeId = pInput->nMadApplicationType;
      }
      break;
      default:
         return  ACDB_ERROR;
      }

      cmd.devId = lsmtblcmd.nDevId;
      cmd.tblId = tblId;
      result = acdbdata_ioctl(ACDBDATACMD_GET_TABLE_INFO, (uint8_t *)&cmd, sizeof(cmd),
         (uint8_t *)&tblInfo, sizeof(tblInfo));
      if (result != ACDB_SUCCESS)
      {
         ACDB_DEBUG_LOG("Failed to fetch the lookup information of the device %08X \n"
            , lsmtblcmd.nDevId);
         return result;
      }
      lutTbl.nLen = READ_UInt32(tblInfo.tblLutChnk.pData);
      lutTbl.pLut = (LSMTblLookupType *)(tblInfo.tblLutChnk.pData + sizeof(lutTbl.nLen));

      result = AcdbDataBinarySearch((void *)lutTbl.pLut, lutTbl.nLen,
         LSM_LUT_INDICES_COUNT, &lsmtblcmd, LSM_CMD_INDICES_COUNT, &index);
      if (result != SEARCH_SUCCESS)
      {
         ACDB_DEBUG_LOG("Failed to fetch the lookup information of the device %08X \n"
            , lsmtblcmd.nDevId);
         return ACDB_INPUT_PARAMS_NOT_FOUND;
      }

      nLookupKey = (uintptr_t)&lutTbl.pLut[index];
      // Now get CDEF info
      ACDB_MEM_CPY(&tempLut, sizeof(tempLut), &lutTbl.pLut[index], sizeof(tempLut));
      ACDB_MEM_CPY(&cdefTbl.nLen, sizeof(uint32_t), (tblInfo.tblCdftChnk.pData + tempLut.nCDEFTblOffset), sizeof(uint32_t));

      cdefTbl.pCntDef = (ContentDefInstanceType *)(tblInfo.tblCdftChnk.pData + tempLut.nCDEFTblOffset + sizeof(cdefTbl.nLen));

      // Now get CDOT info
      ACDB_MEM_CPY(&cdotTbl.nLen, sizeof(uint32_t), (tblInfo.tblCdotChnk.pData + tempLut.nCDOTTblOffset), sizeof(uint32_t));
      cdotTbl.pDataOffsets = (uint32_t *)(tblInfo.tblCdotChnk.pData + tempLut.nCDOTTblOffset + sizeof(cdotTbl.nLen));

      if (cdefTbl.nLen != cdotTbl.nLen)
      {
         ACDB_DEBUG_LOG("The no of entries in CDEF and CDOT tables for devid %08X not matching\n", lsmtblcmd.nDevId);
         return ACDB_ERROR;
      }

      switch (queryType)
      {
      case TABLE_CMD:
      {
         AcdbLsmTableCmdType *pInput = (AcdbLsmTableCmdType *)pIn;
         AcdbQueryResponseType *pOutput = (AcdbQueryResponseType *)pOut;
         result = GetMidIidPidCalibTable(tblId, nLookupKey, cdefTbl, cdotTbl, tblInfo.dataPoolChnk,
            pInput->nBufferPointer, pInput->nBufferLength, &pOutput->nBytesUsedInBuffer);
         if (ACDB_SUCCESS != result)
         {
            return result;
         }
      }
      break;
      case TABLE_SIZE_CMD:
      {
         AcdbSizeResponseType *pOutput = (AcdbSizeResponseType *)pOut;
         if (ACDB_SUCCESS != GetMidIidPidCalibTableSize(tblId, nLookupKey, cdefTbl, cdotTbl, tblInfo.dataPoolChnk, &pOutput->nSize))
         {
            return ACDB_ERROR;
         }
      }
      break;
      default:
         return  ACDB_ERROR;
      }

      result = ACDB_SUCCESS;
   }

   return result;
}

int32_t AcdbCmdGetLSMTblInfo(uint32_t queryType, uint8_t *pIn, uint8_t *pOut)
{
   int32_t result = ACDB_SUCCESS;

   if (pIn == NULL || pOut == NULL)
   {
      ACDB_DEBUG_LOG("[ACDB Command]->[AcdbCmdGetLSMTblInfo]->Invalid NULL value parameters are provided\n");
      result = ACDB_BADPARM;
   }
   else
   {
      uint32_t index = 0;
      AcdbTableCmd cmd = { 0, 0 };
      AcdbTableInfo tblInfo = { { 0, NULL }, { 0, NULL }, { 0, NULL } };
      LSMTblLookupTblType lutTbl  = { 0, NULL };
      ContentDefTblType cdefTbl = { 0, NULL };
      ContentDataOffsetsTblType cdotTbl = { 0, NULL };
      uintptr_t nLookupKey = 0;
      LSMTblCmdLookupType lsmtblcmd  = { 0, 0 };
      uint32_t tblId = LSM_TBL;
      LSMTblLookupType tempLut  = { 0, 0, 0, 0 };

      switch (queryType)
      {
      case TABLE_CMD:
      {
         AcdbLsmTableCmdType *pInput = (AcdbLsmTableCmdType *)pIn;
         if (pInput->nBufferPointer == NULL)
         {
            return ACDB_BADPARM;
         }
         lsmtblcmd.nDevId = pInput->nDeviceId;
         lsmtblcmd.nLSMAppTypeId = pInput->nMadApplicationType;
      }
      break;
      case TABLE_SIZE_CMD:
      {
         AcdbLsmTableSizeCmdType *pInput = (AcdbLsmTableSizeCmdType *)pIn;
         lsmtblcmd.nDevId = pInput->nDeviceId;
         lsmtblcmd.nLSMAppTypeId = pInput->nMadApplicationType;
      }
      break;
      default:
         return  ACDB_ERROR;
      }

      cmd.devId = lsmtblcmd.nDevId;
      cmd.tblId = tblId;
      result = acdbdata_ioctl(ACDBDATACMD_GET_TABLE_INFO, (uint8_t *)&cmd, sizeof(cmd),
         (uint8_t *)&tblInfo, sizeof(tblInfo));
      if (result != ACDB_SUCCESS)
      {
         ACDB_DEBUG_LOG("Failed to fetch the lookup information of the device %08X \n"
            , lsmtblcmd.nDevId);
         return result;
      }
      lutTbl.nLen = READ_UInt32(tblInfo.tblLutChnk.pData);
      lutTbl.pLut = (LSMTblLookupType *)(tblInfo.tblLutChnk.pData + sizeof(lutTbl.nLen));

      result = AcdbDataBinarySearch((void *)lutTbl.pLut, lutTbl.nLen,
         LSM_LUT_INDICES_COUNT, &lsmtblcmd, LSM_CMD_INDICES_COUNT, &index);
      if (result != SEARCH_SUCCESS)
      {
         ACDB_DEBUG_LOG("Failed to fetch the lookup information of the device %08X \n"
            , lsmtblcmd.nDevId);
         return ACDB_INPUT_PARAMS_NOT_FOUND;
      }

      nLookupKey = (uintptr_t)&lutTbl.pLut[index];
      // Now get CDEF info
      ACDB_MEM_CPY(&tempLut, sizeof(tempLut), &lutTbl.pLut[index], sizeof(tempLut));
      ACDB_MEM_CPY(&cdefTbl.nLen, sizeof(uint32_t), (tblInfo.tblCdftChnk.pData + tempLut.nCDEFTblOffset), sizeof(uint32_t));

      cdefTbl.pCntDef = (ContentDefType *)(tblInfo.tblCdftChnk.pData + tempLut.nCDEFTblOffset + sizeof(cdefTbl.nLen));

      // Now get CDOT info
      ACDB_MEM_CPY(&cdotTbl.nLen, sizeof(uint32_t), (tblInfo.tblCdotChnk.pData + tempLut.nCDOTTblOffset), sizeof(uint32_t));
      cdotTbl.pDataOffsets = (uint32_t *)(tblInfo.tblCdotChnk.pData + tempLut.nCDOTTblOffset + sizeof(cdotTbl.nLen));

      if (cdefTbl.nLen != cdotTbl.nLen)
      {
         ACDB_DEBUG_LOG("The no of entries in CDEF and CDOT tables for devid %08X not matching\n", lsmtblcmd.nDevId);
         return ACDB_ERROR;
      }

      switch (queryType)
      {
      case TABLE_CMD:
      {
         AcdbLsmTableCmdType *pInput = (AcdbLsmTableCmdType *)pIn;
         AcdbQueryResponseType *pOutput = (AcdbQueryResponseType *)pOut;
         result = GetMidPidCalibTable(tblId, nLookupKey, cdefTbl, cdotTbl, tblInfo.dataPoolChnk,
            pInput->nBufferPointer, pInput->nBufferLength, &pOutput->nBytesUsedInBuffer);
         if (ACDB_SUCCESS != result)
         {
            return result;
         }
      }
      break;
      case TABLE_SIZE_CMD:
      {
         AcdbSizeResponseType *pOutput = (AcdbSizeResponseType *)pOut;
         if (ACDB_SUCCESS != GetMidPidCalibTableSize(tblId, nLookupKey, cdefTbl, cdotTbl, tblInfo.dataPoolChnk, &pOutput->nSize))
         {
            return ACDB_ERROR;
         }
      }
      break;
      default:
         return  ACDB_ERROR;
      }

      result = ACDB_SUCCESS;
   }

   return result;
}

int32_t AcdbCmdGetCodecFeaturesData(AcdbCodecCalDataCmdType *pInput,
   AcdbQueryResponseType *pOutput)
{
   int32_t result = ACDB_SUCCESS;

   if (pInput == NULL || pOutput == NULL)
   {
      ACDB_DEBUG_LOG("[ACDB Command]->[AcdbCmdGetCodecFeaturesData]->System Erorr\n");
      result = ACDB_BADPARM;
   }
   else if (pInput->pBufferPointer == NULL)
   {
      ACDB_DEBUG_LOG("[ACDB Command]->[AcdbCmdGetCodecFeaturesData]->NULL pointer\n");
      result = ACDB_BADPARM;
   }
   else
   {
      uint32_t index = 0;
      AcdbTableCmd cmd = { 0, 0 };
      AcdbTableInfo tblInfo = { { 0, NULL }, { 0, NULL }, { 0, NULL } };
	  CDCFeaturesDataLookupTblType lutTbl = { 0, NULL };
      uintptr_t nLookupKey = 0;
	  CDCFeaturesDataCmdLookupType cdcfdatacmd = { 0, 0 };
      uint32_t tblId = CDC_FEATURES_TBL;

      cdcfdatacmd.nDeviceId = pInput->nDeviceID;
      cdcfdatacmd.nPID = pInput->nCodecFeatureType;

      cmd.devId = cdcfdatacmd.nDeviceId;
      cmd.tblId = tblId;
      result = acdbdata_ioctl(ACDBDATACMD_GET_TABLE_INFO, (uint8_t *)&cmd, sizeof(cmd),
         (uint8_t *)&tblInfo, sizeof(tblInfo));
      if (result != ACDB_SUCCESS)
      {
         ACDB_DEBUG_LOG("Failed to fetch the lookup information of the device %08X \n"
            , cmd.devId);
         return result;
      }
      lutTbl.nLen = READ_UInt32(tblInfo.tblLutChnk.pData);
      lutTbl.pLut = (CDCFeaturesDataLookupType *)(tblInfo.tblLutChnk.pData + sizeof(lutTbl.nLen));
      result = AcdbDataBinarySearch((void *)lutTbl.pLut, lutTbl.nLen,
         CDC_FEATURES_DATA_LUT_INDICES_COUNT, &cdcfdatacmd, CDC_FEATURES_DATA_CMD_INDICES_COUNT, &index);
      if (result != SEARCH_SUCCESS)
      {
         ACDB_DEBUG_LOG("Failed to fetch the lookup information of the device %08X \n"
            , cmd.devId);
         return ACDB_INPUT_PARAMS_NOT_FOUND;
      }

      nLookupKey = (uintptr_t)&lutTbl.pLut[index];
      result = GetCalibData(tblId, nLookupKey, lutTbl.pLut[index].nDataOffset, tblInfo.dataPoolChnk,
         pInput->pBufferPointer, pInput->nBufferLength, &pOutput->nBytesUsedInBuffer);
      if (ACDB_SUCCESS != result)
      {
         return result;
      }
      result = ACDB_SUCCESS;
   }

   return result;
}

int32_t AcdbCmdGetAdieSidetoneTblInfo(uint32_t queryType, uint8_t *pIn, uint8_t *pOut)
{
   int32_t result = ACDB_SUCCESS;

   if (pIn == NULL || pOut == NULL)
   {
      ACDB_DEBUG_LOG("[ACDB Command]->[AcdbCmdGetAdieSidetoneTblInfo]->Invalid NULL value parameters are provided\n");
      result = ACDB_BADPARM;
   }
   else
   {
      uint32_t index = 0;
      AcdbTableCmd cmd = { 0, 0 };
      AcdbTableInfo tblInfo = { { 0, NULL }, { 0, NULL }, { 0, NULL } };
	  AdieSidetoneLookupTblType lutTbl = { 0, NULL };
      ContentDefTblType cdefTbl = { 0, NULL };
      ContentDataOffsetsTblType cdotTbl = { 0, NULL };
      uintptr_t nLookupKey = 0;
	  AdieSidetoneCmdLookupType lsmtblcmd = { 0, 0 };
      uint32_t tblId = ADIE_SIDETONE_TBL;
	  AdieSidetoneLookupType tempLut = { 0, 0, 0, 0};

      switch (queryType)
      {
      case TABLE_CMD:
      {
         AcdbAdieSidetoneTableCmdType *pInput = (AcdbAdieSidetoneTableCmdType *)pIn;
         if (pInput->nBufferPointer == NULL)
         {
            return ACDB_BADPARM;
         }
         lsmtblcmd.nTxDevId = pInput->nTxDeviceId;
         lsmtblcmd.nRxDevId = pInput->nRxDeviceId;
      }
      break;
      default:
         return  ACDB_ERROR;
      }

      cmd.devId = lsmtblcmd.nTxDevId;
      cmd.tblId = tblId;
      result = acdbdata_ioctl(ACDBDATACMD_GET_TABLE_INFO, (uint8_t *)&cmd, sizeof(cmd),
         (uint8_t *)&tblInfo, sizeof(tblInfo));
      if (result != ACDB_SUCCESS)
      {
         ACDB_DEBUG_LOG("Failed to fetch the lookup information of the device %08X \n"
            , lsmtblcmd.nTxDevId);
         return result;
      }
      lutTbl.nLen = READ_UInt32(tblInfo.tblLutChnk.pData);
      lutTbl.pLut = (AdieSidetoneLookupType *)(tblInfo.tblLutChnk.pData + sizeof(lutTbl.nLen));

      result = AcdbDataBinarySearch((void *)lutTbl.pLut, lutTbl.nLen,
         ADST_LUT_INDICES_COUNT, &lsmtblcmd, ADST_CMD_INDICES_COUNT, &index);
      if (result != SEARCH_SUCCESS)
      {
         ACDB_DEBUG_LOG("Failed to fetch the lookup information of the device %08X \n"
            , lsmtblcmd.nTxDevId);
         return ACDB_INPUT_PARAMS_NOT_FOUND;
      }

      nLookupKey = (uintptr_t)&lutTbl.pLut[index];
      // Now get CDEF info
      ACDB_MEM_CPY(&tempLut, sizeof(tempLut), &lutTbl.pLut[index], sizeof(tempLut));
      ACDB_MEM_CPY(&cdefTbl.nLen, sizeof(uint32_t), (tblInfo.tblCdftChnk.pData + tempLut.nCDEFTblOffset), sizeof(uint32_t));
      cdefTbl.pCntDef = (ContentDefType *)(tblInfo.tblCdftChnk.pData + tempLut.nCDEFTblOffset + sizeof(cdefTbl.nLen));

      // Now get CDOT info
      ACDB_MEM_CPY(&cdotTbl.nLen, sizeof(uint32_t), (tblInfo.tblCdotChnk.pData + tempLut.nCDOTTblOffset), sizeof(uint32_t));
      cdotTbl.pDataOffsets = (uint32_t *)(tblInfo.tblCdotChnk.pData + tempLut.nCDOTTblOffset + sizeof(cdotTbl.nLen));

      if (cdefTbl.nLen != cdotTbl.nLen)
      {
         ACDB_DEBUG_LOG("The no of entries in CDEF and CDOT tables for devid %08X not matching\n", lsmtblcmd.nTxDevId);
         return ACDB_ERROR;
      }

      switch (queryType)
      {
      case TABLE_CMD:
      {
         AcdbAdieSidetoneTableCmdType *pInput = (AcdbAdieSidetoneTableCmdType *)pIn;
         AcdbQueryResponseType *pOutput = (AcdbQueryResponseType *)pOut;
         result = GetMidPidCalibTable(tblId, nLookupKey, cdefTbl, cdotTbl, tblInfo.dataPoolChnk,
            pInput->nBufferPointer, pInput->nBufferLength, &pOutput->nBytesUsedInBuffer);
         if (ACDB_SUCCESS != result)
         {
            return result;
         }
      }
      break;
      default:
         return  ACDB_ERROR;
      }

      result = ACDB_SUCCESS;
   }

   return result;
}

int32_t AcdbCmdGetAudioCOPPTopologyData(AcdbQueryCmdType *pInput, AcdbQueryResponseType *pOutput)
{
   int32_t result = ACDB_SUCCESS;

   if (pInput == NULL || pOutput == NULL)
   {
      ACDB_DEBUG_LOG("ACDB_COMMAND: Provided invalid param\n");
      result = ACDB_BADPARM;
   }
   else
   {
      AcdbGlbalPropInfo glbPropInfo = { 0, { 0, NULL } };
      memset(&glbPropInfo, 0, sizeof(glbPropInfo));
      glbPropInfo.pId = AUDIO_COPP_TOPO_INFO_ID;
      result = acdbdata_ioctl(ACDBDATACMD_GET_GLOBAL_PROP, (uint8_t *)&glbPropInfo, sizeof(AcdbGlbalPropInfo),
         (uint8_t *)NULL, 0);
      if (result != ACDB_SUCCESS)
      {
         ACDB_DEBUG_LOG("ACDB_COMMAND: Failed to fetch the property info for pid %08X \n", glbPropInfo.pId);
         return ACDB_DATA_NOT_FOUND;
      }
      if (NULL == glbPropInfo.dataInfo.pData)
      {
         ACDB_DEBUG_LOG("ACDB_COMMAND: pData NULL on fetch the property info for pid %08X \n", glbPropInfo.pId);
         return ACDB_DATA_NOT_FOUND;
      }

      if (pInput->nBufferLength < glbPropInfo.dataInfo.nDataLen)
      {
         ACDB_DEBUG_LOG("ACDB_COMMAND: Insufficient memory provided to fill the audio copp topology info for pid %08X \n", glbPropInfo.pId);
         return ACDB_INSUFFICIENTMEMORY;
      }

      ACDB_MEM_CPY(pInput->pBufferPointer, pInput->nBufferLength, glbPropInfo.dataInfo.pData, glbPropInfo.dataInfo.nDataLen);
      pOutput->nBytesUsedInBuffer = glbPropInfo.dataInfo.nDataLen;
   }
   return result;
}

int32_t AcdbCmdGetAudioCOPPTopologyDataSize(AcdbSizeResponseType *pOutput)
{
   int32_t result = ACDB_SUCCESS;

   if (pOutput == NULL)
   {
      ACDB_DEBUG_LOG("ACDB_COMMAND: Provided invalid param\n");
      result = ACDB_BADPARM;
   }
   else
   {
      AcdbGlbalPropInfo glbPropInfo = { 0, { 0, NULL } };
      memset(&glbPropInfo, 0, sizeof(glbPropInfo));
      glbPropInfo.pId = AUDIO_COPP_TOPO_INFO_ID;
      result = acdbdata_ioctl(ACDBDATACMD_GET_GLOBAL_PROP, (uint8_t *)&glbPropInfo, sizeof(AcdbGlbalPropInfo),
         (uint8_t *)NULL, 0);
      if (result != ACDB_SUCCESS)
      {
         ACDB_DEBUG_LOG("ACDB_COMMAND: Failed to fetch the property info for pid %08X \n", glbPropInfo.pId);
         return ACDB_ERROR;
      }
      if (NULL == glbPropInfo.dataInfo.pData)
      {
         ACDB_DEBUG_LOG("ACDB_COMMAND: pData NULL on fetch the property info for pid %08X \n", glbPropInfo.pId);
         return ACDB_ERROR;
      }

      pOutput->nSize = glbPropInfo.dataInfo.nDataLen;
   }
   return result;
}

int32_t AcdbCmdGetAudioPOPPTopologyData(AcdbQueryCmdType *pInput, AcdbQueryResponseType *pOutput)
{
   int32_t result = ACDB_SUCCESS;

   if (pInput == NULL || pOutput == NULL)
   {
      ACDB_DEBUG_LOG("ACDB_COMMAND: Provided invalid param\n");
      result = ACDB_BADPARM;
   }
   else if (pInput->pBufferPointer == NULL)
   {
      result = ACDB_BADPARM;
   }
   else
   {
      AcdbGlbalPropInfo glbPropInfo = { 0, { 0, NULL } };
      memset(&glbPropInfo, 0, sizeof(glbPropInfo));
      glbPropInfo.pId = AUDIO_POPP_TOPO_INFO_ID;
      result = acdbdata_ioctl(ACDBDATACMD_GET_GLOBAL_PROP, (uint8_t *)&glbPropInfo, sizeof(AcdbGlbalPropInfo),
         (uint8_t *)NULL, 0);
      if (result != ACDB_SUCCESS)
      {
         ACDB_DEBUG_LOG("ACDB_COMMAND: Failed to fetch the property info for pid %08X \n", glbPropInfo.pId);
         return ACDB_DATA_NOT_FOUND;
      }
      if (NULL == glbPropInfo.dataInfo.pData)
      {
         ACDB_DEBUG_LOG("ACDB_COMMAND: pData NULL on fetch the property info for pid %08X \n", glbPropInfo.pId);
         return ACDB_DATA_NOT_FOUND;
      }

      if (pInput->nBufferLength < glbPropInfo.dataInfo.nDataLen)
      {
         ACDB_DEBUG_LOG("ACDB_COMMAND: Insufficient memory provided to fill the audio copp topology info for pid %08X \n", glbPropInfo.pId);
         return ACDB_INSUFFICIENTMEMORY;
      }

      ACDB_MEM_CPY(pInput->pBufferPointer, pInput->nBufferLength, glbPropInfo.dataInfo.pData, glbPropInfo.dataInfo.nDataLen);
      pOutput->nBytesUsedInBuffer = glbPropInfo.dataInfo.nDataLen;
   }
   return result;
}

int32_t AcdbCmdGetAudioPOPPTopologyDataSize(AcdbSizeResponseType *pOutput)
{
   int32_t result = ACDB_SUCCESS;

   if (pOutput == NULL)
   {
      ACDB_DEBUG_LOG("ACDB_COMMAND: Provided invalid param\n");
      result = ACDB_BADPARM;
   }
   else
   {
      AcdbGlbalPropInfo glbPropInfo = { 0, { 0, NULL } };
      memset(&glbPropInfo, 0, sizeof(glbPropInfo));
      glbPropInfo.pId = AUDIO_POPP_TOPO_INFO_ID;
      result = acdbdata_ioctl(ACDBDATACMD_GET_GLOBAL_PROP, (uint8_t *)&glbPropInfo, sizeof(AcdbGlbalPropInfo),
         (uint8_t *)NULL, 0);
      if (result != ACDB_SUCCESS)
      {
         ACDB_DEBUG_LOG("ACDB_COMMAND: Failed to fetch the property info for pid %08X \n", glbPropInfo.pId);
         return ACDB_ERROR;
      }
      if (NULL == glbPropInfo.dataInfo.pData)
      {
         ACDB_DEBUG_LOG("ACDB_COMMAND: pData NULL on fetch the property info for pid %08X \n", glbPropInfo.pId);
         return ACDB_ERROR;
      }

      pOutput->nSize = glbPropInfo.dataInfo.nDataLen;
   }
   return result;
}
int32_t AcdbCmdGetAVCSCustomTopoInfo(AcdbQueryCmdType *pInput, AcdbQueryResponseType *pOutput)
{
   int32_t result = ACDB_SUCCESS;

   if (pInput == NULL || pOutput == NULL)
   {
      ACDB_DEBUG_LOG("ACDB_COMMAND: Provided invalid param\n");
      result = ACDB_BADPARM;
   }
   else if (pInput->pBufferPointer == NULL)
   {
      return ACDB_BADPARM;
   }
   else
   {
      AcdbGlbalPropInfo glbPropInfo = { 0, { 0, NULL } };
      memset(&glbPropInfo, 0, sizeof(glbPropInfo));
      glbPropInfo.pId = ACDB_GBL_PROPID_AVS_TOPO_INFO;
      result = acdbdata_ioctl(ACDBDATACMD_GET_GLOBAL_PROP, (uint8_t *)&glbPropInfo, sizeof(AcdbGlbalPropInfo),
         (uint8_t *)NULL, 0);
      if (result != ACDB_SUCCESS)
      {
         switch (result)
         {
         case ACDB_ERROR:
            ACDB_DEBUG_LOG("ACDB_COMMAND: Global acdb file not loaded to fetch data\n");
            break;

         case ACDB_BADPARM:
            ACDB_DEBUG_LOG("ACDB_COMMAND: Received NULL input for AcdbDataGetGlobalPropData\n");
            break;

         case ACDB_DATA_NOT_FOUND:
            ACDB_DEBUG_LOG("ACDB_COMMAND: Failed to fetch the property info for pid %08X \n", glbPropInfo.pId);
            break;

         default:
            ACDB_DEBUG_LOG("ACDB_COMMAND: Invalid command");
            break;
         }

         return result;
      }

      if (NULL == glbPropInfo.dataInfo.pData)
      {
         ACDB_DEBUG_LOG("ACDB_COMMAND: pData NULL on fetch the property info for pid %08X \n", glbPropInfo.pId);
         return ACDB_DATA_NOT_FOUND;
      }

      if (pInput->nBufferLength < glbPropInfo.dataInfo.nDataLen)
      {
         ACDB_DEBUG_LOG("ACDB_COMMAND: Insufficient memory provided to fill the AVCS custom topo info for pid %08X \n", glbPropInfo.pId);
         return ACDB_INSUFFICIENTMEMORY;
      }

      ACDB_MEM_CPY(pInput->pBufferPointer, pInput->nBufferLength, glbPropInfo.dataInfo.pData, glbPropInfo.dataInfo.nDataLen);
      pOutput->nBytesUsedInBuffer = glbPropInfo.dataInfo.nDataLen;
   }
   return result;
}

int32_t AcdbCmdGetAVCSCustomTopoInfoSize(AcdbSizeResponseType *pOutput)
{
   int32_t result = ACDB_SUCCESS;

   if (pOutput == NULL)
   {
      ACDB_DEBUG_LOG("ACDB_COMMAND: Provided invalid param\n");
      result = ACDB_BADPARM;
   }
   else
   {
      AcdbGlbalPropInfo glbPropInfo = { 0, { 0, NULL } };
      memset(&glbPropInfo, 0, sizeof(glbPropInfo));
      glbPropInfo.pId = ACDB_GBL_PROPID_AVS_TOPO_INFO;
      result = acdbdata_ioctl(ACDBDATACMD_GET_GLOBAL_PROP, (uint8_t *)&glbPropInfo, sizeof(AcdbGlbalPropInfo),
         (uint8_t *)NULL, 0);
      if (result != ACDB_SUCCESS)
      {
         switch (result)
         {
         case ACDB_ERROR:
            ACDB_DEBUG_LOG("ACDB_COMMAND: Global acdb file not loaded to fetch data\n");
            break;

         case ACDB_BADPARM:
            ACDB_DEBUG_LOG("ACDB_COMMAND: Received NULL input for AcdbDataGetGlobalPropData\n");
            break;

         case ACDB_DATA_NOT_FOUND:
            ACDB_DEBUG_LOG("ACDB_COMMAND: Failed to fetch the property info for pid %08X \n", glbPropInfo.pId);
            break;

         default:
            ACDB_DEBUG_LOG("ACDB_COMMAND: Invalid command");
            break;
         }

         return result;
      }
      if (NULL == glbPropInfo.dataInfo.pData)
      {
         ACDB_DEBUG_LOG("ACDB_COMMAND: pData NULL on fetch the property info for pid %08X \n", glbPropInfo.pId);
         return ACDB_DATA_NOT_FOUND;
      }

      pOutput->nSize = glbPropInfo.dataInfo.nDataLen;
   }
   return result;
}
int32_t AcdbCmdGetAVCSCustomTopoInfo_V3(AcdbQueryCmdType *pInput, AcdbQueryResponseType *pOutput)
{
   int32_t result = ACDB_SUCCESS;

   if (pInput == NULL || pOutput == NULL)
   {
      ACDB_DEBUG_LOG("ACDB_COMMAND: Provided invalid param\n");
      result = ACDB_BADPARM;
   }
   else if (pInput->pBufferPointer == NULL)
   {
      return ACDB_BADPARM;
   }
   else
   {
      AcdbGlbalPropInfo glbPropInfo = { 0, { 0, NULL } };
      memset(&glbPropInfo, 0, sizeof(glbPropInfo));
      glbPropInfo.pId = ACDB_GBL_PROPID_AVS_TOPO_INFO_V3;
      result = acdbdata_ioctl(ACDBDATACMD_GET_GLOBAL_PROP, (uint8_t *)&glbPropInfo, sizeof(AcdbGlbalPropInfo),
         (uint8_t *)NULL, 0);
      if (result != ACDB_SUCCESS)
      {
         switch (result)
         {
         case ACDB_ERROR:
            ACDB_DEBUG_LOG("ACDB_COMMAND: Global acdb file not loaded to fetch data\n");
            break;

         case ACDB_BADPARM:
            ACDB_DEBUG_LOG("ACDB_COMMAND: Received NULL input for AcdbDataGetGlobalPropData\n");
            break;

         case ACDB_DATA_NOT_FOUND:
            ACDB_DEBUG_LOG("ACDB_COMMAND: Failed to fetch the property info for pid %08X \n", glbPropInfo.pId);
            break;

         default:
            ACDB_DEBUG_LOG("ACDB_COMMAND: Invalid command");
            break;
         }

         return result;
      }

      if (NULL == glbPropInfo.dataInfo.pData)
      {
         ACDB_DEBUG_LOG("ACDB_COMMAND: pData NULL on fetch the property info for pid %08X \n", glbPropInfo.pId);
         return ACDB_DATA_NOT_FOUND;
      }

      if (pInput->nBufferLength < glbPropInfo.dataInfo.nDataLen)
      {
         ACDB_DEBUG_LOG("ACDB_COMMAND: Insufficient memory provided to fill the AVCS custom topo info V3 for pid %08X \n", glbPropInfo.pId);
         return ACDB_INSUFFICIENTMEMORY;
      }

      ACDB_MEM_CPY(pInput->pBufferPointer, pInput->nBufferLength, glbPropInfo.dataInfo.pData, glbPropInfo.dataInfo.nDataLen);
      pOutput->nBytesUsedInBuffer = glbPropInfo.dataInfo.nDataLen;
   }
   return result;
}

int32_t AcdbCmdGetAVCSCustomTopoInfoSize_V3(AcdbSizeResponseType *pOutput)
{
   int32_t result = ACDB_SUCCESS;

   if (pOutput == NULL)
   {
      ACDB_DEBUG_LOG("ACDB_COMMAND: Provided invalid param\n");
      result = ACDB_BADPARM;
   }
   else
   {
      AcdbGlbalPropInfo glbPropInfo = { 0, { 0, NULL } };
      memset(&glbPropInfo, 0, sizeof(glbPropInfo));
      glbPropInfo.pId = ACDB_GBL_PROPID_AVS_TOPO_INFO_V3;
      result = acdbdata_ioctl(ACDBDATACMD_GET_GLOBAL_PROP, (uint8_t *)&glbPropInfo, sizeof(AcdbGlbalPropInfo),
         (uint8_t *)NULL, 0);
      if (result != ACDB_SUCCESS)
      {
         switch (result)
         {
         case ACDB_ERROR:
            ACDB_DEBUG_LOG("ACDB_COMMAND: Global acdb file not loaded to fetch data\n");
            break;

         case ACDB_BADPARM:
            ACDB_DEBUG_LOG("ACDB_COMMAND: Received NULL input for AcdbDataGetGlobalPropData\n");
            break;

         case ACDB_DATA_NOT_FOUND:
            ACDB_DEBUG_LOG("ACDB_COMMAND: Failed to fetch the property info for pid %08X \n", glbPropInfo.pId);
            break;

         default:
            ACDB_DEBUG_LOG("ACDB_COMMAND: Invalid command");
            break;
         }

         return result;
      }
      if (NULL == glbPropInfo.dataInfo.pData)
      {
         ACDB_DEBUG_LOG("ACDB_COMMAND: pData NULL on fetch the property info for pid %08X \n", glbPropInfo.pId);
         return ACDB_DATA_NOT_FOUND;
      }

      pOutput->nSize = glbPropInfo.dataInfo.nDataLen;
   }
   return result;
}

int32_t AcdbCmdGetAfeTopologyData(AcdbQueryCmdType *pInput, AcdbQueryResponseType *pOutput)
{
   int32_t result = ACDB_SUCCESS;

   if (pInput == NULL || pOutput == NULL)
   {
      ACDB_DEBUG_LOG("ACDB_COMMAND: Provided invalid param\n");
      result = ACDB_BADPARM;
   }
   else
   {
      AcdbGlbalPropInfo glbPropInfo = { 0, { 0, NULL } };
      memset(&glbPropInfo, 0, sizeof(glbPropInfo));
      glbPropInfo.pId = AFE_TOPO_INFO_ID;
      result = acdbdata_ioctl(ACDBDATACMD_GET_GLOBAL_PROP, (uint8_t *)&glbPropInfo, sizeof(AcdbGlbalPropInfo),
         (uint8_t *)NULL, 0);
      if (result != ACDB_SUCCESS)
      {
         ACDB_DEBUG_LOG("ACDB_COMMAND: Failed to fetch the property info for pid %08X \n", glbPropInfo.pId);
         return ACDB_DATA_NOT_FOUND;
      }
      if (NULL == glbPropInfo.dataInfo.pData)
      {
         ACDB_DEBUG_LOG("ACDB_COMMAND: pData NULL on fetch the property info for pid %08X \n", glbPropInfo.pId);
         return ACDB_DATA_NOT_FOUND;
      }

      if (pInput->nBufferLength < glbPropInfo.dataInfo.nDataLen)
      {
         ACDB_DEBUG_LOG("ACDB_COMMAND: Insufficient memory provided to fill the afe topology info for pid %08X \n", glbPropInfo.pId);
         return ACDB_INSUFFICIENTMEMORY;
      }

      ACDB_MEM_CPY(pInput->pBufferPointer, pInput->nBufferLength, glbPropInfo.dataInfo.pData, glbPropInfo.dataInfo.nDataLen);
      pOutput->nBytesUsedInBuffer = glbPropInfo.dataInfo.nDataLen;
   }
   return result;
}

int32_t AcdbCmdGetAfeTopologyDataSize(AcdbSizeResponseType *pOutput)
{
   int32_t result = ACDB_SUCCESS;

   if (pOutput == NULL)
   {
      ACDB_DEBUG_LOG("ACDB_COMMAND: Provided invalid param\n");
      result = ACDB_BADPARM;
   }
   else
   {
      AcdbGlbalPropInfo glbPropInfo = { 0, { 0, NULL } };
      memset(&glbPropInfo, 0, sizeof(glbPropInfo));
      glbPropInfo.pId = AFE_TOPO_INFO_ID;
      result = acdbdata_ioctl(ACDBDATACMD_GET_GLOBAL_PROP, (uint8_t *)&glbPropInfo, sizeof(AcdbGlbalPropInfo),
         (uint8_t *)NULL, 0);
      if (result != ACDB_SUCCESS)
      {
         ACDB_DEBUG_LOG("ACDB_COMMAND: Failed to fetch the property info for pid %08X \n", glbPropInfo.pId);
         return ACDB_ERROR;
      }
      if (NULL == glbPropInfo.dataInfo.pData)
      {
         ACDB_DEBUG_LOG("ACDB_COMMAND: pData NULL on fetch the property info for pid %08X \n", glbPropInfo.pId);
         return ACDB_ERROR;
      }

      pOutput->nSize = glbPropInfo.dataInfo.nDataLen;
   }
   return result;
}

int32_t AcdbCmdGetLsmTopologyData(AcdbQueryCmdType *pInput, AcdbQueryResponseType *pOutput)
{
   int32_t result = ACDB_SUCCESS;

   if (pInput == NULL || pOutput == NULL)
   {
      ACDB_DEBUG_LOG("ACDB_COMMAND: Provided invalid param\n");
      result = ACDB_BADPARM;
   }
   else if (pInput->pBufferPointer == NULL)
   {
      result = ACDB_BADPARM;
   }
   else
   {
      AcdbGlbalPropInfo glbPropInfo = { 0, { 0, NULL } };
      memset(&glbPropInfo, 0, sizeof(glbPropInfo));
      glbPropInfo.pId = LSM_TOPO_INFO_ID;
      result = acdbdata_ioctl(ACDBDATACMD_GET_GLOBAL_PROP, (uint8_t *)&glbPropInfo, sizeof(AcdbGlbalPropInfo),
         (uint8_t *)NULL, 0);
      if (result != ACDB_SUCCESS)
      {
         ACDB_DEBUG_LOG("ACDB_COMMAND: Failed to fetch the property info for pid %08X \n", glbPropInfo.pId);
         return ACDB_DATA_NOT_FOUND;
      }
      if (NULL == glbPropInfo.dataInfo.pData)
      {
         ACDB_DEBUG_LOG("ACDB_COMMAND: pData NULL on fetch the property info for pid %08X \n", glbPropInfo.pId);
         return ACDB_DATA_NOT_FOUND;
      }

      if (pInput->nBufferLength < glbPropInfo.dataInfo.nDataLen)
      {
         ACDB_DEBUG_LOG("ACDB_COMMAND: Insufficient memory provided to fill the lsm topology info for pid %08X \n", glbPropInfo.pId);
         return ACDB_INSUFFICIENTMEMORY;
      }

      ACDB_MEM_CPY(pInput->pBufferPointer, pInput->nBufferLength, glbPropInfo.dataInfo.pData, glbPropInfo.dataInfo.nDataLen);
      pOutput->nBytesUsedInBuffer = glbPropInfo.dataInfo.nDataLen;
   }
   return result;
}

int32_t AcdbCmdGetLsmTopologyDataSize(AcdbSizeResponseType *pOutput)
{
   int32_t result = ACDB_SUCCESS;

   if (pOutput == NULL)
   {
      ACDB_DEBUG_LOG("ACDB_COMMAND: Provided invalid param\n");
      result = ACDB_BADPARM;
   }
   else
   {
      AcdbGlbalPropInfo glbPropInfo = { 0, { 0, NULL } };
      memset(&glbPropInfo, 0, sizeof(glbPropInfo));
      glbPropInfo.pId = LSM_TOPO_INFO_ID;
      result = acdbdata_ioctl(ACDBDATACMD_GET_GLOBAL_PROP, (uint8_t *)&glbPropInfo, sizeof(AcdbGlbalPropInfo),
         (uint8_t *)NULL, 0);
      if (result != ACDB_SUCCESS)
      {
         ACDB_DEBUG_LOG("ACDB_COMMAND: Failed to fetch the property info for pid %08X \n", glbPropInfo.pId);
         return ACDB_ERROR;
      }
      if (NULL == glbPropInfo.dataInfo.pData)
      {
         ACDB_DEBUG_LOG("ACDB_COMMAND: pData NULL on fetch the property info for pid %08X \n", glbPropInfo.pId);
         return ACDB_ERROR;
      }

      pOutput->nSize = glbPropInfo.dataInfo.nDataLen;
   }
   return result;
}

int32_t AcdbCmdGetDeviceProperty(uint32_t queryType, uint8_t *pIn, uint8_t *pOut)
{
   int32_t result = ACDB_SUCCESS;

   if (pIn == NULL || pOut == NULL)
   {
      ACDB_DEBUG_LOG("[ACDB Command]->[AcdbCmdGetDeviceProperty]->Invalid NULL value parameters are provided\n");
      result = ACDB_BADPARM;
   }
   else
   {
      uint8_t *pOutBufPtr = NULL;
      uint32_t nOutBufLength = 0;
      //uint32_t nDeviceId = 0;
	  AcdbDevPropertyType devPropID = ACDB_AVSYNC_INFO;
      AcdbDevPropInfo devPropInfo = { 0, 0, { 0, NULL} };
      memset(&devPropInfo, 0, sizeof(devPropInfo));

      switch (queryType)
      {
      case DATA_CMD:
      {
         AcdbDevPropCmdType *pInput = (AcdbDevPropCmdType *)pIn;
         if (pInput->pBufferPointer == NULL)
         {
            return ACDB_BADPARM;
         }
         devPropInfo.devId = pInput->nDeviceId;
         devPropID = (AcdbDevPropertyType)pInput->nPropID;
         nOutBufLength = pInput->nBufferLength;
         pOutBufPtr = pInput->pBufferPointer;

         if (pOutBufPtr == NULL)
         {
            ACDB_DEBUG_LOG("[ACDB Command]->[AcdbCmdGetDeviceProperty]->Payload buffer pointer is NULL\n");
            return ACDB_BADPARM;
         }
      }
      break;
      case DATA_SIZE_CMD:
      {
         AcdbDevPropSizeCmdType *pInput = (AcdbDevPropSizeCmdType *)pIn;
         devPropInfo.devId = pInput->nDeviceId;
         devPropID = (AcdbDevPropertyType)pInput->nPropID;
      }
      break;
      default:
      {
         ACDB_DEBUG_LOG("[ACDB Command]->[AcdbCmdGetDeviceProperty]->Invalid query %u\n",
            queryType);
         return  ACDB_ERROR;
      }
      }

      switch (devPropID)
      {
      case ACDB_AVSYNC_INFO:
         devPropInfo.pId = AV_SYNC_DELAY;
         break;
      case ACDB_CHANNEL_MAPPING_CONFIG:
         devPropInfo.pId = CHANNELTYPE;
         break;
      case ACDB_DEVICE_TYPE_GROUP_TDM_CONFIG_PARAM:
         devPropInfo.pId = ACDB_DEVICE_TYPE_GROUP_TDM_CONFIG_PARAM;
         break;
      case ACDB_DEVICE_TYPE_TDM_SLOT_CONFIG_V2_PARAM:
         devPropInfo.pId = ACDB_DEVICE_TYPE_TDM_SLOT_CONFIG_V2_PARAM;
         break;

      case ACDB_CHANNEL_MAPPING_CONFIG_V2:
         devPropInfo.pId = CHANNELTYPE_V2;
         break;

		 case ACDB_DEVICE_TYPE_TDM_SLOT_CONFIG_V2_PARAM_V2:
         devPropInfo.pId = ACDB_DEVICE_TYPE_TDM_SLOT_CONFIG_V2_PARAM_V2;
         break;

      default:
      {
         ACDB_DEBUG_LOG("[ACDB Command]->[AcdbCmdGetDeviceProperty]->Invalid device property ID %d\n",
            devPropID);
         return ACDB_ERROR;
      }
      }

      result = acdbdata_ioctl(
         ACDBDATACMD_GET_DEVICE_PROP,
         (uint8_t *)&devPropInfo,
         sizeof(AcdbDevPropInfo),
         (uint8_t *)NULL,
         0
         );
      if (result != ACDB_SUCCESS)
      {
         ACDB_DEBUG_LOG("[ACDB Command]->[AcdbCmdGetDeviceProperty]->Failed to fetch the device property of the device %08X\n",
            devPropInfo.devId);
         return result;
      }

      switch (queryType)
      {
      case DATA_CMD:
      {
         AcdbQueryResponseType *pOutput = (AcdbQueryResponseType *)pOut;

         if (nOutBufLength < devPropInfo.dataInfo.nDataLen)
         {
            ACDB_DEBUG_LOG("[ACDB Command]->[AcdbCmdGetDeviceProperty]->Insufficient buffer size to copy the device property data for device %08X\n",
               devPropInfo.devId);
            return ACDB_INSUFFICIENTMEMORY;
         }

         ACDB_MEM_CPY(pOutBufPtr, nOutBufLength, devPropInfo.dataInfo.pData, devPropInfo.dataInfo.nDataLen);
         pOutput->nBytesUsedInBuffer = devPropInfo.dataInfo.nDataLen;
      }
      break;
      case DATA_SIZE_CMD:
      {
         AcdbSizeResponseType *pOutput = (AcdbSizeResponseType *)pOut;
         pOutput->nSize = devPropInfo.dataInfo.nDataLen;
      }
      break;
      default:
      {
         ACDB_DEBUG_LOG("[ACDB Command]->[AcdbCmdGetDeviceProperty]->Invalid query %u\n", queryType);
         result = ACDB_ERROR;
      }
      }
   }

   return result;
}

int32_t GetVP3InfoFromUseCaseId(int32_t useCaseId, uint32_t *tblId, int32_t *lutIndicesCount, int32_t *cmdIndicesCount, int32_t *tblIndicesCount, int32_t *cdftIndicesCount)
{
   int32_t result = ACDB_SUCCESS;
   switch (useCaseId)
   {
   case ACDB_VP3_VOICE_USECASE:
      *tblId = VOICE_VP3_TBL;
      *lutIndicesCount = VOICE_VP3_LUT_INDICES_COUNT;
      *cmdIndicesCount = VOICE_VP3_CMD_INDICES_COUNT;
      *tblIndicesCount = VOICE_VP3TBL_INDICES_COUNT;
	  *cdftIndicesCount = VOICE_VP3TBL_CDFT_INDICES_COUNT;
      break;
   case ACDB_VP3_AUDIO_REC_USECASE:
      *tblId = AUDIO_REC_VP3_TBL;
      *lutIndicesCount = AUDREC_VP3_LUT_INDICES_COUNT;
      *cmdIndicesCount = AUDREC_VP3_CMD_INDICES_COUNT;
      *tblIndicesCount = AUDREC_VP3TBL_INDICES_COUNT;
	  *cdftIndicesCount = AUDREC_VP3TBL_CDFT_INDICES_COUNT;
      break;
   case ACDB_VP3_AUDIO_WITH_EC_USECASE:
      *tblId = AUDIO_REC_EC_VP3_TBL;
      *lutIndicesCount = AUDREC_EC_VP3_LUT_INDICES_COUNT;
      *cmdIndicesCount = AUDREC_EC_VP3_CMD_INDICES_COUNT;
      *tblIndicesCount = AUDREC_EC_VP3TBL_INDICES_COUNT;
	  *cdftIndicesCount = AUDREC_EC_VP3TBL_CDFT_INDICES_COUNT;
      break;
   default:
      result = ACDB_ERROR;
      break;
   }

   return result;
}

int32_t GetMaxLenPrpty(MaxLenDefPrptyType *maxLenPrpty)
{
   int32_t result = ACDB_SUCCESS;
   AcdbGlbalPropInfo glbPropInfo = { 0, { 0, NULL } };
   memset(&glbPropInfo, 0, sizeof(glbPropInfo));

   glbPropInfo.pId = MID_PID_MAX_LEN;

   result = acdbdata_ioctl(ACDBDATACMD_GET_GLOBAL_PROP, (uint8_t *)&glbPropInfo, sizeof(AcdbGlbalPropInfo),
      (uint8_t *)NULL, 0);
   if (result != ACDB_SUCCESS)
   {
      ACDB_DEBUG_LOG("ACDB_COMMAND: Failed to fetch the property info for pid %08X \n", glbPropInfo.pId);
      return ACDB_DATA_NOT_FOUND;
   }
   if (NULL == glbPropInfo.dataInfo.pData)
   {
      ACDB_DEBUG_LOG("ACDB_COMMAND: pData NULL on fetch the property info for pid %08X \n", glbPropInfo.pId);
      return ACDB_DATA_NOT_FOUND;
   }

   maxLenPrpty->nLen = READ_UInt32(glbPropInfo.dataInfo.pData);
   maxLenPrpty->pCntDef = (MaxLenDefType *)(glbPropInfo.dataInfo.pData + sizeof(maxLenPrpty->nLen));

   return result;
}

int32_t AcdbCmdGetVP3MidPidList(AcdbVP3MidPidListCmdType *pInput,
   AcdbQueryResponseType *pOutput)
{
   int32_t result = ACDB_SUCCESS;
   uint32_t i = 0;
   uint32_t j = 0;

   if (pInput == NULL || pOutput == NULL)
   {
      ACDB_DEBUG_LOG("ACDB_COMMAND: Provided invalid param\n");
      result = ACDB_BADPARM;
   }
   else if (pInput->nBufferPointer == NULL)
   {
      result = ACDB_BADPARM;
   }
   else
   {
      uint32_t index = 0;
      uint32_t tblId = 0;
      int32_t lutIndicesCount = 0;
      int32_t cmdIndicesCount = 0;
      int32_t tblIndicesCount = 0;
	  int32_t cdftIndicesCount = 0;
      uint32_t dataOffset = 0;
      uint32_t copiedMaxLen = 0;
	  MaxLenDefPrptyType maxLenPrpty = { 0, NULL };
      ContentDefTblType cdefTbl = { 0, NULL };
      AcdbTableCmd cmd = { 0, 0 };

      AcdbTableInfo tblInfo = { { 0, NULL }, { 0, NULL }, { 0, NULL } };
      int32_t tblQueryResult = ACDB_ERROR;

      result = GetMaxLenPrpty(&maxLenPrpty);
      if (result != ACDB_SUCCESS)
      {
         ACDB_DEBUG_LOG("ACDB_COMMAND: Could not get maxLen property\n");
         return result;
      }

      tblQueryResult = GetVP3InfoFromUseCaseId(pInput->nUseCaseId, &tblId, &lutIndicesCount, &cmdIndicesCount, &tblIndicesCount, &cdftIndicesCount);

      if (tblQueryResult != ACDB_SUCCESS)
      {
         ACDB_DEBUG_LOG("ACDB_COMMAND: Unable to find tblId for useCaseId %08X \n", pInput->nUseCaseId);
         return ACDB_INPUT_PARAMS_NOT_FOUND;
      }

      if (pInput->nUseCaseId == ACDB_VP3_AUDIO_REC_USECASE)
      {
		  VP3DevDataLookupTblType lutDevTbl = { 0, NULL };
		  VP3DevCmdLookupType vp3Devcmd = { 0 };

         vp3Devcmd.nTxDevId = pInput->nTxDeviceId;

         cmd.devId = pInput->nTxDeviceId;
         cmd.tblId = tblId;
         result = acdbdata_ioctl(ACDBDATACMD_GET_TABLE_INFO, (uint8_t *)&cmd, sizeof(cmd),
            (uint8_t *)&tblInfo, sizeof(tblInfo));
         if (result != ACDB_SUCCESS)
         {
            ACDB_DEBUG_LOG("Failed to fetch the lookup information of the device %08X \n"
               , pInput->nTxDeviceId);
            return result;
         }
         lutDevTbl.nLen = READ_UInt32(tblInfo.tblLutChnk.pData);
         lutDevTbl.pLut = (VP3DevDataLookupType *)(tblInfo.tblLutChnk.pData + sizeof(lutDevTbl.nLen));

         result = AcdbDataBinarySearch((void *)lutDevTbl.pLut, lutDevTbl.nLen,
            lutIndicesCount, &vp3Devcmd, cmdIndicesCount, &index);
         if (result != SEARCH_SUCCESS)
         {
            ACDB_DEBUG_LOG("Failed to fetch the lookup information of the device %08X \n"
               , pInput->nTxDeviceId);
            return ACDB_INPUT_PARAMS_NOT_FOUND;
         }

         // Now get CDEF info
         cdefTbl.nLen = READ_UInt32(tblInfo.tblCdftChnk.pData + lutDevTbl.pLut[index].nCDEFTblOffset);
         cdefTbl.pCntDef = (ContentDefType *)(tblInfo.tblCdftChnk.pData + lutDevTbl.pLut[index].nCDEFTblOffset + sizeof(cdefTbl.nLen));
      }
      else
      {
		  VP3DevPairDataLookupTblType lutDevPairTbl = { 0, NULL };
		  VP3DevPairCmdLookupType vp3DevPaircmd = { 0, 0 };

         vp3DevPaircmd.nRxDevId = pInput->nRxDeviceId;
         vp3DevPaircmd.nTxDevId = pInput->nTxDeviceId;

         cmd.devId = pInput->nTxDeviceId;
         cmd.tblId = tblId;
         result = acdbdata_ioctl(ACDBDATACMD_GET_TABLE_INFO, (uint8_t *)&cmd, sizeof(cmd),
            (uint8_t *)&tblInfo, sizeof(tblInfo));
         if (result != ACDB_SUCCESS)
         {
            ACDB_DEBUG_LOG("Failed to fetch the lookup information of the device %08X \n"
               , pInput->nTxDeviceId);
            return result;
         }
         lutDevPairTbl.nLen = READ_UInt32(tblInfo.tblLutChnk.pData);
         lutDevPairTbl.pLut = (VP3DevPairDataLookupType *)(tblInfo.tblLutChnk.pData + sizeof(lutDevPairTbl.nLen));

         result = AcdbDataBinarySearch((void *)lutDevPairTbl.pLut, lutDevPairTbl.nLen,
            lutIndicesCount, &vp3DevPaircmd, cmdIndicesCount, &index);
         if (result != SEARCH_SUCCESS)
         {
            ACDB_DEBUG_LOG("Failed to fetch the lookup information of the device %08X \n"
               , pInput->nTxDeviceId);
            return ACDB_INPUT_PARAMS_NOT_FOUND;
         }

         // Now get CDEF info
         cdefTbl.nLen = READ_UInt32(tblInfo.tblCdftChnk.pData + lutDevPairTbl.pLut[index].nCDEFTblOffset);
         cdefTbl.pCntDef = (ContentDefType *)(tblInfo.tblCdftChnk.pData + lutDevPairTbl.pLut[index].nCDEFTblOffset + sizeof(cdefTbl.nLen));
      }

      if (pInput->nBufferLength < cdefTbl.nLen * sizeof(MaxLenDefType))
      {
         ACDB_DEBUG_LOG("ACDB_COMMAND:[AcdbCmdGetVP3MidPidList]: Insufficient buffer size to copy VP3 MID PID data\n");
         return ACDB_INSUFFICIENTMEMORY;
      }

      {
         const uint32_t nLen = cdefTbl.nLen;
         dataOffset = 0;
         copiedMaxLen = 0;

         ACDB_MEM_CPY(pInput->nBufferPointer + dataOffset, pInput->nBufferLength - dataOffset, &nLen, sizeof(nLen));
         dataOffset += (uint32_t)sizeof(cdefTbl.nLen);
      }

      for (i = 0; i < cdefTbl.nLen; i++)
      {
         for (j = 0; j < maxLenPrpty.nLen; j++)
         {
            if (cdefTbl.pCntDef[i].nMid == maxLenPrpty.pCntDef[j].nMid &&
               cdefTbl.pCntDef[i].nPid == maxLenPrpty.pCntDef[j].nPid)
            {
               ACDB_MEM_CPY(pInput->nBufferPointer + dataOffset, pInput->nBufferLength - dataOffset, &maxLenPrpty.pCntDef[j].nMid, sizeof(maxLenPrpty.pCntDef[j].nMid));
               dataOffset += (uint32_t)sizeof(maxLenPrpty.pCntDef[j].nMid);

               ACDB_MEM_CPY(pInput->nBufferPointer + dataOffset, pInput->nBufferLength - dataOffset, &maxLenPrpty.pCntDef[j].nPid, sizeof(maxLenPrpty.pCntDef[j].nPid));
               dataOffset += (uint32_t)sizeof(maxLenPrpty.pCntDef[j].nPid);

               ACDB_MEM_CPY(pInput->nBufferPointer + dataOffset, pInput->nBufferLength - dataOffset, &maxLenPrpty.pCntDef[j].nMaxLen, sizeof(maxLenPrpty.pCntDef[j].nMaxLen));
               dataOffset += (uint32_t)sizeof(maxLenPrpty.pCntDef[j].nMaxLen);

               copiedMaxLen++;
            }
         }
      }

      if (cdefTbl.nLen != copiedMaxLen)
      {
         ACDB_DEBUG_LOG("ACDB_COMMAND:[AcdbCmdGetVP3MidPidList]: Insufficient buffer size to copy VP3 MID PID data\n");
         return ACDB_ERROR;
      }

      pOutput->nBytesUsedInBuffer = dataOffset;
   }
   return result;
}

int32_t AcdbCmdGetVP3Data(AcdbVP3CmdType *pInput, AcdbQueryResponseType *pOutput)
{
   int32_t result = ACDB_SUCCESS;

   if (pInput == NULL || pOutput == NULL)
   {
      ACDB_DEBUG_LOG("[ACDB Command]->[AcdbCmdGetVP3Data]->System Erorr\n");
      result = ACDB_BADPARM;
   }
   else if (pInput->nBufferPointer == NULL)
   {
      ACDB_DEBUG_LOG("[ACDB Command]->[AcdbCmdGetVP3Data]->NULL pointer\n");
      result = ACDB_BADPARM;
   }
   else
   {
      uint32_t index = 0;
      AcdbTableCmd cmd = { 0, 0 };
      AcdbTableInfo tblInfo = { { 0, NULL }, { 0, NULL }, { 0, NULL } };

      ContentDefTblType cdefTbl = { 0, NULL };
      ContentDataOffsetsTblType cdotTbl = { 0, NULL };
      uintptr_t nLookupKey = 0;

      uint32_t tblId = 0;
      int32_t lutIndicesCount = 0;
      int32_t cmdIndicesCount = 0;
      int32_t tblIndicesCount = 0;
	  int32_t cdftIndicesCount = 0;
      int32_t tblQueryResult = ACDB_ERROR;

      tblQueryResult = GetVP3InfoFromUseCaseId(pInput->nUseCaseId, &tblId, &lutIndicesCount, &cmdIndicesCount, &tblIndicesCount, &cdftIndicesCount);

      if (tblQueryResult != ACDB_SUCCESS)
      {
         ACDB_DEBUG_LOG("ACDB_COMMAND: Unable to find tblId for useCaseId %08X \n", pInput->nUseCaseId);
         return ACDB_INPUT_PARAMS_NOT_FOUND;
      }

      if (pInput->nUseCaseId == ACDB_VP3_AUDIO_REC_USECASE)
      {
         VP3DevDataLookupTblType lutDevTbl = { 0, NULL };
         VP3DevCmdLookupType vp3Devcmd = { 0 };

         vp3Devcmd.nTxDevId = pInput->nTxDeviceId;

         cmd.devId = vp3Devcmd.nTxDevId;
         cmd.tblId = tblId;
         result = acdbdata_ioctl(ACDBDATACMD_GET_TABLE_INFO, (uint8_t *)&cmd, sizeof(cmd),
            (uint8_t *)&tblInfo, sizeof(tblInfo));
         if (result != ACDB_SUCCESS)
         {
            ACDB_DEBUG_LOG("Failed to fetch the lookup information of the device %08X \n"
               , vp3Devcmd.nTxDevId);
            return result;
         }
         lutDevTbl.nLen = READ_UInt32(tblInfo.tblLutChnk.pData);
         lutDevTbl.pLut = (VP3DevDataLookupType *)(tblInfo.tblLutChnk.pData + sizeof(lutDevTbl.nLen));

         result = AcdbDataBinarySearch((void *)lutDevTbl.pLut, lutDevTbl.nLen,
            lutIndicesCount, &vp3Devcmd, cmdIndicesCount, &index);
         if (result != SEARCH_SUCCESS)
         {
            ACDB_DEBUG_LOG("Failed to fetch the lookup information of the device %08X \n"
               , vp3Devcmd.nTxDevId);
            return ACDB_INPUT_PARAMS_NOT_FOUND;
         }

         nLookupKey = (uintptr_t)&lutDevTbl.pLut[index];
         // Now get CDEF info
         cdefTbl.nLen = READ_UInt32(tblInfo.tblCdftChnk.pData + lutDevTbl.pLut[index].nCDEFTblOffset);
         cdefTbl.pCntDef = (ContentDefType *)(tblInfo.tblCdftChnk.pData + lutDevTbl.pLut[index].nCDEFTblOffset + sizeof(cdefTbl.nLen));

         // Now get CDOT info
         cdotTbl.nLen = READ_UInt32(tblInfo.tblCdotChnk.pData + lutDevTbl.pLut[index].nCDOTTblOffset);
         cdotTbl.pDataOffsets = (uint32_t *)(tblInfo.tblCdotChnk.pData + lutDevTbl.pLut[index].nCDOTTblOffset + sizeof(cdotTbl.nLen));
      }
      else
      {
         VP3DevPairDataLookupTblType lutDevPairTbl = { 0, NULL };
         VP3DevPairCmdLookupType vp3DevPaircmd = { 0, 0 };

         vp3DevPaircmd.nTxDevId = pInput->nTxDeviceId;
         vp3DevPaircmd.nRxDevId = pInput->nRxDeviceId;

         cmd.devId = vp3DevPaircmd.nTxDevId;
         cmd.tblId = tblId;
         result = acdbdata_ioctl(ACDBDATACMD_GET_TABLE_INFO, (uint8_t *)&cmd, sizeof(cmd),
            (uint8_t *)&tblInfo, sizeof(tblInfo));
         if (result != ACDB_SUCCESS)
         {
            ACDB_DEBUG_LOG("Failed to fetch the lookup information of the device %08X \n"
               , vp3DevPaircmd.nTxDevId);
            return result;
         }
         lutDevPairTbl.nLen = READ_UInt32(tblInfo.tblLutChnk.pData);
         lutDevPairTbl.pLut = (VP3DevPairDataLookupType *)(tblInfo.tblLutChnk.pData + sizeof(lutDevPairTbl.nLen));

         result = AcdbDataBinarySearch((void *)lutDevPairTbl.pLut, lutDevPairTbl.nLen,
            lutIndicesCount, &vp3DevPaircmd, cmdIndicesCount, &index);
         if (result != SEARCH_SUCCESS)
         {
            ACDB_DEBUG_LOG("Failed to fetch the lookup information of the device %08X \n"
               , vp3DevPaircmd.nTxDevId);
            return ACDB_INPUT_PARAMS_NOT_FOUND;
         }

         nLookupKey = (uintptr_t)&lutDevPairTbl.pLut[index];
         // Now get CDEF info
         cdefTbl.nLen = READ_UInt32(tblInfo.tblCdftChnk.pData + lutDevPairTbl.pLut[index].nCDEFTblOffset);
         cdefTbl.pCntDef = (ContentDefType *)(tblInfo.tblCdftChnk.pData + lutDevPairTbl.pLut[index].nCDEFTblOffset + sizeof(cdefTbl.nLen));

         // Now get CDOT info
         cdotTbl.nLen = READ_UInt32(tblInfo.tblCdotChnk.pData + lutDevPairTbl.pLut[index].nCDOTTblOffset);
         cdotTbl.pDataOffsets = (uint32_t *)(tblInfo.tblCdotChnk.pData + lutDevPairTbl.pLut[index].nCDOTTblOffset + sizeof(cdotTbl.nLen));
      }

      if (cdefTbl.nLen != cdotTbl.nLen)
      {
         ACDB_DEBUG_LOG("The no of entries in CDEF and CDOT tables for devid %08X not matching\n", pInput->nTxDeviceId);
         return ACDB_ERROR;
      }

      result = GetMidPidCalibData(tblId, nLookupKey, cdefTbl, cdotTbl, tblInfo.dataPoolChnk, pInput->nModuleId,
         pInput->nParamId, pInput->nBufferPointer, pInput->nBufferLength, &pOutput->nBytesUsedInBuffer);
      if (ACDB_SUCCESS != result)
      {
         return result;
      }

      result = ACDB_SUCCESS;
   }

   return result;
}

int32_t AcdbCmdSetVP3Data(AcdbVP3CmdType *pInput)
{
   int32_t result = ACDB_SUCCESS;

   if (pInput == NULL)
   {
      ACDB_DEBUG_LOG("[ACDB Command]->[AcdbCmdSetAudProcData]->Invalid NULL value parameters are provided\n");
      result = ACDB_BADPARM;
   }
   else if (pInput->nBufferPointer == NULL)
   {
      return ACDB_BADPARM;
   }
   else
   {
      uint32_t j = 0;
      uint32_t tblId = 0;
      int32_t lutIndicesCount = 0;
      int32_t cmdIndicesCount = 0;
      int32_t tblIndicesCount = 0;
	  int32_t cdftIndicesCount = 0;
      int32_t tblQueryResult = ACDB_ERROR;
	  MaxLenDefPrptyType maxLenPrpty = { 0, NULL};
      uint32_t persistData = FALSE;
      int32_t persist_result = AcdbCmdIsPersistenceSupported(&persistData);
      if (persist_result != ACDB_SUCCESS)
      {
         persistData = FALSE;
      }

      tblQueryResult = GetVP3InfoFromUseCaseId(pInput->nUseCaseId, &tblId, &lutIndicesCount, &cmdIndicesCount, &tblIndicesCount, &cdftIndicesCount);

      if (tblQueryResult != ACDB_SUCCESS)
      {
         ACDB_DEBUG_LOG("ACDB_COMMAND: Unable to find tblId for useCaseId %08X \n", pInput->nUseCaseId);
         return ACDB_INPUT_PARAMS_NOT_FOUND;
      }

      result = GetMaxLenPrpty(&maxLenPrpty);
      if (result != ACDB_SUCCESS)
      {
         ACDB_DEBUG_LOG("ACDB_COMMAND: Could not get maxLen property\n");
         return result;
      }

      for (j = 0; j < maxLenPrpty.nLen; j++)
      {
         if (pInput->nModuleId == maxLenPrpty.pCntDef[j].nMid &&
            pInput->nParamId == maxLenPrpty.pCntDef[j].nPid)
         {
            if (pInput->nBufferLength > maxLenPrpty.pCntDef[j].nMaxLen)
            {
               return ACDB_BADPARM;
            }
            break;
         }
      }
      if (pInput->nUseCaseId == ACDB_VP3_AUDIO_REC_USECASE)
      {
         const uint32_t finaltblId = tblId;
		 uint32_t offset = 0, size= ((tblIndicesCount + cdftIndicesCount)* (uint32_t)sizeof(uint32_t));		 
         VP3DevCmdLookupType vp3Devcmd = { 0 };
		 uint8_t *buffer = (uint8_t *) malloc(size);
		 if(buffer == NULL)
	     {
			ACDB_DEBUG_LOG("ACDB_COMMAND: Unable to allocate memory for AcdbCmdSetVP3Data - temporary buffer\n");
			return ACDB_INSUFFICIENTMEMORY;
	     }
         vp3Devcmd.nTxDevId = pInput->nTxDeviceId;
         if (finaltblId == AUDIO_REC_VP3_TBL)
         {
			 ACDB_MEM_CPY(buffer+offset, size, &vp3Devcmd, sizeof(vp3Devcmd));
			 offset += (uint32_t)sizeof(vp3Devcmd);
			 ACDB_MEM_CPY(buffer+offset, size, &pInput->nModuleId, sizeof(uint32_t));
			 offset += (uint32_t)sizeof(uint32_t);
			 ACDB_MEM_CPY(buffer+offset, size, &pInput->nParamId, sizeof(uint32_t));
			 offset += (uint32_t)sizeof(uint32_t);
            result = AcdbCmdSetOnlineDataV2(persistData, finaltblId, buffer, tblIndicesCount, cdftIndicesCount, pInput->nBufferPointer, pInput->nBufferLength);

            if (result == ACDB_SUCCESS &&
               ACDB_INIT_SUCCESS == AcdbIsPersistenceSupported())
            {
               result = AcdbCmdSaveDeltaFileData();

               if (result != ACDB_SUCCESS)
               {
                  ACDB_DEBUG_LOG("[ACDB Command]->[AcdbCmdSetVP3Data]->Unable to save delta file data\n");
               }
            }
         }
         else
         {
            ACDB_DEBUG_LOG("ACDB_COMMAND: Invalid tblid being passed for vp3 set data\n");
            return ACDB_BADPARM;
         }
      }
      else
      {
         const uint32_t finaltblId = tblId;
		 uint32_t offset = 0, size= ((tblIndicesCount + cdftIndicesCount)*(uint32_t)sizeof(uint32_t));		 
         VP3DevPairCmdLookupType vp3DevPaircmd = { 0, 0 };
		 uint8_t *buffer = (uint8_t *) malloc(size);
		 if(buffer == NULL)
	     {
			ACDB_DEBUG_LOG("ACDB_COMMAND: Unable to allocate memory for AcdbCmdSetVP3Data - temporary buffer\n");
			return ACDB_INSUFFICIENTMEMORY;
	     }
         vp3DevPaircmd.nTxDevId = pInput->nTxDeviceId;
         vp3DevPaircmd.nRxDevId = pInput->nRxDeviceId;
         if ((finaltblId == VOICE_VP3_TBL) || (finaltblId == AUDIO_REC_EC_VP3_TBL))
         {
			 ACDB_MEM_CPY(buffer+offset, size, &vp3DevPaircmd, sizeof(vp3DevPaircmd));
			 offset += (uint32_t)sizeof(vp3DevPaircmd);
			 ACDB_MEM_CPY(buffer+offset, size, &pInput->nModuleId, sizeof(uint32_t));
			 offset += (uint32_t)sizeof(uint32_t);
			 ACDB_MEM_CPY(buffer+offset, size, &pInput->nParamId, sizeof(uint32_t));
			 offset += (uint32_t)sizeof(uint32_t);
            result = AcdbCmdSetOnlineDataV2(persistData, finaltblId, buffer, tblIndicesCount, cdftIndicesCount, pInput->nBufferPointer, pInput->nBufferLength);

            if (result == ACDB_SUCCESS &&
               ACDB_INIT_SUCCESS == AcdbIsPersistenceSupported())
            {
               result = AcdbCmdSaveDeltaFileData();

               if (result != ACDB_SUCCESS)
               {
                  ACDB_DEBUG_LOG("[ACDB Command]->[AcdbCmdSetVP3Data]->Unable to save delta file data\n");
               }
            }
			ACDB_MEM_FREE(buffer);
         }
         else
         {
			ACDB_MEM_FREE(buffer);
            ACDB_DEBUG_LOG("ACDB_COMMAND: Invalid tblid being passed for vp3 set data\n");
            return ACDB_BADPARM;
         }
      }
   }

   return result;
}

int32_t AcdbCmdDeleteAllDeltaFiles(int32_t *resp)
{
   int32_t result = ACDB_SUCCESS;
   int32_t resp_val = 0;
   if (resp == NULL)
   {
      return ACDB_BADPARM;
   }
   if (ACDB_INIT_SUCCESS == AcdbIsPersistenceSupported())
   {
      resp_val = acdb_delta_data_ioctl(ACDBDELTADATACMD_DELETE_DELTA_ACDB_FILES, NULL, 0, NULL, 0);
      ACDB_MEM_CPY(resp, sizeof(int32_t), &resp_val, sizeof(int32_t));
   }

   return result;
}

int32_t AcdbCmdGetMetaInfoSize(AcdbGetMetaInfoSizeCmdType *pInput, AcdbSizeResponseType *pOutput)
{
   int32_t result = ACDB_SUCCESS;

   if (pInput == NULL || pOutput == NULL)
   {
      ACDB_DEBUG_LOG("[ACDB Command]->[AcdbCmdGetGetMetaInfoSize]->System Erorr\n");
      result = ACDB_BADPARM;
   }
   else
   {
      uint32_t tblLookupRslt = 0;
      AcdbTableCmd cmd = { 0, 0 };
      AcdbTableInfo tblInfo = { { 0, NULL }, { 0, NULL }, { 0, NULL } };
      uint32_t tblId = METAINFO_LOOKUP_TBL;
	  MetaInfoCmdLookupType MIcmd = { 0 };
	  MetaInfoLookupTblType MIlut = { 0, NULL};
      uint32_t index = 0;
      uintptr_t nLookupKey = 0;
      uint8_t *tempBuf = NULL;
      uint32_t datalength = 0;
      uint32_t outputBufLen = 0;
      uint32_t version = 0;
      MIcmd.nKey = pInput->nKey;

      cmd.devId = 0;
      cmd.tblId = tblId;
      tblLookupRslt = acdbdata_ioctl(ACDBDATACMD_GET_TABLE_INFO, (uint8_t *)&cmd, sizeof(cmd),
         (uint8_t *)&tblInfo, sizeof(tblInfo));
      if (tblLookupRslt != ACDB_SUCCESS)
      {
         ACDB_DEBUG_LOG("Failed to fetch the lookup information of the MetaInfo table\n");
         return tblLookupRslt;
      }

      ACDB_MEM_CPY((void *)&MIlut.nLen, sizeof(uint32_t), tblInfo.tblLutChnk.pData, sizeof(uint32_t));
      MIlut.pLut = (MetaInfoLookupType *)(tblInfo.tblLutChnk.pData + sizeof(MIlut.nLen));

      result = AcdbDataBinarySearch((void *)MIlut.pLut, MIlut.nLen,
         MINFO_LUT_INDICES_COUNT, &MIcmd, MINFO_CMD_INDICES_COUNT, &index);
      if (result != SEARCH_SUCCESS)
      {
         ACDB_DEBUG_LOG("Couldnt find the key %08X \n", MIcmd.nKey);
         return ACDB_INPUT_PARAMS_NOT_FOUND;
      }
      nLookupKey = (uintptr_t)&MIlut.pLut[index];
      tempBuf = (uint8_t *)ACDB_MALLOC(MAX_BUFFER_LENGTH);
      if (tempBuf != NULL)
      {
         result = GetCalibData(tblId, nLookupKey, MIlut.pLut[index].nDataOffset, tblInfo.dataPoolChnk,
            tempBuf, MAX_BUFFER_LENGTH, &outputBufLen);

         ACDB_MEM_CPY(&version, sizeof(uint32_t), tempBuf, sizeof(uint32_t));
         ACDB_MEM_CPY(&datalength, sizeof(uint32_t), tempBuf + sizeof(version), sizeof(uint32_t));

         pOutput->nSize = datalength;
         ACDB_MEM_FREE(tempBuf);
      }
      else
      {
         return ACDB_INSUFFICIENTMEMORY;
      }
   }
   return result;
}

int32_t AcdbCmdGetMetaInfo(AcdbGetMetaInfoCmdType *pInput, AcdbQueryResponseType *pOutput)
{
   int32_t result = ACDB_SUCCESS;

   if (pInput == NULL || pOutput == NULL)
   {
      ACDB_DEBUG_LOG("[ACDB Command]->[AcdbCmdGetGetMetaInfo]->System Erorr\n");
      result = ACDB_BADPARM;
   }
   else if (pInput->nBufferPointer == NULL)
   {
      ACDB_DEBUG_LOG("[ACDB Command]->[AcdbCmdGetGetMetaInfo]->NULL pointer\n");
      result = ACDB_BADPARM;
   }
   else
   {
      uint32_t tblLookupRslt = 0;
      AcdbTableCmd cmd = { 0, 0 };
      AcdbTableInfo tblInfo = { { 0, NULL }, { 0, NULL }, { 0, NULL } };
      uint32_t tblId = METAINFO_LOOKUP_TBL;
	  MetaInfoCmdLookupType MIcmd = { 0 };
      MetaInfoLookupTblType MIlut = { 0, NULL};
      uint32_t index = 0;
      uintptr_t nLookupKey = 0;
      uint8_t *tempBuf = NULL;
      uint32_t datalength = 0;
      uint32_t outputBufLen = 0;
      uint32_t version = 0;

      MIcmd.nKey = pInput->nKey;

      cmd.devId = 0;
      cmd.tblId = tblId;

      tblLookupRslt = acdbdata_ioctl(ACDBDATACMD_GET_TABLE_INFO, (uint8_t *)&cmd, sizeof(cmd),
         (uint8_t *)&tblInfo, sizeof(tblInfo));
      if (tblLookupRslt != ACDB_SUCCESS)
      {
         ACDB_DEBUG_LOG("Failed to fetch the lookup information of the MetaInfo table\n");
         return tblLookupRslt;
      }

      ACDB_MEM_CPY((void *)&MIlut.nLen, sizeof(uint32_t), tblInfo.tblLutChnk.pData, sizeof(uint32_t));
      MIlut.pLut = (MetaInfoLookupType *)(tblInfo.tblLutChnk.pData + sizeof(MIlut.nLen));

      result = AcdbDataBinarySearch((void *)MIlut.pLut, MIlut.nLen,
         MINFO_LUT_INDICES_COUNT, &MIcmd, MINFO_CMD_INDICES_COUNT, &index);
      if (result != SEARCH_SUCCESS)
      {
         ACDB_DEBUG_LOG("Couldnt find the key %08X \n", MIcmd.nKey);
         return ACDB_INPUT_PARAMS_NOT_FOUND;
      }

      nLookupKey = (uintptr_t)&MIlut.pLut[index];
      tempBuf = (uint8_t *)ACDB_MALLOC(MAX_BUFFER_LENGTH);
      if (tempBuf != NULL)
      {
         result = GetCalibData(tblId, nLookupKey, MIlut.pLut[index].nDataOffset, tblInfo.dataPoolChnk,
            tempBuf, MAX_BUFFER_LENGTH, &outputBufLen);
         ACDB_MEM_CPY(&version, sizeof(uint32_t), tempBuf, sizeof(uint32_t));
         ACDB_MEM_CPY(&datalength, sizeof(uint32_t), tempBuf + sizeof(uint32_t), sizeof(uint32_t));
         if (pInput->nBufferLength >= datalength)
         {
            ACDB_MEM_CPY(pInput->nBufferPointer, datalength, tempBuf + 2 * sizeof(uint32_t), datalength);
            pOutput->nBytesUsedInBuffer = datalength;
         }
         else
         {
            ACDB_DEBUG_LOG("Insufficient Memory. Input Buffer Length : %u Meta Info Size : %u\n", pInput->nBufferLength, datalength);
            ACDB_MEM_FREE(tempBuf);
            return ACDB_INSUFFICIENTMEMORY;
         }

         ACDB_MEM_FREE(tempBuf);
      }
      else
      {
         return ACDB_INSUFFICIENTMEMORY;
      }
   }
   return result;
}

int32_t AcdbCmdGetAcdbSwVersionV3(AcdbModuleVersionTypeV3 *pOutput)
{
   int32_t result = ACDB_SUCCESS;

   uint32_t major = ACDB_SOFTWARE_VERSION_MAJOR;
   uint32_t minor = ACDB_SOFTWARE_VERSION_MINOR;
   uint32_t revision = ACDB_SOFTWARE_VERSION_REVISION;
   uint32_t cplInfo = ACDB_SOFTWARE_VERSION_CPLINFO;

   if (pOutput != NULL)
   {
      ACDB_MEM_CPY(&pOutput->major, sizeof(pOutput->major), &major, sizeof(major));
      ACDB_MEM_CPY(&pOutput->minor, sizeof(pOutput->minor), &minor, sizeof(minor));
      ACDB_MEM_CPY(&pOutput->revision, sizeof(pOutput->revision), &revision, sizeof(revision));
      ACDB_MEM_CPY(&pOutput->cplInfo, sizeof(pOutput->cplInfo), &cplInfo, sizeof(cplInfo));
   }
   else
   {
      return ACDB_BADPARM;
   }
   return result;
}

int32_t AcdbCmdGetWdspSubgraphList(uint32_t queryType, uint8_t *pIn, uint8_t *pOut)
{
   int32_t result = ACDB_SUCCESS;
   if (pIn == NULL || pOut == NULL)
   {
      ACDB_DEBUG_LOG("[ACDB Command]->[WdspCmdGetSubgraphList]->Invalid NULL value parameters are provided\n");
      result = ACDB_BADPARM;
   }
   else
   {
      uint8_t *pOutBufPtr = NULL;
      uint32_t size = 0;
	  WdspGetSubgraphListQry subgraphListQry = { 0, 0, { 0, NULL } };
      memset(&subgraphListQry, 0, sizeof(subgraphListQry));

      switch (queryType)
      {
      case DATA_CMD:
      {
         WdspSubgraphListCmdType *pInput = (WdspSubgraphListCmdType *)pIn;
         if (pInput->nBufferPointer == NULL)
         {
            return ACDB_BADPARM;
         }
         subgraphListQry.devId = pInput->nDeviceId;
         subgraphListQry.usecaseId = pInput->nUsecaseId;
         pOutBufPtr = pInput->nBufferPointer;

         if (pOutBufPtr == NULL)
         {
            ACDB_DEBUG_LOG("[ACDB Command]->[WdspCmdGetSubgraphList]->Payload buffer pointer is NULL\n");
            return ACDB_BADPARM;
         }
      }
      break;
      case DATA_SIZE_CMD:
      {
         WdspSubgraphListSizeCmdType *pInput = (WdspSubgraphListSizeCmdType *)pIn;
         subgraphListQry.devId = pInput->nDeviceId;
         subgraphListQry.usecaseId = pInput->nUsecaseId;
      }
      break;
      default:
      {
         ACDB_DEBUG_LOG("[ACDB Command]->[WdspCmdGetSubgraphList]->Invalid query %u\n",
            queryType);
         return  ACDB_ERROR;
      }
      }

      result = acdbdata_ioctl(
         ACDBDATACMD_GET_WDSP_SUBGRAPHLIST,
         (uint8_t *)&subgraphListQry,
         sizeof(subgraphListQry),
         (uint8_t *)NULL,
         0
         );
      if (result != ACDB_SUCCESS)
      {
         ACDB_DEBUG_LOG("[ACDB Command]->[WdspCmdGetSubgraphList]->Failed to fetch the device property of the device %08X\n",
            subgraphListQry.devId);
         return result;
      }
      else
      {
         uint32_t idx = 0, index = 0;
         //Here data found is for all usecase IDs, we have to find for one specific usecase ID.
         uint8_t *buffer = subgraphListQry.dataInfo.pData;
         uint32_t numOfUsecaseIds = READ_UInt32(buffer);
         uint32_t offset = sizeof(uint32_t);
         int8_t usecaseFound = 0;
         uint32_t numBytesRemaining = subgraphListQry.dataInfo.nDataLen;
         for (idx = 0; idx < numOfUsecaseIds; idx++)
         {
            uint32_t usecaseID = READ_UInt32(buffer + offset);
            uint32_t sgtCount = 0, MidIidCount = 0;
            offset += (uint32_t)sizeof(uint32_t); //For usecase ID which is read from buffer.
            if (usecaseID == subgraphListQry.usecaseId)
            {
               uint32_t outputBufferOffset = 0;
               WdspSubgraphListCmdType *pInput = (WdspSubgraphListCmdType *)pIn;
               sgtCount = READ_UInt32(buffer + offset);
               if (queryType == DATA_CMD)
               {
                  ACDB_MEM_CPY(pInput->nBufferPointer, numBytesRemaining, &sgtCount, sizeof(uint32_t));
               }
               size += (uint32_t)sizeof(uint32_t);
               offset += (uint32_t)sizeof(uint32_t); //For sgtcount which is read from buffer.
               outputBufferOffset += (uint32_t)sizeof(uint32_t); //SGTCount added
               numBytesRemaining -= (uint32_t)sizeof(uint32_t);

               for (index = 0; index < sgtCount; index++)
               {
                  if (queryType == DATA_CMD)
                  {
                     ACDB_MEM_CPY(pInput->nBufferPointer + outputBufferOffset, numBytesRemaining, buffer + offset, sizeof(uint32_t));//SGTID
                  }
                  size += (uint32_t)sizeof(uint32_t);
                  offset += (uint32_t)sizeof(uint32_t); //For SGTID which is read from buffer.
                  outputBufferOffset += (uint32_t)sizeof(uint32_t); //SGTID added to output fuffer
                  numBytesRemaining -= (uint32_t)sizeof(uint32_t);

                  MidIidCount = READ_UInt32(buffer + offset);
                  offset += (uint32_t)sizeof(uint32_t); //MID IID count
                  offset += (uint32_t)(MidIidCount * 2 * (uint32_t)sizeof(uint32_t));
               }
               usecaseFound = 1;
               break;
            }
            else
            {
               sgtCount = READ_UInt32(buffer + offset);
               offset += (uint32_t)sizeof(uint32_t); //For sgtcount which is read from buffer.
               for (index = 0; index < sgtCount; index++)
               {
                  offset += (uint32_t)sizeof(uint32_t); //subgraph ID
                  MidIidCount = READ_UInt32(buffer + offset);
                  offset += (uint32_t)sizeof(uint32_t); //MID IID count
                  offset += (MidIidCount * 2 * (uint32_t)sizeof(uint32_t));
               }
            }
         }
         if (usecaseFound == 0)
         {
            ACDB_DEBUG_LOG("ACDBFILE_MGR: Couldnt find given Usecase ID: %08X for device ID: %08X \n", subgraphListQry.usecaseId, subgraphListQry.devId);
            return ACDB_ERROR;
         }
      }

      switch (queryType)
      {
      case DATA_CMD:
      {
         AcdbQueryResponseType *pOutput = (AcdbQueryResponseType *)pOut;
         pOutput->nBytesUsedInBuffer = size;
      }
      break;
      case DATA_SIZE_CMD:
      {
         AcdbSizeResponseType *pOutput = (AcdbSizeResponseType *)pOut;
         pOutput->nSize = size;
      }
      break;
      default:
      {
         ACDB_DEBUG_LOG("[ACDB Command]->[WdspCmdGetSubgraphList]->Invalid query %u\n", queryType);
         result = ACDB_ERROR;
      }
      }
   }

   return result;
}

int32_t AcdbGetWdspModuleInfo(uint32_t queryType, uint8_t *pIn, uint8_t *pOut, uint32_t MID, uint32_t IID)
{
   int32_t result = ACDB_SUCCESS;

   if (pIn == NULL || pOut == NULL)
   {
      ACDB_DEBUG_LOG("[ACDB Command]->[AcdbGetWdspModuleInfo]->Invalid NULL value parameters are provided\n");
      result = ACDB_BADPARM;
   }
   else
   {
      uint32_t index = 0;
      uint32_t tblId = WDSP_MOD_INFO_DATA_TBL_ID;
      AcdbTableCmd cmd = { 0, 0 };
      AcdbTableInfo tblInfo = { { 0, NULL }, { 0, NULL }, { 0, NULL } };
	  WdspModuleInfoDataLookupTblType lutTbl = { 0, NULL };
	  ContentDefWithOnlyParamTblType cdefTbl = { 0, NULL };
      ContentDataOffsetsTblType cdotTbl = { 0, NULL };
      //uintptr_t nLookupKey = 0;
	  WdspModuleInfoCmdLookupType moduleConncmd = { 0, 0 };
	  WdspModuleInfoDataLookupType tempLut = { 0, 0, 0, 0};

      switch (queryType)
      {
      case TABLE_CMD:
      {
         AcdbWdspDidUcidSubGraphIDType *pInput = (AcdbWdspDidUcidSubGraphIDType *)pIn;
         if (pInput->nBufferPointer == NULL)
         {
            return ACDB_BADPARM;
         }
         cmd.devId = pInput->nDeviceId;
         moduleConncmd.nMId = MID;
         moduleConncmd.nIId = IID;
      }
      break;
      case TABLE_SIZE_CMD:
      {
         WdspDidUcidSubGraphIDSizeCmdType *pInput = (WdspDidUcidSubGraphIDSizeCmdType *)pIn;
         cmd.devId = pInput->nDeviceId;
         moduleConncmd.nMId = MID;
         moduleConncmd.nIId = IID;
      }
      break;
      default:
         return  ACDB_ERROR;
      }

      cmd.tblId = tblId;
      result = acdbdata_ioctl(ACDBDATACMD_GET_TABLE_INFO, (uint8_t *)&cmd, sizeof(cmd),
         (uint8_t *)&tblInfo, sizeof(tblInfo));
      if (result != ACDB_SUCCESS)
      {
         ACDB_DEBUG_LOG("Failed to fetch the lookup information of the device %08X \n"
            , cmd.devId);
         return result;
      }
      lutTbl.nLen = READ_UInt32(tblInfo.tblLutChnk.pData);
      lutTbl.pLut = (WdspModuleInfoDataLookupType *)(tblInfo.tblLutChnk.pData + sizeof(lutTbl.nLen));

      result = AcdbDataBinarySearch((void *)lutTbl.pLut, lutTbl.nLen,
         WDSP_MODULE_INFO_LUT_INDICES_COUNT, &moduleConncmd, WDSP_MODULE_INFO_CMD_INDICES_COUNT, &index);
      if (result != SEARCH_SUCCESS)
      {
         ACDB_DEBUG_LOG("Failed to fetch the lookup information of the device %08X \n"
            , cmd.devId);
         return ACDB_INPUT_PARAMS_NOT_FOUND;
      }

      //nLookupKey = (uintptr_t)&lutTbl.pLut[index];
      // Now get CDEF info
      ACDB_MEM_CPY(&tempLut, sizeof(tempLut), &lutTbl.pLut[index], sizeof(tempLut));
      ACDB_MEM_CPY(&cdefTbl.nLen, sizeof(uint32_t), (tblInfo.tblCdftChnk.pData + tempLut.nCDEFTblOffset), sizeof(uint32_t));
      cdefTbl.pCntDef = (ContentDefIdType *)(tblInfo.tblCdftChnk.pData + tempLut.nCDEFTblOffset + sizeof(cdefTbl.nLen));

      // Now get CDOT info
      ACDB_MEM_CPY(&cdotTbl.nLen, sizeof(uint32_t), tblInfo.tblCdotChnk.pData + tempLut.nCDOTTblOffset, sizeof(uint32_t));
      cdotTbl.pDataOffsets = (uint32_t *)(tblInfo.tblCdotChnk.pData + tempLut.nCDOTTblOffset + sizeof(cdotTbl.nLen));

      if (cdefTbl.nLen != cdotTbl.nLen)
      {
         ACDB_DEBUG_LOG("The no of entries in CDEF and CDOT tables for devid %08X not matching\n", cmd.devId);
         return ACDB_ERROR;
      }

      switch (queryType)
      {
      case TABLE_CMD:
      {
         AcdbWdspDidUcidSubGraphIDType *pInput = (AcdbWdspDidUcidSubGraphIDType *)pIn;
         AcdbQueryResponseType *pOutput = (AcdbQueryResponseType *)pOut;
         result = GetModuleMetaInfoWithoutPadding(tblId, cdefTbl, cdotTbl, tblInfo.dataPoolChnk,
            pInput->nBufferPointer, pInput->nBufferLength, &pOutput->nBytesUsedInBuffer);
         if (ACDB_SUCCESS != result)
         {
            return result;
         }
      }
      break;
      case TABLE_SIZE_CMD:
      {
         AcdbSizeResponseType *pOutput = (AcdbSizeResponseType *)pOut;
         if (ACDB_SUCCESS != GetModuleMetaInfoSizeWithoutPadding(tblId, cdefTbl, cdotTbl, tblInfo.dataPoolChnk, &pOutput->nSize))
         {
            return ACDB_ERROR;
         }
      }
      break;
      default:
         return  ACDB_ERROR;
      }

      result = ACDB_SUCCESS;
   }

   return result;
}

int32_t AcdbGetWdspSubGraphDefinitions(uint32_t queryType, uint8_t *pIn, uint8_t *pOut)
{
   int32_t result = ACDB_SUCCESS;
   uint32_t idx = 0;
   uint8_t *buffer = NULL;
   uint32_t midIidCount = 0;

   if (pIn == NULL || pOut == NULL)
   {
      ACDB_DEBUG_LOG("[ACDB Command]->[AcdbGetWdspSubGraphDefinitions]->Invalid NULL value parameters are provided\n");
      result = ACDB_BADPARM;
   }
   else
   {
      uint8_t *pOutBufPtr = NULL;
      uint32_t nOutBufLength = 0;
      uint32_t outputBufPayloadOffset = 0;
	  WdspGetSubgraphDefnQry subgraphDefnQry = { 0, 0, 0, { 0, NULL } };
      memset(&subgraphDefnQry, 0, sizeof(subgraphDefnQry));

      switch (queryType)
      {
      case TABLE_CMD:
      {
         AcdbWdspDidUcidSubGraphIDType *pInput = (AcdbWdspDidUcidSubGraphIDType *)pIn;
         if (pInput->nBufferPointer == NULL)
         {
            return ACDB_BADPARM;
         }
         subgraphDefnQry.devId = pInput->nDeviceId;
         subgraphDefnQry.usecaseId = pInput->nUsecaseId;
         subgraphDefnQry.subgraphId = pInput->nSubGraphId;

         nOutBufLength = pInput->nBufferLength;
         pOutBufPtr = pInput->nBufferPointer;

         if (pOutBufPtr == NULL)
         {
            ACDB_DEBUG_LOG("[ACDB Command]->[AcdbGetWdspSubGraphDefinitions]->Payload buffer pointer is NULL\n");
            return ACDB_BADPARM;
         }
      }
      break;
      case TABLE_SIZE_CMD:
      {
         WdspDidUcidSubGraphIDSizeCmdType *pInput = (WdspDidUcidSubGraphIDSizeCmdType *)pIn;
         subgraphDefnQry.devId = pInput->nDeviceId;
         subgraphDefnQry.usecaseId = pInput->nUsecaseId;
         subgraphDefnQry.subgraphId = pInput->nSubGraphId;
      }
      break;
      default:
      {
         ACDB_DEBUG_LOG("[ACDB Command]->[AcdbGetWdspSubGraphDefinitions]->Invalid query %u\n",
            queryType);
         return  ACDB_ERROR;
      }
      }

      result = acdbdata_ioctl(
         ACDBDATACMD_GET_WDSP_SUBGRAPH_DEFINITION,
         (uint8_t *)&subgraphDefnQry,
         sizeof(subgraphDefnQry),
         (uint8_t *)NULL,
         0
         );
      if (result != ACDB_SUCCESS)
      {
         ACDB_DEBUG_LOG("[ACDB Command]->[AcdbGetWdspSubGraphDefinitions]->Failed to fetch the device property of the device %08X\n",
            subgraphDefnQry.devId);
         return result;
      }

      switch (queryType)
      {
      case TABLE_CMD:
      {
         AcdbWdspDidUcidSubGraphIDType *pInput = (AcdbWdspDidUcidSubGraphIDType *)pIn;
         AcdbQueryResponseType *pOutput = (AcdbQueryResponseType *)pOut;
         uint32_t size = 0;
         uint32_t bufferSize = pInput->nBufferLength;
         uint32_t globalPropertyDataOffset = 0;
		 AcdbWdspDidUcidSubGraphIDType *pModuleInfoInput = NULL;
         if (nOutBufLength < subgraphDefnQry.dataInfo.nDataLen)
         {
            ACDB_DEBUG_LOG("[ACDB Command]->[AcdbGetWdspSubGraphDefinitions]->Insufficient buffer size to copy the device property data for device %08X\n",
               subgraphDefnQry.devId);
            return ACDB_INSUFFICIENTMEMORY;
         }
         buffer = subgraphDefnQry.dataInfo.pData;
         midIidCount = READ_UInt32(buffer);
         globalPropertyDataOffset = sizeof(uint32_t);
         if (bufferSize < (midIidCount * (sizeof(uint32_t) + sizeof(uint16_t))))
         {
            ACDB_DEBUG_LOG("[ACDB Command]->[AcdbGetWdspSubGraphDefinitions]->Insufficient buffer size to copy the device property data for device %08X\n",
               subgraphDefnQry.devId);
            return ACDB_INSUFFICIENTMEMORY;
         }
		 
		 pModuleInfoInput = (AcdbWdspDidUcidSubGraphIDType *)malloc(sizeof(AcdbWdspDidUcidSubGraphIDType));
         if (NULL == pModuleInfoInput)
               return ACDB_INSUFFICIENTMEMORY;

         for (idx = 0; idx < midIidCount; idx++)
         {
            
            uint32_t mID = 0;
            uint32_t iID = 0;
            
            mID = READ_UInt32(buffer + globalPropertyDataOffset);
            globalPropertyDataOffset += (uint32_t)sizeof(uint32_t);

            iID = ((uint16_t)READ_UInt32(buffer + globalPropertyDataOffset));
            globalPropertyDataOffset += (uint32_t)sizeof(uint32_t);

            ACDB_MEM_CPY(pInput->nBufferPointer + outputBufPayloadOffset, bufferSize, &mID, sizeof(uint32_t));
            outputBufPayloadOffset += (uint32_t)sizeof(uint32_t);
            size += (uint32_t)sizeof(uint32_t);
            bufferSize -= (uint32_t)sizeof(uint32_t);

            ACDB_MEM_CPY(pInput->nBufferPointer + outputBufPayloadOffset, bufferSize, (uint8_t *)&iID, (uint32_t)sizeof(uint16_t));
            outputBufPayloadOffset += (uint32_t)sizeof(uint16_t);
            size += (uint32_t)sizeof(uint16_t);
            bufferSize -= (uint32_t)sizeof(uint16_t);

            pModuleInfoInput->nDeviceId = pInput->nDeviceId;
            pModuleInfoInput->nSubGraphId = pInput->nSubGraphId;
            pModuleInfoInput->nUsecaseId = pInput->nUsecaseId;
            pModuleInfoInput->nBufferLength = bufferSize; //Redusing the size we already added MID and IID to input buffer
            pModuleInfoInput->nBufferPointer = pInput->nBufferPointer + outputBufPayloadOffset; //Moving the buffer pointer
            result = AcdbGetWdspModuleInfo(TABLE_CMD, (uint8_t *)pModuleInfoInput, (uint8_t *)pOutput, mID, iID); //Updating i_pin and o_pin in input buffer

            if (result == ACDB_SUCCESS)
            {
               outputBufPayloadOffset += pOutput->nBytesUsedInBuffer;
               size += pOutput->nBytesUsedInBuffer;
            }
            else
            {
               result = ACDB_BADSTATE;
            }
         }
		 ACDB_MEM_FREE(pModuleInfoInput);
         pOutput->nBytesUsedInBuffer = size;
      }
      break;
      case TABLE_SIZE_CMD:
      {
         AcdbWdspDidUcidSubGraphIDType *pInput = (AcdbWdspDidUcidSubGraphIDType *)pIn;
         AcdbSizeResponseType *pOutput = (AcdbSizeResponseType *)pOut;
         uint32_t size = 0;
         uint32_t globalPropertyDataOffset = sizeof(uint32_t);
         buffer = subgraphDefnQry.dataInfo.pData;
         midIidCount = READ_UInt32(buffer);

         for (idx = 0; idx < midIidCount; idx++)
         {
            uint32_t mID = 0;
            uint32_t iID = 0;

            mID = READ_UInt32(buffer + globalPropertyDataOffset);
            globalPropertyDataOffset += (uint32_t)sizeof(uint32_t);
            size += (uint32_t)sizeof(uint32_t);

            iID = READ_UInt32(buffer + globalPropertyDataOffset);
            globalPropertyDataOffset += (uint32_t)sizeof(uint32_t);
            size += (uint32_t)sizeof(uint16_t); //Instance ID will be written as 16bit unsigned integer in payload.

            result = AcdbGetWdspModuleInfo(TABLE_SIZE_CMD, (uint8_t *)pInput, (uint8_t *)pOutput, mID, iID); //Updating i_pin and o_pin in input buffer

            size += pOutput->nSize;
         }
         pOutput->nSize = size;
      }
      break;
      default:
      {
         ACDB_DEBUG_LOG("[ACDB Command]->[AcdbGetWdspSubGraphDefinitions]->Invalid query %u\n", queryType);
         result = ACDB_ERROR;
      }
      }
   }

   return result;
}

int32_t AcdbGetWdspCalibrationData(uint32_t queryType, uint8_t *pIn, uint8_t *pOut, uint32_t MID, uint32_t IID)
{
   int32_t result = ACDB_SUCCESS;

   if (pIn == NULL || pOut == NULL)
   {
      ACDB_DEBUG_LOG("[ACDB Command]->[AcdbGetWdspCalibrationData]->Invalid NULL value parameters are provided\n");
      result = ACDB_BADPARM;
   }
   else
   {
      uint32_t index = 0;
      uint32_t tblId = WDSP_MODULE_CAL_TBL_ID;
      AcdbTableCmd cmd = { 0, 0 };
      AcdbTableInfo tblInfo = { { 0, NULL }, { 0, NULL }, { 0, NULL } };
	  WdspModuleCalibDataLookupTblType lutTbl = { 0, NULL };
      ContentDefWithOnlyParamTblType cdefTbl = { 0, NULL };
      ContentDataOffsetsTblType cdotTbl = { 0, NULL };
      uintptr_t nLookupKey = 0;
	  WdspModuleCalibCmdLookupType moduleCalibcmd = { 0, 0 };
	  WdspModuleCalibDataLookupType tempLut = { 0, 0, 0, 0 };

      switch (queryType)
      {
      case TABLE_CMD:
      {
         AcdbWdspDidUcidSubGraphIDType *pInput = (AcdbWdspDidUcidSubGraphIDType *)pIn;
         if (pInput->nBufferPointer == NULL)
         {
            return ACDB_BADPARM;
         }
         cmd.devId = pInput->nDeviceId;
         moduleCalibcmd.nMId = MID;
         moduleCalibcmd.nIId = IID;
      }
      break;
      case TABLE_SIZE_CMD:
      {
         WdspDidUcidSubGraphIDSizeCmdType *pInput = (WdspDidUcidSubGraphIDSizeCmdType *)pIn;
         cmd.devId = pInput->nDeviceId;
         moduleCalibcmd.nMId = MID;
         moduleCalibcmd.nIId = IID;
      }
      break;
      default:
         return  ACDB_ERROR;
      }

      cmd.tblId = tblId;
      result = acdbdata_ioctl(ACDBDATACMD_GET_TABLE_INFO, (uint8_t *)&cmd, sizeof(cmd),
         (uint8_t *)&tblInfo, sizeof(tblInfo));
      if (result != ACDB_SUCCESS)
      {
         ACDB_DEBUG_LOG("Failed to fetch the lookup information of the device %08X \n"
            , cmd.devId);
         return result;
      }
      lutTbl.nLen = READ_UInt32(tblInfo.tblLutChnk.pData);
      lutTbl.pLut = (WdspModuleCalibDataLookupType *)(tblInfo.tblLutChnk.pData + sizeof(lutTbl.nLen));

      result = AcdbDataBinarySearch((void *)lutTbl.pLut, lutTbl.nLen,
         WDSP_MODULE_CALIB_LUT_INDICES_COUNT, &moduleCalibcmd, WDSP_MODULE_CALIB_CMD_INDICES_COUNT, &index);
      if (result != SEARCH_SUCCESS)
      {
         ACDB_DEBUG_LOG("Failed to fetch the lookup information of the device %08X \n"
            , cmd.devId);
         return ACDB_INPUT_PARAMS_NOT_FOUND;
      }

      nLookupKey = (uintptr_t)&lutTbl.pLut[index];
      // Now get CDEF info
      ACDB_MEM_CPY(&tempLut, sizeof(tempLut), &lutTbl.pLut[index], sizeof(tempLut));
      ACDB_MEM_CPY(&cdefTbl.nLen, sizeof(uint32_t), (tblInfo.tblCdftChnk.pData + tempLut.nCDEFTblOffset), sizeof(uint32_t));
      cdefTbl.pCntDef = (ContentDefIdType *)(tblInfo.tblCdftChnk.pData + tempLut.nCDEFTblOffset + sizeof(cdefTbl.nLen));

      // Now get CDOT info
      ACDB_MEM_CPY(&cdotTbl.nLen, sizeof(uint32_t), tblInfo.tblCdotChnk.pData + tempLut.nCDOTTblOffset, sizeof(uint32_t));
      cdotTbl.pDataOffsets = (uint32_t *)(tblInfo.tblCdotChnk.pData + tempLut.nCDOTTblOffset + sizeof(cdotTbl.nLen));

      if (cdefTbl.nLen != cdotTbl.nLen)
      {
         ACDB_DEBUG_LOG("The no of entries in CDEF and CDOT tables for devid %08X not matching\n", cmd.devId);
         return ACDB_ERROR;
      }

      switch (queryType)
      {
      case TABLE_CMD:
      {
         AcdbWdspDidUcidSubGraphIDType *pInput = (AcdbWdspDidUcidSubGraphIDType *)pIn;
         AcdbQueryResponseType *pOutput = (AcdbQueryResponseType *)pOut;
         result = GetIdcalibTable(tblId, nLookupKey, cdefTbl, cdotTbl, tblInfo.dataPoolChnk,
            pInput->nBufferPointer, pInput->nBufferLength, &pOutput->nBytesUsedInBuffer);
         if (ACDB_SUCCESS != result)
         {
            return result;
         }
      }
      break;
      case TABLE_SIZE_CMD:
      {
         AcdbSizeResponseType *pOutput = (AcdbSizeResponseType *)pOut;

         if(pOutput->nSize == 0)
         {
             ACDB_DEBUG_LOG("Recieved Size pointer with NULL value for tblid = %d\n", tblId);
             return ACDB_BADPARM;
         }

         if (ACDB_SUCCESS != GetIdcalibTableSize(tblId, cdefTbl, cdotTbl, tblInfo.dataPoolChnk, &pOutput->nSize))
         {
            return ACDB_ERROR;
         }
      }
      break;
      default:
         return  ACDB_ERROR;
      }

      result = ACDB_SUCCESS;
   }

   return result;
}

int32_t AcdbCmdGetWdspModuleCalibration(uint32_t queryType, uint8_t *pIn, uint8_t *pOut)
{
   int32_t result = ACDB_SUCCESS;
   uint32_t idx = 0;
   uint8_t *buffer = NULL;
   uint32_t midIidCount = 0;
   if (pIn == NULL || pOut == NULL)
   {
      ACDB_DEBUG_LOG("[ACDB Command]->[WdspCmdGetModuleCalibration]->Invalid NULL value parameters are provided\n");
      result = ACDB_BADPARM;
   }
   else
   {
      uint8_t *pOutBufPtr = NULL;
      uint32_t nOutBufLength = 0;
      uint32_t outputBufPayloadOffset = 0;
      WdspGetSubgraphDefnQry subgraphDefnQry = { 0, 0, 0, { 0, NULL } };
      memset(&subgraphDefnQry, 0, sizeof(subgraphDefnQry));

      switch (queryType)
      {
      case TABLE_CMD:
      {
         AcdbWdspDidUcidSubGraphIDType *pInput = (AcdbWdspDidUcidSubGraphIDType *)pIn;
         if (pInput->nBufferPointer == NULL)
         {
            return ACDB_BADPARM;
         }
         subgraphDefnQry.devId = pInput->nDeviceId;
         subgraphDefnQry.usecaseId = pInput->nUsecaseId;
         subgraphDefnQry.subgraphId = pInput->nSubGraphId;

         nOutBufLength = pInput->nBufferLength;
         pOutBufPtr = pInput->nBufferPointer;

         if (pOutBufPtr == NULL)
         {
            ACDB_DEBUG_LOG("[ACDB Command]->[WdspCmdGetModuleCalibration]->Payload buffer pointer is NULL\n");
            return ACDB_BADPARM;
         }
      }
      break;
      case TABLE_SIZE_CMD:
      {
         WdspDidUcidSubGraphIDSizeCmdType *pInput = (WdspDidUcidSubGraphIDSizeCmdType *)pIn;
         subgraphDefnQry.devId = pInput->nDeviceId;
         subgraphDefnQry.usecaseId = pInput->nUsecaseId;
         subgraphDefnQry.subgraphId = pInput->nSubGraphId;
      }
      break;
      default:
      {
         ACDB_DEBUG_LOG("[ACDB Command]->[WdspCmdGetModuleCalibration]->Invalid query %u\n",
            queryType);
         return  ACDB_ERROR;
      }
      }

      result = acdbdata_ioctl(
         ACDBDATACMD_GET_WDSP_SUBGRAPH_DEFINITION,
         (uint8_t *)&subgraphDefnQry,
         sizeof(subgraphDefnQry),
         (uint8_t *)NULL,
         0
         );
      if (result != ACDB_SUCCESS)
      {
         ACDB_DEBUG_LOG("[ACDB Command]->[WdspCmdGetModuleCalibration]->Failed to fetch the device property of the device %08X\n",
            subgraphDefnQry.devId);
         return result;
      }

      switch (queryType)
      {
      case TABLE_CMD:
      {
         AcdbWdspDidUcidSubGraphIDType *pInput = (AcdbWdspDidUcidSubGraphIDType *)pIn;
         AcdbQueryResponseType *pOutput = (AcdbQueryResponseType *)pOut;
         uint32_t size = 0;
         uint32_t globalPropertyDataOffset = 0;
         uint32_t bufferSize = pInput->nBufferLength;
		 AcdbWdspDidUcidSubGraphIDType *pModuleCalibInput = NULL;
         if (nOutBufLength < subgraphDefnQry.dataInfo.nDataLen)
         {
            ACDB_DEBUG_LOG("[ACDB Command]->[WdspCmdGetModuleCalibration]->Insufficient buffer size to copy the device property data for device %08X\n",
               subgraphDefnQry.devId);
            return ACDB_INSUFFICIENTMEMORY;
         }

         buffer = subgraphDefnQry.dataInfo.pData;
         midIidCount = READ_UInt32(buffer);
         globalPropertyDataOffset = sizeof(uint32_t);
         if (bufferSize < (midIidCount * (sizeof(uint32_t) + sizeof(uint16_t))))
         {
            ACDB_DEBUG_LOG("[ACDB Command]->[WdspCmdGetModuleCalibration]->Insufficient buffer size to copy the device property data for device %08X\n",
               subgraphDefnQry.devId);
            return ACDB_INSUFFICIENTMEMORY;
         }

		 pModuleCalibInput = (AcdbWdspDidUcidSubGraphIDType *)malloc(sizeof(AcdbWdspDidUcidSubGraphIDType));
            if (NULL == pModuleCalibInput)
               return ACDB_INSUFFICIENTMEMORY;

         for (idx = 0; idx < midIidCount; idx++)
         {
            uint32_t mID = 0;
            uint32_t iID = 0;

            mID = READ_UInt32(buffer + globalPropertyDataOffset);
            globalPropertyDataOffset += (uint32_t)sizeof(uint32_t);

            iID = READ_UInt32(buffer + globalPropertyDataOffset);
            globalPropertyDataOffset += (uint32_t)sizeof(uint32_t);

            pModuleCalibInput->nDeviceId = pInput->nDeviceId;
            pModuleCalibInput->nSubGraphId = pInput->nSubGraphId;
            pModuleCalibInput->nUsecaseId = pInput->nUsecaseId;
            pModuleCalibInput->nBufferLength = bufferSize; //Redusing the size we already added MID and IID to input buffer
            pModuleCalibInput->nBufferPointer = pOutBufPtr; //Moving the buffer pointer

            result = AcdbGetWdspCalibrationData(TABLE_CMD, (uint8_t *)pModuleCalibInput, (uint8_t *)pOutput, mID, iID);

            if (result == ACDB_SUCCESS)
            {
               outputBufPayloadOffset += pOutput->nBytesUsedInBuffer;
               size += pOutput->nBytesUsedInBuffer;
               pOutBufPtr += pOutput->nBytesUsedInBuffer;
            }
            else
            {
               result = ACDB_BADSTATE;
			   ACDB_MEM_FREE(pModuleCalibInput);
               return result;
            }
         }
		 ACDB_MEM_FREE(pModuleCalibInput);
         pOutput->nBytesUsedInBuffer = size;
      }
      break;
      case TABLE_SIZE_CMD:
      {
         AcdbWdspDidUcidSubGraphIDType *pInput = (AcdbWdspDidUcidSubGraphIDType *)pIn;
         AcdbSizeResponseType *pOutput = (AcdbSizeResponseType *)pOut;
         uint32_t size = 0;
         uint32_t globalPropertyDataOffset = sizeof(uint32_t);
         buffer = subgraphDefnQry.dataInfo.pData;
         midIidCount = READ_UInt32(buffer);

         for (idx = 0; idx < midIidCount; idx++)
         {
            uint32_t mID = 0;
            uint32_t iID = 0;

            mID = READ_UInt32(buffer + globalPropertyDataOffset);
            globalPropertyDataOffset += (uint32_t)sizeof(uint32_t);

            iID = READ_UInt32(buffer + globalPropertyDataOffset);
            globalPropertyDataOffset += (uint32_t)sizeof(uint32_t);

            result = AcdbGetWdspCalibrationData(TABLE_SIZE_CMD, (uint8_t *)pInput, (uint8_t *)pOutput, mID, iID); //Updating i_pin and o_pin in input buffer
            size += pOutput->nSize;
         }
         pOutput->nSize = size;
      }
      break;
      default:
      {
         ACDB_DEBUG_LOG("[ACDB Command]->[WdspCmdGetModuleCalibration]->Invalid query %u\n", queryType);
         result = ACDB_ERROR;
      }
      }
   }

   return result;
}

int32_t AcdbCmdAppendWdspHeader(uint8_t *pBuffer, uint8_t *pOut, uint8_t **paramSizeOffset, enum PidType paramType)
{
   int32_t result = ACDB_SUCCESS;
   AcdbQueryResponseType *pOutput = (AcdbQueryResponseType *)pOut;
   WdspFrameworkIds wdspfwkIDS = { 0, { 0, NULL } };
   result = acdbdata_ioctl(
      ACDBDATACMD_GET_WDSP_FRAMEWORKIDS,
      (uint8_t *)&wdspfwkIDS,
      sizeof(wdspfwkIDS),
      (uint8_t *)NULL,
      0
      );
   if (result == ACDB_SUCCESS)
   {
      uint32_t outputBufPayloadOffset = 0;
      uint32_t offset = 0;
      uint16_t reserved = 0, IID_fwk = 0;
      offset += (uint32_t)sizeof(uint32_t);

      ACDB_MEM_CPY(pBuffer + outputBufPayloadOffset, sizeof(uint32_t), wdspfwkIDS.dataInfo.pData + offset, sizeof(uint32_t)); //MID_FWK
      outputBufPayloadOffset += (uint32_t)sizeof(uint32_t);
      offset += (uint32_t)sizeof(uint32_t);

      IID_fwk = (uint16_t)READ_UInt32(wdspfwkIDS.dataInfo.pData + offset);
      ACDB_MEM_CPY(pBuffer + outputBufPayloadOffset, sizeof(uint32_t), &IID_fwk, sizeof(uint16_t)); //IID_FWK
      outputBufPayloadOffset += (uint32_t)sizeof(uint16_t);
      offset += (uint32_t)sizeof(uint32_t);

      ACDB_MEM_CPY(pBuffer + outputBufPayloadOffset, sizeof(uint16_t), &reserved, sizeof(uint16_t)); //Reserved
      outputBufPayloadOffset += (uint32_t)sizeof(uint16_t);

      if (paramType == PID_FWK_MODULE_INFO_LIST)
      {
         ACDB_MEM_CPY(pBuffer + outputBufPayloadOffset, sizeof(uint32_t), wdspfwkIDS.dataInfo.pData + offset, sizeof(uint32_t)); //PID_FWK_MODULE_INFO_LIST
         outputBufPayloadOffset += (uint32_t)sizeof(uint32_t);
         offset += (uint32_t)sizeof(uint32_t);
      }
      else if (paramType == PID_FWK_CONNECT)
      {
         offset += (uint32_t)sizeof(uint32_t); //Skip PID_FWK_MODULE_INFO_LIST and mode to next data.
         ACDB_MEM_CPY(pBuffer + outputBufPayloadOffset, sizeof(uint32_t), wdspfwkIDS.dataInfo.pData + offset, sizeof(uint32_t)); //PID_FWK_CONNECT
         outputBufPayloadOffset += (uint32_t)sizeof(uint32_t);
         offset += (uint32_t)sizeof(uint32_t);
      }

      *paramSizeOffset = pBuffer + outputBufPayloadOffset;
      //Have to save this to write the param size after calculating the entire param size
      outputBufPayloadOffset += (uint32_t)sizeof(uint16_t); //moving to next assuming param size is added

      ACDB_MEM_CPY(pBuffer + outputBufPayloadOffset, sizeof(uint16_t), &reserved, sizeof(uint16_t)); //Reserved
      outputBufPayloadOffset += (uint32_t)sizeof(uint16_t);

      pOutput->nBytesUsedInBuffer = ACDB_WDSP_FWK_HEADER_SIZE;
   }
   return result;
}

int32_t AcdbCmdGetWdspSubGraphModuleInfo(uint32_t queryType, uint8_t *pIn, uint8_t *pOut)
{
   int32_t result = ACDB_SUCCESS;

   if (pIn == NULL || pOut == NULL)
   {
      ACDB_DEBUG_LOG("[ACDB Command]->[WdspGetSubGraphModuleInfo]->Invalid NULL value parameters are provided\n");
      result = ACDB_BADPARM;
   }
   else
   {
      switch (queryType)
      {
      case TABLE_CMD:
      {
         AcdbWdspDidUcidSubGraphIDType *pInput = (AcdbWdspDidUcidSubGraphIDType *)pIn;
         AcdbQueryResponseType *pOutput = (AcdbQueryResponseType *)pOut;
         uint8_t *paramSizeOffset = NULL;
         if (pInput->nBufferLength < ACDB_WDSP_FWK_HEADER_SIZE) //Input buffer length should be minimum of that header length.
         {
            ACDB_DEBUG_LOG("ACDB_COMMAND: Buffer size insufficient to copy the subgraph module info data for devid %08X \n", pInput->nDeviceId);
            return ACDB_INSUFFICIENTMEMORY;
         }

         result = AcdbCmdAppendWdspHeader(pInput->nBufferPointer, pOut, &paramSizeOffset, PID_FWK_MODULE_INFO_LIST);
         if (result == ACDB_SUCCESS)
         {
			 AcdbWdspDidUcidSubGraphIDType *pModuleInfoInput = NULL;
            pModuleInfoInput = (AcdbWdspDidUcidSubGraphIDType *)malloc(sizeof(AcdbWdspDidUcidSubGraphIDType));
            if (NULL == pModuleInfoInput)
               return ACDB_INSUFFICIENTMEMORY;

            pModuleInfoInput->nDeviceId = pInput->nDeviceId;
            pModuleInfoInput->nSubGraphId = pInput->nSubGraphId;
            pModuleInfoInput->nUsecaseId = pInput->nUsecaseId;
            pModuleInfoInput->nBufferLength = (uint32_t)(pInput->nBufferLength - ACDB_WDSP_FWK_HEADER_SIZE); //Header is already written in the buffer.
            pModuleInfoInput->nBufferPointer = pInput->nBufferPointer + pOutput->nBytesUsedInBuffer; //Moving the buffer pointer

            result = AcdbGetWdspSubGraphDefinitions(TABLE_CMD, (uint8_t *)pModuleInfoInput, pOut);
            if (result == ACDB_SUCCESS)
            {
               ACDB_MEM_CPY(paramSizeOffset, sizeof(uint16_t), (uint8_t *)&pOutput->nBytesUsedInBuffer, sizeof(uint16_t));
               pOutput->nBytesUsedInBuffer = (uint32_t)(pOutput->nBytesUsedInBuffer + ACDB_WDSP_FWK_HEADER_SIZE);
            }
			ACDB_MEM_FREE(pModuleInfoInput);
         }
      }
      break;
      case TABLE_SIZE_CMD:
      {
         AcdbSizeResponseType *pOutput = (AcdbSizeResponseType *)pOut;
         result = AcdbGetWdspSubGraphDefinitions(TABLE_SIZE_CMD, pIn, pOut);
         pOutput->nSize += (uint32_t)(ACDB_WDSP_FWK_HEADER_SIZE);
      }
      break;
      default:
         return  ACDB_ERROR;
      }
      result = ACDB_SUCCESS;
   }
   return result;
}

int32_t AcdbCmdGetWdspModuleConnections(uint32_t queryType, uint8_t *pIn, uint8_t *pOut)
{
   int32_t result = ACDB_SUCCESS;

   if (pIn == NULL || pOut == NULL)
   {
      ACDB_DEBUG_LOG("[ACDB Command]->[WdspCmdGetSubGraphConnection]->Invalid NULL value parameters are provided\n");
      result = ACDB_BADPARM;
   }
   else
   {
      uint32_t index = 0;
      uint32_t tblId = WDSP_MODULE_CONN_TBL_ID;
      AcdbTableCmd cmd = { 0, 0 };
      AcdbTableInfo tblInfo = { { 0, NULL }, { 0, NULL }, { 0, NULL } };
	  WdspModuleConnDataLookupTblType lutTbl = { 0, NULL };
      ContentDefWithInstanceTblType cdefTbl = { 0, NULL };
      ContentDataOffsetsTblType cdotTbl = { 0, NULL };
      //uintptr_t nLookupKey = 0;
	  WdspModuleConnCmdLookupType moduleConncmd = { 0, 0, 0 };
	  WdspModuleConnDataLookupType tempLut = { 0, 0, 0, 0, 0 };

      switch (queryType)
      {
      case TABLE_CMD:
      {
         AcdbWdspDidUcidSubGraphIDType *pInput = (AcdbWdspDidUcidSubGraphIDType *)pIn;
         if (pInput->nBufferPointer == NULL)
         {
            return ACDB_BADPARM;
         }
         moduleConncmd.nDeviceId = pInput->nDeviceId;
         moduleConncmd.nUsecaseId = pInput->nUsecaseId;
         moduleConncmd.nSubgraphId = pInput->nSubGraphId;
      }
      break;
      case TABLE_SIZE_CMD:
      {
         WdspDidUcidSubGraphIDSizeCmdType *pInput = (WdspDidUcidSubGraphIDSizeCmdType *)pIn;
         moduleConncmd.nDeviceId = pInput->nDeviceId;
         moduleConncmd.nUsecaseId = pInput->nUsecaseId;
         moduleConncmd.nSubgraphId = pInput->nSubGraphId;
      }
      break;
      default:
         return  ACDB_ERROR;
      }

      cmd.devId = moduleConncmd.nDeviceId;
      cmd.tblId = tblId;
      result = acdbdata_ioctl(ACDBDATACMD_GET_TABLE_INFO, (uint8_t *)&cmd, sizeof(cmd),
         (uint8_t *)&tblInfo, sizeof(tblInfo));
      if (result != ACDB_SUCCESS)
      {
         ACDB_DEBUG_LOG("Failed to fetch the lookup information of the device %08X \n"
            , moduleConncmd.nDeviceId);
         return result;
      }
      lutTbl.nLen = READ_UInt32(tblInfo.tblLutChnk.pData);
      lutTbl.pLut = (WdspModuleConnDataLookupType *)(tblInfo.tblLutChnk.pData + sizeof(lutTbl.nLen));

      result = AcdbDataBinarySearch((void *)lutTbl.pLut, lutTbl.nLen,
         WDSP_MODULE_CONN_LUT_INDICES_COUNT, &moduleConncmd, WDSP_MODULE_CONN_CMD_INDICES_COUNT, &index);
      if (result != SEARCH_SUCCESS)
      {
         switch (queryType)
         {
         case TABLE_CMD:
         {
            AcdbQueryResponseType *pOutput = (AcdbQueryResponseType *)pOut;
            pOutput->nBytesUsedInBuffer = 0;
         }
         case TABLE_SIZE_CMD:
         {
            AcdbSizeResponseType *pOutput = (AcdbSizeResponseType *)pOut;
            pOutput->nSize = 0;
         }
         }
         return ACDB_SUCCESS;
      }

      //nLookupKey = (uintptr_t)&lutTbl.pLut[index];
      // Now get CDEF info
      ACDB_MEM_CPY(&tempLut, sizeof(tempLut), &lutTbl.pLut[index], sizeof(tempLut));
      ACDB_MEM_CPY(&cdefTbl.nLen, sizeof(uint32_t), (tblInfo.tblCdftChnk.pData + tempLut.nCDEFTblOffset), sizeof(uint32_t));
      cdefTbl.pCntDef = (ContentDefInstanceType *)(tblInfo.tblCdftChnk.pData + tempLut.nCDEFTblOffset + sizeof(cdefTbl.nLen));

      // Now get CDOT info
      ACDB_MEM_CPY(&cdotTbl.nLen, sizeof(uint32_t), tblInfo.tblCdotChnk.pData + tempLut.nCDOTTblOffset, sizeof(uint32_t));
      cdotTbl.pDataOffsets = (uint32_t *)(tblInfo.tblCdotChnk.pData + tempLut.nCDOTTblOffset + sizeof(cdotTbl.nLen));

      if (cdefTbl.nLen != cdotTbl.nLen)
      {
         ACDB_DEBUG_LOG("The no of entries in CDEF and CDOT tables for devid %08X not matching\n", moduleConncmd.nDeviceId);
         return ACDB_ERROR;
      }

      switch (queryType)
      {
      case TABLE_CMD:
      {
         AcdbWdspDidUcidSubGraphIDType *pInput = (AcdbWdspDidUcidSubGraphIDType *)pIn;
         AcdbQueryResponseType *pOutput = (AcdbQueryResponseType *)pOut;

         //First Add WDSP framework Header.
         //Header format: mid_fwk(4) + iid_fwk(2) + reserved(2) + pid_fwk(4) + paramsize(2) + reserved(2)
         uint8_t *paramSizeOffset = NULL;
         uint32_t headerSize = 0;
         uint32_t remainingBuffer = pInput->nBufferLength;
         if (remainingBuffer < ACDB_WDSP_FWK_HEADER_SIZE) //Input buffer length should be minimum of that header length.
         {
            ACDB_DEBUG_LOG("ACDB_COMMAND: Buffer size insufficient to copy the subgraph module info data for devid %08X \n", pInput->nDeviceId);
            return ACDB_INSUFFICIENTMEMORY;
         }
         result = AcdbCmdAppendWdspHeader(pInput->nBufferPointer, pOut, &paramSizeOffset, PID_FWK_CONNECT);
         if (ACDB_SUCCESS != result)
         {
            return result;
         }
         headerSize = pOutput->nBytesUsedInBuffer;
         remainingBuffer -= headerSize;

         result = GetMidPidCalibTableWithIid(tblId, cdefTbl, cdotTbl, tblInfo.dataPoolChnk,
            pInput->nBufferPointer + headerSize, remainingBuffer, &pOutput->nBytesUsedInBuffer);
         ACDB_MEM_CPY(paramSizeOffset, pInput->nBufferLength, &pOutput->nBytesUsedInBuffer, sizeof(uint32_t)); //Updating param size in wdsp frameworkheader
         pOutput->nBytesUsedInBuffer += headerSize;
         if (ACDB_SUCCESS != result)
         {
            return result;
         }
      }
      break;
      case TABLE_SIZE_CMD:
      {
         AcdbSizeResponseType *pOutput = (AcdbSizeResponseType *)pOut;
         if (ACDB_SUCCESS != GetMidPidCalibTableSizeWithIid(tblId, cdefTbl, cdotTbl, tblInfo.dataPoolChnk, &pOutput->nSize))
         {
            return ACDB_ERROR;
         }
         pOutput->nSize += (uint32_t)(ACDB_WDSP_FWK_HEADER_SIZE);
      }
      break;
      default:
         return  ACDB_ERROR;
      }

      result = ACDB_SUCCESS;
   }

   return result;
}

int32_t AcdbCmdGetWdspSubGraphConnections(uint32_t queryType, uint8_t *pIn, uint8_t *pOut)
{
   int32_t result = ACDB_SUCCESS;

   if (pIn == NULL || pOut == NULL)
   {
      ACDB_DEBUG_LOG("[ACDB Command]->[WdspCmdGetSubGraphConnection]->Invalid NULL value parameters are provided\n");
      result = ACDB_BADPARM;
   }
   else
   {
      uint32_t index = 0;
      uint32_t tblId = WDSP_SUBGRAPH_CONN_TBL_ID;
      AcdbTableCmd cmd = { 0, 0 };
      AcdbTableInfo tblInfo = { { 0, NULL }, { 0, NULL }, { 0, NULL } };
	  WdspSubgraphConnDataLookupTblType lutTbl = { 0, NULL };
      ContentDefWithInstanceTblType cdefTbl = { 0, NULL };
      ContentDataOffsetsTblType cdotTbl = { 0, NULL };
      //uintptr_t nLookupKey = 0;
	  WdspSubgraphConnCmdLookupType moduleConncmd = { 0, 0, 0, 0 };
	  WdspSubgraphConnDataLookupType tempLut = { 0, 0, 0, 0, 0, 0};

      switch (queryType)
      {
      case TABLE_CMD:
      {
         AcdbWdspDidUcidSubGraphIDPairType *pInput = (AcdbWdspDidUcidSubGraphIDPairType *)pIn;
         if (pInput->nBufferPointer == NULL)
         {
            return ACDB_BADPARM;
         }
         moduleConncmd.nDeviceId = pInput->nDeviceId;
         moduleConncmd.nUsecaseId = pInput->nUsecaseId;
         moduleConncmd.nSubgraphSrc = pInput->nSubGraphIdSrc;
         moduleConncmd.nSubgraphDst = pInput->nSubGraphIdDst;
      }
      break;
      case TABLE_SIZE_CMD:
      {
         WdspSubgraphConnectionsSizeCmdType *pInput = (WdspSubgraphConnectionsSizeCmdType *)pIn;
         moduleConncmd.nDeviceId = pInput->nDeviceId;
         moduleConncmd.nUsecaseId = pInput->nUsecaseId;
         moduleConncmd.nSubgraphSrc = pInput->nSubGraphIdSrc;
         moduleConncmd.nSubgraphDst = pInput->nSubGraphIdDst;
      }
      break;
      default:
         return  ACDB_ERROR;
      }

      cmd.devId = moduleConncmd.nDeviceId;
      cmd.tblId = tblId;
      result = acdbdata_ioctl(ACDBDATACMD_GET_TABLE_INFO, (uint8_t *)&cmd, sizeof(cmd),
         (uint8_t *)&tblInfo, sizeof(tblInfo));
      if (result != ACDB_SUCCESS)
      {
         ACDB_DEBUG_LOG("Failed to fetch the lookup information of the device %08X \n"
            , moduleConncmd.nDeviceId);
         return result;
      }
      lutTbl.nLen = READ_UInt32(tblInfo.tblLutChnk.pData);
      lutTbl.pLut = (WdspSubgraphConnDataLookupType *)(tblInfo.tblLutChnk.pData + sizeof(lutTbl.nLen));

      result = AcdbDataBinarySearch((void *)lutTbl.pLut, lutTbl.nLen,
         WDSP_SUBGRAPH_CONN_LUT_INDICES_COUNT, &moduleConncmd, WDSP_SUBGRAPH_CONN_CMD_INDICES_COUNT, &index);
      if (result != SEARCH_SUCCESS)
      {
         switch (queryType)
         {
         case TABLE_CMD:
         {
            AcdbQueryResponseType *pOutput = (AcdbQueryResponseType *)pOut;
            pOutput->nBytesUsedInBuffer = 0;
         }
         break;
         case TABLE_SIZE_CMD:
         {
            AcdbSizeResponseType *pOutput = (AcdbSizeResponseType *)pOut;
            pOutput->nSize = 0;
         }
         break;
         }
         return ACDB_SUCCESS;
      }

      //nLookupKey = (uintptr_t)&lutTbl.pLut[index];
      // Now get CDEF info
      ACDB_MEM_CPY(&tempLut, sizeof(tempLut), &lutTbl.pLut[index], sizeof(tempLut));
      ACDB_MEM_CPY(&cdefTbl.nLen, sizeof(uint32_t), (tblInfo.tblCdftChnk.pData + tempLut.nCDEFTblOffset), sizeof(uint32_t));
      cdefTbl.pCntDef = (ContentDefInstanceType *)(tblInfo.tblCdftChnk.pData + tempLut.nCDEFTblOffset + sizeof(cdefTbl.nLen));

      // Now get CDOT info
      ACDB_MEM_CPY(&cdotTbl.nLen, sizeof(uint32_t), tblInfo.tblCdotChnk.pData + tempLut.nCDOTTblOffset, sizeof(uint32_t));
      cdotTbl.pDataOffsets = (uint32_t *)(tblInfo.tblCdotChnk.pData + tempLut.nCDOTTblOffset + sizeof(cdotTbl.nLen));

      if (cdefTbl.nLen != cdotTbl.nLen)
      {
         ACDB_DEBUG_LOG("The no of entries in CDEF and CDOT tables for devid %08X not matching\n", moduleConncmd.nDeviceId);
         return ACDB_ERROR;
      }

      switch (queryType)
      {
      case TABLE_CMD:
      {
         AcdbWdspDidUcidSubGraphIDPairType *pInput = (AcdbWdspDidUcidSubGraphIDPairType *)pIn;
         AcdbQueryResponseType *pOutput = (AcdbQueryResponseType *)pOut;

         //First Add WDSP framework Header.
         //Header format: mid_fwk(4) + iid_fwk(2) + reserved(2) + pid_fwk(4) + paramsize(2) + reserved(2)
         uint8_t *paramSizeOffset = NULL;
         uint32_t headerSize = 0;
         uint32_t remainingBuffer = pInput->nBufferLength;
         if (remainingBuffer < ACDB_WDSP_FWK_HEADER_SIZE) //Input buffer length should be minimum of that header length.
         {
            ACDB_DEBUG_LOG("ACDB_COMMAND: Buffer size insufficient to copy the subgraph module info data for devid %08X \n", pInput->nDeviceId);
            return ACDB_INSUFFICIENTMEMORY;
         }
         result = AcdbCmdAppendWdspHeader(pInput->nBufferPointer, pOut, &paramSizeOffset, PID_FWK_CONNECT);
         if (ACDB_SUCCESS != result)
         {
            return result;
         }
         headerSize = pOutput->nBytesUsedInBuffer;
         remainingBuffer -= headerSize;

         result = GetMidPidCalibTableWithIid(tblId, cdefTbl, cdotTbl, tblInfo.dataPoolChnk,
            pInput->nBufferPointer + headerSize, remainingBuffer, &pOutput->nBytesUsedInBuffer);
         ACDB_MEM_CPY(paramSizeOffset, pInput->nBufferLength, &pOutput->nBytesUsedInBuffer, sizeof(uint32_t)); //Updating param size in wdsp frameworkheader
         pOutput->nBytesUsedInBuffer += headerSize;
         if (ACDB_SUCCESS != result)
         {
            return result;
         }
      }
      break;
      case TABLE_SIZE_CMD:
      {
         AcdbSizeResponseType *pOutput = (AcdbSizeResponseType *)pOut;
         if (ACDB_SUCCESS != GetMidPidCalibTableSizeWithIid(tblId, cdefTbl, cdotTbl, tblInfo.dataPoolChnk, &pOutput->nSize))
         {
            return ACDB_ERROR;
         }

		 pOutput->nSize += (uint32_t)(ACDB_WDSP_FWK_HEADER_SIZE);
      }
      break;
      default:
         return  ACDB_ERROR;
      }

      result = ACDB_SUCCESS;
   }

   return result;
}

int32_t AcdbCmdGetWdspBootTimeModulesList(uint32_t queryType, uint8_t *pIn, uint8_t *pOut)
{
   int32_t result = ACDB_SUCCESS;

   if (pOut == NULL || (queryType == TABLE_SIZE_CMD && pIn != NULL))
   {
      ACDB_DEBUG_LOG("[ACDB Command]->[WdspCmdGetBootTimeModules]->Invalid NULL value parameters are provided\n");
      result = ACDB_BADPARM;
   }
   else
   {
      uint32_t index = 0;
      uint32_t tblId = WDSP_DYN_MOD_INFO_TBL_ID;
      AcdbTableCmd cmd = { 0, 0 };
      AcdbTableInfo tblInfo = { { 0, NULL }, { 0, NULL }, { 0, NULL } };
	  WdspDynamicModuleInfoLookupTblType lutTbl = { 0, NULL };
      ContentDefWithOnlyParamTblType cdefTbl = { 0, NULL };
	  WdspDynamicModuleInfoCmdLookupType bootModulescmd = { 0 };
	  WdspDynamicModuleInfoLookupType tempLut = { 0, 0, 0 };

      bootModulescmd.nModuleType = 0; //For boot time modules module type should be 0.

      cmd.tblId = tblId;
      result = acdbdata_ioctl(ACDBDATACMD_GET_TABLE_INFO, (uint8_t *)&cmd, sizeof(cmd),
         (uint8_t *)&tblInfo, sizeof(tblInfo));
      if (result != ACDB_SUCCESS)
      {
         ACDB_DEBUG_LOG("Failed to fetch the lookup information of the table %08X \n"
            , tblId);
         return result;
      }
      lutTbl.nLen = READ_UInt32(tblInfo.tblLutChnk.pData);
      lutTbl.pLut = (WdspDynamicModuleInfoLookupType *)(tblInfo.tblLutChnk.pData + sizeof(lutTbl.nLen));

      result = AcdbDataBinarySearch((void *)lutTbl.pLut, lutTbl.nLen,
         WDSP_DYNAMIC_MODULE_INFO_LUT_INDICES_COUNT, &bootModulescmd, WDSP_DYNAMIC_MODULE_INFO_CMD_INDICES_COUNT, &index);
      if (result != SEARCH_SUCCESS)
      {
         ACDB_DEBUG_LOG("Failed to fetch the lookup information of the dynamic moduletype \n");
         return ACDB_INPUT_PARAMS_NOT_FOUND;
      }

      // Now get CDEF info
      ACDB_MEM_CPY(&tempLut, sizeof(tempLut), &lutTbl.pLut[index], sizeof(tempLut));
      ACDB_MEM_CPY(&cdefTbl.nLen, sizeof(uint32_t), (tblInfo.tblCdftChnk.pData + tempLut.nCDEFTblOffset), sizeof(uint32_t));
      cdefTbl.pCntDef = (ContentDefIdType *)(tblInfo.tblCdftChnk.pData + tempLut.nCDEFTblOffset + sizeof(cdefTbl.nLen));

      switch (queryType)
      {
      case TABLE_CMD:
      {
         AcdbWdspBootTimeModuleReqType *pInput = (AcdbWdspBootTimeModuleReqType *)pIn;
         AcdbQueryResponseType *pOutput = (AcdbQueryResponseType *)pOut;
         ACDB_MEM_CPY(pInput->nBufferPointer, pInput->nBufferLength, &cdefTbl.nLen, sizeof(uint32_t));
         ACDB_MEM_CPY(pInput->nBufferPointer + sizeof(uint32_t), pInput->nBufferLength, (uint8_t *)cdefTbl.pCntDef, cdefTbl.nLen* sizeof(uint32_t));
         pOutput->nBytesUsedInBuffer = (uint32_t)(sizeof(uint32_t) + (cdefTbl.nLen * sizeof(uint32_t)));
         if (ACDB_SUCCESS != result)
         {
            return result;
         }
      }
      break;
      case TABLE_SIZE_CMD:
      {
         AcdbSizeResponseType *pOutput = (AcdbSizeResponseType *)pOut;
         pOutput->nSize = (uint32_t)(sizeof(uint32_t) + (cdefTbl.nLen * sizeof(uint32_t))); //cdef chunk contains list of modules.
      }
      break;
      default:
         return  ACDB_ERROR;
      }

      result = ACDB_SUCCESS;
   }

   return result;
}
uint32_t AcdbCmdWdspIsModulePresent(uint32_t queryType, uint8_t *pIn, uint32_t moduleID)
{
   uint32_t isFound = 0, i = 0, offset = 0;
   uint32_t numModules = 0;
   uint8_t *inputBuffer = NULL;
   switch (queryType)
   {
   case TABLE_CMD:
   {
      AcdbWdspGetDynamicModuleReqType *pInput = (AcdbWdspGetDynamicModuleReqType *)pIn;
      numModules = pInput->nNumOfModules;
      inputBuffer = pInput->pBuff;
   }
   break;
   case TABLE_SIZE_CMD:
   {
      WdspGetDynamicModuleInfoSizeCmdType *pInput = (WdspGetDynamicModuleInfoSizeCmdType *)pIn;
      numModules = pInput->nNumOfModules;
      inputBuffer = pInput->pBuff;
   }
   break;
   default:
      return isFound;
   }
   for (i = 0; i < numModules; i++)
   {
      uint32_t mID = READ_UInt32(inputBuffer + offset);
      offset += (uint32_t)sizeof(uint32_t);
      if (mID == moduleID)
      {
         isFound = 1;
         break;
      }
   }
   return isFound;
}

int32_t AcdbCmdGetWdspDynamicModuleMetaInfo(uint32_t queryType, uint8_t *pIn, uint8_t *pOut)
{
   int32_t result = ACDB_SUCCESS;

   if (pIn == NULL || pOut == NULL)
   {
      ACDB_DEBUG_LOG("[WDSP Command]->[ACDB_WDSP_CMD_GET_MODULE_LOADING_META_INFO]->Invalid NULL value parameters are provided\n");
      result = ACDB_BADPARM;
   }
   else
   {
      uint32_t index = 0, j = 0, moduleCount = 0;
      uint32_t tblId = WDSP_DYN_MOD_INFO_TBL_ID;
      AcdbTableCmd cmd = { 0, 0 };
      AcdbTableInfo tblInfo = { { 0, NULL }, { 0, NULL }, { 0, NULL } };
      WdspDynamicModuleInfoLookupTblType lutTbl = { 0, NULL };
      ContentDefWithOnlyParamTblType cdefTbl = { 0, NULL };
      ContentDataOffsetsTblType cdotTbl = { 0, NULL };
	  WdspDynamicModuleInfoCmdLookupType moduleInfocmd = { 0 };
	  WdspDynamicModuleInfoLookupType tempLut = { 0, 0, 0 };

      switch (queryType)
      {
      case TABLE_CMD:
      {
         AcdbWdspGetDynamicModuleReqType *pInput = (AcdbWdspGetDynamicModuleReqType *)pIn;
         moduleInfocmd.nModuleType = pInput->nModuleType;
      }
      break;
      case TABLE_SIZE_CMD:
      {
         WdspGetDynamicModuleInfoSizeCmdType *pInput = (WdspGetDynamicModuleInfoSizeCmdType *)pIn;
         moduleInfocmd.nModuleType = pInput->nModuleType;
      }
      break;
      default:
         return  ACDB_ERROR;
      }

      cmd.tblId = tblId;
      result = acdbdata_ioctl(ACDBDATACMD_GET_TABLE_INFO, (uint8_t *)&cmd, sizeof(cmd),
         (uint8_t *)&tblInfo, sizeof(tblInfo));
      if (result != ACDB_SUCCESS)
      {
         ACDB_DEBUG_LOG("Failed to fetch the lookup information of the table %08X \n"
            , tblId);
         return result;
      }
      lutTbl.nLen = READ_UInt32(tblInfo.tblLutChnk.pData);
      lutTbl.pLut = (WdspDynamicModuleInfoLookupType *)(tblInfo.tblLutChnk.pData + sizeof(lutTbl.nLen));

      result = AcdbDataBinarySearch((void *)lutTbl.pLut, lutTbl.nLen,
         WDSP_DYNAMIC_MODULE_INFO_LUT_INDICES_COUNT, &moduleInfocmd, WDSP_DYNAMIC_MODULE_INFO_CMD_INDICES_COUNT, &index);
      if (result != SEARCH_SUCCESS)
      {
         ACDB_DEBUG_LOG("Failed to fetch the lookup information of the module type %08X \n"
            , moduleInfocmd.nModuleType);
         return ACDB_INPUT_PARAMS_NOT_FOUND;
      }

      // Now get CDEF info
      ACDB_MEM_CPY(&tempLut, sizeof(tempLut), &lutTbl.pLut[index], sizeof(tempLut));
      ACDB_MEM_CPY(&cdefTbl.nLen, sizeof(uint32_t), (tblInfo.tblCdftChnk.pData + tempLut.nCDEFTblOffset), sizeof(uint32_t));
      cdefTbl.pCntDef = (ContentDefIdType *)(tblInfo.tblCdftChnk.pData + tempLut.nCDEFTblOffset + sizeof(cdefTbl.nLen));

      // Now get CDOT info
      ACDB_MEM_CPY(&cdotTbl.nLen, sizeof(uint32_t), tblInfo.tblCdotChnk.pData + tempLut.nCDOTTblOffset, sizeof(uint32_t));
      cdotTbl.pDataOffsets = (uint32_t *)(tblInfo.tblCdotChnk.pData + tempLut.nCDOTTblOffset + sizeof(cdotTbl.nLen));

      if (cdefTbl.nLen != cdotTbl.nLen)
      {
         ACDB_DEBUG_LOG("The no of entries in CDEF and CDOT tables for module type %08X not matching\n", moduleInfocmd.nModuleType);
         return ACDB_ERROR;
      }

      for (j = 0; j < cdefTbl.nLen; j++)
      {
         uint32_t moduleID = cdefTbl.pCntDef[j].nID;
         if (AcdbCmdWdspIsModulePresent(queryType, pIn, moduleID))
         {
            moduleCount++;
         }
      }

      switch (queryType)
      {
      case TABLE_CMD:
      {
         AcdbWdspGetDynamicModuleReqType *pInput = (AcdbWdspGetDynamicModuleReqType *)pIn;
         AcdbQueryResponseType *pOutput = (AcdbQueryResponseType *)pOut;
         uint32_t offset = 0;
         uint32_t size = 0, i = 0;
         AcdbDataType cData = { 0, NULL };
         uint32_t nMemBytesLeft = pInput->nBufferLength;

         ACDB_MEM_CPY(pInput->nBufferPointer + offset, nMemBytesLeft, &moduleCount, sizeof(uint32_t));
         offset += (uint32_t)sizeof(uint32_t);
         nMemBytesLeft -= (uint32_t)sizeof(uint32_t);
         size += (uint32_t)sizeof(uint32_t);

         for (i = 0; i < cdefTbl.nLen; i++)
         {
            uint32_t moduleID = cdefTbl.pCntDef[i].nID;
            uint32_t nPaddedBytes = 0;
            cData.nLen = READ_UInt32(tblInfo.dataPoolChnk.pData + cdotTbl.pDataOffsets[i]);
            cData.pData = tblInfo.dataPoolChnk.pData + cdotTbl.pDataOffsets[i] + sizeof(cData.nLen);
            if (cData.nLen % 4)
            {
               nPaddedBytes = 4 - cData.nLen % 4;
            }
            if (AcdbCmdWdspIsModulePresent(queryType, pIn, moduleID))
            {
               ACDB_MEM_CPY(pInput->nBufferPointer + offset, nMemBytesLeft, &moduleID, sizeof(uint32_t));
               size += (uint32_t)sizeof(uint32_t);
               offset += (uint32_t)sizeof(uint32_t);
               nMemBytesLeft -= (uint32_t)sizeof(uint32_t);
               ACDB_MEM_CPY((void *)(pInput->nBufferPointer + offset), nMemBytesLeft, (const void *)cData.pData, cData.nLen);
               size += cData.nLen + nPaddedBytes;
               offset += cData.nLen + nPaddedBytes;
               nMemBytesLeft -= (cData.nLen + nPaddedBytes);
            }
         }
         pOutput->nBytesUsedInBuffer = size;
      }
      break;
      case TABLE_SIZE_CMD:
      {
         AcdbSizeResponseType *pOutput = (AcdbSizeResponseType *)pOut;
         uint32_t size = 0, i = 0;
         AcdbDataType cData = { 0, NULL };

         size += (uint32_t)sizeof(uint32_t); //module count

         for (i = 0; i < cdefTbl.nLen; i++)
         {
            uint32_t moduleID = cdefTbl.pCntDef[i].nID;
            uint32_t nPaddedBytes = 0;
            cData.nLen = READ_UInt32(tblInfo.dataPoolChnk.pData + cdotTbl.pDataOffsets[i]);
            cData.pData = tblInfo.dataPoolChnk.pData + cdotTbl.pDataOffsets[i] + sizeof(cData.nLen);
            if (cData.nLen % 4)
            {
               nPaddedBytes = 4 - cData.nLen % 4;
            }
            if (AcdbCmdWdspIsModulePresent(queryType, pIn, moduleID))
            {
               size += cData.nLen + (uint32_t)sizeof(uint32_t) + nPaddedBytes;
            }
         }
         pOutput->nSize = size;
      }
      break;
      default:
         return  ACDB_ERROR;
      }

      result = ACDB_SUCCESS;
   }

   return result;
}
int32_t AcdbCmdGetAdspInstanceSupportInfo(AcdbQueryCmdType *pInput, AcdbQueryResponseType *pOutput)
{
   int32_t result = ACDB_SUCCESS;

   if (pInput == NULL || pOutput == NULL)
   {
      ACDB_DEBUG_LOG("ACDB_COMMAND: Provided invalid param\n");
      result = ACDB_BADPARM;
   }
   else if (pInput->pBufferPointer == NULL)
   {
      ACDB_DEBUG_LOG("ACDB_COMMAND: Provided invalid param\n");
      result = ACDB_BADPARM;
   }
   else
   {
      AcdbGlbalPropInfo glbPropInfo = { 0, { 0, NULL } };
      memset(&glbPropInfo, 0, sizeof(glbPropInfo));
      glbPropInfo.pId = ADSP_INSTANCE_ID_INFO;
      result = acdbdata_ioctl(ACDBDATACMD_GET_GLOBAL_PROP, (uint8_t *)&glbPropInfo, sizeof(AcdbGlbalPropInfo),
         (uint8_t *)NULL, 0);
      if (result != ACDB_SUCCESS)
      {
         ACDB_DEBUG_LOG("ACDB_COMMAND: Failed to fetch the property info for pid %08X \n", glbPropInfo.pId);
         return ACDB_DATA_NOT_FOUND;
      }
      if (NULL == glbPropInfo.dataInfo.pData)
      {
         ACDB_DEBUG_LOG("ACDB_COMMAND: pData NULL on fetch the property info for pid %08X \n", glbPropInfo.pId);
         return ACDB_DATA_NOT_FOUND;
      }

      if (pInput->nBufferLength < glbPropInfo.dataInfo.nDataLen)
      {
         ACDB_DEBUG_LOG("ACDB_COMMAND: Insufficient memory provided to fill the audio copp topology info for pid %08X \n", glbPropInfo.pId);
         return ACDB_INSUFFICIENTMEMORY;
      }

      ACDB_MEM_CPY(pInput->pBufferPointer, pInput->nBufferLength, glbPropInfo.dataInfo.pData, glbPropInfo.dataInfo.nDataLen);
      pOutput->nBytesUsedInBuffer = glbPropInfo.dataInfo.nDataLen;
   }
   return result;
}

int32_t AcdbCmdGetAudProcIntanceInfo(uint32_t queryType, uint8_t *pIn, uint8_t *pOut)
{
   int32_t result = ACDB_SUCCESS;

   if (pIn == NULL || pOut == NULL)
   {
      ACDB_DEBUG_LOG("[ACDB Command]->[AcdbCmdGetAudProcIntanceInfo]->Invalid NULL value parameters are provided\n");
      result = ACDB_BADPARM;
   }
   else
   {
      uint32_t index = 0;
      uint32_t tblId = AUDPROC_INST_TBL_ID;
      AcdbTableCmd cmd = { 0, 0 };
      AcdbTableInfo tblInfo = { { 0, NULL }, { 0, NULL }, { 0, NULL } };
      AudProcCmnDataLookupTblType lutTbl = { 0, NULL};
      ContentDefWithInstanceTblType cdefTbl = { 0, NULL };
      ContentDataOffsetsTblType cdotTbl = { 0, NULL };
      uintptr_t nLookupKey = 0;
      AudProcCmnCmdLookupType audcmd = { 0, 0, 0 };
      AudProcCmnDataLookupType tempLut = { 0, 0, 0, 0, 0 };

      switch (queryType)
      {
      case TABLE_CMD:
      {
         AcdbAudProcTableCmdType *pInput = (AcdbAudProcTableCmdType *)pIn;
         if (pInput->nBufferPointer == NULL)
         {
            return ACDB_BADPARM;
         }
         audcmd.nDeviceId = pInput->nDeviceId;
         audcmd.nDeviceSampleRateId = pInput->nDeviceSampleRateId;
         audcmd.nApplicationType = pInput->nApplicationType;
      }
      break;
      case TABLE_SIZE_CMD:
      {
         AcdbAudProcTableSizeCmdType *pInput = (AcdbAudProcTableSizeCmdType *)pIn;
         audcmd.nDeviceId = pInput->nDeviceId;
         audcmd.nDeviceSampleRateId = pInput->nDeviceSampleRateId;
         audcmd.nApplicationType = pInput->nApplicationType;
      }
      break;
      case DATA_CMD:
      {
         AcdbAudProcInstanceCmdType *pInput = (AcdbAudProcInstanceCmdType *)pIn;
         if (pInput->nBufferPointer == NULL)
         {
            return ACDB_BADPARM;
         }
         audcmd.nDeviceId = pInput->nDeviceId;
         audcmd.nDeviceSampleRateId = pInput->nDeviceSampleRateId;
         audcmd.nApplicationType = pInput->nApplicationType;
      }
      break;
      default:
         return  ACDB_ERROR;
      }

      cmd.devId = audcmd.nDeviceId;
      cmd.tblId = tblId;
      result = acdbdata_ioctl(ACDBDATACMD_GET_TABLE_INFO, (uint8_t *)&cmd, sizeof(cmd),
         (uint8_t *)&tblInfo, sizeof(tblInfo));
      if (result != ACDB_SUCCESS)
      {
         ACDB_DEBUG_LOG("Failed to fetch the lookup information of the device %08X \n"
            , audcmd.nDeviceId);
         return result;
      }
      lutTbl.nLen = READ_UInt32(tblInfo.tblLutChnk.pData);
      lutTbl.pLut = (AudProcCmnDataLookupType *)(tblInfo.tblLutChnk.pData + sizeof(lutTbl.nLen));

      result = AcdbDataBinarySearch((void *)lutTbl.pLut, lutTbl.nLen,
         AUDPROC_LUT_INDICES_COUNT, &audcmd, AUDPROC_CMD_INDICES_COUNT, &index);
      if (result != SEARCH_SUCCESS)
      {
         ACDB_DEBUG_LOG("Failed to fetch the lookup information of the device %08X \n"
            , audcmd.nDeviceId);
         return ACDB_INPUT_PARAMS_NOT_FOUND;
      }

      nLookupKey = (uintptr_t)&lutTbl.pLut[index];
      // Now get CDEF info
      ACDB_MEM_CPY(&tempLut, sizeof(tempLut), &lutTbl.pLut[index], sizeof(tempLut));
      ACDB_MEM_CPY(&cdefTbl.nLen, sizeof(uint32_t), (tblInfo.tblCdftChnk.pData + tempLut.nCDEFTblOffset), sizeof(uint32_t));
      cdefTbl.pCntDef = (ContentDefInstanceType *)(tblInfo.tblCdftChnk.pData + tempLut.nCDEFTblOffset + sizeof(cdefTbl.nLen));

      // Now get CDOT info
      ACDB_MEM_CPY(&cdotTbl.nLen, sizeof(uint32_t), tblInfo.tblCdotChnk.pData + tempLut.nCDOTTblOffset, sizeof(uint32_t));
      cdotTbl.pDataOffsets = (uint32_t *)(tblInfo.tblCdotChnk.pData + tempLut.nCDOTTblOffset + sizeof(cdotTbl.nLen));

      if (cdefTbl.nLen != cdotTbl.nLen)
      {
         ACDB_DEBUG_LOG("The no of entries in CDEF and CDOT tables for devid %08X not matching\n", audcmd.nDeviceId);
         return ACDB_ERROR;
      }

      switch (queryType)
      {
      case TABLE_CMD:
      {
         AcdbAudProcTableCmdType *pInput = (AcdbAudProcTableCmdType *)pIn;
         AcdbQueryResponseType *pOutput = (AcdbQueryResponseType *)pOut;
         result = GetMidIidPidCalibTable(tblId, nLookupKey, cdefTbl, cdotTbl, tblInfo.dataPoolChnk,
            pInput->nBufferPointer, pInput->nBufferLength, &pOutput->nBytesUsedInBuffer);
         if (ACDB_SUCCESS != result)
         {
            return result;
         }
      }
      break;
      case TABLE_SIZE_CMD:
      {
         AcdbSizeResponseType *pOutput = (AcdbSizeResponseType *)pOut;
         if (ACDB_SUCCESS != GetMidIidPidCalibTableSize(tblId, nLookupKey, cdefTbl, cdotTbl, tblInfo.dataPoolChnk, &pOutput->nSize))
         {
            return ACDB_ERROR;
         }
      }
      break;
      case DATA_CMD:
      {
         AcdbAudProcInstanceCmdType *pInput = (AcdbAudProcInstanceCmdType *)pIn;
         AcdbQueryResponseType *pOutput = (AcdbQueryResponseType *)pOut;
         const uint32_t lim = 0xFFFF;
         uint32_t iid = pInput->nInstanceId & lim;

         result = GetMidIidPidCalibData(tblId, nLookupKey, cdefTbl, cdotTbl, tblInfo.dataPoolChnk, pInput->nModuleId,
            iid, pInput->nParamId, pInput->nBufferPointer, pInput->nBufferLength, &pOutput->nBytesUsedInBuffer);
         if (ACDB_SUCCESS != result)
         {
            return result;
         }
      }
      break;
      default:
         return  ACDB_ERROR;
      }

      result = ACDB_SUCCESS;
   }

   return result;
}

int32_t AcdbCmdGetPersistentAudProcIntanceInfo(uint32_t queryType, uint8_t *pIn, uint8_t *pOut)
{
    int32_t result = ACDB_SUCCESS;

    if (pIn == NULL || pOut == NULL)
    {
        ACDB_DEBUG_LOG("[ACDB Command]->["
            "AcdbCmdGetPersistentAudProcIntanceInfo"
            "]->Invalid NULL value parameters are provided\n");
        result = ACDB_BADPARM;
    }
    else
    {
        uint32_t index = 0;
        uint32_t tblId = AUDPROC_INST_TBL_ID;
        AcdbTableCmd cmd = { 0, 0 };
        AcdbTableInfo tblInfo = { { 0, NULL },{ 0, NULL },{ 0, NULL } };
        AudProcCmnDataLookupTblType lutTbl = { 0, NULL };
        ContentDefWithInstanceTblType cdefTbl = { 0, NULL };
        ContentDataOffsetsTblType cdotTbl = { 0, NULL };
        uintptr_t nLookupKey = 0;
        AudProcCmnCmdLookupType audcmd = { 0, 0, 0 };
        AudProcCmnDataLookupType tempLut = { 0, 0, 0, 0, 0 };

        switch (queryType)
        {
        case TABLE_CMD:
        {
            AcdbAudProcTableCmdType *pInput = (AcdbAudProcTableCmdType *)pIn;
            if (pInput->nBufferPointer == NULL)
            {
                return ACDB_BADPARM;
            }
            audcmd.nDeviceId = pInput->nDeviceId;
            audcmd.nDeviceSampleRateId = pInput->nDeviceSampleRateId;
            audcmd.nApplicationType = pInput->nApplicationType;
        }
        break;
        case TABLE_SIZE_CMD:
        {
            AcdbAudProcTableSizeCmdType *pInput = (AcdbAudProcTableSizeCmdType *)pIn;
            audcmd.nDeviceId = pInput->nDeviceId;
            audcmd.nDeviceSampleRateId = pInput->nDeviceSampleRateId;
            audcmd.nApplicationType = pInput->nApplicationType;
        }
        break;
        case DATA_CMD:
        {
            AcdbAudProcInstanceCmdType *pInput = (AcdbAudProcInstanceCmdType *)pIn;
            if (pInput->nBufferPointer == NULL)
            {
                return ACDB_BADPARM;
            }
            audcmd.nDeviceId = pInput->nDeviceId;
            audcmd.nDeviceSampleRateId = pInput->nDeviceSampleRateId;
            audcmd.nApplicationType = pInput->nApplicationType;
        }
        break;
        default:
            return  ACDB_ERROR;
        }

        cmd.devId = audcmd.nDeviceId;
        cmd.tblId = tblId;
        result = acdbdata_ioctl(ACDBDATACMD_GET_TABLE_INFO, (uint8_t *)&cmd, sizeof(cmd),
            (uint8_t *)&tblInfo, sizeof(tblInfo));
        if (result != ACDB_SUCCESS)
        {
            ACDB_DEBUG_LOG("Failed to fetch the lookup information of the device %08X \n"
                , audcmd.nDeviceId);
            return result;
        }
        lutTbl.nLen = READ_UInt32(tblInfo.tblLutChnk.pData);
        lutTbl.pLut = (AudProcCmnDataLookupType *)(tblInfo.tblLutChnk.pData + sizeof(lutTbl.nLen));

        result = AcdbDataBinarySearch((void *)lutTbl.pLut, lutTbl.nLen,
            AUDPROC_LUT_INDICES_COUNT, &audcmd, AUDPROC_CMD_INDICES_COUNT, &index);
        if (result != SEARCH_SUCCESS)
        {
            ACDB_DEBUG_LOG("Failed to fetch the lookup information of the device %08X \n"
                , audcmd.nDeviceId);
            return ACDB_INPUT_PARAMS_NOT_FOUND;
        }

        nLookupKey = (uintptr_t)&lutTbl.pLut[index];
        // Now get CDEF info
        ACDB_MEM_CPY(&tempLut, sizeof(tempLut), &lutTbl.pLut[index], sizeof(tempLut));
        ACDB_MEM_CPY(&cdefTbl.nLen, sizeof(uint32_t), (tblInfo.tblCdftChnk.pData + tempLut.nCDEFTblOffset), sizeof(uint32_t));
        cdefTbl.pCntDef = (ContentDefInstanceType *)(tblInfo.tblCdftChnk.pData + tempLut.nCDEFTblOffset + sizeof(cdefTbl.nLen));

        // Now get CDOT info
        ACDB_MEM_CPY(&cdotTbl.nLen, sizeof(uint32_t), tblInfo.tblCdotChnk.pData + tempLut.nCDOTTblOffset, sizeof(uint32_t));
        cdotTbl.pDataOffsets = (uint32_t *)(tblInfo.tblCdotChnk.pData + tempLut.nCDOTTblOffset + sizeof(cdotTbl.nLen));

        if (cdefTbl.nLen != cdotTbl.nLen)
        {
            ACDB_DEBUG_LOG("The no of entries in CDEF and CDOT tables for devid %08X not matching\n", audcmd.nDeviceId);
            return ACDB_ERROR;
        }

        switch (queryType)
        {
        case TABLE_CMD:
        {
            AcdbAudProcTableCmdType *pInput = (AcdbAudProcTableCmdType *)pIn;
            AcdbQueryResponseType *pOutput = (AcdbQueryResponseType *)pOut;
            result = GetPersistentMidIidPidCalibTable(
                tblId, nLookupKey, cdefTbl, cdotTbl, tblInfo.dataPoolChnk,
                pInput->nBufferPointer, pInput->nBufferLength,
                &pOutput->nBytesUsedInBuffer);
            if (ACDB_SUCCESS != result)
            {
                return result;
            }
        }
        break;
        case TABLE_SIZE_CMD:
        {
            AcdbSizeResponseType *pOutput = (AcdbSizeResponseType *)pOut;
            result = GetPersistentMidIidPidCalibTableSize(
                tblId, nLookupKey, cdefTbl, cdotTbl,
                tblInfo.dataPoolChnk, &pOutput->nSize);
            if (ACDB_SUCCESS != result)
            {
                return result;
            }
        }
        break;
        case DATA_CMD:
        {
            AcdbAudProcInstanceCmdType *pInput = (AcdbAudProcInstanceCmdType *)pIn;
            AcdbQueryResponseType *pOutput = (AcdbQueryResponseType *)pOut;
            const uint32_t lim = 0xFFFF;
            uint32_t iid = pInput->nInstanceId & lim;

            result = GetPersistentMidIidPidCalibData(
                tblId, nLookupKey, cdefTbl, cdotTbl, tblInfo.dataPoolChnk,
                pInput->nModuleId, iid, pInput->nParamId,
                pInput->nBufferPointer, pInput->nBufferLength,
                &pOutput->nBytesUsedInBuffer);
            if (ACDB_SUCCESS != result)
            {
                return result;
            }
        }
        break;
        default:
            return  ACDB_ERROR;
        }

        result = ACDB_SUCCESS;
    }

    return result;
}

int32_t AcdbCmdGetAudProcInstanceGainDepStepInfo(uint32_t queryType, uint8_t *pIn, uint8_t *pOut)
{
   int32_t result = ACDB_SUCCESS;

   if (pIn == NULL || pOut == NULL)
   {
      ACDB_DEBUG_LOG("[ACDB Command]->[AcdbCmdGetAudProcInstanceGainDepStepInfo]->Invalid Null input provided\n");
      result = ACDB_BADPARM;
   }
   else
   {
      uint32_t index = 0;
      AcdbTableCmd cmd = { 0, 0 };
      AcdbTableInfo tblInfo = { { 0, NULL }, { 0, NULL }, { 0, NULL } };
      AudProcGainDepDataLookupTblType lutTbl = { 0, NULL };
      ContentDefWithInstanceTblType cdefTbl = { 0, NULL };
      ContentDataOffsetsTblType cdotTbl = { 0, NULL };
      uintptr_t nLookupKey = 0;
      AudProcGainDepCmdLookupType audcmd = { 0, 0, 0 };
      uint32_t tblId = AUDPROCVOL_INST_TBL_ID;
      AudProcGainDepDataLookupType tempLut = { 0, 0, 0, 0, 0};

      switch (queryType)
      {
      case TABLE_CMD:
      {
         AcdbAudProcGainDepVolTblStepCmdType *pInput = (AcdbAudProcGainDepVolTblStepCmdType *)pIn;
         if (pInput->nBufferPointer == NULL)
         {
            return ACDB_BADPARM;
         }
         audcmd.nDeviceId = pInput->nDeviceId;
         audcmd.nApplicationType = pInput->nApplicationType;
         audcmd.nVolIdx = pInput->nVolumeIndex;
      }
      break;
      case TABLE_SIZE_CMD:
      {
         AcdbAudProcGainDepVolTblStepSizeCmdType *pInput = (AcdbAudProcGainDepVolTblStepSizeCmdType *)pIn;
         audcmd.nDeviceId = pInput->nDeviceId;
         audcmd.nApplicationType = pInput->nApplicationType;
         audcmd.nVolIdx = pInput->nVolumeIndex;
      }
      break;
      default:
         return  ACDB_ERROR;
      }

      cmd.devId = audcmd.nDeviceId;
      cmd.tblId = tblId;
      result = acdbdata_ioctl(ACDBDATACMD_GET_TABLE_INFO, (uint8_t *)&cmd, sizeof(cmd),
         (uint8_t *)&tblInfo, sizeof(tblInfo));
      if (result != ACDB_SUCCESS)
      {
         ACDB_DEBUG_LOG("Failed to fetch the lookup information of the device %08X \n"
            , audcmd.nDeviceId);
         return result;
      }

      lutTbl.nLen = READ_UInt32(tblInfo.tblLutChnk.pData);
      lutTbl.pLut = (AudProcGainDepDataLookupType *)(tblInfo.tblLutChnk.pData + sizeof(lutTbl.nLen));

      result = AcdbDataBinarySearch((void *)lutTbl.pLut, lutTbl.nLen,
         AUDPROC_GAIN_DEP_LUT_INDICES_COUNT, &audcmd, AUDPROC_GAIN_DEP_CMD_INDICES_COUNT, &index);
      if (result != SEARCH_SUCCESS)
      {
         ACDB_DEBUG_LOG("Failed to fetch the lookup information of the device %08X \n"
            , audcmd.nDeviceId);
         return ACDB_INPUT_PARAMS_NOT_FOUND;
      }

      // Now get CDEF info
      nLookupKey = (uintptr_t)&lutTbl.pLut[index];
      ACDB_MEM_CPY(&tempLut, sizeof(tempLut), &lutTbl.pLut[index], sizeof(tempLut));
      ACDB_MEM_CPY(&cdefTbl.nLen, sizeof(uint32_t), (tblInfo.tblCdftChnk.pData + tempLut.nCDEFTblOffset), sizeof(uint32_t));
      cdefTbl.pCntDef = (ContentDefInstanceType *)(tblInfo.tblCdftChnk.pData + tempLut.nCDEFTblOffset + sizeof(cdefTbl.nLen));

      // Now get CDOT info
      ACDB_MEM_CPY(&cdotTbl.nLen, sizeof(uint32_t), (tblInfo.tblCdotChnk.pData + tempLut.nCDOTTblOffset), sizeof(uint32_t));

      cdotTbl.pDataOffsets = (uint32_t *)(tblInfo.tblCdotChnk.pData + tempLut.nCDOTTblOffset + sizeof(cdotTbl.nLen));

      if (cdefTbl.nLen != cdotTbl.nLen)
      {
         ACDB_DEBUG_LOG("The no of entries in CDEF and CDOT tables for devid %08X not matching \n", audcmd.nDeviceId);
         return ACDB_ERROR;
      }

      switch (queryType)
      {
      case TABLE_CMD:
      {
         AcdbAudProcGainDepVolTblStepCmdType *pInput = (AcdbAudProcGainDepVolTblStepCmdType *)pIn;
         AcdbQueryResponseType *pOutput = (AcdbQueryResponseType *)pOut;
         result = GetMidIidPidCalibTable(tblId, nLookupKey, cdefTbl, cdotTbl, tblInfo.dataPoolChnk,
            pInput->nBufferPointer, pInput->nBufferLength, &pOutput->nBytesUsedInBuffer);
         if (ACDB_SUCCESS != result)
         {
            return result;
         }
      }
      break;
      case TABLE_SIZE_CMD:
      {
         AcdbSizeResponseType *pOutput = (AcdbSizeResponseType *)pOut;
         if (ACDB_SUCCESS != GetMidIidPidCalibTableSize(tblId, nLookupKey, cdefTbl, cdotTbl, tblInfo.dataPoolChnk, &pOutput->nSize))
         {
            return ACDB_ERROR;
         }
      }
      break;
      default:
         return  ACDB_ERROR;
      }

      result = ACDB_SUCCESS;
   }

   return result;
}

int32_t AcdbCmdGetAudStreamInstanceInfo(uint32_t queryType, uint8_t *pIn, uint8_t *pOut)
{
   int32_t result = ACDB_SUCCESS;

   if (pIn == NULL || pOut == NULL)
   {
      ACDB_DEBUG_LOG("[ACDB Command]->[AcdbCmdGetAudStreamTableSize]->System Erorr\n");
      result = ACDB_BADPARM;
   }
   else
   {
      uint32_t index = 0;
      AcdbTableCmd cmd = { 0, 0 };
      AcdbTableInfo tblInfo = { { 0, NULL }, { 0, NULL }, { 0, NULL } };
      AudStreamDataLookupTblType lutTbl = { 0, NULL };
      ContentDefWithInstanceTblType cdefTbl = { 0, NULL };
      ContentDataOffsetsTblType cdotTbl = { 0, NULL };
      uintptr_t nLookupKey = 0;
      AudStreamCmdLookupType audstrmcmd = { 0 };
      uint32_t tblId = AUDSTRM_INST_TBL_ID;
      AudStreamDataLookupType tempLut = { 0, 0, 0 };

      memset(&cmd, 0, sizeof(AcdbTableCmd));
      memset(&tblInfo, 0, sizeof(AcdbTableInfo));
      memset(&lutTbl, 0, sizeof(AudStreamDataLookupTblType));
      memset(&cdefTbl, 0, sizeof(ContentDefWithInstanceTblType));
      memset(&cdotTbl, 0, sizeof(ContentDataOffsetsTblType));
      memset(&audstrmcmd, 0, sizeof(AudStreamCmdLookupType));

      switch (queryType)
      {
      case TABLE_CMD:
      {
         AcdbAudStrmTableV2CmdType *pInput = (AcdbAudStrmTableV2CmdType *)pIn;
         if (pInput->nBufferPointer == NULL)
         {
            return ACDB_BADPARM;
         }
         audstrmcmd.nApplicationType = pInput->nApplicationTypeId;
      }
      break;
      case TABLE_SIZE_CMD:
      {
         AcdbAudStrmTblSizeCmdType *pInput = (AcdbAudStrmTblSizeCmdType *)pIn;
         audstrmcmd.nApplicationType = pInput->nApplicationTypeId;
      }
      break;
      case DATA_CMD:
      {
         AcdbAudStrmInstanceCmdType *pInput = (AcdbAudStrmInstanceCmdType *)pIn;
         if (pInput->nBufferPointer == NULL)
         {
            return ACDB_BADPARM;
         }
         audstrmcmd.nApplicationType = pInput->nApplicationTypeId;
      }
      break;
      default:
         return  ACDB_ERROR;
      }

      cmd.devId = 0;
      cmd.tblId = tblId;

      result = acdbdata_ioctl(ACDBDATACMD_GET_TABLE_INFO, (uint8_t *)&cmd, sizeof(cmd),
         (uint8_t *)&tblInfo, sizeof(tblInfo));
      if (result != ACDB_SUCCESS)
      {
         ACDB_DEBUG_LOG("Failed to fetch the lookup information of the appid %08X \n"
            , audstrmcmd.nApplicationType);
         return result;
      }

      lutTbl.nLen = READ_UInt32(tblInfo.tblLutChnk.pData);
      lutTbl.pLut = (AudStreamDataLookupType *)(tblInfo.tblLutChnk.pData + sizeof(lutTbl.nLen));
      result = AcdbDataBinarySearch((void *)lutTbl.pLut, lutTbl.nLen,
         AUDSTREAM_LUT_INDICES_COUNT, &audstrmcmd, AUDSTREAM_CMD_INDICES_COUNT, &index);
      if (result != SEARCH_SUCCESS)
      {
         ACDB_DEBUG_LOG("Failed to fetch the lookup information of the appid %08X  \n"
            , audstrmcmd.nApplicationType);
         return ACDB_INPUT_PARAMS_NOT_FOUND;
      }

      nLookupKey = (uintptr_t)&lutTbl.pLut[index];
      // Now get CDEF info
      ACDB_MEM_CPY(&tempLut, sizeof(tempLut), &lutTbl.pLut[index], sizeof(tempLut));
      ACDB_MEM_CPY(&cdefTbl.nLen, sizeof(uint32_t), (tblInfo.tblCdftChnk.pData + tempLut.nCDEFTblOffset), sizeof(uint32_t));
      cdefTbl.pCntDef = (ContentDefInstanceType *)(tblInfo.tblCdftChnk.pData + tempLut.nCDEFTblOffset + sizeof(cdefTbl.nLen));

      // Now get CDOT info
      ACDB_MEM_CPY(&cdotTbl.nLen, sizeof(uint32_t), (tblInfo.tblCdotChnk.pData + tempLut.nCDOTTblOffset), sizeof(uint32_t));
      cdotTbl.pDataOffsets = (uint32_t *)(tblInfo.tblCdotChnk.pData + tempLut.nCDOTTblOffset + sizeof(cdotTbl.nLen));

      if (cdefTbl.nLen != cdotTbl.nLen)
      {
         ACDB_DEBUG_LOG("The no of entries in CDEF and CDOT tables for appid %08X \n", audstrmcmd.nApplicationType);
         return ACDB_ERROR;
      }

      switch (queryType)
      {
      case TABLE_CMD:
      {
         AcdbAudStrmTableV2CmdType *pInput = (AcdbAudStrmTableV2CmdType *)pIn;
         AcdbQueryResponseType *pOutput = (AcdbQueryResponseType *)pOut;
         result = GetMidIidPidCalibTable(tblId, nLookupKey, cdefTbl, cdotTbl, tblInfo.dataPoolChnk,
            pInput->nBufferPointer, pInput->nBufferLength, &pOutput->nBytesUsedInBuffer);
         if (ACDB_SUCCESS != result)
         {
            return result;
         }
      }
      break;
      case TABLE_SIZE_CMD:
      {
         AcdbSizeResponseType *pOutput = (AcdbSizeResponseType *)pOut;
         if (ACDB_SUCCESS != GetMidIidPidCalibTableSize(tblId, nLookupKey, cdefTbl, cdotTbl, tblInfo.dataPoolChnk, &pOutput->nSize))
         {
            return ACDB_ERROR;
         }
      }
      break;
      case DATA_CMD:
      {
         AcdbAudStrmInstanceCmdType *pInput = (AcdbAudStrmInstanceCmdType *)pIn;
         AcdbQueryResponseType *pOutput = (AcdbQueryResponseType *)pOut;
         const uint32_t lim = 0xFFFF;
         uint32_t iid = pInput->nInstanceId & lim;

         result = GetMidIidPidCalibData(tblId, nLookupKey, cdefTbl, cdotTbl, tblInfo.dataPoolChnk, pInput->nModuleId,
            iid, pInput->nParamId, pInput->nBufferPointer, pInput->nBufferLength, &pOutput->nBytesUsedInBuffer);
         if (ACDB_SUCCESS != result)
         {
            return result;
         }
      }
      break;
      default:
         return  ACDB_ERROR;
      }
      result = ACDB_SUCCESS;
   }

   return result;
}

int32_t AcdbCmdGetAfeInstanceCmnInfo(uint32_t queryType, uint8_t *pIn, uint8_t *pOut)
{
   int32_t result = ACDB_SUCCESS;

   if (pIn == NULL || pOut == NULL)
   {
      ACDB_DEBUG_LOG("[ACDB Command]->[AcdbCmdGetAfeInstanceCmnInfo]->Invalid NULL value parameters are provided\n");
      result = ACDB_BADPARM;
   }
   else
   {
      uint32_t index = 0;
      AcdbTableCmd cmd = { 0, 0 };
      AcdbTableInfo tblInfo = { { 0, NULL }, { 0, NULL }, { 0, NULL } };
      AfeCmnDataLookupTblType lutTbl = { 0, NULL };
      ContentDefWithInstanceTblType cdefTbl = { 0, NULL };
      ContentDataOffsetsTblType cdotTbl = { 0, NULL };
      uintptr_t nLookupKey = 0;
	  AfeCmnCmdLookupType afecmncmd = { 0, 0 };
      uint32_t tblId = AFECMN_INST_TBL_ID;

	  AfeCmnDataLookupType tempLut = { 0, 0, 0, 0 };
      switch (queryType)
      {
      case TABLE_CMD:
      {
         AcdbAfeCommonTableCmdType *pInput = (AcdbAfeCommonTableCmdType *)pIn;
         if (pInput->nBufferPointer == NULL)
         {
            return ACDB_BADPARM;
         }
         afecmncmd.nDeviceId = pInput->nDeviceId;
         afecmncmd.nDeviceSampleRateId = pInput->nSampleRateId;
      }
      break;
      case TABLE_SIZE_CMD:
      {
         AcdbAfeCommonTableSizeCmdType *pInput = (AcdbAfeCommonTableSizeCmdType *)pIn;
         afecmncmd.nDeviceId = pInput->nDeviceId;
         afecmncmd.nDeviceSampleRateId = pInput->nSampleRateId;
      }
      break;
      case DATA_CMD:
      {
         AcdbAfeInstanceCmnDataCmdType *pInput = (AcdbAfeInstanceCmnDataCmdType *)pIn;
         if (pInput->nBufferPointer == NULL)
         {
            return ACDB_BADPARM;
         }
         afecmncmd.nDeviceId = pInput->nDeviceId;
         afecmncmd.nDeviceSampleRateId = pInput->nAfeSampleRateId;
      }
      break;
      default:
         return  ACDB_ERROR;
      }
      //acdb_translate_sample_rate(afecmncmd.nDeviceSampleRateId,&afecmncmd.nDeviceSampleRateId);

      cmd.devId = afecmncmd.nDeviceId;
      cmd.tblId = tblId;
      result = acdbdata_ioctl(ACDBDATACMD_GET_TABLE_INFO, (uint8_t *)&cmd, sizeof(cmd),
         (uint8_t *)&tblInfo, sizeof(tblInfo));

      if (result != ACDB_SUCCESS)
      {
         ACDB_DEBUG_LOG("Failed to fetch the lookup information of the device %08X \n"
            , afecmncmd.nDeviceId);
         return result;
      }

      lutTbl.nLen = READ_UInt32(tblInfo.tblLutChnk.pData);
      lutTbl.pLut = (AfeCmnDataLookupType *)(tblInfo.tblLutChnk.pData + sizeof(lutTbl.nLen));

      result = AcdbDataBinarySearch((void *)lutTbl.pLut, lutTbl.nLen,
         AFECMN_LUT_INDICES_COUNT, &afecmncmd, AFECMN_CMD_INDICES_COUNT, &index);
      if (result != SEARCH_SUCCESS)
      {
         ACDB_DEBUG_LOG("Failed to fetch the lookup information of the device %08X \n"
            , afecmncmd.nDeviceId);
         return ACDB_INPUT_PARAMS_NOT_FOUND;
      }

      nLookupKey = (uintptr_t)&lutTbl.pLut[index];
      // Now get CDEF info
      ACDB_MEM_CPY(&tempLut, sizeof(tempLut), &lutTbl.pLut[index], sizeof(tempLut));
      ACDB_MEM_CPY(&cdefTbl.nLen, sizeof(uint32_t), (tblInfo.tblCdftChnk.pData + tempLut.nCDEFTblOffset), sizeof(uint32_t));
      cdefTbl.pCntDef = (ContentDefInstanceType *)(tblInfo.tblCdftChnk.pData + tempLut.nCDEFTblOffset + sizeof(cdefTbl.nLen));

      // Now get CDOT info
      ACDB_MEM_CPY(&cdotTbl.nLen, sizeof(uint32_t), (tblInfo.tblCdotChnk.pData + tempLut.nCDOTTblOffset), sizeof(uint32_t));
      cdotTbl.pDataOffsets = (uint32_t *)(tblInfo.tblCdotChnk.pData + tempLut.nCDOTTblOffset + sizeof(cdotTbl.nLen));

      if (cdefTbl.nLen != cdotTbl.nLen)
      {
         ACDB_DEBUG_LOG("The no of entries in CDEF and CDOT tables for devid %08X not matching\n", afecmncmd.nDeviceId);
         return ACDB_ERROR;
      }

      switch (queryType)
      {
      case TABLE_CMD:
      {
         AcdbAfeCommonTableCmdType *pInput = (AcdbAfeCommonTableCmdType *)pIn;
         AcdbQueryResponseType *pOutput = (AcdbQueryResponseType *)pOut;
         result = GetMidIidPidCalibTable(tblId, nLookupKey, cdefTbl, cdotTbl, tblInfo.dataPoolChnk,
            pInput->nBufferPointer, pInput->nBufferLength, &pOutput->nBytesUsedInBuffer);
         if (ACDB_SUCCESS != result)
         {
            return result;
         }
      }
      break;
      case TABLE_SIZE_CMD:
      {
         AcdbSizeResponseType *pOutput = (AcdbSizeResponseType *)pOut;
         if (ACDB_SUCCESS != GetMidIidPidCalibTableSize(tblId, nLookupKey, cdefTbl, cdotTbl, tblInfo.dataPoolChnk, &pOutput->nSize))
         {
            return ACDB_ERROR;
         }
      }
      break;
      case DATA_CMD:
      {
         AcdbAfeInstanceCmnDataCmdType *pInput = (AcdbAfeInstanceCmnDataCmdType *)pIn;
         AcdbQueryResponseType *pOutput = (AcdbQueryResponseType *)pOut;
         result = GetMidIidPidCalibData(tblId, nLookupKey, cdefTbl, cdotTbl, tblInfo.dataPoolChnk, pInput->nModuleId, (uint32_t)pInput->nInstanceId,
            pInput->nParamId, pInput->nBufferPointer, pInput->nBufferLength, &pOutput->nBytesUsedInBuffer);
         if (ACDB_SUCCESS != result)
         {
            return result;
         }
      }
      break;
      default:
         return  ACDB_ERROR;
      }

      result = ACDB_SUCCESS;
   }

   return result;
}

int32_t AcdbCmdGetAVCSCustomTopoInfoWithInstanceId(AcdbQueryCmdType *pInput, AcdbQueryResponseType *pOutput)
{
   int32_t result = ACDB_SUCCESS;

   if (pInput == NULL || pOutput == NULL)
   {
      ACDB_DEBUG_LOG("ACDB_COMMAND: Provided invalid param\n");
      result = ACDB_BADPARM;
   }
   else if (pInput->pBufferPointer == NULL)
   {
      return ACDB_BADPARM;
   }
   else
   {
      AcdbGlbalPropInfo glbPropInfo = { 0, { 0, NULL } };
      memset(&glbPropInfo, 0, sizeof(glbPropInfo));
      glbPropInfo.pId = AVCS_TOPOLOGY_INFO_GLOBAL_PROPERTY_V2;
      result = acdbdata_ioctl(ACDBDATACMD_GET_GLOBAL_PROP, (uint8_t *)&glbPropInfo, sizeof(AcdbGlbalPropInfo),
         (uint8_t *)NULL, 0);
      if (result != ACDB_SUCCESS)
      {
         switch (result)
         {
         case ACDB_ERROR:
            ACDB_DEBUG_LOG("ACDB_COMMAND: Global acdb file not loaded to fetch data\n");
            break;

         case ACDB_BADPARM:
            ACDB_DEBUG_LOG("ACDB_COMMAND: Received NULL input for AcdbDataGetGlobalPropData\n");
            break;

         case ACDB_DATA_NOT_FOUND:
            ACDB_DEBUG_LOG("ACDB_COMMAND: Failed to fetch the property info for pid %08X \n", glbPropInfo.pId);
            break;

         default:
            ACDB_DEBUG_LOG("ACDB_COMMAND: Invalid command");
            break;
         }

         return result;
      }

      if (NULL == glbPropInfo.dataInfo.pData)
      {
         ACDB_DEBUG_LOG("ACDB_COMMAND: pData NULL on fetch the property info for pid %08X \n", glbPropInfo.pId);
         return ACDB_DATA_NOT_FOUND;
      }

      if (pInput->nBufferLength < glbPropInfo.dataInfo.nDataLen)
      {
         ACDB_DEBUG_LOG("ACDB_COMMAND: Insufficient memory provided to fill the AVCS custom topo info for pid %08X \n", glbPropInfo.pId);
         return ACDB_INSUFFICIENTMEMORY;
      }

      ACDB_MEM_CPY(pInput->pBufferPointer, pInput->nBufferLength, glbPropInfo.dataInfo.pData, glbPropInfo.dataInfo.nDataLen);
      pOutput->nBytesUsedInBuffer = glbPropInfo.dataInfo.nDataLen;
   }
   return result;
}

int32_t AcdbCmdGetAVCSCustomTopoInfoSizeWithInstanceId(AcdbSizeResponseType *pOutput)
{
   int32_t result = ACDB_SUCCESS;

   if (pOutput == NULL)
   {
      ACDB_DEBUG_LOG("ACDB_COMMAND: Provided invalid param\n");
      result = ACDB_BADPARM;
   }
   else
   {
      AcdbGlbalPropInfo glbPropInfo = { 0, { 0, NULL } };
      memset(&glbPropInfo, 0, sizeof(glbPropInfo));
      glbPropInfo.pId = AVCS_TOPOLOGY_INFO_GLOBAL_PROPERTY_V2;
      result = acdbdata_ioctl(ACDBDATACMD_GET_GLOBAL_PROP, (uint8_t *)&glbPropInfo, sizeof(AcdbGlbalPropInfo),
         (uint8_t *)NULL, 0);
      if (result != ACDB_SUCCESS)
      {
         switch (result)
         {
         case ACDB_ERROR:
            ACDB_DEBUG_LOG("ACDB_COMMAND: Global acdb file not loaded to fetch data\n");
            break;

         case ACDB_BADPARM:
            ACDB_DEBUG_LOG("ACDB_COMMAND: Received NULL input for AcdbDataGetGlobalPropData\n");
            break;

         case ACDB_DATA_NOT_FOUND:
            ACDB_DEBUG_LOG("ACDB_COMMAND: Failed to fetch the property info for pid %08X \n", glbPropInfo.pId);
            break;

         default:
            ACDB_DEBUG_LOG("ACDB_COMMAND: Invalid command");
            break;
         }

         return result;
      }
      if (NULL == glbPropInfo.dataInfo.pData)
      {
         ACDB_DEBUG_LOG("ACDB_COMMAND: pData NULL on fetch the property info for pid %08X \n", glbPropInfo.pId);
         return ACDB_DATA_NOT_FOUND;
      }

      pOutput->nSize = glbPropInfo.dataInfo.nDataLen;
   }
   return result;
}

int32_t AcdbCmdSetAudProcInstanceData(AcdbAudProcInstanceCmdType *pInput)
{
   int32_t result = ACDB_SUCCESS;
   uint32_t persistData = FALSE;
   int32_t persist_result = AcdbCmdIsPersistenceSupported(&persistData);
   if (persist_result != ACDB_SUCCESS)
   {
      persistData = FALSE;
   }

   if (pInput == NULL)
   {
      ACDB_DEBUG_LOG("[ACDB Command]->[AcdbCmdSetAudProcInstanceData]->Invalid NULL value parameters are provided\n");
      result = ACDB_BADPARM;
   }
   else if (pInput->nBufferPointer == NULL)
   {
      result = ACDB_BADPARM;
   }
   else
   {
      const uint32_t tblId = AUDPROC_INST_TBL_ID;
      uint32_t iid = pInput->nInstanceId & 0xFFFF;
	  uint32_t offset = 0, size = ((AUDPROCTBL_INST_INDICES_COUNT+ AUDPROCTBL_INST_CDFT_INDICES_COUNT)*sizeof(uint32_t));	 
      AudProcCmnCmdLookupType audcmd = { 0, 0, 0 };
	   uint8_t *buffer = (uint8_t *)malloc(size);
	  if(buffer == NULL)
	  {
			ACDB_DEBUG_LOG("ACDB_COMMAND: Unable to allocate memory for AcdbCmdSetAudProcInstanceData - temporary buffer\n");
			return ACDB_INSUFFICIENTMEMORY;
	  }
      audcmd.nDeviceId = pInput->nDeviceId;
      audcmd.nDeviceSampleRateId = pInput->nDeviceSampleRateId;
      audcmd.nApplicationType = pInput->nApplicationType;
	  ACDB_MEM_CPY(buffer, size, (uint8_t *)&audcmd, AUDPROCTBL_INST_INDICES_COUNT *(uint32_t)sizeof(uint32_t));
	  offset += (uint32_t)(AUDPROCTBL_INST_INDICES_COUNT *sizeof(uint32_t));
	  ACDB_MEM_CPY(buffer + offset, sizeof(uint32_t), &pInput->nModuleId, sizeof(uint32_t));
	  offset += (uint32_t)sizeof(uint32_t);
	  ACDB_MEM_CPY(buffer + offset, sizeof(uint32_t), &iid, sizeof(uint32_t));
	  offset += (uint32_t)sizeof(uint32_t);
	  ACDB_MEM_CPY(buffer + offset, sizeof(uint32_t), &pInput->nParamId, sizeof(uint32_t));
	  offset += (uint32_t)sizeof(uint32_t);
      result = AcdbCmdSetOnlineDataV2(persistData, tblId, buffer, AUDPROCTBL_INST_INDICES_COUNT, AUDPROCTBL_INST_CDFT_INDICES_COUNT, pInput->nBufferPointer, pInput->nBufferLength);

      if (result == ACDB_SUCCESS &&
         ACDB_INIT_SUCCESS == AcdbIsPersistenceSupported())
      {
         result = AcdbCmdSaveDeltaFileData();

         if (result != ACDB_SUCCESS)
         {
            ACDB_DEBUG_LOG("[ACDB Command]->[AcdbCmdSetAudProcInstanceData]->Unable to save delta file data\n");
         }
      }
   }

   return result;
}

int32_t AcdbCmdSetAudProcInstanceInfo(AcdbAudProcTableCmdType *pInput)
{
   //Helper Structure to copy one ParamData entry's header information
   typedef struct _AudProcTableEntryHeader {
      uint32_t nModuleId;
      uint16_t nInstanceId;
      uint16_t nIReserved;
      uint32_t nParamId;
      uint16_t nParamSize; //multiple of 4
      uint16_t nReserved; // Must be 0
   } AudProcInstanceTableEntryHeader;

   int32_t result = ACDB_SUCCESS;
   uint32_t remaining_bufLength = 0;
   uint32_t offSet = 0;
   uint32_t iid = 0;

   AudProcInstanceTableEntryHeader oneEntryHeader = { 0, 0, 0, 0, 0, 0 };
   uint8_t  *pOneEntryBuffer = NULL;

   AudProcCmnCmdLookupType audProcCmnLookupCmd = { 0, 0, 0 };
   uint32_t persistData = FALSE;
   int32_t persist_result = 0;

   if (pInput == NULL)
   {
      ACDB_DEBUG_LOG("[ACDB Command]->[AcdbCmdSetAudProcInfo]->Invalid NULL value parameters are provided\n");
      return ACDB_BADPARM;
   }
   else if (pInput->nBufferPointer == NULL)
   {
      return ACDB_BADPARM;
   }

   remaining_bufLength = pInput->nBufferLength;

   persist_result = AcdbCmdIsPersistenceSupported(&persistData);
   if (persist_result != ACDB_SUCCESS)
   {
      persistData = FALSE;
   }

   audProcCmnLookupCmd.nDeviceId = pInput->nDeviceId;
   audProcCmnLookupCmd.nDeviceSampleRateId = pInput->nDeviceSampleRateId;
   audProcCmnLookupCmd.nApplicationType = pInput->nApplicationType;

   //Iterate over each entry
   while (remaining_bufLength > sizeof(AudProcInstanceTableEntryHeader))
   {
      //Copy one entry header
      ACDB_MEM_CPY(&oneEntryHeader, sizeof(oneEntryHeader), pInput->nBufferPointer + offSet, sizeof(oneEntryHeader));

      //Update book keeping
      remaining_bufLength = remaining_bufLength - (uint32_t)sizeof(oneEntryHeader);
      offSet = (uint32_t)(offSet + sizeof(oneEntryHeader));

      if (remaining_bufLength >= oneEntryHeader.nParamSize)
	  {
		  uint32_t offset = 0, size = ((AUDPROCTBL_INST_INDICES_COUNT+ AUDPROCTBL_INST_CDFT_INDICES_COUNT)*sizeof(uint32_t));
		  uint8_t *buffer = (uint8_t *)malloc(size);
		  if(buffer == NULL)
		  {
				ACDB_DEBUG_LOG("ACDB_COMMAND: Unable to allocate memory for AcdbCmdSetAudProcInstanceInfo - temporary buffer\n");
				return ACDB_INSUFFICIENTMEMORY;
	      }
		  //get pointer to one entry's param data
		  pOneEntryBuffer = &pInput->nBufferPointer[offSet];

		  //Update book keeping
		  remaining_bufLength = remaining_bufLength - oneEntryHeader.nParamSize;
		  offSet = offSet + oneEntryHeader.nParamSize;
		  iid = oneEntryHeader.nInstanceId & 0xFFFF;

		  ACDB_MEM_CPY(buffer, size, (uint8_t *)&audProcCmnLookupCmd, AUDPROCTBL_INST_INDICES_COUNT *(uint32_t)sizeof(uint32_t));
		  offset += (uint32_t)(AUDPROCTBL_INST_INDICES_COUNT *sizeof(uint32_t));
		  ACDB_MEM_CPY(buffer + offset, sizeof(uint32_t), &oneEntryHeader.nModuleId, sizeof(uint32_t));
		  offset += (uint32_t)sizeof(uint32_t);
		  ACDB_MEM_CPY(buffer + offset, sizeof(uint32_t), &iid, sizeof(uint32_t));
		  offset += (uint32_t)sizeof(uint32_t);
		  ACDB_MEM_CPY(buffer + offset, sizeof(uint32_t), &oneEntryHeader.nParamId, sizeof(uint32_t));
		  offset += (uint32_t)sizeof(uint32_t);

		  //Set one ParamData entry
		  result = AcdbCmdSetOnlineDataV2(persistData, AUDPROC_INST_TBL_ID, buffer, AUDPROCTBL_INST_INDICES_COUNT, AUDPROCTBL_INST_CDFT_INDICES_COUNT, pOneEntryBuffer, oneEntryHeader.nParamSize);

		  if (result != ACDB_SUCCESS)
		  {
			  ACDB_DEBUG_LOG("[ACDB Command]->[AcdbCmdSetAudProcInfo]->Err:%0x Failed to set AudProcData for...\n", result);
			  ACDB_DEBUG_LOG("ModID:%0x PID:%0x ParamSize:%0x", oneEntryHeader.nModuleId, oneEntryHeader.nParamId, oneEntryHeader.nParamSize);
			  return result;
		  }
	  }
      else
      {
         ACDB_DEBUG_LOG("[ACDB Command]->[AcdbCmdSetAudProcInfo]->Unexpected buffer length. Residual of %u bytes found\n", remaining_bufLength);
         result = ACDB_BADPARM;
         return result;
      }
   }

   if (remaining_bufLength > 0)
   {
      ACDB_DEBUG_LOG("[ACDB Command]->[AcdbCmdSetAudProcInstanceInfo]->Unexpected buffer length. Residual of %u bytes found\n", remaining_bufLength);
   }

   if (result == ACDB_SUCCESS &&
      ACDB_INIT_SUCCESS == AcdbIsPersistenceSupported())
   {
      result = AcdbCmdSaveDeltaFileData();
      if (result != ACDB_SUCCESS)
      {
         ACDB_DEBUG_LOG("[ACDB Command]->[AcdbCmdSetAudProcInstanceInfo]->Unable to save delta file data\n");
      }
   }

   result = ACDB_SUCCESS;

   return result;
}

int32_t AcdbCmdSetAudStreamInstanceInfo(AcdbAudStrmTableV2CmdType *pInput)
{
   //Helper Structure to copy one ParamData entry's header information
   typedef struct _AudStrmTableEntryHeader {
      uint32_t nModuleId;
      uint16_t nInstanceId;
      uint16_t nReserved1;
      uint32_t nParamId;
      uint16_t nParamSize; //multiple of 4
      uint16_t nReserved; // Must be 0
   } AudStrmTableEntryHeader;

   int32_t result = ACDB_SUCCESS;
   uint32_t remaining_bufLength = 0;
   uint32_t offSet = 0;
   uint32_t iid = 0;

   AudStrmTableEntryHeader oneEntryHeader = { 0, 0, 0, 0, 0, 0 };
   uint8_t  *pOneEntryBuffer = NULL;

   AudStreamCmdLookupType audStrmCmnLookupCmd = { 0 };
   uint32_t persistData = FALSE;

   int32_t persist_result = 0;

   if (pInput == NULL)
   {
      ACDB_DEBUG_LOG("[ACDB Command]->[AcdbCmdSetAudStreamInstanceInfo]->Invalid NULL value parameters are provided\n");
      return ACDB_BADPARM;
   }
   else if (pInput->nBufferPointer == NULL)
   {
      return ACDB_BADPARM;
   }

   remaining_bufLength = pInput->nBufferLength;

   persist_result = AcdbCmdIsPersistenceSupported(&persistData);
   if (persist_result != ACDB_SUCCESS)
   {
      persistData = FALSE;
   }
   audStrmCmnLookupCmd.nApplicationType = pInput->nApplicationTypeId;

   //Iterate over each entry
   while (remaining_bufLength > sizeof(AudStrmTableEntryHeader))
   {
      //Copy one entry header
      ACDB_MEM_CPY(&oneEntryHeader, sizeof(oneEntryHeader), pInput->nBufferPointer + offSet, sizeof(oneEntryHeader));

      //Update book keeping
      remaining_bufLength = remaining_bufLength - (uint32_t)sizeof(oneEntryHeader);
      offSet = (uint32_t)(offSet + sizeof(oneEntryHeader));

      if (remaining_bufLength >= oneEntryHeader.nParamSize)
	  {
		  uint32_t offset = 0, size = ((AUDSTREAMTBL_INST_INDICES_COUNT+ AUDSTREAMTBL_INST_CDFT_INDICES_COUNT)*sizeof(uint32_t));
		  uint8_t *buffer = (uint8_t *)malloc(size);
		  if(buffer == NULL)
		  {
				ACDB_DEBUG_LOG("ACDB_COMMAND: Unable to allocate memory for AcdbCmdSetAudStreamInstanceInfo - temporary buffer\n");
				return ACDB_INSUFFICIENTMEMORY;
	      }
		  //get pointer to one entry's param data
		  pOneEntryBuffer = &pInput->nBufferPointer[offSet];

		  //Update book keeping
		  remaining_bufLength = remaining_bufLength - oneEntryHeader.nParamSize;
		  offSet = offSet + oneEntryHeader.nParamSize;
		  iid = GetUint32IID(oneEntryHeader.nInstanceId);

		  ACDB_MEM_CPY(buffer, size, (uint8_t *)&audStrmCmnLookupCmd, AUDSTREAMTBL_INST_INDICES_COUNT *(uint32_t)sizeof(uint32_t));
		  offset += (uint32_t)(AUDSTREAMTBL_INST_INDICES_COUNT *sizeof(uint32_t));
		  ACDB_MEM_CPY(buffer + offset, sizeof(uint32_t), &oneEntryHeader.nModuleId, sizeof(uint32_t));
		  offset += (uint32_t)sizeof(uint32_t);
		  ACDB_MEM_CPY(buffer + offset, sizeof(uint32_t), &iid, sizeof(uint32_t));
		  offset += (uint32_t)sizeof(uint32_t);
		  ACDB_MEM_CPY(buffer + offset, sizeof(uint32_t), &oneEntryHeader.nParamId, sizeof(uint32_t));
		  offset += (uint32_t)sizeof(uint32_t);

		  //Set one ParamData entry
		  result = AcdbCmdSetOnlineDataV2(persistData, AUDSTRM_INST_TBL_ID, buffer, AUDSTREAMTBL_INST_INDICES_COUNT, AUDSTREAMTBL_INST_CDFT_INDICES_COUNT, pOneEntryBuffer, oneEntryHeader.nParamSize);

		  if (result != ACDB_SUCCESS)
		  {
			  ACDB_DEBUG_LOG("[ACDB Command]->[AcdbCmdSetAudStreamInstanceInfo]->Err:%0x Failed to set AudProcData for...\n", result);
			  ACDB_DEBUG_LOG("ModID:%0x PID:%0x ParamSize:%0x \n", oneEntryHeader.nModuleId, oneEntryHeader.nParamId, oneEntryHeader.nParamSize);
			  return result;
		  }
	  }
      else
      {
         ACDB_DEBUG_LOG("[ACDB Command]->[AcdbCmdSetAudStreamInstanceInfo]->Unexpected buffer length. Residual of %u bytes found\n", remaining_bufLength);
         result = ACDB_BADPARM;
         return result;
      }
   }

   if (remaining_bufLength > 0)
   {
      ACDB_DEBUG_LOG("[ACDB Command]->[AcdbCmdSetAudStreamInstanceInfo]->Unexpected buffer length. Residual of %u bytes found\n", remaining_bufLength);
   }

   if (result == ACDB_SUCCESS &&
      ACDB_INIT_SUCCESS == AcdbIsPersistenceSupported())
   {
      result = AcdbCmdSaveDeltaFileData();
      if (result != ACDB_SUCCESS)
      {
         ACDB_DEBUG_LOG("[ACDB Command]->[AcdbCmdSetAudStreamInstanceInfo]->Unable to save delta file data\n");
      }
   }

   result = ACDB_SUCCESS;

   return result;
}

int32_t AcdbCmdSetAudStreamInstanceData(AcdbAudStrmInstanceCmdType *pInput)
{
   int32_t result = ACDB_SUCCESS;
   uint32_t persistData = FALSE;
   int32_t persist_result = AcdbCmdIsPersistenceSupported(&persistData);
   if (persist_result != ACDB_SUCCESS)
   {
      persistData = FALSE;
   }

   if (pInput == NULL)
   {
      ACDB_DEBUG_LOG("[ACDB Command]->[AcdbCmdSetAudStreamData]->Invalid NULL value parameters are provided\n");
      result = ACDB_BADPARM;
   }
   else if (pInput->nBufferPointer == NULL)
   {
      return ACDB_BADPARM;
   }
   else
   {
      const uint32_t tblId = AUDSTRM_INST_TBL_ID;
      const uint32_t iid = GetUint32IID(pInput->nInstanceId);
	  uint32_t offset = 0, size = ((AUDSTREAMTBL_INST_INDICES_COUNT+ AUDSTREAMTBL_INST_CDFT_INDICES_COUNT)*sizeof(uint32_t));	  
      AudStreamCmdLookupType audstrmcmd = { 0 };
	  uint8_t *buffer = (uint8_t *)malloc(size);
	  if(buffer == NULL)
	  {
			ACDB_DEBUG_LOG("ACDB_COMMAND: Unable to allocate memory for AcdbCmdSetAudStreamInstanceData - temporary buffer\n");
			return ACDB_INSUFFICIENTMEMORY;
	  }
      audstrmcmd.nApplicationType = pInput->nApplicationTypeId;

	  ACDB_MEM_CPY(buffer, size, (uint8_t *)&audstrmcmd, AUDSTREAMTBL_INST_INDICES_COUNT *(uint32_t)sizeof(uint32_t));
	  offset += (uint32_t)(AUDSTREAMTBL_INST_INDICES_COUNT *sizeof(uint32_t));
	  ACDB_MEM_CPY(buffer + offset, sizeof(uint32_t), &pInput->nModuleId, sizeof(uint32_t));
	  offset += (uint32_t)sizeof(uint32_t);
	  ACDB_MEM_CPY(buffer + offset, sizeof(uint32_t), &iid, sizeof(uint32_t));
	  offset += (uint32_t)sizeof(uint32_t);
	  ACDB_MEM_CPY(buffer + offset, sizeof(uint32_t), &pInput->nParamId, sizeof(uint32_t));
	  offset += (uint32_t)sizeof(uint32_t);
      result = AcdbCmdSetOnlineDataV2(persistData, tblId, buffer, AUDSTREAMTBL_INST_INDICES_COUNT, AUDSTREAMTBL_INST_CDFT_INDICES_COUNT, pInput->nBufferPointer, pInput->nBufferLength);

      if (result == ACDB_SUCCESS &&
         ACDB_INIT_SUCCESS == AcdbIsPersistenceSupported())
      {
         result = AcdbCmdSaveDeltaFileData();

         if (result != ACDB_SUCCESS)
         {
            ACDB_DEBUG_LOG("[ACDB Command]->[AcdbCmdSetAudStreamData]->Unable to save delta file data\n");
         }
      }
   }

   return result;
}

int32_t AcdbCmdGetCodecTopId(int deviceid, AcdbGetTopologyIdRspType *pOutput)
{
   int32_t result = ACDB_PARMNOTFOUND;

   if (pOutput == NULL)
   {
      ACDB_DEBUG_LOG("[ACDB Command]->[AcdbCmdGetCodecTopId]->System Erorr\n");
      result = ACDB_BADPARM;
   }
   else
   {
      AcdbDevPropInfo devPropInfo = { 0, 0, { 0, NULL } };
      devPropInfo.devId = deviceid;
      devPropInfo.pId = CODEC_GATEWAYTOPOLOGY;
      result = acdbdata_ioctl(ACDBDATACMD_GET_DEVICE_PROP, (uint8_t *)&devPropInfo, sizeof(AcdbDevPropInfo),
         (uint8_t *)NULL, 0);
      if (result != ACDB_SUCCESS)
      {
         ACDB_DEBUG_LOG("ACDB_COMMAND: Failed to fetch the codec topo info for devid %08X \n", deviceid);
         return ACDB_PARMNOTFOUND;
      }
      pOutput->nTopologyId = READ_UInt32(devPropInfo.dataInfo.pData);
   }

   return result;
}
int32_t AcdbCmdGetPPSequenceInfo(uint32_t queryType, uint8_t *pIn, uint8_t *pOut)
{
   int32_t result = ACDB_SUCCESS;

   if (pIn == NULL || pOut == NULL)
   {
      ACDB_DEBUG_LOG("[ACDB Command]->[AcdbCmdGetPPSequenceInfo]->Invalid NULL value parameters are provided\n");
      result = ACDB_BADPARM;
   }
   else
   {
	   AcdbGetTopologyIdRspType topinfo = { 0 };
      uint8_t* pTopInfo = NULL;
      uint32_t offset = 0, nDeviceID = 0;
      AcdbGlbalPropInfo glbPropInfo = { 0, { 0, NULL } };
      glbPropInfo.pId = CODEC_PP_SEQUENCE;
      result = acdbdata_ioctl(ACDBDATACMD_GET_GLOBAL_PROP, (uint8_t *)&glbPropInfo, sizeof(AcdbGlbalPropInfo),
         (uint8_t *)NULL, 0);
      if (result != ACDB_SUCCESS)
      {
         ACDB_DEBUG_LOG("ACDB_COMMAND: Failed to fetch the property info for pid %08X \n", glbPropInfo.pId);
         return result;
      }
      if (NULL == glbPropInfo.dataInfo.pData)
      {
         ACDB_DEBUG_LOG("ACDB_COMMAND: pData NULL on fetch the property info for pid %08X \n", glbPropInfo.pId);
         return ACDB_ERROR;
      }
      switch (queryType)
      {
      case DATA_CMD:
      {
         //fill the output buffer
         GetPPSequenceDataType *pInput = (GetPPSequenceDataType *)pIn;
         nDeviceID = pInput->nDeviceId;
      }
      break;
      case DATA_SIZE_CMD:
      {
         GetPPSequenceCmdType *pInput = (GetPPSequenceCmdType *)pIn;
         nDeviceID = pInput->nDeviceId;
      }
      break;
      default:
         return ACDB_ERROR;
      }

      topinfo.nTopologyId = 0;
      result = AcdbCmdGetCodecTopId(nDeviceID, &topinfo);
      if (result != ACDB_SUCCESS)
      {
         ACDB_DEBUG_LOG("ACDB_COMMAND: Failed to fetch the property info for pid %08X \n", glbPropInfo.pId);
         return result;
      }

      //// FIND THE SEQUENCE AND LENGTH

      pTopInfo = (uint8_t *)glbPropInfo.dataInfo.pData;
      result = ACDB_PARMNOTFOUND;
      while (offset <= glbPropInfo.dataInfo.nDataLen)
      {
         uint32_t topid = 0, size = 0;
         ACDB_MEM_CPY(&topid, sizeof(uint32_t), pTopInfo + offset, sizeof(uint32_t));
         offset += (uint32_t)(sizeof(uint32_t));
         ACDB_MEM_CPY(&size, sizeof(uint32_t), pTopInfo + offset, sizeof(uint32_t));
         if (topid == topinfo.nTopologyId)
         {
            switch (queryType)
            {
            case DATA_CMD:
            {
               //fill the output buffer
               AcdbQueryResponseType *pOutput = (AcdbQueryResponseType *)pOut;
               GetPPSequenceDataType *pInput = (GetPPSequenceDataType *)pIn;
               if (pInput->nBufferLength < size*sizeof(uint32_t) + sizeof(uint32_t))
                  return ACDB_INSUFFICIENTMEMORY;
               ACDB_MEM_CPY(pInput->nBufferPointer, size*sizeof(uint32_t) + sizeof(uint32_t), pTopInfo + offset, size*sizeof(uint32_t) + sizeof(uint32_t));
               pOutput->nBytesUsedInBuffer = (uint32_t)(size*sizeof(uint32_t) + sizeof(uint32_t));
            }
            break;
            case DATA_SIZE_CMD:
            {
               AcdbSizeResponseType *pOutput = (AcdbSizeResponseType *)pOut;
               pOutput->nSize = (uint32_t)(size*sizeof(uint32_t) + sizeof(uint32_t));
            }
            break;
            default:
               return ACDB_ERROR;
            }
            return ACDB_SUCCESS;
         }
         else
         {
            offset += (uint32_t)sizeof(uint32_t);
            offset += (uint32_t)(size*sizeof(uint32_t));
         }
      }
   }

   return result;
}
int32_t AcdbCmdGetCodecPPDataInfo(uint32_t queryType, uint8_t *pIn, uint8_t *pOut)
{
   int32_t result = ACDB_SUCCESS;

   if (pIn == NULL || pOut == NULL)
   {
      ACDB_DEBUG_LOG("[ACDB Command]->[AcdbCmdGetCodecPPDataInfo]->Invalid NULL value parameters are provided\n");
      result = ACDB_BADPARM;
   }
   else
   {
      uint32_t index = 0, i = 0;
      AcdbTableCmd cmd = { 0, 0 };
      AcdbTableInfo tblInfo = { { 0, NULL }, { 0, NULL }, { 0, NULL } };
	  CodecGainDataLookupTblType lutTbl = { 0, NULL };
      ContentDefTblType cdefTbl = { 0, NULL };
      ContentDataOffsetsTblType cdotTbl = { 0, NULL };
      uintptr_t nLookupKey = 0;
	  AcdbCodecGainCmdType codecGaincmd = { 0, 0, 0, 0, 0, NULL};
	  CodecGainDataLookupType tempLut = { 0, 0, 0, 0, 0 };
      uint32_t tblId = CODEC_GAIN_TBL;
      codecGaincmd.nDeviceId = 0;
      codecGaincmd.nGainStep = 0;
      codecGaincmd.nModuleId = 0;
      codecGaincmd.nParamId = 0;

      switch (queryType)
      {
      case DATA_CMD:
      {
         AcdbCodecGainCmdType *pInput = (AcdbCodecGainCmdType *)pIn;
         codecGaincmd.nDeviceId = pInput->nDeviceId;
         codecGaincmd.nGainStep = pInput->nGainStep;
         codecGaincmd.nModuleId = pInput->nModuleId;
         codecGaincmd.nParamId = pInput->nParamId;
      }
      break;
      case DATA_SIZE_CMD:
      {
         AcdbCodecGainSizeCmdType *pInput = (AcdbCodecGainSizeCmdType *)pIn;
         codecGaincmd.nDeviceId = pInput->nDeviceId;
         codecGaincmd.nGainStep = pInput->nGainStep;
         codecGaincmd.nModuleId = pInput->nModuleId;
         codecGaincmd.nParamId = pInput->nParamId;
      }
      break;

      default:
         return ACDB_ERROR;
      }

      cmd.devId = codecGaincmd.nDeviceId;
      cmd.tblId = tblId;
      result = acdbdata_ioctl(ACDBDATACMD_GET_TABLE_INFO, (uint8_t *)&cmd, sizeof(cmd),
         (uint8_t *)&tblInfo, sizeof(tblInfo));
      if (result != ACDB_SUCCESS)
      {
         ACDB_DEBUG_LOG("Failed to fetch the lookup information of the device %08X \n"
            , codecGaincmd.nDeviceId);
         return result;
      }
      lutTbl.nLen = READ_UInt32(tblInfo.tblLutChnk.pData);
      lutTbl.pLut = (CodecGainDataLookupType*)(tblInfo.tblLutChnk.pData + sizeof(lutTbl.nLen));
      for (i = 0; i < lutTbl.nLen; i++)
      {
         if (codecGaincmd.nDeviceId == lutTbl.pLut[i].nDeviceId && codecGaincmd.nGainStep == lutTbl.pLut[i].nGainStep)
         {
            index = i;
            nLookupKey = (uintptr_t)&lutTbl.pLut[index];
            ACDB_MEM_CPY(&tempLut, sizeof(tempLut), &lutTbl.pLut[index], sizeof(tempLut));
            ACDB_MEM_CPY(&cdefTbl.nLen, sizeof(uint32_t), (tblInfo.tblCdftChnk.pData + tempLut.nCDEFTblOffset), sizeof(uint32_t));

            cdefTbl.pCntDef = (ContentDefType *)(tblInfo.tblCdftChnk.pData + tempLut.nCDEFTblOffset + sizeof(cdefTbl.nLen));

            // Now get CDOT info
            ACDB_MEM_CPY(&cdotTbl.nLen, sizeof(uint32_t), (tblInfo.tblCdotChnk.pData + tempLut.nCDOTTblOffset), sizeof(uint32_t));
            cdotTbl.pDataOffsets = (uint32_t *)(tblInfo.tblCdotChnk.pData + tempLut.nCDOTTblOffset + sizeof(cdotTbl.nLen));

            if (cdefTbl.nLen != cdotTbl.nLen)
            {
               ACDB_DEBUG_LOG("The no of entries in CDEF and CDOT tables for devid %08X not matching\n", codecGaincmd.nDeviceId);
               return ACDB_ERROR;
            }

            switch (queryType)
            {
            case DATA_CMD:
            {
               AcdbCodecGainCmdType *pInput = (AcdbCodecGainCmdType *)pIn;
               AcdbQueryResponseType *pOutput = (AcdbQueryResponseType *)pOut;
               result = GetMidPidCalibData(tblId, nLookupKey, cdefTbl, cdotTbl, tblInfo.dataPoolChnk, pInput->nModuleId,
                  pInput->nParamId, pInput->nBufferPointer, pInput->nBufferLength, &pOutput->nBytesUsedInBuffer);
               if (ACDB_SUCCESS == result || ACDB_INSUFFICIENTMEMORY == result)
               {
                  return result;
               }
            }
            break;
            case DATA_SIZE_CMD:
            {
               //AcdbCodecGainCmdType *pInput = (AcdbCodecGainCmdType *)pIn;
               AcdbSizeResponseType *pOutput = (AcdbSizeResponseType *)pOut;
               if (ACDB_SUCCESS == GetMidPidCalibTableSize(tblId, nLookupKey, cdefTbl, cdotTbl, tblInfo.dataPoolChnk, &pOutput->nSize))
               {
                  return ACDB_SUCCESS;
               }
            }
            break;

            default:
               return ACDB_ERROR;
            }
         }
      }
      result = ACDB_PARMNOTFOUND;
   }
   return result;
}
int32_t AcdbCmdGetCodecPPTableInfo(uint32_t queryType, uint8_t *pIn, uint8_t *pOut)
{
   int32_t result = ACDB_SUCCESS;

   if (pIn == NULL || pOut == NULL)
   {
      ACDB_DEBUG_LOG("[ACDB Command]->[AcdbCmdGetCodecPPTableInfo]->Invalid NULL value parameters are provided\n");
      result = ACDB_BADPARM;
   }
   else
   {
      uint32_t index = 0;
      AcdbTableCmd cmd = { 0, 0 };
      AcdbTableInfo tblInfo = { { 0, NULL }, { 0, NULL }, { 0, NULL } };
      CodecGainDataLookupTblType lutTbl = { 0, NULL };
      ContentDefTblType cdefTbl = { 0, NULL };
      ContentDataOffsetsTblType cdotTbl = { 0, NULL };
      uintptr_t nLookupKey = 0;
	  AcdbCodecGainTableCmdType codecGaincmd = { 0, 0, 0, 0, NULL };
      CodecGainDataLookupType tempLut = { 0, 0, 0, 0, 0 };
      uint32_t tblId = CODEC_GAIN_TBL;
      codecGaincmd.nDeviceId = 0;
      codecGaincmd.nGainStep = 0;
      codecGaincmd.nGainType = 0;
      switch (queryType)
      {
      case TABLE_CMD:
      {
         AcdbCodecGainTableCmdType *pInput = (AcdbCodecGainTableCmdType *)pIn;
         codecGaincmd.nDeviceId = pInput->nDeviceId;
         codecGaincmd.nGainStep = pInput->nGainStep;
         codecGaincmd.nGainType = pInput->nGainType;
      }
      break;
      case TABLE_SIZE_CMD:
      {
         AcdbCodecGainTableSizeCmdType *pInput = (AcdbCodecGainTableSizeCmdType *)pIn;
         codecGaincmd.nDeviceId = pInput->nDeviceId;
         codecGaincmd.nGainStep = pInput->nGainStep;
         codecGaincmd.nGainType = pInput->nGainType;
      }
      break;

      default:
         return ACDB_ERROR;
      }

      cmd.devId = codecGaincmd.nDeviceId;
      cmd.tblId = tblId;
      result = acdbdata_ioctl(ACDBDATACMD_GET_TABLE_INFO, (uint8_t *)&cmd, sizeof(cmd),
         (uint8_t *)&tblInfo, sizeof(tblInfo));
      if (result != ACDB_SUCCESS)
      {
         ACDB_DEBUG_LOG("Failed to fetch the lookup information of the device %08X \n"
            , codecGaincmd.nDeviceId);
         return result;
      }
      lutTbl.nLen = READ_UInt32(tblInfo.tblLutChnk.pData);
      lutTbl.pLut = (CodecGainDataLookupType*)(tblInfo.tblLutChnk.pData + sizeof(lutTbl.nLen));

      result = AcdbDataBinarySearch((void *)lutTbl.pLut, lutTbl.nLen,
         CODEC_PP_GAIN_LUT_INDICES_COUNT, &codecGaincmd, CODEC_PP_GAIN_CMD_INDICES_COUNT, &index);
      if (result != SEARCH_SUCCESS)
      {
         ACDB_DEBUG_LOG("Failed to fetch the lookup information of the device %08X \n"
            , codecGaincmd.nDeviceId);
         return ACDB_PARMNOTFOUND;
      }

      nLookupKey = (uintptr_t)&lutTbl.pLut[index];
      // Now get CDEF info
      ACDB_MEM_CPY(&tempLut, sizeof(tempLut), &lutTbl.pLut[index], sizeof(tempLut));
      ACDB_MEM_CPY(&cdefTbl.nLen, sizeof(uint32_t), (tblInfo.tblCdftChnk.pData + tempLut.nCDEFTblOffset), sizeof(uint32_t));

      cdefTbl.pCntDef = (ContentDefType *)(tblInfo.tblCdftChnk.pData + tempLut.nCDEFTblOffset + sizeof(cdefTbl.nLen));

      // Now get CDOT info
      ACDB_MEM_CPY(&cdotTbl.nLen, sizeof(uint32_t), (tblInfo.tblCdotChnk.pData + tempLut.nCDOTTblOffset), sizeof(uint32_t));
      cdotTbl.pDataOffsets = (uint32_t *)(tblInfo.tblCdotChnk.pData + tempLut.nCDOTTblOffset + sizeof(cdotTbl.nLen));

      if (cdefTbl.nLen != cdotTbl.nLen)
      {
         ACDB_DEBUG_LOG("The no of entries in CDEF and CDOT tables for devid %08X not matching\n", codecGaincmd.nDeviceId);
         return ACDB_ERROR;
      }

      switch (queryType)
      {
      case TABLE_CMD:
      {
         AcdbCodecGainTableCmdType *pInput = (AcdbCodecGainTableCmdType *)pIn;
         AcdbQueryResponseType *pOutput = (AcdbQueryResponseType *)pOut;
         result = GetMidPidCalibTable(tblId, nLookupKey, cdefTbl, cdotTbl, tblInfo.dataPoolChnk,
            pInput->nBufferPointer, pInput->nBufferLength, &pOutput->nBytesUsedInBuffer);
         if (ACDB_SUCCESS != result)
         {
            return result;
         }
      }
      break;
      case TABLE_SIZE_CMD:
      {
         AcdbSizeResponseType *pOutput = (AcdbSizeResponseType *)pOut;
         if (ACDB_SUCCESS != GetMidPidCalibTableSize(tblId, nLookupKey, cdefTbl, cdotTbl, tblInfo.dataPoolChnk, &pOutput->nSize))
         {
            return ACDB_ERROR;
         }
      }
      break;
      default:
         return ACDB_ERROR;
      }

      result = ACDB_SUCCESS;
   }

   return result;
}
int32_t AcdbCmdGetCodecPPCmnTableInfo(uint32_t queryType, uint8_t *pIn, uint8_t *pOut)
{
   int32_t result = ACDB_SUCCESS;

   if (pIn == NULL || pOut == NULL)
   {
      ACDB_DEBUG_LOG("[ACDB Command]->[AcdbCmdGetCodecPPTableInfo]->Invalid NULL value parameters are provided\n");
      result = ACDB_BADPARM;
   }
   else
   {
      uint32_t index = 0;
      AcdbTableCmd cmd = { 0, 0 };
      AcdbTableInfo tblInfo = { { 0, NULL }, { 0, NULL }, { 0, NULL } };
	  CodecCommonDataLookupTblType lutTbl = { 0, NULL };
      ContentDefTblType cdefTbl = { 0, NULL };
      ContentDataOffsetsTblType cdotTbl = { 0, NULL };
      uintptr_t nLookupKey = 0;
	  AcdbCodecCommonTableCmdType codecGaincmd = { 0, 0, 0, NULL };
	  CodecCommonTblLookupType tempLut = { 0, 0, 0, 0 };
      uint32_t tblId = CODEC_COMMON_TBL;
      codecGaincmd.nDeviceId = 0;
      codecGaincmd.nDeviceSampleRateId = 0;
      switch (queryType)
      {
      case TABLE_CMD:
      {
         AcdbCodecCommonTableCmdType *pInput = (AcdbCodecCommonTableCmdType *)pIn;
         codecGaincmd.nDeviceId = pInput->nDeviceId;
         codecGaincmd.nDeviceSampleRateId = pInput->nDeviceSampleRateId;
      }
      break;
      case TABLE_SIZE_CMD:
      {
         AcdbCodecCommonTableSizeCmdType *pInput = (AcdbCodecCommonTableSizeCmdType *)pIn;
         codecGaincmd.nDeviceId = pInput->nDeviceId;
         codecGaincmd.nDeviceSampleRateId = pInput->nDeviceSampleRateId;
      }
      break;

      default:
         return ACDB_ERROR;
      }

      cmd.devId = codecGaincmd.nDeviceId;
      cmd.tblId = tblId;
      result = acdbdata_ioctl(ACDBDATACMD_GET_TABLE_INFO, (uint8_t *)&cmd, sizeof(cmd),
         (uint8_t *)&tblInfo, sizeof(tblInfo));
      if (result != ACDB_SUCCESS)
      {
         ACDB_DEBUG_LOG("Failed to fetch the lookup information of the device %08X \n"
            , codecGaincmd.nDeviceId);
         return result;
      }
      lutTbl.nLen = READ_UInt32(tblInfo.tblLutChnk.pData);
      lutTbl.pLut = (CodecCommonTblLookupType*)(tblInfo.tblLutChnk.pData + sizeof(lutTbl.nLen));

      result = AcdbDataBinarySearch((void *)lutTbl.pLut, lutTbl.nLen,
         CODEC_PP_COMMON_LUT_INDICES_COUNT, &codecGaincmd, CODEC_PP_COMMON_CMD_INDICES_COUNT, &index);
      if (result != SEARCH_SUCCESS)
      {
         ACDB_DEBUG_LOG("Failed to fetch the lookup information of the device %08X \n"
            , codecGaincmd.nDeviceId);
         return ACDB_PARMNOTFOUND;
      }

      nLookupKey = (uintptr_t)&lutTbl.pLut[index];
      // Now get CDEF info
      ACDB_MEM_CPY(&tempLut, sizeof(tempLut), &lutTbl.pLut[index], sizeof(tempLut));
      ACDB_MEM_CPY(&cdefTbl.nLen, sizeof(uint32_t), (tblInfo.tblCdftChnk.pData + tempLut.nCDEFTblOffset), sizeof(uint32_t));

      cdefTbl.pCntDef = (ContentDefType *)(tblInfo.tblCdftChnk.pData + tempLut.nCDEFTblOffset + sizeof(cdefTbl.nLen));

      // Now get CDOT info
      ACDB_MEM_CPY(&cdotTbl.nLen, sizeof(uint32_t), (tblInfo.tblCdotChnk.pData + tempLut.nCDOTTblOffset), sizeof(uint32_t));
      cdotTbl.pDataOffsets = (uint32_t *)(tblInfo.tblCdotChnk.pData + tempLut.nCDOTTblOffset + sizeof(cdotTbl.nLen));

      if (cdefTbl.nLen != cdotTbl.nLen)
      {
         ACDB_DEBUG_LOG("The no of entries in CDEF and CDOT tables for devid %08X not matching\n", codecGaincmd.nDeviceId);
         return ACDB_ERROR;
      }

      switch (queryType)
      {
      case TABLE_CMD:
      {
         AcdbCodecCommonTableCmdType *pInput = (AcdbCodecCommonTableCmdType *)pIn;
         AcdbQueryResponseType *pOutput = (AcdbQueryResponseType *)pOut;
         result = GetMidPidCalibTable(tblId, nLookupKey, cdefTbl, cdotTbl, tblInfo.dataPoolChnk,
            pInput->nBufferPointer, pInput->nBufferLength, &pOutput->nBytesUsedInBuffer);
         if (ACDB_SUCCESS != result)
         {
            return result;
         }
      }
      break;
      case TABLE_SIZE_CMD:
      {
         AcdbSizeResponseType *pOutput = (AcdbSizeResponseType *)pOut;
         if (ACDB_SUCCESS != GetMidPidCalibTableSize(tblId, nLookupKey, cdefTbl, cdotTbl, tblInfo.dataPoolChnk, &pOutput->nSize))
         {
            return ACDB_ERROR;
         }
      }
      break;

      default:
         return ACDB_ERROR;
      }

      result = ACDB_SUCCESS;
   }

   return result;
}

int32_t AcdbCmdGetCodecGainLevelsInfo(uint32_t queryType, uint8_t * pOutput)
{
   int32_t result = ACDB_SUCCESS;

   if (pOutput == NULL)
   {
      ACDB_DEBUG_LOG("ACDB_COMMAND: Provided invalid param\n");
      result = ACDB_BADPARM;
   }
   else
   {
      AcdbGlbalPropInfo glbPropInfo = { 0, { 0, NULL } };
      glbPropInfo.pId = CODEC_GAINS_STEPS;
      result = acdbdata_ioctl(ACDBDATACMD_GET_GLOBAL_PROP, (uint8_t *)&glbPropInfo, sizeof(AcdbGlbalPropInfo),
         (uint8_t *)NULL, 0);
      if (result != ACDB_SUCCESS)
      {
         ACDB_DEBUG_LOG("ACDB_COMMAND: Failed to fetch the property info for pid %08X \n", glbPropInfo.pId);
         return result;
      }
      if (NULL == glbPropInfo.dataInfo.pData)
      {
         ACDB_DEBUG_LOG("ACDB_COMMAND: pData NULL on fetch the property info for pid %08X \n", glbPropInfo.pId);
         return ACDB_ERROR;
      }
      switch (queryType)
      {
      case DATA_CMD:
      {
         AcdbCodecGainLevelsRspType *pout = (AcdbCodecGainLevelsRspType *)pOutput;
         pout->NoOfentries = READ_UInt32(glbPropInfo.dataInfo.pData);
         ACDB_MEM_CPY(pout->Payload, (size_t)(pout->NoOfentries) * 3 * sizeof(uint32_t), glbPropInfo.dataInfo.pData + sizeof(uint32_t), (size_t)(pout->NoOfentries) * 3 * sizeof(uint32_t));
      }
      break;
      case DATA_SIZE_CMD:
      {
		  AcdbCodecGainLevelsRspType pout = { 0 , NULL };
         AcdbSizeResponseType* p = (AcdbSizeResponseType *)pOutput;
         pout.NoOfentries = READ_UInt32(glbPropInfo.dataInfo.pData);
         p->nSize = (uint32_t)(pout.NoOfentries * 3 * (uint32_t)sizeof(uint32_t));
      }
      break;
      default:
         return ACDB_ERROR;
      }
   }
   return result;
}

int32_t AcdbCmdGetCodecParamIDInfo(AcdbCodecParamIDCmdType *pInput,
   AcdbQueryResponseType *pOutput)
{
   int32_t result = ACDB_SUCCESS;

   if (pOutput == NULL)
   {
      ACDB_DEBUG_LOG("ACDB_COMMAND: Provided invalid param\n");
      result = ACDB_BADPARM;
   }
   else
   {
      uint32_t index = 0;
      AcdbTableCmd cmd = { 0, 0 };
      AcdbTableInfo tblInfo = { { 0, NULL }, { 0, NULL }, { 0, NULL } };
      ADIEDataLookupTblType lutTbl = { 0, NULL };
      ADIECmdLookupType adiecmd = { 0, 0 };
      uint32_t tblId = ADIE_CODEC_TBL;
      AcdbQueryResponseType *pOut = (AcdbQueryResponseType *)pOutput;
      adiecmd.nCodecId = pInput->ulCodecPathId;

      cmd.devId = 0;  //adie calibration is not device based, so make it zero
      cmd.tblId = tblId;
      result = acdbdata_ioctl(ACDBDATACMD_GET_TABLE_INFO, (uint8_t *)&cmd, sizeof(cmd),
         (uint8_t *)&tblInfo, sizeof(tblInfo));
      if (result != ACDB_SUCCESS)
      {
         ACDB_DEBUG_LOG("Failed to fetch the lookup information of the codecpathid %08X \n"
            , adiecmd.nCodecId);
         return result;
      }
      lutTbl.nLen = READ_UInt32(tblInfo.tblLutChnk.pData);
      lutTbl.pLut = (ADIEDataLookupType *)(tblInfo.tblLutChnk.pData + sizeof(lutTbl.nLen));
      result = AcdbDataBinarySearch((void *)lutTbl.pLut, lutTbl.nLen,
         ADIE_LUT_INDICES_COUNT, &adiecmd, ADIE_CMD_INDICES_COUNT - 1, &index);
      if (result != SEARCH_SUCCESS)
      {
         ACDB_DEBUG_LOG("Failed to fetch the lookup information of the codecid %08X \n"
            , adiecmd.nCodecId);
         return ACDB_INPUT_PARAMS_NOT_FOUND;
      }
      if (pInput->nBufferLength < sizeof(uint32_t))
         return ACDB_INSUFFICIENTMEMORY;

      ACDB_MEM_CPY(pInput->nBufferPointer, sizeof(uint32_t), &lutTbl.pLut[index].nPID, sizeof(uint32_t));
      pOut->nBytesUsedInBuffer = sizeof(uint32_t);

      if (ACDB_SUCCESS != result)
      {
         return result;
      }
      result = ACDB_SUCCESS;
   }
   return result;
}
int32_t AcdbCmdGetGroupDeviceListInfo(uint32_t queryType, uint8_t *pOut)
{
   int32_t result = ACDB_SUCCESS;
   AcdbGroupDeviceListRspType *pout = NULL;
   AcdbSizeResponseType* p = NULL;
   if (pOut == NULL)
   {
      ACDB_DEBUG_LOG("[ACDB Command]->[AcdbCmdGetAANCTblCmnInfo]->Invalid NULL value parameters are provided\n");
      result = ACDB_BADPARM;
   }
   else
   {
      uint32_t i = 0;
      int32_t dstOffset = 0, devicecount = 0;
      AcdbDevices *pDevs = (AcdbDevices *)ACDB_MALLOC(sizeof(AcdbDevices));
      if (pDevs == NULL)
      {
         ACDB_DEBUG_LOG("ACDB_COMMAND: Unable to allocate memory for AcdbDevices\n");
         return ACDB_INSUFFICIENTMEMORY;
      }

      memset((void*)pDevs, 0, sizeof(AcdbDevices));

      result = acdbdata_ioctl(ACDBDATACMD_GET_DEVICE_LIST, (uint8_t *)pDevs, sizeof(AcdbDevices),
         (uint8_t *)NULL, 0);
      if (result != ACDB_SUCCESS)
      {
         ACDB_DEBUG_LOG("ACDB_COMMAND: Failed to fetch the devicelist to fill the device capabilities info\n");
         ACDB_MEM_FREE(pDevs);
         return result;
      }
      switch (queryType)
      {
      case DATA_CMD:
      {
         pout = (AcdbGroupDeviceListRspType *)pOut;
         pout->NoOfentries = 0;
      }
      break;
      case DATA_SIZE_CMD:
      {
         p = (AcdbSizeResponseType *)pOut;
         p->nSize = 0;
      }
      break;

      default:
         if (pDevs != NULL)
            ACDB_MEM_FREE(pDevs);
         return ACDB_ERROR;
      }

      for (i = 0; i < pDevs->noOfDevs; i++)
      {
         AcdbDevPropInfo dpCmnInfo = { 0, 0, { 0, NULL } };

         dpCmnInfo.devId = pDevs->devList[i];
         dpCmnInfo.pId = CODEC_GROUP_DEVICE_LIST;
         result = acdbdata_ioctl(ACDBDATACMD_GET_DEVICE_PROP, (uint8_t *)&dpCmnInfo, sizeof(AcdbDevPropInfo),
            (uint8_t *)NULL, 0);
         if (result == ACDB_SUCCESS)
         {
            uint32_t value = (uint32_t)(*dpCmnInfo.dataInfo.pData);
            if (value != GROUP_DEVICE_ATTRIBUTE_VALUE)
               continue;
            devicecount++;

            if (queryType == DATA_CMD)
            {
               pout = (AcdbGroupDeviceListRspType *)pOut;
               ACDB_MEM_CPY((void*)(pout->Payload + dstOffset), sizeof(uint32_t), (void*)&pDevs->devList[i], sizeof(uint32_t));
               dstOffset += (int32_t)sizeof(uint32_t);
               pout->NoOfentries++;
            }
         }
      }
      if (queryType == DATA_SIZE_CMD)
      {
         p = (AcdbSizeResponseType *)pOut;
         p->nSize = (uint32_t)(devicecount*sizeof(uint32_t));
      }
      ACDB_MEM_FREE(pDevs);
   }

   return ACDB_SUCCESS;
}

int32_t AcdbCmdGetTdmMultiLaneConfigData(uint32_t queryType, AcdbTdmLaneConfigQueryCmdType *pInput, uint8_t *pOut)
{
   int32_t result = ACDB_SUCCESS;
   AcdbQueryResponseType *pout = NULL;
   AcdbSizeResponseType* p = NULL;
   uint8_t isFound = 0;
   if (pOut == NULL || pInput == NULL)
   {
      ACDB_DEBUG_LOG("[ACDB Command]->[AcdbCmdGetAANCTblCmnInfo]->Invalid NULL value parameters are provided\n");
      result = ACDB_BADPARM;
   }
   else
   {
	   AcdbGlbalPropInfo glbPropInfo = { 0, { 0, NULL } };
	   uint32_t count = 0,offset = 0;
	   uint32_t i = 0;
      memset(&glbPropInfo, 0, sizeof(glbPropInfo));
      glbPropInfo.pId = TDM_MULTI_LANE_CONFIG;
      result = acdbdata_ioctl(ACDBDATACMD_GET_GLOBAL_PROP, (uint8_t *)&glbPropInfo, sizeof(AcdbGlbalPropInfo),
         (uint8_t *)NULL, 0);
      if (result != ACDB_SUCCESS)
      {
         ACDB_DEBUG_LOG("ACDB_COMMAND: Failed to fetch the property info for pid %08X \n", glbPropInfo.pId);
         return ACDB_DATA_NOT_FOUND;
      }
      if (NULL == glbPropInfo.dataInfo.pData)
      {
         ACDB_DEBUG_LOG("ACDB_COMMAND: pData NULL on fetch the property info for pid %08X \n", glbPropInfo.pId);
         return ACDB_DATA_NOT_FOUND;
      }


		 count = READ_UInt32(glbPropInfo.dataInfo.pData);
		 offset = (uint32_t)sizeof(uint32_t);
		 isFound = 0;
		 for(i=0;i<count;i++)
		 {
			 uint32_t groupID = READ_UInt16(glbPropInfo.dataInfo.pData + offset);
			 offset += (uint32_t)sizeof(uint16_t);
			 if(groupID == pInput->nGroupID)
			 {
				 isFound = 1;
				 break;
			 }
			 else
			 {
				 offset += (uint32_t)sizeof(uint16_t); //enable
				 offset += (uint32_t)sizeof(uint16_t); //lane mask
			 }
		 }
		 if(isFound == 0)
		 {
			 ACDB_DEBUG_LOG("ACDB_COMMAND: Cannot find the group ID in the multi lane config property data. Group id %08X \n", pInput->nGroupID);
			 return ACDB_DATA_NOT_FOUND;
		 }
      
      
      switch (queryType)
      {
      case DATA_CMD:
		  {
			  pout = (AcdbQueryResponseType *)pOut;
			  ACDB_MEM_CPY(pInput->pBufferPointer, pInput->nBufferLength, glbPropInfo.dataInfo.pData+offset, sizeof(uint16_t)+sizeof(uint16_t));
			  pout->nBytesUsedInBuffer = sizeof(uint16_t)+sizeof(uint16_t);
		  }
      break;
      case DATA_SIZE_CMD:
      {
         p = (AcdbSizeResponseType *)pOut;
         p->nSize = sizeof(uint16_t)+sizeof(uint16_t); //enable + lane mask.
      }
      break;

      default:
         return ACDB_ERROR;
      }
   }

   return ACDB_SUCCESS;

}
