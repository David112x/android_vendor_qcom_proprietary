/*===========================================================================

acdb_new_linked_list.c

DESCRIPTION
This file defines methods used to access and manipulate ACDB data structures.

REFERENCES
None.

Copyright (c) 2018 Qualcomm Technologies, Inc.
All Rights Reserved.
Confidential and Proprietary - Qualcomm Technologies, Inc.
===========================================================================*/
/* $Header: //source/qcom/qct/multimedia2/Audio/audcal4/acdb_sw/main/latest/acdb/src/acdb_new_linked_list.c#2 $ */
/*===========================================================================

EDIT HISTORY FOR FILE

This section contains comments describing changes made to this file.
Notice that changes are listed in reverse chronological order.

$Header: //source/qcom/qct/multimedia2/Audio/acdb

when who what, where, why
-------- --- ----------------------------------------------------------
05/28/14 mh Removed KW warnings and errors
02/14/14 avi Support ACDB persistence.
07/23/10 ernanl Introduce new function used in ACDB_DM_Ioctl.
07/06/10 ernanl Initial revision.

===========================================================================*/

/* ---------------------------------------------------------------------------
* Include Files
*--------------------------------------------------------------------------- */

#include "acdb.h"
#include "acdb_os_includes.h"
#include "acdb_linked_list.h"
#include "acdb_new_linked_list.h"
#include "acdb_datainfo.h"
#include "acdb_utility.h"
/* ---------------------------------------------------------------------------
* Preprocessor Definitions and Constants
*--------------------------------------------------------------------------- */

/* ---------------------------------------------------------------------------
* Type Declarations
*--------------------------------------------------------------------------- */

/* ---------------------------------------------------------------------------
* Global Data Definitions
*--------------------------------------------------------------------------- */

/* ---------------------------------------------------------------------------
* Static Variable Definitions
*--------------------------------------------------------------------------- */

/* ---------------------------------------------------------------------------
* Static Function Declarations and Definitions
*--------------------------------------------------------------------------- */

int32_t CreateAudioTableNodeOnHeap(AcdbDataLookupKeyType *pKey,
	AcdbInstanceAudioTblNodeType *pTblOnHeap,
	AcdbInstanceAudioTblType **ppTblNode
	)
{
	int32_t result = ACDB_BADPARM;

	if (pTblOnHeap != NULL && pKey != NULL)
	{
		AcdbInstanceAudioTblType *pCur = NULL;

		if (pTblOnHeap == NULL)
		{
			AcdbInstanceAudioTblNodeType *pTblOnHeap = (AcdbInstanceAudioTblNodeType *)ACDB_MALLOC(sizeof(AcdbInstanceAudioTblNodeType));
			pTblOnHeap->pTblHead = NULL;
			pTblOnHeap->pTblTail = NULL;
		}

		if (pTblOnHeap->pTblHead == NULL)
		{
			pTblOnHeap->pTblHead = (AcdbInstanceAudioTblType *)ACDB_MALLOC(sizeof(AcdbInstanceAudioTblType));
			if (pTblOnHeap->pTblHead != NULL)
			{
				pTblOnHeap->pTblTail = pTblOnHeap->pTblHead;

				pCur = pTblOnHeap->pTblHead;
				pCur->pTopologyNode = NULL;

				pCur->pKey = (AcdbDataLookupKeyType*)ACDB_MALLOC(sizeof(AcdbDataLookupKeyType));
				if (pCur->pKey != NULL)
				{
					ACDB_MEM_CPY((void*)pCur->pKey,sizeof(AcdbDataLookupKeyType),(void*)pKey,sizeof(AcdbDataLookupKeyType));

					*ppTblNode = pCur;
					pCur->pNext = NULL;

					result = ACDB_SUCCESS;
				}
				else
				{//malloc pCur->pKey failed
					result = ACDB_BADSTATE;
				}
			}
			else
			{//malloc pTblOnHeap->pTblHead failed
				result = ACDB_BADSTATE;
			}
		}
		else
		{
			pCur = (AcdbInstanceAudioTblType*)ACDB_MALLOC(sizeof(AcdbInstanceAudioTblType));
			if (pCur != NULL)
			{
				pCur->pTopologyNode = NULL;

				pCur->pKey = (AcdbDataLookupKeyType*)ACDB_MALLOC(sizeof(AcdbDataLookupKeyType));
				if (pCur->pKey != NULL)
				{
					ACDB_MEM_CPY((void*)pCur->pKey,sizeof(AcdbDataLookupKeyType),(void*)pKey,sizeof(AcdbDataLookupKeyType));
					*ppTblNode = pCur;
					pCur->pNext = NULL;

					pTblOnHeap->pTblTail->pNext = pCur;
					pTblOnHeap->pTblTail = pCur;
					pTblOnHeap->pTblTail->pNext = NULL;

					result = ACDB_SUCCESS;
				}
				else
				{//malloc pCur->pKey failed
					result = ACDB_BADSTATE;
					ACDB_MEM_FREE(pCur);
				}
			}
			else
			{//malloc pCur failed
				result = ACDB_BADSTATE;
			}
		}
	}
	else
	{
		ACDB_DEBUG_LOG ("[ACDB Linked_List ERROR]->[CreateTableNodeOnHeap]->NULL Input pointer");
	}
	return result;
}

int32_t CreateVoiceTableNodeOnHeap(AcdbDataLookupCVDKeyType *pKey,
	AcdbInstanceVoiceTblNodeType *pTblOnHeap,
	AcdbInstanceVoiceTblType **ppTblNode
	)
{
	int32_t result = ACDB_BADPARM;

	if (pTblOnHeap != NULL && pKey != NULL)
	{
		AcdbInstanceVoiceTblType *pCur = NULL;

		if (pTblOnHeap == NULL)
		{
			pTblOnHeap = (AcdbInstanceVoiceTblNodeType *)ACDB_MALLOC(sizeof(AcdbInstanceVoiceTblNodeType));
			pTblOnHeap->pTblHead = NULL;
			pTblOnHeap->pTblTail = NULL;
		}

		if (pTblOnHeap->pTblHead == NULL)
		{
			pTblOnHeap->pTblHead = (AcdbInstanceVoiceTblType *)ACDB_MALLOC(sizeof(AcdbInstanceVoiceTblType));
			if (pTblOnHeap->pTblHead != NULL)
			{
				pTblOnHeap->pTblTail = pTblOnHeap->pTblHead;

				pCur = pTblOnHeap->pTblHead;
				pCur->pPrimaryKeyNode = (AcdbInstancePrimaryKeyNodeType *)ACDB_MALLOC(sizeof(AcdbInstancePrimaryKeyNodeType));
            if(pCur->pPrimaryKeyNode != NULL)
            {
               pCur->pPrimaryKeyNode->pTblHead = NULL;
               pCur->pPrimaryKeyNode->pTblTail = NULL;
            }
            else
            {
            result = ACDB_BADSTATE;
            }

				pCur->pKey = (AcdbDataLookupTblKeyType*)ACDB_MALLOC(sizeof(AcdbDataLookupTblKeyType));
				if (pCur->pKey != NULL)
				{
               pCur->pKey->nTableId = pKey->nTableId;

					*ppTblNode = pCur;
					pCur->pNext = NULL;

					result = ACDB_SUCCESS;
				}
				else
				{//malloc pCur->pKey failed
					result = ACDB_BADSTATE;
				}
			}
			else
			{//malloc pTblOnHeap->pTblHead failed
				result = ACDB_BADSTATE;
			}
		}
		else
		{
			pCur = (AcdbInstanceVoiceTblType*)ACDB_MALLOC(sizeof(AcdbInstanceVoiceTblType));
			if (pCur != NULL)
			{
				pCur->pPrimaryKeyNode = (AcdbInstancePrimaryKeyNodeType *)ACDB_MALLOC(sizeof(AcdbInstancePrimaryKeyNodeType));
            if(pCur->pPrimaryKeyNode != NULL)
            {
               pCur->pPrimaryKeyNode->pTblHead = NULL;
               pCur->pPrimaryKeyNode->pTblTail = NULL;
            }
            else
            {
            result = ACDB_BADSTATE;
            }

				pCur->pKey = (AcdbDataLookupTblKeyType*)ACDB_MALLOC(sizeof(AcdbDataLookupTblKeyType));
				if (pCur->pKey != NULL)
				{
               pCur->pKey->nTableId = pKey->nTableId;
					*ppTblNode = pCur;
					pCur->pNext = NULL;

					pTblOnHeap->pTblTail->pNext = pCur;
					pTblOnHeap->pTblTail = pCur;
					pTblOnHeap->pTblTail->pNext = NULL;

					result = ACDB_SUCCESS;
				}
				else
				{//malloc pCur->pKey failed
					result = ACDB_BADSTATE;
					ACDB_MEM_FREE(pCur);
				}
			}
			else
			{//malloc pCur failed
				result = ACDB_BADSTATE;
			}
		}
	}
	else
	{
		ACDB_DEBUG_LOG ("[ACDB Linked_List ERROR]->[CreateTableNodeOnHeap]->NULL Input pointer");
	}
	return result;
}

int32_t CreateAudioTopologyNodeOnHeap(uintptr_t cdft,
	uint32_t *pParamId,
	AcdbDynamicUniqueDataType *pDataNode,
	AcdbInstanceAudioTblType *pTblNodeOnHeap
	)
{
	int32_t result = ACDB_BADPARM;

	if (cdft != (uintptr_t)NULL && pParamId != NULL && pDataNode != NULL && pTblNodeOnHeap != NULL)
	{
		AcdbInstanceTopologyType* pCurTop = NULL;

		if (pTblNodeOnHeap->pTopologyNode == NULL)
		{
			pTblNodeOnHeap->pTopologyNode = (AcdbInstanceTopologyNodeType*)ACDB_MALLOC(sizeof(AcdbInstanceTopologyNodeType));
			if (pTblNodeOnHeap->pTopologyNode != NULL)
			{
				pTblNodeOnHeap->pTopologyNode->pTopHead = (AcdbInstanceTopologyType*)ACDB_MALLOC(sizeof(AcdbInstanceTopologyType));
				if (pTblNodeOnHeap->pTopologyNode->pTopHead != NULL)
				{
					pTblNodeOnHeap->pTopologyNode->pTopTail = pTblNodeOnHeap->pTopologyNode->pTopHead;

					pCurTop = pTblNodeOnHeap->pTopologyNode->pTopHead;
					pCurTop->nCDFTLookupKey = cdft;
					pCurTop->pDataNode = pDataNode;
					pCurTop->pDataNode->refcount ++;

					pCurTop->pNext = NULL;

					result = ACDB_SUCCESS;
				}
				else
				{//malloc pTblNodeOnHeap->pTopologyNode->pTopHead failed
					result = ACDB_BADSTATE;
					ACDB_MEM_FREE(pTblNodeOnHeap->pTopologyNode);
					pTblNodeOnHeap->pTopologyNode = NULL;
				}
			}
			else
			{//malloc pTblNodeOnHeap->pTopologyNode failed
				result = ACDB_BADSTATE;
			}
		}//Create Data Node on Heap
		else
		{
			pCurTop = (AcdbInstanceTopologyType *)ACDB_MALLOC(sizeof(AcdbInstanceTopologyType));
			if (pCurTop != NULL)
			{
				pCurTop->nCDFTLookupKey = cdft;
				pCurTop->pDataNode = pDataNode;
				pCurTop->pDataNode->refcount ++;

				pTblNodeOnHeap->pTopologyNode->pTopTail->pNext = pCurTop;
				pTblNodeOnHeap->pTopologyNode->pTopTail = pCurTop;
				pTblNodeOnHeap->pTopologyNode->pTopTail->pNext = NULL;

				result = ACDB_SUCCESS;
			}
			else
			{
				result = ACDB_BADSTATE;
			}
		}//Create Data Node on Heap
	}//check if Null Input
	else
	{
		ACDB_DEBUG_LOG ("[ACDB Linked_List ERROR]->[CreateTopologyNodeOnHeap]->NULL Input pointer");
	}
	return result;
}

int32_t CreateVoiceTopologyNodeOnHeap(uintptr_t cdft,
	uint32_t *pParamId,
	AcdbDynamicUniqueDataType *pDataNode,
	AcdbInstanceSecondaryKeyType *pSecondaryKeyNodeOnHeap
	)
{
	int32_t result = ACDB_BADPARM;

	if (cdft != (uintptr_t)NULL && pParamId != NULL && pDataNode != NULL && pSecondaryKeyNodeOnHeap != NULL)
	{
		AcdbInstanceTopologyType* pCurTop = NULL;

		if (pSecondaryKeyNodeOnHeap->pTopologyNode == NULL)
		{
			pSecondaryKeyNodeOnHeap->pTopologyNode = (AcdbInstanceTopologyNodeType*)ACDB_MALLOC(sizeof(AcdbInstanceTopologyNodeType));
			if (pSecondaryKeyNodeOnHeap->pTopologyNode != NULL)
			{
				pSecondaryKeyNodeOnHeap->pTopologyNode->pTopHead = (AcdbInstanceTopologyType*)ACDB_MALLOC(sizeof(AcdbInstanceTopologyType));
				if (pSecondaryKeyNodeOnHeap->pTopologyNode->pTopHead != NULL)
				{
					pSecondaryKeyNodeOnHeap->pTopologyNode->pTopTail = pSecondaryKeyNodeOnHeap->pTopologyNode->pTopHead;

					pCurTop = pSecondaryKeyNodeOnHeap->pTopologyNode->pTopHead;
					pCurTop->nCDFTLookupKey = cdft;
					pCurTop->pDataNode = pDataNode;
					pCurTop->pDataNode->refcount ++;

					pCurTop->pNext = NULL;

					result = ACDB_SUCCESS;
				}
				else
				{//malloc pSecondaryKeyNodeOnHeap->pTopologyNode->pTopHead failed
					result = ACDB_BADSTATE;
				}
			}
			else
			{//malloc pSecondaryKeyNodeOnHeap->pTopologyNode failed
				result = ACDB_BADSTATE;
			}
		}//Create Data Node on Heap
		else
		{
			pCurTop = (AcdbInstanceTopologyType *)ACDB_MALLOC(sizeof(AcdbInstanceTopologyType));
			if (pCurTop != NULL)
			{
				pCurTop->nCDFTLookupKey = cdft;
				pCurTop->pDataNode = pDataNode;
				pCurTop->pDataNode->refcount ++;

				pSecondaryKeyNodeOnHeap->pTopologyNode->pTopTail->pNext = pCurTop;
				pSecondaryKeyNodeOnHeap->pTopologyNode->pTopTail = pCurTop;
				pSecondaryKeyNodeOnHeap->pTopologyNode->pTopTail->pNext = NULL;

				result = ACDB_SUCCESS;
			}
			else
			{
				result = ACDB_BADSTATE;
			}
		}//Create Data Node on Heap
	}//check if Null Input
	else
	{
		ACDB_DEBUG_LOG ("[ACDB Linked_List ERROR]->[CreateTopologyNodeOnHeap]->NULL Input pointer");
	}
	return result;
}

int32_t CreatePrimaryKeyNodeOnHeap(AcdbDataLookupCVDKeyType *pKey,
	AcdbInstanceVoiceTblType *pTblNode,
	AcdbInstancePrimaryKeyType **ppPrimaryKeyNode
	)
{
	int32_t result = ACDB_BADPARM;

	if (pTblNode != NULL && pTblNode->pPrimaryKeyNode != NULL && pKey != NULL)
	{
		AcdbInstancePrimaryKeyType *pCur = NULL;

		if (pTblNode->pPrimaryKeyNode->pTblHead == NULL)
		{
			pTblNode->pPrimaryKeyNode->pTblHead = (AcdbInstancePrimaryKeyType *)ACDB_MALLOC(sizeof(AcdbInstancePrimaryKeyType));
			if (pTblNode->pPrimaryKeyNode->pTblHead != NULL)
			{
				pTblNode->pPrimaryKeyNode->pTblTail = pTblNode->pPrimaryKeyNode->pTblHead;

				pCur = pTblNode->pPrimaryKeyNode->pTblHead;
				pCur->pSecondaryNode = (AcdbInstanceSecondaryKeyNodeType *)ACDB_MALLOC(sizeof(AcdbInstanceSecondaryKeyNodeType));
            if(pCur->pSecondaryNode != NULL)
            {
            pCur->pSecondaryNode->pTblHead = NULL;
            pCur->pSecondaryNode->pTblTail = NULL;
            }
            else
            {
            result = ACDB_BADSTATE;
            }

				pCur->pKey = (AcdbDataLookupPrimaryKeyType*)ACDB_MALLOC(sizeof(AcdbDataLookupPrimaryKeyType));
				if (pCur->pKey != NULL)
				{
               pCur->pKey->nLookupKey = pKey->nLookupKey;

					*ppPrimaryKeyNode = pCur;
					pCur->pNext = NULL;

					result = ACDB_SUCCESS;
				}
				else
				{//malloc pCur->pKey failed
					result = ACDB_BADSTATE;
				}
			}
			else
			{//malloc pTblOnHeap->pTblHead failed
				result = ACDB_BADSTATE;
			}
		}
		else
		{
			pCur = (AcdbInstancePrimaryKeyType*)ACDB_MALLOC(sizeof(AcdbInstancePrimaryKeyType));
			if (pCur != NULL)
			{
				pCur->pSecondaryNode = (AcdbInstanceSecondaryKeyNodeType *)ACDB_MALLOC(sizeof(AcdbInstanceSecondaryKeyNodeType));
            if(pCur->pSecondaryNode != NULL)
            {
            pCur->pSecondaryNode->pTblHead = NULL;
            pCur->pSecondaryNode->pTblTail = NULL;
            }
            else
            {
            result = ACDB_BADSTATE;
            }

				pCur->pKey = (AcdbDataLookupPrimaryKeyType*)ACDB_MALLOC(sizeof(AcdbDataLookupPrimaryKeyType));
				if (pCur->pKey != NULL)
				{
               pCur->pKey->nLookupKey = pKey->nLookupKey;
					*ppPrimaryKeyNode = pCur;
					pCur->pNext = NULL;

					pTblNode->pPrimaryKeyNode->pTblTail->pNext = pCur;
					pTblNode->pPrimaryKeyNode->pTblTail = pCur;
					pTblNode->pPrimaryKeyNode->pTblTail->pNext = NULL;

					result = ACDB_SUCCESS;
				}
				else
				{//malloc pCur->pKey failed
					result = ACDB_BADSTATE;
					ACDB_MEM_FREE(pCur);
				}
			}
			else
			{//malloc pCur failed
				result = ACDB_BADSTATE;
			}
		}
	}
	else
	{
		ACDB_DEBUG_LOG ("[ACDB Linked_List ERROR]->[CreateTableNodeOnHeap]->NULL Input pointer");
	}
	return result;
}

int32_t CreateSecondaryKeyNodeOnHeap(AcdbDataLookupCVDKeyType *pKey,
	AcdbInstancePrimaryKeyType *pPrimaryKeyNode,
	AcdbInstanceSecondaryKeyType **ppSecondaryKeyNode
	)
{
	int32_t result = ACDB_BADPARM;

	if (pPrimaryKeyNode != NULL && pPrimaryKeyNode->pSecondaryNode != NULL && pKey != NULL)
	{
		AcdbInstanceSecondaryKeyType *pCur = NULL;

		if (pPrimaryKeyNode->pSecondaryNode->pTblHead == NULL)
		{
			pPrimaryKeyNode->pSecondaryNode->pTblHead = (AcdbInstanceSecondaryKeyType *)ACDB_MALLOC(sizeof(AcdbInstanceSecondaryKeyType));
			if (pPrimaryKeyNode->pSecondaryNode->pTblHead != NULL)
			{
				pPrimaryKeyNode->pSecondaryNode->pTblTail = pPrimaryKeyNode->pSecondaryNode->pTblHead;

				pCur = pPrimaryKeyNode->pSecondaryNode->pTblHead;
				pCur->pTopologyNode = NULL;

				pCur->pKey = (AcdbDataLookupSecondaryKeyType*)ACDB_MALLOC(sizeof(AcdbDataLookupSecondaryKeyType));
				if (pCur->pKey != NULL)
				{
               pCur->pKey->nSecondaryLookupKey = pKey->nSecondaryLookupKey;

					*ppSecondaryKeyNode = pCur;
					pCur->pNext = NULL;

					result = ACDB_SUCCESS;
				}
				else
				{//malloc pCur->pKey failed
					result = ACDB_BADSTATE;
				}
			}
			else
			{//malloc pTblOnHeap->pTblHead failed
				result = ACDB_BADSTATE;
			}
		}
		else
		{
			pCur = (AcdbInstanceSecondaryKeyType*)ACDB_MALLOC(sizeof(AcdbInstanceSecondaryKeyType));
			if (pCur != NULL)
			{
				pCur->pTopologyNode = NULL;

				pCur->pKey = (AcdbDataLookupSecondaryKeyType*)ACDB_MALLOC(sizeof(AcdbDataLookupSecondaryKeyType));
				if (pCur->pKey != NULL)
				{
               pCur->pKey->nSecondaryLookupKey = pKey->nSecondaryLookupKey;
					*ppSecondaryKeyNode = pCur;
					pCur->pNext = NULL;

					pPrimaryKeyNode->pSecondaryNode->pTblTail->pNext = pCur;
					pPrimaryKeyNode->pSecondaryNode->pTblTail = pCur;
					pPrimaryKeyNode->pSecondaryNode->pTblTail->pNext = NULL;

					result = ACDB_SUCCESS;
				}
				else
				{//malloc pCur->pKey failed
					result = ACDB_BADSTATE;
					ACDB_MEM_FREE(pCur);
				}
			}
			else
			{//malloc pCur failed
				result = ACDB_BADSTATE;
			}
		}
	}
	else
	{
		ACDB_DEBUG_LOG ("[ACDB Linked_List ERROR]->[CreateTableNodeOnHeap]->NULL Input pointer");
	}
	return result;
}

int32_t FreeAudioTableNode(AcdbDataLookupKeyType *pKey,
	AcdbInstanceAudioTblNodeType *pTblOnHeap
	)
{
	int32_t result = ACDB_BADPARM;
	int32_t memcmpResult = 0;

	if (pKey != NULL && pTblOnHeap != NULL)
	{
		if (pTblOnHeap->pTblHead != NULL)
		{
			AcdbInstanceAudioTblType* pPrev = pTblOnHeap->pTblHead;
			AcdbInstanceAudioTblType* pCur = pPrev->pNext;

			memcmpResult = memcmp((void*)pKey,(void*)pPrev->pKey,sizeof(AcdbDataLookupKeyType));
			if (memcmpResult == ACDB_SUCCESS)
			{
				pTblOnHeap->pTblHead = pCur;
				if(pCur == NULL)
				{
					pTblOnHeap->pTblTail = NULL;
				}
				ACDB_MEM_FREE(pPrev->pKey);
				ACDB_MEM_FREE(pPrev);
			}
			if (memcmpResult != ACDB_SUCCESS)
			{
				while (pCur)
				{
					memcmpResult = memcmp((void*)pKey,(void*)pCur->pKey,sizeof(AcdbDataLookupKeyType));
					if (memcmpResult == ACDB_SUCCESS)
					{
						break;
					}
					pPrev = pPrev->pNext;
					pCur = pCur->pNext;
				}
				if (pCur != NULL)
				{
					pPrev->pNext = pCur->pNext;
					if(pCur->pNext == NULL)
					{
						pTblOnHeap->pTblTail = pPrev;
					}
					ACDB_MEM_FREE(pCur->pKey);
					ACDB_MEM_FREE(pCur);
				}
			}
		}
		result = ACDB_SUCCESS;
	}
	else
	{
		ACDB_DEBUG_LOG ("[ACDB Linked_List ERROR]->[FreeTableNode]->NULL Input pointer");
	}
	return result;
}

int32_t FreeVoiceTableNode(AcdbDataLookupCVDKeyType *pCVDKey,
	AcdbInstanceVoiceTblNodeType *pTblOnHeap
	)
{
	int32_t result = ACDB_BADPARM;

   if (pCVDKey != NULL && pTblOnHeap != NULL)
   {
      if (pTblOnHeap->pTblHead != NULL)
      {
         AcdbInstanceVoiceTblType* pPrev = pTblOnHeap->pTblHead;
         AcdbInstanceVoiceTblType* pCur = pPrev->pNext;

         if (pPrev->pKey->nTableId == pCVDKey->nTableId)
         {
            result = FreeInstancePrimaryNode(pCVDKey, pPrev->pPrimaryKeyNode);

            if(pPrev->pPrimaryKeyNode->pTblHead == NULL)
            {
               pTblOnHeap->pTblHead = pCur;
               if(pCur == NULL)
               {
                  pTblOnHeap->pTblTail = NULL;
               }

               ACDB_MEM_FREE(pPrev->pPrimaryKeyNode);
               ACDB_MEM_FREE(pPrev->pKey);
               ACDB_MEM_FREE(pPrev);
            }
         }
         else
         {
            while (pCur)
            {
               if (pCur->pKey->nTableId == pCVDKey->nTableId)
               {
                  break;
               }
               pPrev = pPrev->pNext;
               pCur = pCur->pNext;
            }

            if (pCur != NULL)
            {
               result = FreeInstancePrimaryNode(pCVDKey, pCur->pPrimaryKeyNode);

               if(pCur->pPrimaryKeyNode->pTblHead == NULL)
               {
                  pPrev->pNext = pCur->pNext;
                  if(pCur->pNext == NULL)
                  {
                     pTblOnHeap->pTblTail = pPrev;
                  }
                  ACDB_MEM_FREE(pCur->pKey);
                  ACDB_MEM_FREE(pCur);
               }
            }
         }
      }

		result = ACDB_SUCCESS;
	}
	else
	{
		ACDB_DEBUG_LOG ("[ACDB Linked_List ERROR]->[FreeTableNode]->NULL Input pointer");
	}
	return result;
}

int32_t FreeInstancePrimaryNode(AcdbDataLookupCVDKeyType *pCVDKey,
	AcdbInstancePrimaryKeyNodeType *pPrimaryNodeOnHeap
	)
{
	int32_t result = ACDB_BADPARM;

   if (pCVDKey != NULL && pPrimaryNodeOnHeap != NULL)
   {
      if (pPrimaryNodeOnHeap->pTblHead != NULL)
      {
         AcdbInstancePrimaryKeyType* pPrev = pPrimaryNodeOnHeap->pTblHead;
         AcdbInstancePrimaryKeyType* pCur = pPrev->pNext;

         if(pPrev != NULL && pPrev->pKey != NULL)
         {
            if (pPrev->pKey->nLookupKey == pCVDKey->nLookupKey)
            {
               result = FreeInstanceSecondaryNode(pCVDKey, pPrev->pSecondaryNode);

               if(pPrev->pSecondaryNode->pTblHead == NULL)
               {
                  pPrimaryNodeOnHeap->pTblHead = pCur;
                  if(pCur == NULL)
                  {
                     pPrimaryNodeOnHeap->pTblTail = NULL;
                  }

                  ACDB_MEM_FREE(pPrev->pSecondaryNode);
                  ACDB_MEM_FREE(pPrev->pKey);
                  ACDB_MEM_FREE(pPrev);
               }
            }
            else
            {
               while (pCur)
               {
                  if (pCur->pKey->nLookupKey == pCVDKey->nLookupKey)
                  {
                     break;
                  }
                  pPrev = pPrev->pNext;
                  pCur = pCur->pNext;
               }

               if (pCur != NULL)
               {
                  result = FreeInstanceSecondaryNode(pCVDKey, pCur->pSecondaryNode);

                  if(pCur->pSecondaryNode->pTblHead == NULL)
                  {
                     pPrev->pNext = pCur->pNext;
                     if(pCur->pNext == NULL)
                     {
                        pPrimaryNodeOnHeap->pTblTail = pPrev;
                     }
                     ACDB_MEM_FREE(pCur->pKey);
                     ACDB_MEM_FREE(pCur);
                  }
               }
            }
         }
      }

		result = ACDB_SUCCESS;
	}
	else
	{
		ACDB_DEBUG_LOG ("[ACDB Linked_List ERROR]->[FreeTableNode]->NULL Input pointer");
	}
	return result;
}

int32_t FreeInstanceSecondaryNode(AcdbDataLookupCVDKeyType *pKey,
	AcdbInstanceSecondaryKeyNodeType *pSecondaryNodeOnHeap
	)
{
	int32_t result = ACDB_BADPARM;

	if (pKey != NULL && pSecondaryNodeOnHeap != NULL)
	{
		if (pSecondaryNodeOnHeap->pTblHead != NULL)
		{
			AcdbInstanceSecondaryKeyType* pPrev = pSecondaryNodeOnHeap->pTblHead;
			AcdbInstanceSecondaryKeyType* pCur = pPrev->pNext;

			if (pPrev->pKey->nSecondaryLookupKey == pKey->nSecondaryLookupKey)
			{
				pSecondaryNodeOnHeap->pTblHead = pCur;
				if(pCur == NULL)
				{
					pSecondaryNodeOnHeap->pTblTail = NULL;
				}
				ACDB_MEM_FREE(pPrev->pKey);
				ACDB_MEM_FREE(pPrev);
			}
         else
			{
				while (pCur)
				{
					if (pCur->pKey->nSecondaryLookupKey == pKey->nSecondaryLookupKey)
					{
						break;
					}
					pPrev = pPrev->pNext;
					pCur = pCur->pNext;
				}
				if (pCur != NULL)
				{
					pPrev->pNext = pCur->pNext;
					if(pCur->pNext == NULL)
					{
						pSecondaryNodeOnHeap->pTblTail = pPrev;
					}
					ACDB_MEM_FREE(pCur->pKey);
					ACDB_MEM_FREE(pCur);
				}
			}
		}
		result = ACDB_SUCCESS;
	}
	else
	{
		ACDB_DEBUG_LOG ("[ACDB Linked_List ERROR]->[FreeSecondaryNodeEx]->NULL Input pointer");
	}
	return result;
}

int32_t FreeAudioTopologyNode(uintptr_t cdft,
	uint32_t *pParamId,
	AcdbInstanceAudioTblType *pTblNode,
	uint32_t *fReeTblResult
	)
{
	int32_t result = ACDB_BADPARM;
	int32_t memcmpResult = 0;

	if (cdft != (uintptr_t)NULL && pParamId != NULL && pTblNode != NULL)
	{
		if (pTblNode->pTopologyNode != NULL)
		{
			if (pTblNode->pTopologyNode->pTopHead != NULL)
			{
				AcdbInstanceTopologyType* pPrev = pTblNode->pTopologyNode->pTopHead;
				AcdbInstanceTopologyType* pCur = pPrev->pNext;
				uint32_t numCdftIndices = 0;
				uint32_t numofTblIndices = 0;
				uint32_t nonModuleTblFOund = 0;
				uint32_t numOfCmdIndices = 0;
				if (ACDB_SUCCESS != Get_table_indices_count(pTblNode->pKey->nTableId,&numofTblIndices, &nonModuleTblFOund, &numCdftIndices, &numOfCmdIndices))
				{
					return ACDB_ERROR;
				}


				memcmpResult = memcmp((void*)cdft,(void*)pPrev->nCDFTLookupKey,sizeof(uint32_t)*numCdftIndices);
				if (memcmpResult == ACDB_SUCCESS)
				{
					memcmpResult = memcmp((void*)pParamId,(void*)&pPrev->pDataNode->ulParamId,sizeof(uint32_t));
					if (memcmpResult == ACDB_SUCCESS)
					{
						pTblNode->pTopologyNode->pTopHead = pCur;
						if(pCur == NULL)
						{
							pTblNode->pTopologyNode->pTopTail = NULL;
							*fReeTblResult = ACDB_HEAP_FREE_NODE;
						}
						// if refcount > 0, decrease reference
						if (pPrev->pDataNode->refcount > 0)
						{
							pPrev->pDataNode->refcount--;
						}
						ACDB_MEM_FREE(pPrev);
					}
				}
				if (memcmpResult != ACDB_SUCCESS)
				{
					while (pCur)
					{
						memcmpResult = memcmp((void*)cdft,(void*)pCur->nCDFTLookupKey,sizeof(uint32_t)*numCdftIndices);
						if (memcmpResult == ACDB_SUCCESS)
						{
							memcmpResult = memcmp((void*)pParamId,(void*)&pCur->pDataNode->ulParamId,sizeof(uint32_t));
							{
								if (memcmpResult == ACDB_SUCCESS)
								{
									break;
								}
							}
						}
						pPrev = pPrev->pNext;
						pCur = pCur->pNext;
					}//Searching for the node
					if (pCur != NULL)
					{
						pPrev->pNext = pCur->pNext;
						if(pCur->pNext == NULL)
						{
							pTblNode->pTopologyNode->pTopTail = pPrev;
						}
						// if refcount > 0, decrease reference
						if (pCur->pDataNode->refcount > 0)
						{
							pCur->pDataNode->refcount--;
						}
						ACDB_MEM_FREE(pCur); //Free current node
					}
				}
			}
		}
		result = ACDB_SUCCESS;
	}//Input not NULL
	else
	{
		ACDB_DEBUG_LOG ("[ACDB Linked_List ERROR]->[FreeTopologyNode]->NULL Input pointer");
	}
	return result;
}

int32_t FreeVoiceTopologyNode(uintptr_t cdft,
	uint32_t cdftIndicesCount,
	uint32_t *pParamId,
	AcdbInstanceSecondaryKeyType *pSecondaryKeyNode,
	uint32_t *fReeTblResult
	)
{
	int32_t result = ACDB_BADPARM;
	int32_t memcmpResult = 0;

	if (cdft != (uintptr_t)NULL && pParamId != NULL && pSecondaryKeyNode != NULL)
	{
		if (pSecondaryKeyNode->pTopologyNode != NULL)
		{
			if (pSecondaryKeyNode->pTopologyNode->pTopHead != NULL)
			{
				AcdbInstanceTopologyType* pPrev = pSecondaryKeyNode->pTopologyNode->pTopHead;
				AcdbInstanceTopologyType* pCur = pPrev->pNext;

				memcmpResult = memcmp((void*)cdft,(void*)pPrev->nCDFTLookupKey,sizeof(uint32_t)*cdftIndicesCount);
				if (memcmpResult == ACDB_SUCCESS)
				{
					memcmpResult = memcmp((void*)pParamId,(void*)&pPrev->pDataNode->ulParamId,sizeof(uint32_t));
					if (memcmpResult == ACDB_SUCCESS)
					{
						pSecondaryKeyNode->pTopologyNode->pTopHead = pCur;
						if(pCur == NULL)
						{
							pSecondaryKeyNode->pTopologyNode->pTopTail = NULL;
							*fReeTblResult = ACDB_HEAP_FREE_NODE;
						}
						// if refcount > 0, decrease reference
						if (pPrev->pDataNode->refcount > 0)
						{
							pPrev->pDataNode->refcount--;
						}
						ACDB_MEM_FREE(pPrev);
					}
				}
				if (memcmpResult != ACDB_SUCCESS)
				{
					while (pCur)
					{
						memcmpResult = memcmp((void*)cdft,(void*)pCur->nCDFTLookupKey,sizeof(uint32_t)*cdftIndicesCount);
						if (memcmpResult == ACDB_SUCCESS)
						{
							memcmpResult = memcmp((void*)pParamId,(void*)&pCur->pDataNode->ulParamId,sizeof(uint32_t));
							{
								if (memcmpResult == ACDB_SUCCESS)
								{
									break;
								}
							}
						}
						pPrev = pPrev->pNext;
						pCur = pCur->pNext;
					}//Searching for the node
					if (pCur != NULL)
					{
						pPrev->pNext = pCur->pNext;
						if(pCur->pNext == NULL)
						{
							pSecondaryKeyNode->pTopologyNode->pTopTail = pPrev;
						}
						// if refcount > 0, decrease reference
						if (pCur->pDataNode->refcount > 0)
						{
							pCur->pDataNode->refcount--;
						}
						ACDB_MEM_FREE(pCur); //Free current node
					}
				}
			}
		}
		result = ACDB_SUCCESS;
	}//Input not NULL
	else
	{
		ACDB_DEBUG_LOG ("[ACDB Linked_List ERROR]->[FreeTopologyNode]->NULL Input pointer");
	}
	return result;
}

int32_t FindAudioTableNodeOnHeap(AcdbDataLookupKeyType *pKey,
	AcdbInstanceAudioTblNodeType *pTblOnHeap,
	AcdbInstanceAudioTblType **ppTblNode
	)
{
	int32_t result = ACDB_PARMNOTFOUND;

	if (pKey != NULL && pTblOnHeap != NULL)
	{
		AcdbInstanceAudioTblType *pCur = NULL;
		int32_t memcmpResult = 0;

		if (pTblOnHeap->pTblHead != NULL)
		{
			pCur = pTblOnHeap->pTblHead;

			while (pCur)
			{
				memcmpResult = memcmp((void*)pCur->pKey, (void*)pKey, sizeof(AcdbDataLookupKeyType));
				if (memcmpResult == ACDB_SUCCESS)
				{
					result = ACDB_SUCCESS;
					*ppTblNode = pCur;
					break;
				}//if found matched Key, break
				pCur = pCur->pNext;
			}
		}//pTblHead not created yet, return
	}//Not NULL ptr
	else
	{
		result = ACDB_BADPARM;

		ACDB_DEBUG_LOG ("[ACDB Linked_List ERROR]->[FindTableNodeOnHeap]->NULL Input pointer");
	}//Error message
	return result;
}

int32_t FindVoiceTableNodeOnHeap(AcdbDataLookupCVDKeyType *pKey,
	AcdbInstanceVoiceTblNodeType *pTblOnHeap,
	AcdbInstanceVoiceTblType **ppTblNode
	)
{
	int32_t result = ACDB_PARMNOTFOUND;

	if (pKey != NULL && pTblOnHeap != NULL)
	{
		AcdbInstanceVoiceTblType *pCur = NULL;

		if (pTblOnHeap->pTblHead != NULL)
		{
			pCur = pTblOnHeap->pTblHead;

			while (pCur)
			{
            if(pCur->pKey == NULL)
               break;

				if (pCur->pKey->nTableId == pKey->nTableId)
				{
					result = ACDB_SUCCESS;
					*ppTblNode = pCur;
					break;
				}//if found matched Key, break
				pCur = pCur->pNext;
			}
		}//pTblHead not created yet, return
	}//Not NULL ptr
	else
	{
		result = ACDB_BADPARM;

		ACDB_DEBUG_LOG ("[ACDB Linked_List ERROR]->[FindTableNodeOnHeap]->NULL Input pointer");
	}//Error message
	return result;
}
int32_t FindInstancePrimaryKeyNodeOnHeap(AcdbDataLookupCVDKeyType *pKey,
	AcdbInstancePrimaryKeyNodeType *pPrimaryKeyOnHeap,
	AcdbInstancePrimaryKeyType **ppPrimaryNode
	)
{
	int32_t result = ACDB_PARMNOTFOUND;

	if (pKey != NULL && pPrimaryKeyOnHeap != NULL)
	{
		AcdbInstancePrimaryKeyType *pCur = NULL;

		if (pPrimaryKeyOnHeap->pTblHead != NULL)
		{
			pCur = pPrimaryKeyOnHeap->pTblHead;

			while (pCur)
			{
            if(pCur->pKey == NULL)
               break;

				if (pCur->pKey->nLookupKey == pKey->nLookupKey)
				{
					result = ACDB_SUCCESS;
					*ppPrimaryNode = pCur;
					break;
				}//if found matched Key, break
				pCur = pCur->pNext;
			}
		}//pTblHead not created yet, return
	}//Not NULL ptr
	else
	{
		result = ACDB_BADPARM;

		ACDB_DEBUG_LOG ("[ACDB Linked_List ERROR]->[FindPrimaryKeyNodeOnHeapEx]->NULL Input pointer");
	}//Error message
	return result;
}
int32_t FindInstanceSecondaryKeyNodeOnHeap(AcdbDataLookupCVDKeyType *pKey,
	AcdbInstanceSecondaryKeyNodeType *pSecondaryKeyOnHeap,
	AcdbInstanceSecondaryKeyType **ppSecondaryNode
	)
{
	int32_t result = ACDB_PARMNOTFOUND;

	if (pKey != NULL && pSecondaryKeyOnHeap != NULL)
	{
		AcdbInstanceSecondaryKeyType *pCur = NULL;

		if (pSecondaryKeyOnHeap->pTblHead != NULL)
		{
			pCur = pSecondaryKeyOnHeap->pTblHead;

			while (pCur)
         {
            if(pCur->pKey == NULL)
               break;

				if (pCur->pKey->nSecondaryLookupKey == pKey->nSecondaryLookupKey)
				{
					result = ACDB_SUCCESS;
					*ppSecondaryNode = pCur;
					break;
				}//if found matched Key, break
				pCur = pCur->pNext;
			}
		}//pTblHead not created yet, return
	}//Not NULL ptr
	else
	{
		result = ACDB_BADPARM;

		ACDB_DEBUG_LOG ("[ACDB Linked_List ERROR]->[FindSecondaryKeyNodeOnHeapEx]->NULL Input pointer");
	}//Error message
	return result;
}
int32_t FindInstanceSecondaryKeyNodeFromCVDKeyOnHeap(AcdbDataLookupCVDKeyType *pKey,
   AcdbInstanceVoiceTblNodeType *pTblOnHeap,
   AcdbInstanceSecondaryKeyType **ppSecondaryNode
   )
{
   int32_t result = ACDB_BADPARM;

   if(pKey != NULL && pTblOnHeap != NULL)
   {
      AcdbInstanceVoiceTblType *pTblNode = NULL;

      result = FindVoiceTableNodeOnHeap((AcdbDataLookupCVDKeyType*) pKey,
         (AcdbInstanceVoiceTblNodeType*) pTblOnHeap,
         (AcdbInstanceVoiceTblType**) &pTblNode
         );
      if (result == ACDB_SUCCESS)
      {
         AcdbInstancePrimaryKeyType *pPrimaryKeyNode = NULL;

         result = FindInstancePrimaryKeyNodeOnHeap((AcdbDataLookupCVDKeyType*) pKey,
            (AcdbInstancePrimaryKeyNodeType*) pTblNode->pPrimaryKeyNode,
            (AcdbInstancePrimaryKeyType**) &pPrimaryKeyNode
            );

         if(result == ACDB_SUCCESS)
         {
            AcdbInstanceSecondaryKeyType *pSecondaryKeyNode = NULL;

            result = FindInstanceSecondaryKeyNodeOnHeap((AcdbDataLookupCVDKeyType*) pKey,
               (AcdbInstanceSecondaryKeyNodeType*) pPrimaryKeyNode->pSecondaryNode,
               (AcdbInstanceSecondaryKeyType**) &pSecondaryKeyNode
               );

            if(result == ACDB_SUCCESS)
            {
               *ppSecondaryNode = pSecondaryKeyNode;
            }
         }
      }
   }
   return result;
}
int32_t FindAudioTopologyNodeOnHeap(uintptr_t cdft,
	uint32_t *pParamId,
	AcdbInstanceAudioTblType *pTblNode,
	AcdbInstanceTopologyType **ppTopNode
	)
{
	int32_t result = ACDB_PARMNOTFOUND;
	int32_t memcmpResult = 0;

	if (cdft != (uintptr_t)NULL && pTblNode != NULL && ppTopNode != NULL && pParamId != NULL)
	{
		if (pTblNode->pTopologyNode != NULL)
		{
			uint32_t numCdftIndices = 0;
			uint32_t numofTblIndices = 0;
			uint32_t nonModuleTblFOund = 0;
			uint32_t numOfCmdIndices = 0;
			if (ACDB_SUCCESS != Get_table_indices_count(pTblNode->pKey->nTableId,&numofTblIndices, &nonModuleTblFOund, &numCdftIndices, &numOfCmdIndices))
			{
				return ACDB_ERROR;
			}
			if (pTblNode->pTopologyNode->pTopHead != NULL)
			{
				AcdbInstanceTopologyType *pCurTopology = NULL;
				pCurTopology = pTblNode->pTopologyNode->pTopHead;

				while (pCurTopology)
				{
					memcmpResult = memcmp((void *)pCurTopology->nCDFTLookupKey, (void *)cdft, sizeof(uint32_t)*numOfCmdIndices);
					if(memcmpResult == ACDB_SUCCESS)
					{
						memcmpResult = memcmp((void*)&pCurTopology->pDataNode->ulParamId,
							(void*)pParamId,sizeof(uint32_t));
						if (memcmpResult == ACDB_SUCCESS)
						{
							result = ACDB_SUCCESS;
							*ppTopNode = pCurTopology;
							break;
						}
					}
					pCurTopology = pCurTopology->pNext;
				}
			}
		}//pTblHead not created yet, return
	}//Not NULL ptr
	else
	{
		result = ACDB_BADPARM;

		ACDB_DEBUG_LOG ("[ACDB Linked_List ERROR]->[FindTopologyNodeOnHeap]->NULL Input pointer");
	}//Error message
	return result;
}
int32_t FindVoiceTopologyNodeOnHeap(uintptr_t cdft,
	uint32_t cdftIndicesCount,
	uint32_t *pParamId,
	AcdbInstanceSecondaryKeyType *pSecondaryKeyNode,
	AcdbInstanceTopologyType **ppTopNode
	)
{
	int32_t result = ACDB_PARMNOTFOUND;
	int32_t memcmpResult = 0;

	if (cdft != (uintptr_t)NULL && pSecondaryKeyNode != NULL && ppTopNode != NULL && pParamId != NULL)
	{
		if (pSecondaryKeyNode->pTopologyNode != NULL)
		{
			if (pSecondaryKeyNode->pTopologyNode->pTopHead != NULL)
			{
				AcdbInstanceTopologyType *pCurTopology = NULL;
				pCurTopology = pSecondaryKeyNode->pTopologyNode->pTopHead;

				while (pCurTopology)
				{
					memcmpResult = memcmp((void *)pCurTopology->nCDFTLookupKey, (void *)cdft, sizeof(uint32_t)*cdftIndicesCount);
					if (memcmpResult == ACDB_SUCCESS && pCurTopology->pDataNode != NULL)
					{
						memcmpResult = memcmp((void*)&pCurTopology->pDataNode->ulParamId,
							(void*)pParamId,sizeof(uint32_t));
						if (memcmpResult == ACDB_SUCCESS)
						{
							result = ACDB_SUCCESS;
							*ppTopNode = pCurTopology;
							break;
						}
					}
					pCurTopology = pCurTopology->pNext;
				}
			}
		}//pTblHead not created yet, return
	}//Not NULL ptr
	else
	{
		result = ACDB_BADPARM;

		ACDB_DEBUG_LOG ("[ACDB Linked_List ERROR]->[FindTopologyNodeOnHeap]->NULL Input pointer");
	}//Error message
	return result;
}

int32_t GetInstanceSecondaryNodeFromCVDKeyOnHeap(AcdbDataLookupCVDKeyType *pKey,
	AcdbInstanceVoiceTblNodeType *pTblOnHeap,
	AcdbInstanceSecondaryKeyType **ppSecondaryKeyNode
	)
{
	int32_t result = ACDB_BADPARM;

	if (pTblOnHeap != NULL && pKey != NULL)
	{
      AcdbInstanceVoiceTblType *pTblNode = NULL;

      result = FindVoiceTableNodeOnHeap((AcdbDataLookupCVDKeyType*) pKey,
         (AcdbInstanceVoiceTblNodeType*) pTblOnHeap,
         (AcdbInstanceVoiceTblType**) &pTblNode
         );

      if(result == ACDB_PARMNOTFOUND)
      {
         result = CreateVoiceTableNodeOnHeap((AcdbDataLookupCVDKeyType*) pKey,
            (AcdbInstanceVoiceTblNodeType*) pTblOnHeap,
            (AcdbInstanceVoiceTblType**) &pTblNode
            );
      }
      if(result == ACDB_SUCCESS)
      {
         AcdbInstancePrimaryKeyType *pPrimaryNode = NULL;

         result = FindInstancePrimaryKeyNodeOnHeap((AcdbDataLookupCVDKeyType*) pKey,
            (AcdbInstancePrimaryKeyNodeType*) pTblNode->pPrimaryKeyNode,
            (AcdbInstancePrimaryKeyType**) &pPrimaryNode
            );

         if(result == ACDB_PARMNOTFOUND)
         {
            result = CreatePrimaryKeyNodeOnHeap((AcdbDataLookupCVDKeyType*) pKey,
               (AcdbInstanceVoiceTblType*) pTblNode,
               (AcdbInstancePrimaryKeyType**) &pPrimaryNode
               );
         }
         if(result == ACDB_SUCCESS)
         {
            AcdbInstanceSecondaryKeyType *pSecondaryNode = NULL;

            result = FindInstanceSecondaryKeyNodeOnHeap((AcdbDataLookupCVDKeyType*) pKey,
               (AcdbInstanceSecondaryKeyNodeType*) pPrimaryNode->pSecondaryNode,
               (AcdbInstanceSecondaryKeyType**) &pSecondaryNode
               );

               if(result == ACDB_PARMNOTFOUND)
               {
                  result = CreateSecondaryKeyNodeOnHeap((AcdbDataLookupCVDKeyType*) pKey,
                     (AcdbInstancePrimaryKeyType*) pPrimaryNode,
                     (AcdbInstanceSecondaryKeyType**) &pSecondaryNode
                     );
               }

            if(result == ACDB_SUCCESS)
            {
               *ppSecondaryKeyNode = pSecondaryNode;
            }
         }
      }
   }
	else
	{
		ACDB_DEBUG_LOG ("[ACDB Linked_List ERROR]->[CreateTableNodeOnHeap]->NULL Input pointer");
	}
	return result;
}