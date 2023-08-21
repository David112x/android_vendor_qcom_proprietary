/*===========================================================================
FILE: acdb_override.c

OVERVIEW: This file contains the implementaion of the heap optimization
API and functions
DEPENDENCIES: None

Copyright (c) 2018 Qualcomm Technologies, Inc.
All Rights Reserved.
Confidential and Proprietary - Qualcomm Technologies, Inc.
========================================================================== */

/*===========================================================================
EDIT HISTORY FOR MODULE

This section contains comments describing changes made to the module.
Notice that changes are listed in reverse chronological order. Please
use ISO format for dates.

$Header: //source/qcom/qct/multimedia2/Audio/audcal4/acdb_sw/dev/ADSP_instanceID/acdb/src/acdb_instance_override.c#2 $

when who what, where, why
---------- --- -----------------------------------------------------
2014-05-28 mh SW migration from 32-bit to 64-bit architecture
2014-02-14 avi Support ACDB persistence.
2013-06-07 avi Support Voice Volume boost feature
2010-07-23 ernanl Initial implementation of the Acdb_DM_Ioctl API and
associated helper methods.
========================================================================== */

#include "acdb.h"
#include "acdb_init_utility.h"
#include "acdb_private.h"
#include "acdb_linked_list.h"
#include "acdb_datainfo.h"
#include "acdb_instance_override.h"
#include "acdb_new_linked_list.h"
#include "acdb_data_mgr.h"
#include "acdb_utility.h"
/* ---------------------------------------------------------------------------
* Preprocessor Definitions and Constants
*--------------------------------------------------------------------------- */

/* ---------------------------------------------------------------------------
* Type Declarations
*--------------------------------------------------------------------------- */

#define UNREFERENCED_VAR(param)

/* ---------------------------------------------------------------------------
* Global Data Definitions
*--------------------------------------------------------------------------- */
static AcdbInstanceAudioTblNodeType *g_pTbl = NULL;
static AcdbInstanceVoiceTblNodeType *g_pVoiceTblInstance = NULL;
static AcdbDynamicDataNodeType *g_pData = NULL;
static AcdbDynamicAdieTblNodeType *g_pAdieTbl = NULL;
static AcdbDynamicDeltaFileDataTypeV2 *g_pInstanceDeltaData[ACDB_MAX_ACDB_FILES] = { NULL };

/* ---------------------------------------------------------------------------
* Static Variable Definitions
*--------------------------------------------------------------------------- */

/* ---------------------------------------------------------------------------
* Static Function Declarations and Definitions
*--------------------------------------------------------------------------- */

int32_t Acdb_SetInstanceDataCal(uint32_t persistData,
   uint32_t fileIndex,
   AcdbDataLookupKeyType *pKey,
   uintptr_t cdft,
   uint32_t *pParamId,
   const uint8_t *pFileDataBuf,
   const uint32_t nFileDataLen,
   uint8_t *pInputBufPtr,
   uint32_t InputBufLen,
   uint8_t *pIndices,
   uint32_t nIndexCount
   )
{
   int32_t result = ACDB_SUCCESS;
   int32_t deltaResult = ACDB_SUCCESS;
   uint32_t nIdxCount = 0;
   uint32_t nCdftCount = 0;
   uint32_t ncmdCount = 0;
   uint32_t nonModuleTblFound = 0;

   if (pKey != NULL && cdft != (uintptr_t)NULL && pParamId != NULL
      && pInputBufPtr != NULL && InputBufLen != 0
      && pFileDataBuf != NULL) //nFileDataLen can be Zero for VP3
   {
      if (Get_table_indices_count(pKey->nTableId, &nIdxCount, &nonModuleTblFound, &nCdftCount,&ncmdCount) != ACDB_SUCCESS)
      {
         return ACDB_ERROR;
      }

      if ((InputBufLen != nFileDataLen) ||
         (memcmp(pInputBufPtr, pFileDataBuf, nFileDataLen)))
      {
         result = ACDB_PARMNOTFOUND;
      }
      if (result == ACDB_SUCCESS)
      {//If data node exist on default,check table node on heap
         AcdbInstanceAudioTblType *pTblNode = NULL;
         result = FindAudioTableNodeOnHeap((AcdbDataLookupKeyType*)pKey,
            (AcdbInstanceAudioTblNodeType*)g_pTbl,
            (AcdbInstanceAudioTblType**)&pTblNode
            );

         if (result == ACDB_SUCCESS)
         {
            AcdbInstanceTopologyType *pTopNode = NULL;
            result = FindAudioTopologyNodeOnHeap(cdft,
               (uint32_t*)pParamId,
               (AcdbInstanceAudioTblType*)pTblNode,
               (AcdbInstanceTopologyType**)&pTopNode
               );
            if (result == ACDB_SUCCESS)
            {//Free Topology Node
               uint32_t fReeTblResult = ACDB_HEAP_NOTFREE_NODE;
               result = FreeAudioTopologyNode(cdft,
                  (uint32_t*)pParamId,
                  (AcdbInstanceAudioTblType*)pTblNode,
                  (uint32_t*)&fReeTblResult
                  );

               if (persistData == TRUE &&
                  ACDB_UTILITY_INIT_SUCCESS == AcdbIsPersistenceSupported())
               {
                  if (fileIndex < ACDB_MAX_ACDB_FILES)
                  {
                     // remove delta data node as well.
                     uint32_t mid = 0, iid = 0;

                     AcdbDeltaDataKeyTypeV2 nKeyToCheck = { 0, 0, NULL };
                     if (nCdftCount == NON_INSTANCE_CDFT_INDICES_COUNT)
                     {
                        if (ACDB_SUCCESS != GetMidFromIndices(pIndices, nIdxCount, &mid))
                        {
                           ACDB_DEBUG_LOG("Not able to get MID and IID");
                           return ACDB_ERROR;
                        }
                        CreateDeltaDataKey(&nKeyToCheck, pKey->nTableId, nIndexCount, pIndices, mid, *pParamId);
                        deltaResult = FreeDeltaDataNodeV2(nKeyToCheck,
                           g_pInstanceDeltaData[fileIndex]);
                        if (deltaResult != ACDB_SUCCESS)
                        {
                           // error deleting node, should i return error or continue?
                           ACDB_DEBUG_LOG("[ACDB Override ERROR]->[Acdb_SetDataCal]-> Error deleting delta file node.");
                        }
                     }
                     else if (nCdftCount == INSTANCE_CDFT_INDICES_COUNT)
                     {
                        if (ACDB_SUCCESS != GetMidIidFromCdft((uint8_t *)cdft, &mid, &iid))
                        {
                           ACDB_DEBUG_LOG("Not able to get MID and IID");
                           return ACDB_ERROR;
                        }
                        CreateInstanceDeltaDataKey(&nKeyToCheck, pKey->nTableId, nIndexCount, pIndices, mid, *pParamId, iid);
                        deltaResult = FreeDeltaDataNodeV2(nKeyToCheck,
                           g_pInstanceDeltaData[fileIndex]);
                        if (deltaResult != ACDB_SUCCESS)
                        {
                           // error deleting node, should i return error or continue?
                           ACDB_DEBUG_LOG("[ACDB Override ERROR]->[Acdb_SetDataCal]-> Error deleting delta file node.");
                        }
                     }
                  }
               }

               if (fReeTblResult == ACDB_HEAP_FREE_NODE)
               {//Free Table Node if topology node no longer exist on table node
                  result = FreeAudioTableNode((AcdbDataLookupKeyType*)pKey,
                     (AcdbInstanceAudioTblNodeType*)g_pTbl
                     );
               }
            }
            if (result == ACDB_SUCCESS)
            {
               result = FreeDataNode((uint32_t*)pParamId,
                  (AcdbDynamicDataNodeType*)g_pData
                  );
            }
         }
         if (result == ACDB_PARMNOTFOUND)
         {
            result = ACDB_SUCCESS;
         }
      }
      else if (result == ACDB_PARMNOTFOUND)
      {//Data not member of static data
         AcdbDynamicUniqueDataType* pDataNode = NULL;
         uint32_t dataType = ACDB_HEAP_DATA_FOUND;

         result = IsDataOnHeap((uint32_t*)pParamId,
            (uint8_t *)pInputBufPtr,
            (uint32_t)InputBufLen,
            (AcdbDynamicDataNodeType*)g_pData
            );
         if (result == ACDB_PARMNOTFOUND)
         {
            result = CreateDataNodeOnHeap((uint32_t*)pParamId,
               (uint8_t *)pInputBufPtr,
               (uint32_t)InputBufLen,
               (AcdbDynamicDataNodeType*)g_pData,
               (AcdbDynamicUniqueDataType**)&pDataNode
               );
            dataType = ACDB_HEAP_DATA_NOT_FOUND;
         }//Data Node not found, create data node and return its address
         else if (result == ACDB_SUCCESS)
         {
            result = FindDataNodeOnHeap((uint32_t*)pParamId,
               (uint8_t *)pInputBufPtr,
               (uint32_t)InputBufLen,
               (AcdbDynamicDataNodeType*)g_pData,
               (AcdbDynamicUniqueDataType**)&pDataNode
               );
         }//if Data node found, find data node ptr address
         if (result == ACDB_SUCCESS)
         {
            AcdbInstanceAudioTblType *pTblNode = NULL;
            result = FindAudioTableNodeOnHeap((AcdbDataLookupKeyType*)pKey,
               (AcdbInstanceAudioTblNodeType*)g_pTbl,
               (AcdbInstanceAudioTblType**)&pTblNode
               );
            if (result == ACDB_PARMNOTFOUND)
            {
               result = CreateAudioTableNodeOnHeap((AcdbDataLookupKeyType*)pKey,
                  (AcdbInstanceAudioTblNodeType*)g_pTbl,
                  (AcdbInstanceAudioTblType**)&pTblNode
                  );
            }//table not created, create table node.
            if (result == ACDB_SUCCESS)
            {
               AcdbInstanceTopologyType *pTopNode = NULL;
               result = FindAudioTopologyNodeOnHeap(cdft,
                  (uint32_t*)pParamId,
                  (AcdbInstanceAudioTblType*)pTblNode,
                  (AcdbInstanceTopologyType**)&pTopNode
                  );
               if (result == ACDB_SUCCESS)
               {
                  if (dataType == ACDB_HEAP_DATA_NOT_FOUND || pTopNode->pDataNode != pDataNode)
                  {
                     //Condition to decrement refcount
                     //1. check if found data node is different than what has already stored in the organization
                     //2. a new data node was created
                     //Decrease reference count from previous data node
                     pTopNode->pDataNode->refcount--;
                     //if data node reference = 0, free the data node
                     if (pTopNode->pDataNode->refcount == 0)
                     {
                        result = FreeDataNode((uint32_t*)pParamId,
                           (AcdbDynamicDataNodeType*)g_pData
                           );
                     }
                     //Link to new added data node
                     pTopNode->pDataNode = pDataNode;
                     if (pTopNode->pDataNode != NULL)
                     {
                        pTopNode->pDataNode->refcount++;
                     }

                     // update delta data node reference.
                     if (persistData == TRUE &&
                        ACDB_UTILITY_INIT_SUCCESS == AcdbIsPersistenceSupported() &&
                        fileIndex < ACDB_MAX_ACDB_FILES)
                     {
                        uint32_t mid = 0, iid = 0;
                        AcdbDeltaDataKeyTypeV2 nKeyToFind = { 0, 0, NULL };
                        if (nCdftCount == NON_INSTANCE_CDFT_INDICES_COUNT)
                        {
                           if (ACDB_SUCCESS != GetMidFromIndices(pIndices, nIdxCount, &mid))
                           {
                              ACDB_DEBUG_LOG("Not able to get MID and IID");
                              return ACDB_ERROR;
                           }
                           CreateDeltaDataKey(&nKeyToFind, pKey->nTableId, nIndexCount, pIndices, mid, *pParamId);
                           deltaResult = UpdateDeltaDataNodeOnHeapV2(nKeyToFind,
                              (AcdbDynamicUniqueDataType*)pDataNode,
                              g_pInstanceDeltaData[fileIndex]
                              );
                        }
                        else if (nCdftCount == INSTANCE_CDFT_INDICES_COUNT)
                        {
                           if (ACDB_SUCCESS != GetMidIidFromCdft((uint8_t *)cdft, &mid, &iid))
                           {
                              ACDB_DEBUG_LOG("Not able to get MID and IID");
                              return ACDB_ERROR;
                           }
                           CreateInstanceDeltaDataKey(&nKeyToFind, pKey->nTableId, nIndexCount, pIndices, mid, *pParamId, iid);
                           deltaResult = UpdateDeltaDataNodeOnHeapV2(nKeyToFind,
                              (AcdbDynamicUniqueDataType*)pDataNode,
                              g_pInstanceDeltaData[fileIndex]
                              );
                        }
                        if (deltaResult != ACDB_SUCCESS)
                        {
                           // error updating node, should i return error or continue?
                           ACDB_DEBUG_LOG("[ACDB Override ERROR]->[Acdb_SetInstanceDataCal]-> Error updating delta file node on heap.");
                        }
                     }
                  }
               }
               else if (result == ACDB_PARMNOTFOUND)
               {
                  result = CreateAudioTopologyNodeOnHeap(cdft,
                     (uint32_t*)pParamId,
                     (AcdbDynamicUniqueDataType*)pDataNode,
                     (AcdbInstanceAudioTblType*)pTblNode
                     );

                  // create delta data node as well.
                  if (persistData == TRUE &&
                     ACDB_UTILITY_INIT_SUCCESS == AcdbIsPersistenceSupported() &&
                     fileIndex < ACDB_MAX_ACDB_FILES)
				  {
					  uint32_t mid = 0, iid = 0;
					  AcdbDeltaDataKeyTypeV2 *pKeyToCreate = (AcdbDeltaDataKeyTypeV2 *)ACDB_MALLOC(sizeof(AcdbDeltaDataKeyTypeV2));
					  if(pKeyToCreate == NULL)
					  {
						  ACDB_DEBUG_LOG("ACDB_INSTANCE_OVERRIDE:Unable to allocate memory for pKeyToCreate\n");
						  return ACDB_INSUFFICIENTMEMORY;
					  }

					  pKeyToCreate->pIndices = NULL;

					  if (nCdftCount == NON_INSTANCE_CDFT_INDICES_COUNT)
					  {
						  if (ACDB_SUCCESS != GetMidFromIndices(pIndices, nIdxCount, &mid))
						  {
							  ACDB_MEM_FREE(pKeyToCreate);
							  ACDB_DEBUG_LOG("Not able to get MID and IID");
							  return ACDB_ERROR;
						  }

						  result = CreateDeltaDataKey(pKeyToCreate, pKey->nTableId, nIndexCount, pIndices, mid, *pParamId);
						  if(result != ACDB_SUCCESS)
						  {
							  ACDB_DEBUG_LOG("ACDB_INSTANCE_OVERRIDE: Unable to create delta data key \n");
							  ACDB_MEM_FREE(pKeyToCreate);
							  return result;
						  }

						  deltaResult = CreateDeltaDataNodeOnHeapV2(pKeyToCreate,
							  (AcdbDynamicUniqueDataType*)pDataNode,
							  &g_pInstanceDeltaData[fileIndex]
						  );
					  }
					  else if (nCdftCount == INSTANCE_CDFT_INDICES_COUNT)
					  {
						  if (ACDB_SUCCESS != GetMidIidFromCdft((uint8_t *)cdft, &mid, &iid))
						  {
							  ACDB_MEM_FREE(pKeyToCreate);
							  ACDB_DEBUG_LOG("Not able to get MID and IID");
							  return ACDB_ERROR;
						  }

						  result = CreateInstanceDeltaDataKey(pKeyToCreate, pKey->nTableId, nIndexCount, pIndices, mid, *pParamId, iid);
						  if(result != ACDB_SUCCESS)
						  {
							  ACDB_DEBUG_LOG("ACDB_INSTANCE_OVERRIDE: Unable to create instance delta data key \n");
							  ACDB_MEM_FREE(pKeyToCreate);
							  return result;
						  }
						  deltaResult = CreateDeltaDataNodeOnHeapV2(pKeyToCreate,
							  (AcdbDynamicUniqueDataType*)pDataNode,
							  &g_pInstanceDeltaData[fileIndex]
						  );
					  }
					  if (deltaResult != ACDB_SUCCESS)
					  {
						  // error adding node, should i return error or continue?
						  ACDB_DEBUG_LOG("[ACDB Override ERROR]->[Acdb_SetInstanceDataCal]-> Error creating delta file node on heap.");
					  }

					  if(pKeyToCreate != NULL)
					  {
						  if(pKeyToCreate->pIndices != NULL)
						  {
							  ACDB_MEM_FREE(pKeyToCreate->pIndices);
							  pKeyToCreate->pIndices = NULL;
						  }

						  ACDB_MEM_FREE(pKeyToCreate);
						  pKeyToCreate = NULL;
					  }
				  }
               }

               //Create Topology node
            }//Create Table node
         }//Create Data node
      }
   }
   return result;
}

int32_t Acdb_GetInstanceDataCal(AcdbDataLookupKeyType *pKey,
   uintptr_t cdft,
   uint32_t *pParamId,
   AcdbDynamicUniqueDataType **ppDataNode
   )
{
   int32_t result = ACDB_BADPARM;

   if (pKey != NULL && cdft != (uintptr_t)NULL && pParamId && ppDataNode != NULL)
   {
      AcdbInstanceAudioTblType *pTblNode = NULL;
      result = FindAudioTableNodeOnHeap((AcdbDataLookupKeyType*)pKey,
         (AcdbInstanceAudioTblNodeType*)g_pTbl,
         (AcdbInstanceAudioTblType**)&pTblNode
         );
      if (result == ACDB_SUCCESS)
      {
         AcdbInstanceTopologyType *pTopNode = NULL;
         result = FindAudioTopologyNodeOnHeap(cdft,
            (uint32_t*)pParamId,
            (AcdbInstanceAudioTblType*)pTblNode,
            (AcdbInstanceTopologyType**)&pTopNode
            );
         if (result == ACDB_SUCCESS)
         {
            *ppDataNode = pTopNode->pDataNode;
         }
      }
   }
   else
   {
      ACDB_DEBUG_LOG("[ACDB Override ERROR]->[Acdb_GetInstanceDataCal]->NULL Input pointer");
   }
   return result;
}

int32_t Acdb_GetInstanceVoiceDataCal(AcdbDataLookupCVDKeyType *pKey,
   uintptr_t cdft,
   uint32_t *pParamId,
   AcdbDynamicUniqueDataType **ppDataNode
   )
{
   int32_t result = ACDB_BADPARM;

   if (pKey != NULL && cdft != (uintptr_t)NULL && pParamId && ppDataNode != NULL)
   {
      AcdbInstanceSecondaryKeyType *pSecondaryKeyNode = NULL;

      result = FindInstanceSecondaryKeyNodeFromCVDKeyOnHeap((AcdbDataLookupCVDKeyType*)pKey,
         (AcdbInstanceVoiceTblNodeType*)g_pVoiceTblInstance,
         (AcdbInstanceSecondaryKeyType**)&pSecondaryKeyNode
         );
      if (result == ACDB_SUCCESS)
	  {
		  uint32_t numCdftIndices = 0;
		  uint32_t numofTblIndices = 0;
		  uint32_t nonModuleTblFOund = 0;
		  uint32_t numOfCmdIndices = 0;
		  AcdbInstanceTopologyType *pTopNode = NULL;
		  if (ACDB_SUCCESS != Get_table_indices_count(pKey->nTableId,&numofTblIndices, &nonModuleTblFOund, &numCdftIndices, &numOfCmdIndices))
		  {
			  return ACDB_ERROR;
		  }	  
		  
		  result = FindVoiceTopologyNodeOnHeap(cdft,
			  numCdftIndices,
			  (uint32_t*)pParamId,
			  (AcdbInstanceSecondaryKeyType*)pSecondaryKeyNode,
			  (AcdbInstanceTopologyType**)&pTopNode
			  );
		  if (result == ACDB_SUCCESS)
		  {
			  *ppDataNode = pTopNode->pDataNode;
		  }
	  }
   }
   else
   {
      ACDB_DEBUG_LOG("[ACDB Override ERROR]->[Acdb_GetDataCal]->NULL Input pointer");
   }
   return result;
}

int32_t Acdb_SetInstanceVoiceDataCal(uint32_t persistData,
   uint32_t fileIndex,
   AcdbDataLookupCVDKeyType *pKey,
   uintptr_t cdft,
   uint32_t *pParamId,
   const uint8_t *pFileDataBuf,
   const uint32_t nFileDataLen,
   uint8_t *pInputBufPtr,
   uint32_t InputBufLen,
   uint8_t *pIndices,
   uint32_t nIndexCount
   )
{
   int32_t result = ACDB_SUCCESS;
   int32_t deltaResult = ACDB_SUCCESS;
   uint32_t nIdxCount = 0;
   uint32_t nCdftCount = 0;
   uint32_t ncmdCount = 0;
   uint32_t nonModuleTblFound = 0;

   if (pKey != NULL && cdft != (uintptr_t)NULL && pParamId != NULL
      && pInputBufPtr != NULL && InputBufLen != 0
      && pFileDataBuf != NULL) //nFileDataLen can be Zero for VP3
   {
      if (Get_table_indices_count(pKey->nTableId, &nIdxCount, &nonModuleTblFound, &nCdftCount,&ncmdCount) != ACDB_SUCCESS)
      {
         return ACDB_ERROR;
      }

      if ((InputBufLen != nFileDataLen) ||
         (memcmp(pInputBufPtr, pFileDataBuf, nFileDataLen)))
      {
         result = ACDB_PARMNOTFOUND;
      }
      if (result == ACDB_SUCCESS)
      {//If data node exist on default,check table node on heap
         AcdbInstanceSecondaryKeyType *pSecondaryKeyNode = NULL;
		 /*uint32_t inputData=0, fileData =0;
		 inputData = READ_UInt32(pInputBufPtr);
		 fileData = READ_UInt32((uint8_t *)pFileDataBuf);*/
         result = FindInstanceSecondaryKeyNodeFromCVDKeyOnHeap((AcdbDataLookupCVDKeyType*)pKey,
            (AcdbInstanceVoiceTblNodeType*)g_pVoiceTblInstance,
            (AcdbInstanceSecondaryKeyType**)&pSecondaryKeyNode
            );

         if (result == ACDB_SUCCESS)
		 {
			 uint32_t numCdftIndices = 0;
			 uint32_t numofTblIndices = 0;
			 uint32_t nonModuleTblFOund = 0;
			 uint32_t numOfCmdIndices = 0;
			 AcdbInstanceTopologyType *pTopNode = NULL;
			 if (ACDB_SUCCESS != Get_table_indices_count(pKey->nTableId,&numofTblIndices, &nonModuleTblFOund, &numCdftIndices, &numOfCmdIndices))
			 {
				 return ACDB_ERROR;
			 }
			 result = FindVoiceTopologyNodeOnHeap(cdft,
				 numCdftIndices,
				 (uint32_t*)pParamId,
				 (AcdbInstanceSecondaryKeyType*)pSecondaryKeyNode,
				 (AcdbInstanceTopologyType**)&pTopNode
				 );
			 if (result == ACDB_SUCCESS)
			 {//Free Topology Node
				 uint32_t fReeTblResult = ACDB_HEAP_NOTFREE_NODE;
				 result = FreeVoiceTopologyNode(cdft,
					 numCdftIndices,
					 (uint32_t*)pParamId,
					 (AcdbInstanceSecondaryKeyType*)pSecondaryKeyNode,
					 (uint32_t*)&fReeTblResult
					 );

				 if (persistData == TRUE &&
					 ACDB_UTILITY_INIT_SUCCESS == AcdbIsPersistenceSupported())
				 {
					 if (fileIndex < ACDB_MAX_ACDB_FILES)
					 {
						 // remove delta data node as well.
						 AcdbDeltaDataKeyTypeV2 nKeyToCheck = { 0, 0, NULL };
						 uint32_t mid = 0, iid = 0;
						 if (nCdftCount == NON_INSTANCE_CDFT_INDICES_COUNT)
						 {
							 if (ACDB_SUCCESS != GetMidFromIndices(pIndices,nIndexCount ,&mid))
							 {
								 ACDB_DEBUG_LOG("Not able to get MID and IID");
								 return ACDB_ERROR;
							 }
							 CreateDeltaDataKey(&nKeyToCheck, pKey->nTableId, nIndexCount, pIndices, mid, *pParamId);
							 deltaResult = FreeDeltaDataNodeV2(nKeyToCheck,
								 g_pInstanceDeltaData[fileIndex]);
							 if (deltaResult != ACDB_SUCCESS)
							 {
								 // error deleting node, should i return error or continue?
								 ACDB_DEBUG_LOG("[ACDB Override ERROR]->[Acdb_SetDataCal]-> Error deleting delta file node.");
							 }
						 }
						 else if (nCdftCount == INSTANCE_CDFT_INDICES_COUNT)
						 {
							 if (ACDB_SUCCESS != GetMidIidFromCdft((uint8_t *)cdft,&mid, &iid))
							 {
								 ACDB_DEBUG_LOG("Not able to get MID and IID");
								 return ACDB_ERROR;
							 }
							 CreateInstanceDeltaDataKey(&nKeyToCheck, pKey->nTableId, nIndexCount, pIndices, mid, *pParamId, iid);
							 deltaResult = FreeDeltaDataNodeV2(nKeyToCheck,
								 g_pInstanceDeltaData[fileIndex]);
							 if (deltaResult != ACDB_SUCCESS)
							 {
								 // error deleting node, should i return error or continue?
								 ACDB_DEBUG_LOG("[ACDB Override ERROR]->[Acdb_SetDataCal]-> Error deleting delta file node.");
							 }
						 }
					 }
				 }

				 if (fReeTblResult == ACDB_HEAP_FREE_NODE)
				 {//Free Table Node if topology node no longer exist on table node
					 result = FreeVoiceTableNode((AcdbDataLookupCVDKeyType*)pKey,
						 (AcdbInstanceVoiceTblNodeType*)g_pVoiceTblInstance
						 );
				 }
			 }
			 if (result == ACDB_SUCCESS)
			 {
				 result = FreeDataNode((uint32_t*)pParamId,
					 (AcdbDynamicDataNodeType*)g_pData
					 );
			 }
		 }
         if (result == ACDB_PARMNOTFOUND)
         {
            result = ACDB_SUCCESS;
         }
      }
      else if (result == ACDB_PARMNOTFOUND)
      {//Data not member of static data
         AcdbDynamicUniqueDataType* pDataNode = NULL;
         uint32_t dataType = ACDB_HEAP_DATA_FOUND;

         result = IsDataOnHeap((uint32_t*)pParamId,
            (uint8_t *)pInputBufPtr,
            (uint32_t)InputBufLen,
            (AcdbDynamicDataNodeType*)g_pData
            );
         if (result == ACDB_PARMNOTFOUND)
         {
            result = CreateDataNodeOnHeap((uint32_t*)pParamId,
               (uint8_t *)pInputBufPtr,
               (uint32_t)InputBufLen,
               (AcdbDynamicDataNodeType*)g_pData,
               (AcdbDynamicUniqueDataType**)&pDataNode
               );
            dataType = ACDB_HEAP_DATA_NOT_FOUND;
         }//Data Node not found, create data node and return its address
         else if (result == ACDB_SUCCESS)
         {
            result = FindDataNodeOnHeap((uint32_t*)pParamId,
               (uint8_t *)pInputBufPtr,
               (uint32_t)InputBufLen,
               (AcdbDynamicDataNodeType*)g_pData,
               (AcdbDynamicUniqueDataType**)&pDataNode
               );
         }//if Data node found, find data node ptr address
         if (result == ACDB_SUCCESS && pDataNode != NULL)
         {
            AcdbInstanceSecondaryKeyType *pSecondaryKeyNode = NULL;
            result = GetInstanceSecondaryNodeFromCVDKeyOnHeap((AcdbDataLookupCVDKeyType*)pKey,
               (AcdbInstanceVoiceTblNodeType*)g_pVoiceTblInstance,
               (AcdbInstanceSecondaryKeyType**)&pSecondaryKeyNode
               );
            if (result == ACDB_SUCCESS)
            {
               AcdbInstanceTopologyType *pTopNode = NULL;
			   uint32_t numCdftIndices = 0;
				uint32_t numofTblIndices = 0;
				uint32_t nonModuleTblFOund = 0;
				uint32_t numOfCmdIndices = 0;
				if (ACDB_SUCCESS != Get_table_indices_count(pKey->nTableId,&numofTblIndices, &nonModuleTblFOund, &numCdftIndices, &numOfCmdIndices))
				{
					return ACDB_ERROR;
				}
               result = FindVoiceTopologyNodeOnHeap(cdft,
				   numCdftIndices,
                  (uint32_t*)pParamId,
                  (AcdbInstanceSecondaryKeyType*)pSecondaryKeyNode,
                  (AcdbInstanceTopologyType**)&pTopNode
                  );
               if (result == ACDB_SUCCESS)
               {
                  if (dataType == ACDB_HEAP_DATA_NOT_FOUND || pTopNode->pDataNode != pDataNode)
                  {
                     //Condition to decrement refcount
                     //1. check if found data node is different than what has already stored in the organization
                     //2. a new data node was created
                     //Decrease reference count from previous data node
                     pTopNode->pDataNode->refcount--;
                     //if data node reference = 0, free the data node
                     if (pTopNode->pDataNode->refcount == 0)
                     {
                        result = FreeDataNode((uint32_t*)pParamId,
                           (AcdbDynamicDataNodeType*)g_pData
                           );
                     }
                     //Link to new added data node
                     pTopNode->pDataNode = pDataNode;
                     if (pTopNode->pDataNode != NULL)
                     {
                        pTopNode->pDataNode->refcount++;
                     }

                     // update delta data node reference.
                     if (persistData == TRUE &&
                        ACDB_UTILITY_INIT_SUCCESS == AcdbIsPersistenceSupported() &&
                        fileIndex < ACDB_MAX_ACDB_FILES)
                     {
                        AcdbDeltaDataKeyTypeV2 nKeyToFind = { 0, 0, NULL };
                        uint32_t mid = 0, iid = 0;
                        if (nCdftCount == NON_INSTANCE_CDFT_INDICES_COUNT)
                        {
                           if (ACDB_SUCCESS != GetMidFromIndices(pIndices,nIndexCount, &mid))
                           {
                              ACDB_DEBUG_LOG("Not able to get MID and IID");
                              return ACDB_ERROR;
                           }
                           CreateDeltaDataKey(&nKeyToFind, pKey->nTableId, nIndexCount, pIndices, mid, *pParamId);
                           deltaResult = UpdateDeltaDataNodeOnHeapV2(nKeyToFind,
                              (AcdbDynamicUniqueDataType*)pDataNode,
                              g_pInstanceDeltaData[fileIndex]
                              );
                        }
                        else if (nCdftCount == INSTANCE_CDFT_INDICES_COUNT)
                        {
                           if (ACDB_SUCCESS != GetMidIidFromCdft((uint8_t *)cdft,&mid, &iid))
                           {
                              ACDB_DEBUG_LOG("Not able to get MID and IID");
                              return ACDB_ERROR;
                           }
                           CreateInstanceDeltaDataKey(&nKeyToFind, pKey->nTableId, nIndexCount, pIndices, mid, *pParamId, iid);
                           deltaResult = UpdateDeltaDataNodeOnHeapV2(nKeyToFind,
                              (AcdbDynamicUniqueDataType*)pDataNode,
                              g_pInstanceDeltaData[fileIndex]
                              );
                        }

                        if (deltaResult != ACDB_SUCCESS)
                        {
                           // error updating node, should i return error or continue?
                           ACDB_DEBUG_LOG("[ACDB Override ERROR]->[Acdb_SetDataCal]-> Error updating delta file node on heap.");
                        }
                     }
                  }
               }
               else if (result == ACDB_PARMNOTFOUND)
               {
                  result = CreateVoiceTopologyNodeOnHeap(cdft,
                     (uint32_t*)pParamId,
                     (AcdbDynamicUniqueDataType*)pDataNode,
                     (AcdbInstanceSecondaryKeyType*)pSecondaryKeyNode
                     );

                  // create delta data node as well.
                  if (persistData == TRUE &&
                     ACDB_UTILITY_INIT_SUCCESS == AcdbIsPersistenceSupported() &&
                     fileIndex < ACDB_MAX_ACDB_FILES)
                  {
                     uint32_t mid = 0, iid = 0;
                     AcdbDeltaDataKeyTypeV2 *pKeyToCreate = (AcdbDeltaDataKeyTypeV2 *)ACDB_MALLOC(sizeof(AcdbDeltaDataKeyTypeV2));
					 if(pKeyToCreate == NULL)
					 {
						ACDB_DEBUG_LOG("ACDB_INSTANCE_OVERRIDE:Unable to allocate memory for pKeyToCreate\n");
						 return ACDB_INSUFFICIENTMEMORY;
					 }
					 pKeyToCreate->pIndices = NULL;
                     if (nCdftCount == NON_INSTANCE_CDFT_INDICES_COUNT)
                     {
                        if (ACDB_SUCCESS != GetMidFromIndices(pIndices,nIndexCount, &mid))
                        {
						   ACDB_MEM_FREE(pKeyToCreate);
                           ACDB_DEBUG_LOG("Not able to get MID and IID");
                           return ACDB_ERROR;
                        }
						if(CreateDeltaDataKey(pKeyToCreate, pKey->nTableId, nIndexCount, pIndices, mid, *pParamId) != ACDB_INSUFFICIENTMEMORY)
						{
							deltaResult = CreateDeltaDataNodeOnHeapV2(pKeyToCreate,
                           (AcdbDynamicUniqueDataType*)pDataNode,
                           &g_pInstanceDeltaData[fileIndex]
                           );
						}
						else
						{
							ACDB_DEBUG_LOG("Not able to allocate memory for delta key");
							ACDB_MEM_FREE(pKeyToCreate);
							pKeyToCreate = NULL;
                           return ACDB_ERROR;
						}
                     }
                     else if (nCdftCount == INSTANCE_CDFT_INDICES_COUNT)
                     {
                        if (ACDB_SUCCESS != GetMidIidFromCdft((uint8_t *)cdft, &mid, &iid))
                        {
							ACDB_MEM_FREE(pKeyToCreate);
                           ACDB_DEBUG_LOG("Not able to get MID and IID");
                           return ACDB_ERROR;
                        }
                        if(CreateInstanceDeltaDataKey(pKeyToCreate, pKey->nTableId, nIndexCount, pIndices, mid, *pParamId, iid)!= ACDB_INSUFFICIENTMEMORY)
						{
							deltaResult = CreateDeltaDataNodeOnHeapV2(pKeyToCreate,
                           (AcdbDynamicUniqueDataType*)pDataNode,
                           &g_pInstanceDeltaData[fileIndex]
                           );
						}
						else
						{
							ACDB_DEBUG_LOG("Not able to allocate memory for instance delta key");
							ACDB_MEM_FREE(pKeyToCreate);
							return ACDB_ERROR;
						}
                     }
                     if (deltaResult != ACDB_SUCCESS)
                     {
                        // error adding node, should i return error or continue?
                        ACDB_DEBUG_LOG("[ACDB Override ERROR]->[Acdb_SetDataCal]-> Error creating delta file node on heap.");
                     }

					 if(pKeyToCreate != NULL)
					  {
						  if(pKeyToCreate->pIndices != NULL)
						  {
							  ACDB_MEM_FREE(pKeyToCreate->pIndices);
							  pKeyToCreate->pIndices = NULL;
						  }

						  ACDB_MEM_FREE(pKeyToCreate);
						  pKeyToCreate = NULL;
					  }
                  }
               }

               //Create Topology node
            }//Create Table node
         }//Create Data node
      }
   }
   return result;
}

int32_t Acdb_Voice_DM_Instance_Ioctl(uint32_t Acdb_DM_CMD_Id,
   uint32_t persistData,
   uint32_t fileIndex,
   AcdbDataLookupCVDKeyType *pKey,
   uintptr_t cdftLookup,
   uint32_t *pParamId,
   uint8_t *pInputBuf,
   const uint32_t InputBufLen,
   const uint8_t *pFileDataBuf,
   const uint32_t nFileDataLen,
   uint8_t *pOutBuff,
   const size_t nOutBuffLen,
   uint32_t *nOutBuffBytesUsed,
   AcdbDynamicUniqueDataType **ppDataNode,
   uint8_t* pIndices,
   uint32_t nIndexCount
   )
{
   int32_t result = ACDB_SUCCESS;
   // dummy code for compilation purposes.
   if (nOutBuffLen == 0 || pOutBuff == NULL)
   {
   }

   //UNREFERENCED_VAR(nOutBuffBytesUsed);
   if (nOutBuffBytesUsed != NULL)
      *nOutBuffBytesUsed = 0;

   //Global Variable Initialization
   if (g_pVoiceTblInstance == NULL)
   {
      g_pVoiceTblInstance = (AcdbInstanceVoiceTblNodeType*)ACDB_MALLOC(sizeof(AcdbInstanceVoiceTblNodeType));
      if (g_pVoiceTblInstance != NULL)
      {
         g_pVoiceTblInstance->pTblHead = NULL;
         g_pVoiceTblInstance->pTblTail = NULL;
      }
      else
      {
         result = ACDB_BADSTATE;
      }
   }
   if (g_pData == NULL)
   {
      g_pData = (AcdbDynamicDataNodeType*)ACDB_MALLOC(sizeof(AcdbDynamicDataNodeType));
      if (g_pData != NULL)
      {
         g_pData->pDatalHead = NULL;
         g_pData->pDatalTail = NULL;
      }
      else
      {
         result = ACDB_BADSTATE;
      }
   }

   if (g_pAdieTbl == NULL)
	{
		g_pAdieTbl = (AcdbDynamicAdieTblNodeType*)ACDB_MALLOC(sizeof(AcdbDynamicAdieTblNodeType));
		if (g_pAdieTbl != NULL)
		{
			g_pAdieTbl->pAdieTblHead = NULL;
			g_pAdieTbl->pAdieTblTail = NULL;
		}
		else
		{
			result = ACDB_BADSTATE;
		}
	}

   switch (Acdb_DM_CMD_Id)
   {
   case ACDB_GET_DATA:

      result = Acdb_GetInstanceVoiceDataCal((AcdbDataLookupCVDKeyType*)pKey,
         cdftLookup,
         (uint32_t*)pParamId,
         (AcdbDynamicUniqueDataType**)&(*ppDataNode)
         );
      break;

   case ACDB_SET_DATA:

      result = Acdb_SetInstanceVoiceDataCal(persistData,
         fileIndex,
         (AcdbDataLookupCVDKeyType*)pKey,
         cdftLookup,
         (uint32_t*)pParamId,
         (const uint8_t*)pFileDataBuf,
         nFileDataLen,
         (uint8_t*)pInputBuf,
         (uint32_t)InputBufLen,
         (uint8_t*)pIndices,
         (uint32_t)nIndexCount
         );

      break;
   }
   return result;
}

int32_t Acdb_GetInstanceTblEntries(uint8_t *pCmd, uint32_t nCmdSize, uint8_t *pRsp, size_t nRspSizse)
{
   int32_t result = ACDB_SUCCESS;
   AcdbQueryTblEntriesCmdType *pInput = (AcdbQueryTblEntriesCmdType *)pCmd;
   AcdbQueryResponseType *pOut = (AcdbQueryResponseType *)pRsp;
   uint32_t noOfTableIndices = 0;
   uint32_t curNoOfEntriesOffset = 0;
   uint32_t noOfEntriesCopied = 0;
   //uint32_t noOfEntriesRemaining=0;
   //uint32_t noOfBytesRemaining=pInput->nBuffSize;
   uint32_t noOfBytesReqPerEntry = 0;
   uint32_t offset = 0;
   uint8_t nMemExhausted = 0;
   uint32_t nonModuleTblFound = 0;
   uint32_t noOfCdftIndices = 0;
   uint32_t noOfCmdIndices = 0;
   //uint32_t nActualRemainingEntries=0;

   if (pCmd == NULL || nCmdSize != sizeof(AcdbQueryTblEntriesCmdType) ||
      pRsp == NULL || nRspSizse != sizeof(AcdbQueryResponseType) ||
      pInput->pBuff == NULL || pInput->nBuffSize < 4)
   {
      ACDB_DEBUG_LOG("Invalid input params provided to retrieve the data");
      return ACDB_ERROR;
   }

   result = Get_table_indices_count(pInput->nTblId, &noOfTableIndices, &nonModuleTblFound, &noOfCdftIndices,&noOfCmdIndices);
   if (result != ACDB_SUCCESS)
   {
	   ACDB_DEBUG_LOG("[ACPH Online]->[set_acdb_data]->Failed. Could not find number of indices of the table:[%u]\n", pInput->nTblId);
      return result;
   }

   offset = sizeof(noOfEntriesCopied);
   if (nonModuleTblFound == 0)
   {
      if (pInput->nTblId == VOCPROC_DYN_INST_TBL_ID || pInput->nTblId == VOCPROC_STAT_INST_TBL_ID || pInput->nTblId == VOCPROC_DYNAMIC_TBL || pInput->nTblId == VOCPROC_STATIC_TBL)
      {
         AcdbInstanceVoiceTblType *pCur = NULL;
         noOfBytesReqPerEntry = (noOfTableIndices + 2) * (uint32_t)sizeof(uint32_t); //2 is added to include cdftPtr and pid

         if (g_pVoiceTblInstance == NULL || g_pVoiceTblInstance->pTblHead == NULL)
         {
            ACDB_DEBUG_LOG("No entries are there on heap\n");
            return ACDB_ERROR;
         }
         pCur = g_pVoiceTblInstance->pTblHead;
         while (pCur != NULL)
         {
            if (pCur->pKey->nTableId == pInput->nTblId)
            {
               AcdbInstancePrimaryKeyType *pPrimaryKeyNode = pCur->pPrimaryKeyNode->pTblHead;
               while (pPrimaryKeyNode != NULL)
               {
                  AcdbInstanceSecondaryKeyType *pSecondaryKeyNode = pPrimaryKeyNode->pSecondaryNode->pTblHead;
                  while (pSecondaryKeyNode != NULL)
                  {
                     AcdbInstanceTopologyType *pCurTop = pSecondaryKeyNode->pTopologyNode->pTopHead;
                     while (pCurTop != NULL)
                     {
                        if (pInput->nTblEntriesOffset <= curNoOfEntriesOffset)
                        {
                           if (((pInput->nBuffSize - offset) >= noOfBytesReqPerEntry))
                           {
                              uint8_t* pLut = (uint8_t *)pPrimaryKeyNode->pKey->nLookupKey;
                              uint8_t* pSecLut = (uint8_t *)pSecondaryKeyNode->pKey->nSecondaryLookupKey;
							  uint8_t* pcdft = (uint8_t *) pCurTop->nCDFTLookupKey;
                              ACDB_MEM_CPY(pInput->pBuff + offset, ((noOfCmdIndices)*sizeof(uint32_t)), pSecLut, (noOfCmdIndices*sizeof(uint32_t)));
                              offset += (uint32_t)((noOfCmdIndices)*sizeof(uint32_t));
                              ACDB_MEM_CPY(pInput->pBuff + offset, ((size_t)(noOfTableIndices - noOfCmdIndices)*sizeof(uint32_t)), pLut, (size_t)(noOfTableIndices - noOfCmdIndices)*sizeof(uint32_t));
                              offset += (noOfTableIndices - noOfCmdIndices)*(uint32_t)sizeof(uint32_t);
							  ACDB_MEM_CPY(pInput->pBuff + offset, (noOfCdftIndices*sizeof(uint32_t)), pcdft, (noOfCdftIndices*sizeof(uint32_t)));
                              offset += (uint32_t)(noOfCdftIndices*sizeof(uint32_t));
                              ++noOfEntriesCopied;
                           }
                           else
                           {
                              nMemExhausted = 1;
                              break;
                           }
                        }
                        pCurTop = pCurTop->pNext;
                        ++curNoOfEntriesOffset;
                     }

                     pSecondaryKeyNode = pSecondaryKeyNode->pNext;
                  }

                  pPrimaryKeyNode = pPrimaryKeyNode->pNext;
               }
            }

            if (nMemExhausted == 1)
               break;
            pCur = pCur->pNext;
         }
      }
      else
      {
         AcdbInstanceAudioTblType *pCur = NULL;
		 noOfBytesReqPerEntry = (noOfTableIndices + noOfCdftIndices) * (uint32_t)sizeof(uint32_t);

         if (g_pTbl == NULL || g_pTbl->pTblHead == NULL)
         {
            ACDB_DEBUG_LOG("No entries are there on heap\n");
            return ACDB_ERROR;
         }
         pCur = g_pTbl->pTblHead;
         while (pCur != NULL)
         {
            if (pCur->pKey->nTableId == pInput->nTblId)
            {
               AcdbInstanceTopologyType *pCurTop = pCur->pTopologyNode->pTopHead;
               while (pCurTop != NULL)
               {
                  if (pInput->nTblEntriesOffset <= curNoOfEntriesOffset)
                  {
                     if (((pInput->nBuffSize - offset) >= noOfBytesReqPerEntry))
                     {
                        uint8_t* pLut = (uint8_t *)pCur->pKey->nLookupKey;
						uint8_t* pcdft = (uint8_t *) pCurTop->nCDFTLookupKey;
                        ACDB_MEM_CPY(pInput->pBuff + offset, (noOfTableIndices*sizeof(uint32_t)), pLut, (noOfTableIndices*sizeof(uint32_t)));
                        offset += (uint32_t)(noOfTableIndices*sizeof(uint32_t));
                        ACDB_MEM_CPY(pInput->pBuff + offset, (noOfCdftIndices*sizeof(uint32_t)), pcdft,(noOfCdftIndices*sizeof(uint32_t)));
                        offset += (uint32_t)(noOfCdftIndices*sizeof(uint32_t));
                        ++noOfEntriesCopied;
                     }
                     else
                     {
                        nMemExhausted = 1;
                        break;
                     }
                  }
                  pCurTop = pCurTop->pNext;
                  ++curNoOfEntriesOffset;
               }
            }
            if (nMemExhausted == 1)
               break;
            pCur = pCur->pNext;
         }
      }
   }
   else
   {
      AcdbDynamicAdieTblType *pCur = NULL;
      noOfBytesReqPerEntry = (uint32_t)(noOfTableIndices * sizeof(uint32_t)); //1 is added to include pid as well
      if (g_pAdieTbl == NULL || g_pAdieTbl->pAdieTblHead == NULL)
      {
         ACDB_DEBUG_LOG("No entries are there on heap\n");
         return ACDB_ERROR;
      }
      pCur = g_pAdieTbl->pAdieTblHead;
      while (pCur != NULL)
      {
         if (pCur->pKey->nTableId == pInput->nTblId)
         {
            //AcdbDynamicUniqueDataType *pCurData = pCur->pDataNode;
            //while(pCurData != NULL)
            //{
            // pCurData = pCurData->pNext;
            //}
            if (pInput->nTblEntriesOffset <= curNoOfEntriesOffset)
            {
               if ((pInput->nBuffSize - offset) >= noOfBytesReqPerEntry)
               {
                  uint8_t* pLut = (uint8_t *)pCur->pKey->nLookupKey;
                  ACDB_MEM_CPY(pInput->pBuff + offset, (noOfTableIndices*sizeof(uint32_t)), pLut, (noOfTableIndices*sizeof(uint32_t)));
                  offset += (uint32_t)(noOfTableIndices*sizeof(uint32_t));
                  //ACDB_MEM_CPY(pInput->pBuff+offset,&pCurTop->ulModuleId,sizeof(uint32_t));
                  //offset += sizeof(uint32_t);
                  //ACDB_MEM_CPY(pInput->pBuff+offset,&pCurTop->pDataNode->ulParamId,noOfTableIndices*sizeof(uint32_t));
                  //offset += sizeof(uint32_t);
                  ++noOfEntriesCopied;
               }
               else
               {
                  nMemExhausted = 1;
                  break;
               }
            }
            //pCurTop = pCurTop->pNext;
            ++curNoOfEntriesOffset;
         }
         pCur = pCur->pNext;
      }
   }

   //if(curNoOfEntriesOffset <= pInput->nTblEntriesOffset)
   //{
   // ACDB_DEBUG_LOG("Invalid offset request is provided to retrieve the table entries\n");
   // return ACDB_ERROR;
   //}
   //nActualRemainingEntries = curNoOfEntriesOffset - pInput->nTblEntriesOffset;
   //nActualRemainingEntries = (curNoOfEntriesOffset>pInput->nRequiredNoOfTblEntries)?pInput->nRequiredNoOfTblEntries:curNoOfEntriesOffset;
   //noOfEntriesRemaining = 0;
   //if(nActualRemainingEntries <= pInput->nRequiredNoOfTblEntries)
   // noOfEntriesRemaining = 0;
   //else
   // noOfEntriesRemaining = nActualRemainingEntries - pInput->nRequiredNoOfTblEntries;
   ACDB_MEM_CPY(pInput->pBuff, sizeof(noOfEntriesCopied), &noOfEntriesCopied, sizeof(noOfEntriesCopied));
   //ACDB_MEM_CPY(pInput->pBuff+sizeof(uint32_t),&noOfEntriesRemaining,sizeof(noOfEntriesRemaining));
   pOut->nBytesUsedInBuffer = offset;
   return result;
}

int32_t Acdb_Instance_sys_reset(void)
{
   int32_t result = ACDB_SUCCESS;

   //Free general table on Heap
   if (g_pTbl != NULL)
   {
      AcdbInstanceAudioTblType *pCur = NULL;
      pCur = g_pTbl->pTblHead;
      while (pCur != NULL)
      {
         AcdbInstanceTopologyType *pCurTop = NULL;
         if (pCur->pTopologyNode != NULL)
         {
            pCurTop = pCur->pTopologyNode->pTopHead;
            while (pCurTop)
            {
               pCur->pTopologyNode->pTopHead = pCurTop->pNext;
               ACDB_MEM_FREE(pCurTop);
               pCurTop = pCur->pTopologyNode->pTopHead;
            }
            if (pCurTop != NULL)
            {
               ACDB_MEM_FREE(pCur->pTopologyNode->pTopHead);
            }
         }
         g_pTbl->pTblHead = pCur->pNext;
         ACDB_MEM_FREE(pCur->pKey);
         ACDB_MEM_FREE(pCur->pTopologyNode);
         ACDB_MEM_FREE(pCur);
         pCur = g_pTbl->pTblHead;
         if (pCur == NULL)
         {
            g_pTbl->pTblTail = NULL;
         }
      }
      ACDB_MEM_FREE(g_pTbl);
      g_pTbl = NULL;
   }

   if (g_pVoiceTblInstance != NULL)
   {
      AcdbInstanceVoiceTblType *pCur = NULL;
      pCur = g_pVoiceTblInstance->pTblHead;
      while (pCur != NULL)
      {
         if (pCur->pPrimaryKeyNode != NULL)
         {
            AcdbInstancePrimaryKeyType *pPrimaryCur = NULL;
            pPrimaryCur = pCur->pPrimaryKeyNode->pTblHead;

            while (pPrimaryCur != NULL)
            {
               if (pPrimaryCur->pSecondaryNode != NULL)
               {
                  AcdbInstanceSecondaryKeyType *pSecondaryCur = pPrimaryCur->pSecondaryNode->pTblHead;

                  while (pSecondaryCur != NULL)
                  {
                     if (pSecondaryCur->pTopologyNode != NULL)
                     {
                        AcdbInstanceTopologyType *pCurTop = NULL;
                        pCurTop = pSecondaryCur->pTopologyNode->pTopHead;
                        while (pCurTop)
                        {
                           pSecondaryCur->pTopologyNode->pTopHead = pCurTop->pNext;
                           ACDB_MEM_FREE(pCurTop);
                           pCurTop = pSecondaryCur->pTopologyNode->pTopHead;
                        }
                        if (pCurTop != NULL)
                        {
                           ACDB_MEM_FREE(pSecondaryCur->pTopologyNode->pTopHead);
                        }
                     }

                     pPrimaryCur->pSecondaryNode->pTblHead = pSecondaryCur->pNext;
                     ACDB_MEM_FREE(pSecondaryCur->pKey);
                     ACDB_MEM_FREE(pSecondaryCur);
                     pSecondaryCur = pPrimaryCur->pSecondaryNode->pTblHead;
                  }

                  if (pSecondaryCur != NULL)
                  {
                     ACDB_MEM_FREE(pPrimaryCur->pSecondaryNode->pTblHead);
                  }
               }

               pCur->pPrimaryKeyNode->pTblHead = pPrimaryCur->pNext;
               ACDB_MEM_FREE(pPrimaryCur->pKey);
               ACDB_MEM_FREE(pPrimaryCur);
               pPrimaryCur = pCur->pPrimaryKeyNode->pTblHead;
            }

            if (pPrimaryCur != NULL)
            {
               ACDB_MEM_FREE(pCur->pPrimaryKeyNode->pTblHead);
            }

            g_pVoiceTblInstance->pTblHead = pCur->pNext;
            ACDB_MEM_FREE(pCur->pKey);
            ACDB_MEM_FREE(pCur);
            pCur = g_pVoiceTblInstance->pTblHead;
            if (pCur == NULL)
            {
               g_pVoiceTblInstance->pTblTail = NULL;
            }
         }
      }
      ACDB_MEM_FREE(g_pVoiceTblInstance);
      g_pVoiceTblInstance = NULL;
   }
   // Free delta meta data from Heap.
   {
      int32_t idx = 0;
      for (idx = 0; idx < ACDB_MAX_ACDB_FILES; idx++)
      {
         AcdbDynamicDeltaFileDataTypeV2 *pDeltaData = g_pInstanceDeltaData[idx];
         if (pDeltaData != NULL)
         {
            AcdbDynamicDeltaInstanceTypeV2 *pHead = pDeltaData->pFileHead;
            while (pHead)
            {
               AcdbDeltaDataKeyTypeV2 *pKey = NULL;
               pDeltaData->pFileHead = pHead->pNext;
               pKey = pHead->pKey;

               if (pKey != NULL)
               {
                  if (pKey->pIndices != NULL)
                  {
                     ACDB_MEM_FREE(pKey->pIndices);
                     pKey->pIndices = NULL;
                  }
               }
               ACDB_MEM_FREE(pKey);
               pKey = NULL;

               ACDB_MEM_FREE(pHead);
               pHead = NULL;

               pHead = pDeltaData->pFileHead;
            }
            pDeltaData->pFileTail = NULL;

            ACDB_MEM_FREE(g_pInstanceDeltaData[idx]);
            g_pInstanceDeltaData[idx] = NULL;
         }
      }
   }

   //Free Data on Heap
   if (g_pData != NULL)
   {
      AcdbDynamicUniqueDataType *pCurData = g_pData->pDatalHead;

      while (pCurData)
      {
         g_pData->pDatalHead = pCurData->pNext;
         ACDB_MEM_FREE(pCurData->ulDataBuf);
         ACDB_MEM_FREE(pCurData);
         pCurData = g_pData->pDatalHead;
      }
      if (pCurData != NULL)
      {
         ACDB_MEM_FREE(g_pData->pDatalHead);
      }
      ACDB_MEM_FREE(g_pData);
      g_pData = NULL;
   }

   //Free General Info on Heap

   //Free Adie Table on Heap
   if (g_pAdieTbl != NULL)
   {
      AcdbDynamicAdieTblType *pCurAdieTbl = NULL;
      pCurAdieTbl = g_pAdieTbl->pAdieTblHead;
      while (pCurAdieTbl)
      {
         g_pAdieTbl->pAdieTblHead = pCurAdieTbl->pNext;
         ACDB_MEM_FREE(pCurAdieTbl->pKey);
         ACDB_MEM_FREE(pCurAdieTbl);
         pCurAdieTbl = g_pAdieTbl->pAdieTblHead;
      }
      if (pCurAdieTbl != NULL)
      {
         ACDB_MEM_FREE(g_pAdieTbl->pAdieTblHead);
      }
      ACDB_MEM_FREE(g_pAdieTbl);
      g_pAdieTbl = NULL;
   }

   return result;
}

int32_t Acdb_GetInstanceDeltaFileLength(int32_t fileIndex, uint8_t *outputBuf, size_t outputBufLen)
{
   int32_t result = ACDB_SUCCESS;
   uint32_t *pSize = (uint32_t *)outputBuf;

   if (pSize == NULL || outputBufLen != sizeof(uint32_t))
   {
      return ACDB_BADPARM;
   }

   *pSize = 0;
   if (fileIndex >= 0 && fileIndex < ACDB_MAX_ACDB_FILES)
   {
      if (g_pInstanceDeltaData[fileIndex] != NULL)
      {
         AcdbDynamicDeltaInstanceTypeV2 *pDeltaData = g_pInstanceDeltaData[fileIndex]->pFileHead;

         while (pDeltaData != NULL)
         {
            if (pDeltaData->pKey == NULL || pDeltaData->pDataNode == NULL)
            {
               ACDB_DEBUG_LOG("[ACDB Override][Acdb_GetDeltaFileLength] NULL key/data found in file with index %d!", fileIndex);
               return ACDB_ERROR;
            }

            *pSize += (uint32_t)sizeof(uint32_t); //pDeltaData->pKey->nTableId
            *pSize += (uint32_t)sizeof(uint32_t); //pDeltaData->pKey->nIndicesCount
            *pSize += (uint32_t)(pDeltaData->pKey->nIndicesCount * sizeof(uint32_t)); //pDeltaData->pKey->pIndices

            *pSize += (uint32_t)sizeof(uint32_t); //pDeltaData->pDataNode->ulDataLen
            *pSize += pDeltaData->pDataNode->ulDataLen; //pDeltaData->pDataNode->ulDataBuf

            pDeltaData = pDeltaData->pNext;
         }

         *pSize += (uint32_t)sizeof(uint32_t); // for no_entries.
      }
   }

   return result;
}

int32_t Acdb_GetInstanceDeltaFileData(int32_t fileIndex, uint8_t *outputBuf, size_t outputBufLen)
{
   int32_t result = ACDB_ERROR;
   uint8_t *newFileBuf = NULL;
   uint32_t offset = sizeof(uint32_t);
   uint32_t cur_delta_data_count = 0;

   //expected format
   // <no_entries>
   //<tblId1 (uint32_t) | indicesCount1 (uint32_t) | pIndices1 (uint32_t * indicesCount) | mid1 (uint32_t) | pid1 (uint32_t) | dataLen1 (uint32_t) | pData1 (uint8_t * dataLen)>
   //<tblId2 (uint32_t) | indicesCount2 (uint32_t) | pIndices2 (uint32_t * indicesCount) | mid2 (uint32_t) | pid2 (uint32_t) | dataLen2 (uint32_t) | pData2 (uint8_t * dataLen)>
   //...
   //...
   //<tblIdn (uint32_t) | indicesCountn (uint32_t) | pIndicesn (uint32_t * indicesCount) | midn (uint32_t) | pidn (uint32_t) | dataLenn (uint32_t) | pDatan (uint8_t * dataLen)>

   if (outputBuf == NULL || outputBufLen == 0)
   {
      return ACDB_BADPARM;
   }

   newFileBuf = outputBuf;
   if (fileIndex >= 0 && fileIndex < ACDB_MAX_ACDB_FILES)
   {
      if (g_pInstanceDeltaData[fileIndex] != NULL)
      {
         AcdbDynamicDeltaInstanceTypeV2 *pDeltaData = g_pInstanceDeltaData[fileIndex]->pFileHead;

         while (pDeltaData != NULL)
         {
            AcdbDeltaDataKeyTypeV2 *pKey = pDeltaData->pKey;
            AcdbDynamicUniqueDataType * pDataNode = pDeltaData->pDataNode;
            if (pKey == NULL || pDataNode == NULL)
            {
               ACDB_DEBUG_LOG("[ACDB Override][Acdb_GetDeltaFileData] NULL key/data found in file with index %d!", fileIndex);
               return ACDB_ERROR;
            }

            ACDB_MEM_CPY(newFileBuf + offset, outputBufLen, &pKey->nTableId, sizeof(uint32_t));
            offset += (uint32_t)sizeof(uint32_t);

            ACDB_MEM_CPY(newFileBuf + offset, outputBufLen, &pKey->nIndicesCount, sizeof(uint32_t));
            offset += (uint32_t)sizeof(uint32_t);

            ACDB_MEM_CPY(newFileBuf + offset, outputBufLen, pKey->pIndices, sizeof(uint32_t) * (pKey->nIndicesCount));
            offset += (uint32_t)sizeof(uint32_t) * (pKey->nIndicesCount);

            ACDB_MEM_CPY(newFileBuf + offset, outputBufLen, &pDataNode->ulDataLen, sizeof(uint32_t));
            offset += (uint32_t)sizeof(uint32_t);

            ACDB_MEM_CPY(newFileBuf + offset, outputBufLen, pDataNode->ulDataBuf, pDataNode->ulDataLen);
            offset += pDataNode->ulDataLen;

            cur_delta_data_count++;

            pDeltaData = pDeltaData->pNext;
         }

         ACDB_MEM_CPY(newFileBuf, outputBufLen, &cur_delta_data_count, sizeof(uint32_t));

         result = ACDB_SUCCESS;
      }
   }

   return result;
}

int32_t Acdb_GetInstanceNoOfTblEntries(uint8_t *pCmd, uint32_t nCmdSize, uint8_t *pRsp, size_t nRspSizse)
{
   int32_t result = ACDB_SUCCESS;
   AcdbQueryNoOfTblEntriesCmdType *pInput = (AcdbQueryNoOfTblEntriesCmdType *)pCmd;
   AcdbRespNoOfTblEntriesCmdType *pOut = (AcdbRespNoOfTblEntriesCmdType *)pRsp;
   uint32_t nonModuleTbl = 0;

   if (pCmd == NULL || nCmdSize != sizeof(AcdbQueryNoOfTblEntriesCmdType) ||
      pRsp == NULL || nRspSizse != sizeof(AcdbRespNoOfTblEntriesCmdType))
   {
      ACDB_DEBUG_LOG("Invalid input params provided to retrieve the data");
      return ACDB_ERROR;
   }
   switch (pInput->nTblId)
   {
   case AUDPROC_GAIN_INDP_TBL:
   case AUDPROC_COPP_GAIN_DEP_TBL:
   case AUDPROC_AUD_VOL_TBL:
   case AUD_STREAM_TBL:
   case VOCPROC_GAIN_INDP_TBL:
   case VOCPROC_COPP_GAIN_DEP_TBL:
   case VOC_STREAM_TBL:
   case VOCPROC_DYNAMIC_TBL:
   case VOCPROC_STATIC_TBL:
   case VOC_STREAM2_TBL:
   case AFE_TBL:
   case AFE_CMN_TBL:
   case VOCPROC_DEV_CFG_TBL:
   case LSM_TBL:
   case ADIE_SIDETONE_TBL:
   case AANC_CFG_TBL:
   case VOCPROC_COPP_GAIN_DEP_V2_TBL:
   case VOICE_VP3_TBL:
   case AUDIO_REC_VP3_TBL:
   case AUDIO_REC_EC_VP3_TBL:
   case VOCPROC_DYN_INST_TBL_ID:
   case VOCPROC_STAT_INST_TBL_ID:
   case VOCSTRM_INST_TBL_ID:
   case AUDPROC_INST_TBL_ID:
   case AUDPROCVOL_INST_TBL_ID:
   case AFECMN_INST_TBL_ID:
   case AUDSTRM_INST_TBL_ID:
   case LSM_INST_TBL_ID:
      nonModuleTbl = 0;
      break;
   case ADIE_ANC_TBL:
   case ADIE_CODEC_TBL:
   case GLOBAL_DATA_TBL:
   case CDC_FEATURES_TBL:
   case METAINFO_LOOKUP_TBL:
      nonModuleTbl = 1;
      break;
   default:
      ACDB_DEBUG_LOG("Provided invalid tableid");
      return ACDB_ERROR;
   }
   pOut->nNoOfEntries = 0;
   if (nonModuleTbl == 0)
   {
      if (pInput->nTblId == VOCPROC_DYN_INST_TBL_ID || pInput->nTblId == VOCPROC_STAT_INST_TBL_ID)
      {
         AcdbInstanceVoiceTblType *pCur = NULL;

         if (g_pVoiceTblInstance == NULL || g_pVoiceTblInstance->pTblHead == NULL)
         {
            pOut->nNoOfEntries = 0;
            return ACDB_SUCCESS;
         }
         pCur = g_pVoiceTblInstance->pTblHead;
         while (pCur != NULL)
         {
            if (pCur->pKey->nTableId == pInput->nTblId)
            {
               AcdbInstancePrimaryKeyType *pCurPrimaryKey = pCur->pPrimaryKeyNode->pTblHead;
               while (pCurPrimaryKey != NULL)
               {
                  AcdbInstanceSecondaryKeyType *pCurSecondaryKey = pCurPrimaryKey->pSecondaryNode->pTblHead;
                  while (pCurSecondaryKey != NULL)
                  {
                     AcdbInstanceTopologyType *pCurTop = pCurSecondaryKey->pTopologyNode->pTopHead;
                     while (pCurTop != NULL)
                     {
                        pOut->nNoOfEntries++;
                        //AcdbDynamicUniqueDataType *pCurData = pCurTop->pDataNode;
                        //while(pCurData != NULL)
                        //{
                        // pOut->nNoOfEntries++;
                        // pCurData = pCurData->pNext;
                        //}
                        pCurTop = pCurTop->pNext;
                     }
                     pCurSecondaryKey = pCurSecondaryKey->pNext;
                  }
                  pCurPrimaryKey = pCurPrimaryKey->pNext;
               }
            }
            pCur = pCur->pNext;
         }
      }
      else
      {
         AcdbInstanceAudioTblType *pCur = NULL;

         if (g_pTbl == NULL || g_pTbl->pTblHead == NULL)
         {
            pOut->nNoOfEntries = 0;
            return ACDB_SUCCESS;
         }
         pCur = g_pTbl->pTblHead;
         while (pCur != NULL)
         {
            if (pCur->pKey->nTableId == pInput->nTblId)
            {
               AcdbInstanceTopologyType *pCurTop = pCur->pTopologyNode->pTopHead;
               while (pCurTop != NULL)
               {
                  pOut->nNoOfEntries++;
                  //AcdbDynamicUniqueDataType *pCurData = pCurTop->pDataNode;
                  //while(pCurData != NULL)
                  //{
                  // pOut->nNoOfEntries++;
                  // pCurData = pCurData->pNext;
                  //}
                  pCurTop = pCurTop->pNext;
               }
            }
            pCur = pCur->pNext;
         }
      }
   }
   else
   {
	   AcdbDynamicAdieTblType *pCur =  NULL;
      if (g_pAdieTbl == NULL || g_pAdieTbl->pAdieTblHead == NULL)
      {
         pOut->nNoOfEntries = 0;
         return ACDB_SUCCESS;
      }
      pCur = g_pAdieTbl->pAdieTblHead;
      while (pCur != NULL)
      {
         if (pCur->pKey->nTableId == pInput->nTblId)
         {
            pOut->nNoOfEntries++;
            //AcdbDynamicUniqueDataType *pCurData = pCur->pDataNode;
            //while(pCurData != NULL)
            //{
            //
            // pCurData = pCurData->pNext;
            //}
         }
         pCur = pCur->pNext;
      }
   }
   return result;
}

int32_t Acdb_GetAdieTableCal(AcdbDataLookupKeyType *pKey,
   AcdbDynamicUniqueDataType **ppDataNode
   )
{
   int32_t result = ACDB_BADPARM;
   uint32_t devid = 0, pid = 0;
   if (pKey != NULL && ppDataNode != NULL)
   {
      AcdbDynamicAdieTblType *pTblNode = NULL;
	  if (ACDB_SUCCESS != GetDevIdPidFromCdft((uint8_t *)pKey->nLookupKey, &devid, &pid))
	  {
		  ACDB_DEBUG_LOG("Not able to get Device ID and PID");
		  return ACDB_ERROR;
	  }
      result = IsDataNodeOnHeap((uint32_t*)&pid,
         (AcdbDynamicDataNodeType*)g_pData
         );

      if (result == ACDB_SUCCESS)
      {
         result = FindAdieTableNodeOnHeap((AcdbDataLookupKeyType*)pKey,
            (AcdbDynamicAdieTblNodeType*)g_pAdieTbl,
            (AcdbDynamicAdieTblType**)&pTblNode
            );
         if (result == ACDB_SUCCESS && pTblNode != NULL)
         {
            *ppDataNode = pTblNode->pDataNode;
         }
      }
   }
   else
   {
      ACDB_DEBUG_LOG("[ACDB Override ERROR]->[Acdb_GetAdieTableCal]->NULL Input pointer");
   }//Null input
   return result;
}

int32_t Acdb_SetAdieTableCal(uint32_t persistData,
   uint32_t fileIndex,
   AcdbDataLookupKeyType *pKey,
   uint8_t *pInputBufPtr,
   const uint32_t InputBufLen,
   uint8_t *pIndices,
   uint32_t nIndexCount
   )
{
   int32_t result = ACDB_BADPARM;
   int32_t deltaResult = ACDB_BADPARM;

   if (pKey != NULL && pInputBufPtr != NULL && InputBufLen != 0)
   {
      AcdbDynamicUniqueDataType *pDataNode = NULL;
      AcdbDynamicAdieTblType *pTblNode = NULL;
	  uint32_t devid = 0, pid = 0;
      uint32_t dataType = ACDB_HEAP_DATA_FOUND;

	  if (ACDB_SUCCESS != GetDevIdPidFromCdft((uint8_t *)pKey->nLookupKey, &devid, &pid))
	  {
		  ACDB_DEBUG_LOG("Not able to get Device ID and PID");
		  return ACDB_ERROR;
	  }

      result = IsDataOnHeap((uint32_t*)&pid,
         (uint8_t*)pInputBufPtr,
         (uint32_t)InputBufLen,
         (AcdbDynamicDataNodeType*)g_pData
         ); //check if data is on heap
      if (result == ACDB_PARMNOTFOUND)
      {
         result = CreateDataNodeOnHeap((uint32_t*)&pid,
            (uint8_t*)pInputBufPtr,
            (uint32_t)InputBufLen,
            (AcdbDynamicDataNodeType*)g_pData,
            (AcdbDynamicUniqueDataType**)&pDataNode
            ); //if not on heap, create data node
         dataType = ACDB_HEAP_DATA_NOT_FOUND;
      }
      else if (result == ACDB_SUCCESS)
      {
         result = FindDataNodeOnHeap((uint32_t*)&pid,
            (uint8_t*)pInputBufPtr,
            (uint32_t)InputBufLen,
            (AcdbDynamicDataNodeType*)g_pData,
            (AcdbDynamicUniqueDataType**)&pDataNode
            ); //if yes, find the data node
      }
      result = FindAdieTableNodeOnHeap((AcdbDataLookupKeyType*)pKey,
         (AcdbDynamicAdieTblNodeType*)g_pAdieTbl,
         (AcdbDynamicAdieTblType**)&pTblNode
         ); // check if volume table node is on heap
      if (result == ACDB_SUCCESS)
      {
         //Both pDataNode and PTblNode should be filled
         if (pDataNode != NULL && pTblNode != NULL)
         {
            if (dataType == ACDB_HEAP_DATA_NOT_FOUND || pDataNode != pTblNode->pDataNode)
            {
               //Condition to decrement refcount
               //1. check if found data node is different than what has already stored in the organization
               //2. a new data node was creatd
               //Decrease previous data node count
               pTblNode->pDataNode->refcount--;
               if (pTblNode->pDataNode->refcount == 0)
               {
                  result = FreeDataNode((uint32_t*)&pid,
                     (AcdbDynamicDataNodeType*)g_pData
                     );
               }
               //Assign new data node
               pTblNode->pDataNode = pDataNode;
               pTblNode->pDataNode->refcount++;

               // update delta data node reference.
               if (persistData == TRUE &&
                  ACDB_UTILITY_INIT_SUCCESS == AcdbIsPersistenceSupported() &&
                  fileIndex < ACDB_MAX_ACDB_FILES)
               {
				   AcdbDeltaDataKeyTypeV2 nKeyToFind = { 0, 0, NULL };

                  CreateDeltaDataKey(&nKeyToFind, pKey->nTableId, nIndexCount, pIndices, 0, 0);
                  deltaResult = UpdateDeltaDataNodeOnHeapV2(nKeyToFind,
                     (AcdbDynamicUniqueDataType*)pDataNode,
                     g_pInstanceDeltaData[fileIndex]
                     );

                  if (deltaResult != ACDB_SUCCESS)
                  {
                     // error updating node, should i return error or continue?
                     ACDB_DEBUG_LOG("[ACDB Override ERROR]->[Acdb_SetDataCal]-> Error updating delta file node on heap.");
                  }
               }
            }
         }
      }
      else if (result == ACDB_PARMNOTFOUND)
      {
         result = CreateAdieTableNodeOnHeap((AcdbDataLookupKeyType*)pKey,
            (AcdbDynamicUniqueDataType*)pDataNode,
            (AcdbDynamicAdieTblNodeType*)g_pAdieTbl
            ); //if not, create the volume table node

         if (result != ACDB_SUCCESS)
         {
            ACDB_DEBUG_LOG("[ACDB Override ERROR]->[Acdb_SetDataCal]-> Error creating adie table node on heap.");
            return result;
         }

         // create delta data node as well.
         if (persistData == TRUE &&
            ACDB_UTILITY_INIT_SUCCESS == AcdbIsPersistenceSupported() &&
            fileIndex < ACDB_MAX_ACDB_FILES)
         {
            AcdbDeltaDataKeyTypeV2 *pKeyToCreate = (AcdbDeltaDataKeyTypeV2 *)ACDB_MALLOC(sizeof(AcdbDeltaDataKeyTypeV2));

            if (pKeyToCreate == NULL)
            {
               return ACDB_INSUFFICIENTMEMORY;
            }

            CreateDeltaDataKey(pKeyToCreate, pKey->nTableId, nIndexCount, pIndices, 0, 0);
            deltaResult = CreateDeltaDataNodeOnHeapV2(pKeyToCreate,
               (AcdbDynamicUniqueDataType*)pDataNode,
               &g_pInstanceDeltaData[fileIndex]
               );

            if (deltaResult != ACDB_SUCCESS)
            {
               // error adding node, should i return error or continue?
               ACDB_DEBUG_LOG("[ACDB Override ERROR]->[Acdb_SetDataCal]-> Error creating delta file node on heap.");
            }
         }
      }
   }
   else
   {
      ACDB_DEBUG_LOG("[ACDB Override ERROR]->[Acdb_SetAdieTableCal]->NULL Input pointer");
   }//Null input

   return result;
}

int32_t Acdb_DM_Instance_Ioctl(uint32_t Acdb_DM_CMD_Id,
   uint32_t persistData,
   uint32_t fileIndex,
   AcdbDataLookupKeyType *pKey,
   uintptr_t cdftLookup,
   uint32_t *pParamId,
   uint8_t *pInputBuf,
   const uint32_t InputBufLen,
   const uint8_t *pFileDataBuf,
   const uint32_t nFileDataLen,
   uint8_t *pOutBuff,
   const size_t nOutBuffLen,
   uint32_t *nOutBuffBytesUsed,
   AcdbDynamicUniqueDataType **ppDataNode,
   uint8_t* pIndices,
   uint32_t nIndexCount
   )
{
   int32_t result = ACDB_SUCCESS;
   //UNREFERENCED_VAR(nOutBuffBytesUsed);
   if (nOutBuffBytesUsed != NULL)
      *nOutBuffBytesUsed = 0;

   //Global Variable Initialization
   if (g_pTbl == NULL)
   {
      g_pTbl = (AcdbInstanceAudioTblNodeType*)ACDB_MALLOC(sizeof(AcdbInstanceAudioTblNodeType));
      if (g_pTbl != NULL)
      {
         g_pTbl->pTblHead = NULL;
         g_pTbl->pTblTail = NULL;
      }
      else
      {
         result = ACDB_BADSTATE;
      }
   }
   if (g_pData == NULL)
   {
      g_pData = (AcdbDynamicDataNodeType*)ACDB_MALLOC(sizeof(AcdbDynamicDataNodeType));
      if (g_pData != NULL)
      {
         g_pData->pDatalHead = NULL;
         g_pData->pDatalTail = NULL;
      }
      else
      {
         result = ACDB_BADSTATE;
      }
   }

   if (g_pAdieTbl == NULL)
	{
		g_pAdieTbl = (AcdbDynamicAdieTblNodeType*)ACDB_MALLOC(sizeof(AcdbDynamicAdieTblNodeType));
		if (g_pAdieTbl != NULL)
		{
			g_pAdieTbl->pAdieTblHead = NULL;
			g_pAdieTbl->pAdieTblTail = NULL;
		}
		else
		{
			result = ACDB_BADSTATE;
		}
	}

   switch (Acdb_DM_CMD_Id)
   {
   case ACDB_GET_DATA:

      result = Acdb_GetInstanceDataCal((AcdbDataLookupKeyType*)pKey,
         cdftLookup,
         (uint32_t*)pParamId,
         (AcdbDynamicUniqueDataType**)&(*ppDataNode)
         );
      break;

   case ACDB_SET_DATA:

      result = Acdb_SetInstanceDataCal(persistData,
         fileIndex,
         (AcdbDataLookupKeyType*)pKey,
         cdftLookup,
         (uint32_t*)pParamId,
         (const uint8_t*)pFileDataBuf,
         nFileDataLen,
         (uint8_t*)pInputBuf,
         (uint32_t)InputBufLen,
         (uint8_t*)pIndices,
         (uint32_t)nIndexCount
         );
      break;
   case ACDB_GET_NO_OF_TBL_ENTRIES_ON_HEAP:
      result = Acdb_GetInstanceNoOfTblEntries(pInputBuf, InputBufLen, pOutBuff, nOutBuffLen);
      break;
   case ACDB_GET_TBL_ENTRIES_ON_HEAP:
      result = Acdb_GetInstanceTblEntries(pInputBuf, InputBufLen, pOutBuff, nOutBuffLen);
      break;

   case ACDB_SYS_RESET:
      result = Acdb_Instance_sys_reset();
      break;
   case ACDB_GET_DELTA_FILE_LENGTH:
      result = Acdb_GetInstanceDeltaFileLength(fileIndex, pOutBuff, nOutBuffLen);
      break;
   case ACDB_GET_DELTA_FILE_DATA:
      result = Acdb_GetInstanceDeltaFileData(fileIndex, pOutBuff, nOutBuffLen);
      break;

   case ACDB_GET_ADIE_TABLE:

      result = Acdb_GetAdieTableCal((AcdbDataLookupKeyType*)pKey,
         (AcdbDynamicUniqueDataType**)&(*ppDataNode)
         );
      break;

   case ACDB_SET_ADIE_TABLE:

      result = Acdb_SetAdieTableCal(persistData,
         fileIndex,
         (AcdbDataLookupKeyType*)pKey,
         (uint8_t*)pInputBuf,
         (uint32_t)InputBufLen,
         (uint8_t*)pIndices,
         (uint32_t)nIndexCount
         );
      break;
   }

   return result;
}

int32_t CreateInstanceDeltaDataKey(AcdbDeltaDataKeyTypeV2 *nKeyToCheck, uint32_t tblID, uint32_t idxCount, uint8_t* pIndices, uint32_t moduleID, uint32_t paramID, uint32_t iid)
{
   uint8_t *buffer = NULL;
   uint32_t offset = 0;

   buffer = (uint8_t *)malloc(((size_t)idxCount + 3)*(sizeof(uint32_t)));
   if (buffer == NULL)
   {
      return ACDB_INSUFFICIENTMEMORY;
   }
   nKeyToCheck->nTableId = tblID;
   nKeyToCheck->nIndicesCount = idxCount + 3;

   ACDB_MEM_CPY(buffer, ((idxCount + 3)*sizeof(uint32_t)), pIndices, (idxCount*sizeof(uint32_t)));
   offset = (uint32_t)(idxCount*sizeof(uint32_t));
   ACDB_MEM_CPY(buffer+offset, (3 * sizeof(uint32_t)), &moduleID, sizeof(uint32_t));
   offset += (uint32_t)sizeof(uint32_t);
   ACDB_MEM_CPY(buffer+offset, (2 * sizeof(uint32_t)), &iid, sizeof(uint32_t));
   offset += (uint32_t)sizeof(uint32_t);
   ACDB_MEM_CPY(buffer+offset, sizeof(uint32_t), &paramID, sizeof(uint32_t));
   offset += (uint32_t)sizeof(uint32_t);
   nKeyToCheck->pIndices = buffer;

   return ACDB_SUCCESS;
}

int32_t CreateDeltaDataKey(AcdbDeltaDataKeyTypeV2 *nKeyToCheck, uint32_t tblID, uint32_t idxCount, uint8_t* pIndices, uint32_t moduleID, uint32_t paramID)
{
   uint8_t *buffer = NULL;
   uint32_t offset = 0;

   buffer = (uint8_t *)malloc(sizeof(uint32_t)*((size_t)idxCount + 2));
   if (buffer == NULL)
   {
      return ACDB_INSUFFICIENTMEMORY;
   }
   nKeyToCheck->nTableId = tblID;
   nKeyToCheck->nIndicesCount = idxCount + 2;

   ACDB_MEM_CPY(buffer, ((idxCount + 2)*sizeof(uint32_t)), pIndices, (idxCount*sizeof(uint32_t)));
   offset += (uint32_t)(idxCount*sizeof(uint32_t));
   ACDB_MEM_CPY(buffer+offset, (2 * sizeof(uint32_t)), &moduleID, sizeof(uint32_t));
   offset += (uint32_t)sizeof(uint32_t);
   ACDB_MEM_CPY(buffer+offset, sizeof(uint32_t), &paramID, sizeof(uint32_t));
   offset +=  (uint32_t)sizeof(uint32_t);
   nKeyToCheck->pIndices = buffer;

   return ACDB_SUCCESS;
}

int32_t GetMidIidFromCdft(uint8_t* cdftPtr, uint32_t *mid, uint32_t *iid)
{
   *mid = READ_UInt32(cdftPtr);
   *iid = READ_UInt32(cdftPtr + sizeof(uint32_t));
   return ACDB_SUCCESS;
}

int32_t GetMidFromIndices(uint8_t* pIndices, uint32_t nIdxCount, uint32_t *mid)
{
   *mid = READ_UInt32(pIndices + (nIdxCount * sizeof(uint32_t)));
   return ACDB_SUCCESS;
}

int32_t GetDevIdPidFromCdft(uint8_t* cdftPtr, uint32_t *devId, uint32_t *pid)
{
   *devId = READ_UInt32(cdftPtr);
   *pid = READ_UInt32(cdftPtr + sizeof(uint32_t));
   return ACDB_SUCCESS;
}

