/*===========================================================================

acdb_linked_list.c

DESCRIPTION
This file defines methods used to access and manipulate ACDB data structures.

REFERENCES
None.

Copyright (c) 2010-2018 Qualcomm Technologies, Inc.
All Rights Reserved.
Confidential and Proprietary - Qualcomm Technologies, Inc.
===========================================================================*/
/* $Header: //source/qcom/qct/multimedia2/Audio/audcal4/acdb_sw/main/latest/acdb/src/acdb_linked_list.c#18 $ */
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

int32_t IsDataNodeOnHeap(uint32_t *pParamId,
	AcdbDynamicDataNodeType *pUniqDataOnHeap
	)
{
	int32_t result = ACDB_PARMNOTFOUND;
	uint32_t memcmpResult = 0;

	if (pParamId != NULL && pUniqDataOnHeap != NULL)
	{
		AcdbDynamicUniqueDataType *pCur = NULL;

		if (pUniqDataOnHeap->pDatalHead != NULL)
		{
			pCur = pUniqDataOnHeap->pDatalHead;
			//Search Data Node
			if (pCur != NULL)
			{
				while(pCur)
				{
					memcmpResult = memcmp((void*)&pCur->ulParamId,(void*)pParamId, sizeof(uint32_t));
					if (memcmpResult == ACDB_SUCCESS)
					{
						result = ACDB_SUCCESS;
						break;
					}//if found matched Key, get table
					pCur = pCur->pNext;
				}
			}//Search from pHead to pTail Node
		}//if data not exist on heap, pDatalHead != NULL
	}//Not Null Input
	else
	{
		result = ACDB_BADPARM;

		ACDB_DEBUG_LOG ("[ACDB Linked_List ERROR]->[IsDataNodeOnHeap]->NULL Input pointer");
	}//Null input
	return result;
}

int32_t FindDataNodeOnHeap(uint32_t *pParamId,
	uint8_t *pInData,
	const uint32_t InDataLen,
	AcdbDynamicDataNodeType *pUniqDataOnHeap,
	AcdbDynamicUniqueDataType **ppDataNode
	)
{
	int32_t result = ACDB_PARMNOTFOUND;
	int32_t memcmpResult = 0;

	if (pParamId != NULL && pUniqDataOnHeap != NULL && ppDataNode != NULL)
	{
		AcdbDynamicUniqueDataType *pCur = NULL;

		if (pUniqDataOnHeap->pDatalHead != NULL)
		{
			pCur = pUniqDataOnHeap->pDatalHead;
			//Search Data Node
			if (pCur != NULL)
			{
				while(pCur)
				{
					memcmpResult = memcmp((void*)&pCur->ulParamId,(void*)pParamId, sizeof(uint32_t));
					if (memcmpResult == ACDB_SUCCESS)
					{
						if (pCur->ulDataLen == InDataLen)
						{
							memcmpResult = memcmp(pCur->ulDataBuf,pInData,InDataLen);
						}
						else
						{
							memcmpResult = ACDB_BADPARM;
						}
						if (memcmpResult == ACDB_SUCCESS)
						{
							result = ACDB_SUCCESS;
							*ppDataNode = pCur;
							break;
						}
					}//if found matched Key, get table
					pCur = pCur->pNext;
				}
			}//Search from pHead to pTail Node
		}//if data not exist on heap, pDatalHead != NULL
	}//Not Null Input
	else
	{
		result = ACDB_BADPARM;

		ACDB_DEBUG_LOG ("[ACDB Linked_List ERROR]->[FindDataNodeOnHeap]->NULL Input pointer");
	}//Null input
	return result;
}

int32_t IsDataOnHeap(uint32_t *ulParamId,
	uint8_t *pUniqueData,
	const uint32_t ulDataLen,
	AcdbDynamicDataNodeType *pUniqDataOnHeap
	)
{
	int32_t result = ACDB_PARMNOTFOUND;
	int32_t memcmpResult = 0;

	if (pUniqueData != NULL && pUniqDataOnHeap != NULL && ulParamId != NULL && ulDataLen != 0)
	{
		AcdbDynamicUniqueDataType *pDataNode = NULL;
		result = FindDataNodeOnHeap((uint32_t*) ulParamId,
			(uint8_t*) pUniqueData,
			(uint32_t) ulDataLen,
			(AcdbDynamicDataNodeType*) pUniqDataOnHeap,
			(AcdbDynamicUniqueDataType**) &pDataNode
			);
		if (result == ACDB_SUCCESS && pDataNode != NULL)
		{//check data node not equal to NULL
			if (pDataNode->ulDataLen == ulDataLen)
			{
				memcmpResult = memcmp((void*)pUniqueData,(void*)pDataNode->ulDataBuf,pDataNode->ulDataLen);
				if (memcmpResult == ACDB_SUCCESS)
				{
					result = ACDB_SUCCESS;
				}
			}
		}
	}//Not Null Input
	else
	{
		result = ACDB_BADPARM;

		ACDB_DEBUG_LOG ("[ACDB Linked_List ERROR]->[IsDataOnHeap]->NULL Input pointer");
	}//Null input
	return result;
}


int32_t CompareDeltaDataNodeKeysV2(AcdbDeltaDataKeyTypeV2 *pKey1,
	AcdbDeltaDataKeyTypeV2 *pKey2
	)
{
	int32_t result = ACDB_ERROR;
	if(pKey1 != NULL && pKey2 != NULL)
	{
		if(pKey1->pIndices != NULL && pKey2->pIndices != NULL)
		{
			if(pKey1->nTableId == pKey2->nTableId &&
				pKey1->nIndicesCount == pKey2->nIndicesCount &&
				(0 == memcmp((void*)pKey1->pIndices,(void*)pKey2->pIndices,sizeof(uint32_t)*(pKey2->nIndicesCount))))
			{
				result = ACDB_SUCCESS;
			}
		}
	}
	else
	{
		result = ACDB_BADPARM;
	}
	return result;
}


int32_t CreateDeltaDataNodeOnHeapV2(AcdbDeltaDataKeyTypeV2 *pKeyToCreate,
	AcdbDynamicUniqueDataType *pDataNode,
	AcdbDynamicDeltaFileDataTypeV2 **pDeltaDataNode
	)
{
	int32_t result = ACDB_BADPARM;

	if (pDataNode != NULL && pDeltaDataNode != NULL && pKeyToCreate != NULL)
	{
		AcdbDynamicDeltaInstanceTypeV2* pCurTop = NULL;
		AcdbDynamicDeltaFileDataTypeV2 *pDeltaData = *pDeltaDataNode;
		if (pDeltaData == NULL)
		{
			*pDeltaDataNode = (AcdbDynamicDeltaFileDataTypeV2*)ACDB_MALLOC(sizeof(AcdbDynamicDeltaFileDataTypeV2));
			pDeltaData = *pDeltaDataNode;

			if (pDeltaData != NULL)
			{
				pDeltaData->pFileHead = (AcdbDynamicDeltaInstanceTypeV2*)ACDB_MALLOC(sizeof(AcdbDynamicDeltaInstanceTypeV2));
				if (pDeltaData->pFileHead != NULL)
				{
					pDeltaData->pFileTail = pDeltaData->pFileHead;

					pCurTop = pDeltaData->pFileHead;
					pCurTop->pKey =  (AcdbDeltaDataKeyTypeV2*)ACDB_MALLOC(sizeof(AcdbDeltaDataKeyTypeV2));
					if(pCurTop->pKey == NULL)
					{
						ACDB_MEM_FREE(pDeltaDataNode);
						return ACDB_INSUFFICIENTMEMORY;
					}
					pCurTop->pKey->nTableId = pKeyToCreate->nTableId;
					pCurTop->pKey->nIndicesCount = pKeyToCreate->nIndicesCount;
					pCurTop->pKey->pIndices = (uint8_t *)ACDB_MALLOC(sizeof(uint32_t) * (pKeyToCreate->nIndicesCount));
					if(pCurTop->pKey->pIndices == NULL)
					{
						ACDB_MEM_FREE(pDeltaDataNode);
						return ACDB_INSUFFICIENTMEMORY;
					}
					ACDB_MEM_CPY(pCurTop->pKey->pIndices, sizeof(uint32_t) * (pKeyToCreate->nIndicesCount), pKeyToCreate->pIndices, sizeof(uint32_t) * (pKeyToCreate->nIndicesCount));
					pCurTop->pDataNode = pDataNode; // keep copy of the data node here.
					pCurTop->pNext = NULL;
					result = ACDB_SUCCESS;
				}
				else
				{//malloc pDeltaData->pTopHead failed
					result = ACDB_BADSTATE;
				}
			}
			else
			{//malloc pDeltaData failed
				result = ACDB_BADSTATE;
			}
		}//Create Data Node on Heap
		else
		{
			pCurTop = (AcdbDynamicDeltaInstanceTypeV2 *)ACDB_MALLOC(sizeof(AcdbDynamicDeltaInstanceTypeV2));
			if (pCurTop != NULL)
			{
				pCurTop->pKey =  (AcdbDeltaDataKeyTypeV2*)ACDB_MALLOC(sizeof(AcdbDeltaDataKeyTypeV2));
				if(pCurTop->pKey == NULL)
				{
					ACDB_MEM_FREE(pCurTop);
					return ACDB_INSUFFICIENTMEMORY;
				}
				pCurTop->pKey->nTableId = pKeyToCreate->nTableId;
				pCurTop->pKey->nIndicesCount = pKeyToCreate->nIndicesCount;
				pCurTop->pKey->pIndices = (uint8_t *)ACDB_MALLOC(sizeof(uint32_t) * (pKeyToCreate->nIndicesCount));
				if(pCurTop->pKey->pIndices == NULL)
				{
					ACDB_MEM_FREE(pCurTop->pKey);
					ACDB_MEM_FREE(pCurTop);
					return ACDB_INSUFFICIENTMEMORY;
				}
				ACDB_MEM_CPY(pCurTop->pKey->pIndices, sizeof(uint32_t) * (pKeyToCreate->nIndicesCount), pKeyToCreate->pIndices, sizeof(uint32_t) * (pKeyToCreate->nIndicesCount));
				//pCurTop->pKey = pKeyToCreate;
				pCurTop->pDataNode = pDataNode;
				pCurTop->pNext = NULL;

				if(pDeltaData->pFileHead == NULL &&
					pDeltaData->pFileTail == NULL)
				{
					pDeltaData->pFileHead = pCurTop;
					pDeltaData->pFileTail = pCurTop;
				}
				else
				{
					pDeltaData->pFileTail->pNext = pCurTop;
					pDeltaData->pFileTail = pCurTop;
					pDeltaData->pFileTail->pNext = NULL;
				}

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



int32_t UpdateDeltaDataNodeOnHeapV2(AcdbDeltaDataKeyTypeV2 nKeyToCheck,
	AcdbDynamicUniqueDataType *pDataNode,
	AcdbDynamicDeltaFileDataTypeV2 *pDeltaData
	)
{
	int32_t result = ACDB_BADPARM;
	int32_t memcmpResult = 0;

	if (pDeltaData != NULL)
	{
		AcdbDynamicDeltaInstanceTypeV2* pPrev =pDeltaData->pFileHead;
		AcdbDynamicDeltaInstanceTypeV2* pCur = pPrev->pNext;

		memcmpResult = CompareDeltaDataNodeKeysV2(pPrev->pKey, &nKeyToCheck);

		if (memcmpResult == ACDB_SUCCESS)
		{
			pPrev->pDataNode = pDataNode;
			result = ACDB_SUCCESS;
		}
		if (memcmpResult != ACDB_SUCCESS)
		{
			while (pCur)
			{
				memcmpResult = CompareDeltaDataNodeKeysV2(pCur->pKey, &nKeyToCheck);
				if (memcmpResult == ACDB_SUCCESS)
				{
					break;
				}
				pPrev = pPrev->pNext;
				pCur = pCur->pNext;
			}//Searching for the node
			if (pCur != NULL)
			{
				pCur->pDataNode = pDataNode;
				result = ACDB_SUCCESS;
			}
			else
			{
				result = ACDB_ERROR;
			}
		}
	}//Input not NULL
	else
	{
		ACDB_DEBUG_LOG ("[ACDB Linked_List ERROR]->[FreeTopologyNode]->NULL Input pointer");
	}
	return result;
}


int32_t FreeDeltaDataNodeV2(AcdbDeltaDataKeyTypeV2 nKeyToCheck,
	AcdbDynamicDeltaFileDataTypeV2 *pDeltaData
	)
{
	int32_t result = ACDB_BADPARM;
	int32_t memcmpResult = 0;

	if (pDeltaData != NULL)
	{
		AcdbDynamicDeltaInstanceTypeV2* pPrev =pDeltaData->pFileHead;
		AcdbDynamicDeltaInstanceTypeV2* pCur = NULL;
		if(pPrev == NULL)
		{
			return ACDB_BADPARM;
		}

		pCur = pPrev->pNext;

		memcmpResult = CompareDeltaDataNodeKeysV2(pPrev->pKey, &nKeyToCheck);

		if (memcmpResult == ACDB_SUCCESS)
		{
			pDeltaData->pFileHead = pCur;
			if(pCur == NULL)
			{
				pDeltaData->pFileTail = NULL;
			}

			if(pPrev->pKey != NULL)
			{
				if(pPrev->pKey->pIndices != NULL)
				{
					ACDB_MEM_FREE(pPrev->pKey->pIndices);
					pPrev->pKey->pIndices = NULL;
				}

				ACDB_MEM_FREE(pPrev->pKey);
				pPrev->pKey = NULL;
			}

			ACDB_MEM_FREE(pPrev);
			result = ACDB_SUCCESS;
		}
		if (memcmpResult != ACDB_SUCCESS)
		{
			while (pCur)
			{
				memcmpResult = CompareDeltaDataNodeKeysV2(pCur->pKey, &nKeyToCheck);
				if (memcmpResult == ACDB_SUCCESS)
				{
					break;
				}
				pPrev = pPrev->pNext;
				pCur = pCur->pNext;
			}//Searching for the node
			if (pCur != NULL)
			{
				pPrev->pNext = pCur->pNext;
				if(pCur->pNext == NULL)
				{
					pDeltaData->pFileTail = pPrev;
				}
				if(pCur->pKey != NULL)
				{
					if(pCur->pKey->pIndices != NULL)
					{
						ACDB_MEM_FREE(pCur->pKey->pIndices);
						pCur->pKey->pIndices = NULL;
					}

					ACDB_MEM_FREE(pCur->pKey);
					pCur->pKey = NULL;
				}
				ACDB_MEM_FREE(pCur); //Free current node
				result = ACDB_SUCCESS;
			}
			else
			{
				result = ACDB_ERROR;
			}
		}
	}//Input not NULL
	else
	{
		ACDB_DEBUG_LOG ("[ACDB Linked_List ERROR]->[FreeTopologyNode]->NULL Input pointer");
	}
	return result;
}



int32_t CreateDataNodeOnHeap(uint32_t *pParamId,
	uint8_t *pInData,
	const uint32_t InDataLen,
	AcdbDynamicDataNodeType *pUniqDataNode,
	AcdbDynamicUniqueDataType **ppDataNode
	)
{
	int32_t result = ACDB_BADPARM;

	if (pParamId != NULL && pInData != NULL && pUniqDataNode && InDataLen != 0)
	{
		AcdbDynamicUniqueDataType *pCur = NULL;

		if (pUniqDataNode->pDatalHead == NULL)
		{
			pUniqDataNode->pDatalHead = (AcdbDynamicUniqueDataType*)ACDB_MALLOC(sizeof(AcdbDynamicUniqueDataType));
			pUniqDataNode->pDatalTail = pUniqDataNode->pDatalHead;

			if (pUniqDataNode->pDatalHead != NULL)
			{
				pCur = pUniqDataNode->pDatalHead;
				pCur->ulParamId = (uint32_t)*pParamId;
				pCur->ulDataBuf = (uint8_t *)ACDB_MALLOC(InDataLen);
				if (pCur->ulDataBuf != NULL)
				{
					ACDB_MEM_CPY ((void *) pCur->ulDataBuf,InDataLen, (const void *) pInData, InDataLen);
					pCur->ulDataLen = InDataLen;
					pCur->refcount = 0;

					*ppDataNode = pCur;

					pCur->pNext = NULL;

					result = ACDB_SUCCESS;
				}
				else
				{
					result = ACDB_BADSTATE;
				}
			}
			else
			{
				result = ACDB_BADSTATE;
			}
		}//Create Data Node on Heap
		else
		{
			pCur = (AcdbDynamicUniqueDataType *)ACDB_MALLOC(sizeof(AcdbDynamicUniqueDataType));
			if (pCur != NULL)
			{
				pCur->ulParamId = (uint32_t)*pParamId;
				pCur->ulDataBuf = (uint8_t *)ACDB_MALLOC(InDataLen);
				if (pCur->ulDataBuf != NULL)
				{
					ACDB_MEM_CPY ((void *) pCur->ulDataBuf,InDataLen, (const void *) pInData, InDataLen);
					pCur->ulDataLen = InDataLen;
					pCur->refcount = 0;

					*ppDataNode = pCur;

					pUniqDataNode->pDatalTail->pNext = pCur;
					pUniqDataNode->pDatalTail = pCur;
					pUniqDataNode->pDatalTail->pNext = NULL;

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
		}//Create Data Node on Heap
	}//check if Null Input
	else
	{
		ACDB_DEBUG_LOG ("[ACDB Linked_List ERROR]->[CreateDataNodeOnHeap]->NULL Input pointer");
	}//Null input
	return result;
}

int32_t FreeDataNode(uint32_t *pParamId,
	AcdbDynamicDataNodeType *pDataOnHeap
	)
{
	int32_t result = ACDB_BADPARM;
	int32_t memcmpResult = 0;

	if (pParamId != NULL && pDataOnHeap != NULL)
	{
		if (pDataOnHeap->pDatalHead != NULL)
		{
			AcdbDynamicUniqueDataType* pPrev = pDataOnHeap->pDatalHead;
			AcdbDynamicUniqueDataType* pCur = pPrev->pNext;

			memcmpResult = memcmp((void*)pParamId,(void*)&pPrev->ulParamId,sizeof(uint32_t));
			if (memcmpResult == ACDB_SUCCESS) //Only interested in removing the first node
			{
				if (pPrev->refcount == 0)
				{
					pDataOnHeap->pDatalHead = pCur;
					if(pCur == NULL)
					{
						pDataOnHeap->pDatalTail = NULL;
					}
					ACDB_MEM_FREE(pPrev->ulDataBuf);
					ACDB_MEM_FREE(pPrev);
				}
				else
				{
					memcmpResult = ACDB_PARMNOTFOUND;
				}
			}
			if (memcmpResult != ACDB_SUCCESS) //Removing other than the first node
			{
				while (pCur) //Started at the second node
				{
					memcmpResult = memcmp((void*)pParamId,(void*)&pCur->ulParamId,sizeof(uint32_t));;
					if (memcmpResult == ACDB_SUCCESS)
					{
						if (pCur->refcount == 0) //If paramId match and refcount = 0, free the node
						{
							pPrev->pNext = pCur->pNext;
							if(pCur->pNext == NULL)
							{
								pDataOnHeap->pDatalTail = pPrev;
							}
							ACDB_MEM_FREE(pCur->ulDataBuf);
							ACDB_MEM_FREE(pCur);
							break;
						}
					}
					pPrev = pPrev->pNext;
					pCur = pCur->pNext; //Perform comparison in next node
				}
			}
		}
		result = ACDB_SUCCESS;
	}
	else
	{
		ACDB_DEBUG_LOG ("[ACDB Linked_List ERROR]->[FreeDataNode]->NULL Input pointer");
	}
	return result;
}

int32_t FindAdieTableNodeOnHeap(AcdbDataLookupKeyType *pKey,
	AcdbDynamicAdieTblNodeType *pTblOnHeap,
	AcdbDynamicAdieTblType **ppTblNode
	)
{
	int32_t result = ACDB_PARMNOTFOUND;
	int32_t memcmpResult = 0;

	if (pKey != NULL && pTblOnHeap != NULL)
	{
		AcdbDynamicAdieTblType *pCur = NULL;

		if (pTblOnHeap->pAdieTblHead != NULL)
		{
			pCur = pTblOnHeap->pAdieTblHead;

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

		ACDB_DEBUG_LOG ("[ACDB Linked_List ERROR]->[FindAdieTableNodeOnHeap]->NULL Input pointer");
	}//Error message
	return result;
}

int32_t CreateAdieTableNodeOnHeap(AcdbDataLookupKeyType *pKey,
	AcdbDynamicUniqueDataType *pDataNode,
	AcdbDynamicAdieTblNodeType *pTblOnHeap
	)
{
	int32_t result = ACDB_BADPARM;

	if (pTblOnHeap != NULL && pKey != NULL && pDataNode != NULL)
	{
		AcdbDynamicAdieTblType *pCur = NULL;

		if (pTblOnHeap == NULL)
		{
			AcdbDynamicAdieTblNodeType *pTblOnHeap = (AcdbDynamicAdieTblNodeType *)ACDB_MALLOC(sizeof(AcdbDynamicAdieTblNodeType));
			pTblOnHeap->pAdieTblHead = NULL;
			pTblOnHeap->pAdieTblTail = NULL;
		}

		if (pTblOnHeap->pAdieTblHead == NULL)
		{
			pTblOnHeap->pAdieTblHead = (AcdbDynamicAdieTblType *)ACDB_MALLOC(sizeof(AcdbDynamicAdieTblType));
			pTblOnHeap->pAdieTblTail = pTblOnHeap->pAdieTblHead;

			pCur = pTblOnHeap->pAdieTblHead;

			if (pCur != NULL)
			{
				pCur->pKey = (AcdbDataLookupKeyType*)ACDB_MALLOC(sizeof(AcdbDataLookupKeyType));
				if (pCur->pKey != NULL)
				{
					ACDB_MEM_CPY((void*)pCur->pKey,sizeof(AcdbDataLookupKeyType),(void*)pKey,sizeof(AcdbDataLookupKeyType));

					pCur->pDataNode = pDataNode;
					pCur->pDataNode->refcount++;
					pCur->pNext = NULL;

					result = ACDB_SUCCESS;
				}
				else
				{//malloc pCur->pKey failed
					result = ACDB_BADSTATE;
				}
			}
			else
			{//malloc pCur failed
				result = ACDB_BADSTATE;
			}
		}
		else
		{
			pCur = (AcdbDynamicAdieTblType*)ACDB_MALLOC(sizeof(AcdbDynamicAdieTblType));

			if (pCur != NULL)
			{
				pCur->pKey = (AcdbDataLookupKeyType*)ACDB_MALLOC(sizeof(AcdbDataLookupKeyType));

				if (pCur->pKey != NULL)
				{
					ACDB_MEM_CPY((void*)pCur->pKey,sizeof(AcdbDataLookupKeyType),(void*)pKey,sizeof(AcdbDataLookupKeyType));
					pCur->pNext = NULL;

					pCur->pDataNode = pDataNode;
					pCur->pDataNode->refcount++;

					pTblOnHeap->pAdieTblTail->pNext = pCur;
					pTblOnHeap->pAdieTblTail = pCur;
					pTblOnHeap->pAdieTblTail->pNext = NULL;

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
		ACDB_DEBUG_LOG ("[ACDB Linked_List ERROR]->[CreateAdieTableNodeOnHeap]->NULL Input pointer");
	}
	return result;
}
int append(int num,struct node **head )
{
	struct node *temp = NULL,*right = NULL;
	temp= (struct node *)malloc(sizeof(struct node));
	if(temp==NULL)
		return 0;
	temp->data=num;
	if(*head==NULL)
	{
		*head=temp;
		(*head)->next=NULL;
		return 1;
	}
	right=(struct node *)*head;
	while(right->next != NULL)
		right=right->next;
	right->next =temp;
	right=temp;
	right->next=NULL;
	return 1;
}
//this function looks for the index location and updates with new value if the index is not availble appeds the new value
int update(int num,int index,struct node **head )
{
	struct node *temp = NULL;
	int i=0;
	temp=*head;
	while(temp!=NULL)
	{
		if(index==i)
		{
			temp->data=num;
			return 1;
		}
		else
		{
			temp= temp->next;
			i++;
		}
	}
	return append(num,head);
}
//this function looks for element in the list
int Find(int num,struct node **head ,int *index)
{
	struct node *temp = NULL;
	int i=0;
	temp=*head;
	while(temp!=NULL)
	{
		if(temp->data==num)
		{
			*index=i;
			return 1;
		}
		else
		{
			temp= temp->next;
			i++;
		}
	}
	return 0;
}
//this function gets element at random position
int GetAt(int index,struct node **head )
{
	int i=0;
	struct node *temp = NULL;
	temp=*head;
	while(temp!=NULL)
	{
		if(i==index)
			return(temp->data);
		temp= temp->next;
		i++;
	}
	return -1;
}
/* Function to delete the entire linked list */
void deleteList(struct node** head_ref)
{ /* deref head_ref to get the real head */
	struct node* current = *head_ref;
	struct node* next = NULL;
	while (current != NULL)
	{
		next = current->next;
		free(current);
		current = next;
	}
	/* deref head_ref to affect the real head back in the caller. */
	*head_ref = NULL;
}
