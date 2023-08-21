/*===========================================================================
FILE: acdb_init_utility.c

OVERVIEW: This file contains the acdb init utility functions
implemented specifically in the win32 environment.

DEPENDENCIES: None

Copyright (c) 2010-2018, 2020 Qualcomm Technologies, Inc.
All Rights Reserved.
Confidential and Proprietary - Qualcomm Technologies, Inc.
========================================================================== */

/*===========================================================================
EDIT HISTORY FOR MODULE

This section contains comments describing changes made to the module.
Notice that changes are listed in reverse chronological order. Please
use ISO format for dates.

$Header: //source/qcom/qct/multimedia2/Audio/audcal4/acdb_sw/dev/fluence.x/acdb/src/acdb_data_mgr.c#6 $

when who what, where, why
---------- --- -----------------------------------------------------
2020-03-25  kde  Added support for getting offloaded parameter calibration
                 This feature was introduced to load Fluene X Neural Net
                 Models from the file system instead of storing the model
                 in the database
2014-05-28 mh SW migration from 32-bit to 64-bit architecture
2010-07-08 vmn Initial revision.

========================================================================== */

/* ---------------------------------------------------------------------------
* Include Files
*--------------------------------------------------------------------------- */

#include "acdb_data_mgr.h"
#include "acdb_datainfo.h"
#include "acdb_utility.h"
#include "acdb_instance_override.h"
#include "acdb_private.h"
#include "acdb_init_utility.h"

#define UNREFERENCED_VAR(param) (param)

int32_t GetMidPidIndex(ContentDefTblType cdefTbl,uint32_t mid,uint32_t pid,uint32_t *index)
{
	uint32_t i = 0;
	uint8_t blnFound = 0;
	int32_t result = ACDB_ERROR;
	if(index == NULL)
	{
		return ACDB_BADPARM;
	}
	for(i=0;i<cdefTbl.nLen;i++)
	{
		if(cdefTbl.pCntDef[i].nMid == mid &&
			cdefTbl.pCntDef[i].nPid == pid)
		{
			blnFound = 1;
			*index = i;
			break;
		}
	}
	if(blnFound == 1)
		result = ACDB_SUCCESS;
	return result;
}

int32_t GetCalibData(uint32_t tblId,uintptr_t nLookupKey,uint32_t dataOffset,AcdbDataInfo dataPoolChnk,
	uint8_t *pDstBuff, uint32_t nDstBuffLen,uint32_t *pDstBytesUsed)
{
	AcdbDataType cData = { 0, NULL };
	AcdbDataLookupKeyType dataLookupKey = { 0, 0 };
	AcdbDynamicUniqueDataType *pDataNode = NULL;
	uint32_t result = ACDB_SUCCESS;

	dataLookupKey.nTableId = tblId;
	dataLookupKey.nLookupKey = nLookupKey;

	if(pDstBuff == NULL || pDstBytesUsed == NULL)
	{
		return ACDB_BADPARM;
	}

	if(dataPoolChnk.pData == NULL)
	{
		ACDB_DEBUG_LOG("Datapool table not provided to look for the data\n");
		return ACDB_ERROR;
	}
	if(dataOffset >= dataPoolChnk.nDataLen)
	{
		ACDB_DEBUG_LOG("Invalid dataoffset provided to retrieve the data from datapool table\n");
		return ACDB_ERROR;
	}
	// Now get the datalen and data pointer

	result = Acdb_DM_Instance_Ioctl((uint32_t) ACDB_GET_ADIE_TABLE,
		FALSE,
		0,
		(AcdbDataLookupKeyType*) &dataLookupKey,
		0,
		NULL,
		NULL,
		0,
		NULL,
		0,
		NULL,
		0,
		NULL,
		(AcdbDynamicUniqueDataType**) &pDataNode,
		NULL,
		0
		);

	if(result == ACDB_SUCCESS)
	{
		if(pDataNode == NULL)
		{
			ACDB_DEBUG_LOG("Issue with heap, Unable to retrieve data from Heap\n");
			return ACDB_ERROR;
		}
		cData.nLen = pDataNode->ulDataLen;
		cData.pData = pDataNode->ulDataBuf;
	}
	else
	{
		// Now get the datalen and data pointer
		cData.nLen = READ_UInt32(dataPoolChnk.pData + dataOffset);
		cData.pData = dataPoolChnk.pData + dataOffset + sizeof(cData.nLen);
	}

	if(nDstBuffLen < cData.nLen)
	{
		ACDB_DEBUG_LOG("insufficient memory provided to copy the requested data\n");
		return ACDB_INSUFFICIENTMEMORY;
	}

	ACDB_MEM_CPY ((void *) pDstBuff,nDstBuffLen,(const void *)cData.pData,cData.nLen);
	*pDstBytesUsed = cData.nLen;

	return ACDB_SUCCESS;
}

int32_t GetADIEprofileSize(uint32_t tblId,uintptr_t nLookupKey,uint32_t dataOffset,AcdbDataInfo dataPoolChnk,
	uint32_t *pDstBytesUsed)
{
	AcdbDataType cData = { 0, NULL };
	AcdbDataLookupKeyType dataLookupKey = { 0, 0 };
	AcdbDynamicUniqueDataType *pDataNode = NULL;
	uint32_t result = ACDB_SUCCESS;

	dataLookupKey.nTableId = tblId;
	dataLookupKey.nLookupKey = nLookupKey;

	if(dataPoolChnk.pData == NULL)
	{
		ACDB_DEBUG_LOG("Datapool table not provided to look for the data\n");
		return ACDB_ERROR;
	}
	if(dataOffset >= dataPoolChnk.nDataLen)
	{
		ACDB_DEBUG_LOG("Invalid dataoffset provided to retrieve the data from datapool table\n");
		return ACDB_ERROR;
	}
	// Now get the datalen and data pointer

	result = Acdb_DM_Instance_Ioctl((uint32_t) ACDB_GET_ADIE_TABLE,
		FALSE,
		0,
		(AcdbDataLookupKeyType*) &dataLookupKey,
		0,
		NULL,
		NULL,
		0,
		NULL,
		0,
		NULL,
		0,
		NULL,
		(AcdbDynamicUniqueDataType**) &pDataNode,
		NULL,
		0
		);

	if(result == ACDB_SUCCESS)
	{
		if(pDataNode == NULL)
		{
			ACDB_DEBUG_LOG("Issue with heap, Unable to retrieve data from Heap\n");
			return ACDB_ERROR;
		}
		cData.nLen = pDataNode->ulDataLen;
		cData.pData = pDataNode->ulDataBuf;
	}
	else
	{
		// Now get the datalen and data pointer
		cData.nLen = READ_UInt32(dataPoolChnk.pData + dataOffset);
		cData.pData = dataPoolChnk.pData + dataOffset + sizeof(cData.nLen);
	}
	*pDstBytesUsed = cData.nLen;
	return ACDB_SUCCESS;
}

int32_t SetCalibData(uint32_t persistData, uint32_t deltaFileIndex, uint32_t tblId,uintptr_t nLookupKey, uint8_t *pIndices,uint32_t nIdxCount, uint32_t dataOffset,AcdbDataInfo dataPoolChnk,
	uint8_t *pInBuff, uint32_t nInBuffLen)
{
	AcdbDataLookupKeyType dataLookupKey = { 0, 0 };
	AcdbDynamicUniqueDataType *pDataNode = NULL;
	uint32_t result = ACDB_SUCCESS;

	dataLookupKey.nTableId = tblId;
	dataLookupKey.nLookupKey = nLookupKey;

	if(dataPoolChnk.pData == NULL)
	{
		ACDB_DEBUG_LOG("Datapool table not provided to look for the data\n");
		return ACDB_ERROR;
	}
	if(dataOffset >= dataPoolChnk.nDataLen)
	{
		ACDB_DEBUG_LOG("Invalid dataoffset provided to retrieve the data from datapool table\n");
		return ACDB_ERROR;
	}
	// Now get the datalen and data pointer

	result = Acdb_DM_Instance_Ioctl((uint32_t) ACDB_SET_ADIE_TABLE,
		persistData,
		deltaFileIndex,
		(AcdbDataLookupKeyType*) &dataLookupKey,
		0,
		NULL,
		pInBuff,
		nInBuffLen,
		NULL,
		0,
		NULL,
		0,
		NULL,
		(AcdbDynamicUniqueDataType**) &pDataNode,
		pIndices,
		nIdxCount
		);

	if(result != ACDB_SUCCESS)
	{
		ACDB_DEBUG_LOG("Failed to do set operation\n");
		return result;
	}

	return ACDB_SUCCESS;
}

int32_t GetMidPidCalibData(uint32_t tblId,uintptr_t nLookupKey,ContentDefTblType cdefTbl,ContentDataOffsetsTblType cdotTbl,AcdbDataInfo dataPoolChnk,
	uint32_t mid,uint32_t pid,uint8_t *pDstBuff, uint32_t nDstBuffLen,
	uint32_t *pDstBytesUsed)
{
	uint32_t mpindex = 0;
	AcdbDataType cData = { 0, NULL };
	uint32_t result = ACDB_SUCCESS;
	AcdbDataLookupKeyType dataLookupKey = { 0, 0 };
	AcdbDynamicUniqueDataType *pDataNode = NULL;
	uintptr_t cdftLookupKey = 0;

	if(pDstBuff == NULL || pDstBytesUsed == NULL)
	{
		ACDB_DEBUG_LOG("Recieved pDstBuff or pDstBytesUsed buffer pointer with NULL value for tblid = %d\n",tblId);
		return ACDB_BADPARM;
	}

	if(cdefTbl.nLen != cdotTbl.nLen)
	{
		ACDB_DEBUG_LOG("The no of entries in CDEF and CDOT tables are not matching\n");
		return ACDB_ERROR;
	}

	if(dataPoolChnk.pData == NULL || cdefTbl.pCntDef == NULL || cdotTbl.pDataOffsets == NULL)
	{
		ACDB_DEBUG_LOG("Invalid tables provided to retrieve data\n");
		return ACDB_ERROR;
	}

	// Now find the (mid,pid) index from the CDef table
	if(ACDB_SUCCESS != GetMidPidIndex(cdefTbl,mid,pid,&mpindex))
	{
		ACDB_DEBUG_LOG("mid %08X and pid %08X not found\n",mid,pid);
		return ACDB_PARMNOTFOUND;
	}

	if(mpindex >= dataPoolChnk.nDataLen)
	{
		ACDB_DEBUG_LOG("Invalid dataoffset provided to retrieve the data from datapool table\n");
		return ACDB_ERROR;
	}

	dataLookupKey.nTableId = tblId;
	dataLookupKey.nLookupKey = nLookupKey;
	cdftLookupKey = (uintptr_t)&cdefTbl.pCntDef[mpindex];
	result = Acdb_DM_Instance_Ioctl((uint32_t) ACDB_GET_DATA,
		FALSE,
		0,
		(AcdbDataLookupKeyType*) &dataLookupKey,
		cdftLookupKey,
		(uint32_t*) &pid,
		NULL,
		0,
		NULL,
		0,
		NULL,
		0,
		NULL,
		(AcdbDynamicUniqueDataType**) &pDataNode,
		NULL,
		0
		);

	if(result == ACDB_SUCCESS)
	{
		if(pDataNode == NULL)
		{
			ACDB_DEBUG_LOG("Issue with heap, Unable to retrieve data from Heap\n");
			return ACDB_ERROR;
		}
		cData.nLen = pDataNode->ulDataLen;
		cData.pData = pDataNode->ulDataBuf;
	}
	else
	{
		// Now get the datalen and data pointer
		cData.nLen = READ_UInt32(dataPoolChnk.pData + cdotTbl.pDataOffsets[mpindex]);
		cData.pData = dataPoolChnk.pData + cdotTbl.pDataOffsets[mpindex] + sizeof(cData.nLen);
	}

	if(nDstBuffLen < cData.nLen)
	{
		ACDB_DEBUG_LOG("insufficient memory provided to copy the requested data\n");
		return ACDB_INSUFFICIENTMEMORY;
	}

	ACDB_MEM_CPY ((void *) pDstBuff,nDstBuffLen,(const void *)cData.pData,cData.nLen);
	*pDstBytesUsed = cData.nLen;

	return ACDB_SUCCESS;
}

int32_t GetMidPidCalibCVDData(uint32_t tblId,uintptr_t nLookupKey,uintptr_t nSecLookupKey,ContentDefTblType cdefTbl,ContentDataOffsetsTblType cdotTbl,AcdbDataInfo dataPoolChnk,
	uint32_t mid,uint32_t pid,uint8_t *pDstBuff, uint32_t nDstBuffLen,
	uint32_t *pDstBytesUsed)
{
	uint32_t mpindex = 0;
	AcdbDataType cData = { 0, NULL };
	uint32_t result = ACDB_SUCCESS;
	AcdbDataLookupCVDKeyType dataLookupKey = { 0, 0, 0 };
	uintptr_t cdftLookupKey = 0;
	AcdbDynamicUniqueDataType *pDataNode = NULL;
	if(pDstBuff == NULL || pDstBytesUsed == NULL)
	{
		return ACDB_BADPARM;
	}
	if(cdefTbl.nLen != cdotTbl.nLen)
	{
		ACDB_DEBUG_LOG("The no of entries in CDEF and CDOT tables are not matching\n");
		return ACDB_ERROR;
	}

	if(dataPoolChnk.pData == NULL || cdefTbl.pCntDef == NULL || cdotTbl.pDataOffsets == NULL)
	{
		ACDB_DEBUG_LOG("Invalid tables provided to retrieve data\n");
		return ACDB_ERROR;
	}

	// Now find the (mid,pid) index from the CDef table
	if(ACDB_SUCCESS != GetMidPidIndex(cdefTbl,mid,pid,&mpindex))
	{
		ACDB_DEBUG_LOG("mid %08X and pid %08X not found\n",mid,pid);
		return ACDB_PARMNOTFOUND;
	}

	if(mpindex >= dataPoolChnk.nDataLen)
	{
		ACDB_DEBUG_LOG("Invalid dataoffset provided to retrieve the data from datapool table\n");
		return ACDB_ERROR;
	}

	dataLookupKey.nTableId = tblId;
	dataLookupKey.nLookupKey = nLookupKey;
	dataLookupKey.nSecondaryLookupKey = nSecLookupKey;
	cdftLookupKey = (uintptr_t)&cdefTbl.pCntDef[mpindex];
	result = Acdb_Voice_DM_Instance_Ioctl((uint32_t) ACDB_GET_DATA,
		FALSE,
		0,
		(AcdbDataLookupCVDKeyType*) &dataLookupKey,
		cdftLookupKey,
		(uint32_t*) &pid,
		NULL,
		0,
		NULL,
		0,
		NULL,
		0,
		NULL,
		(AcdbDynamicUniqueDataType**) &pDataNode,
		NULL,
		0
		);

	if(result == ACDB_SUCCESS)
	{
		if(pDataNode == NULL)
		{
			ACDB_DEBUG_LOG("Issue with heap, Unable to retrieve data from Heap\n");
			return ACDB_ERROR;
		}
		cData.nLen = pDataNode->ulDataLen;
		cData.pData = pDataNode->ulDataBuf;
	}
	else
	{
		// Now get the datalen and data pointer
		cData.nLen = READ_UInt32(dataPoolChnk.pData + cdotTbl.pDataOffsets[mpindex]);
		cData.pData = dataPoolChnk.pData + cdotTbl.pDataOffsets[mpindex] + sizeof(cData.nLen);
	}

	if(nDstBuffLen < cData.nLen)
	{
		ACDB_DEBUG_LOG("insufficient memory provided to copy the requested data\n");
		return ACDB_INSUFFICIENTMEMORY;
	}

	ACDB_MEM_CPY ((void *) pDstBuff,nDstBuffLen,(const void *)cData.pData,cData.nLen);
	*pDstBytesUsed = cData.nLen;

	return ACDB_SUCCESS;
}
int32_t SetMidPidCalibCVDData(uint32_t persistData, uint32_t deltaFileIndex, uint32_t tblId,uintptr_t nLookupKey,uintptr_t nSecLookupKey, uint8_t *pIndices,uint32_t nIdxCount, ContentDefTblType cdefTbl,ContentDataOffsetsTblType cdotTbl,
	AcdbDataInfo dataPoolChnk, uint32_t mid,uint32_t pid,uint8_t *pInBuff, uint32_t nInBuffLen)
{
	uint32_t result = ACDB_SUCCESS;
	uint32_t mpindex = 0;
	AcdbDataType cData = { 0, NULL };
	uintptr_t cdftLookupKey = 0;
	AcdbDataLookupCVDKeyType dataLookupKey = { 0, 0, 0 };
	if(cdefTbl.nLen != cdotTbl.nLen)
	{
		ACDB_DEBUG_LOG("The no of entries in CDEF and CDOT tables are not matching\n");
		return ACDB_ERROR;
	}

	if(dataPoolChnk.pData == NULL || cdefTbl.pCntDef == NULL || cdotTbl.pDataOffsets == NULL)
	{
		ACDB_DEBUG_LOG("Invalid tables provided to retrieve data\n");
		return ACDB_ERROR;
	}

	// Now find the (mid,pid) index from the CDef table
	if(ACDB_SUCCESS != GetMidPidIndex(cdefTbl,mid,pid,&mpindex))
	{
		ACDB_DEBUG_LOG("mid %08X and pid %08X not found\n",mid,pid);
		return ACDB_ERROR;
	}

	if(mpindex >= dataPoolChnk.nDataLen)
	{
		ACDB_DEBUG_LOG("Invalid dataoffset provided to retrieve the data from datapool table\n");
		return ACDB_ERROR;
	}

	// Now get the datalen and data pointer
	cData.nLen = READ_UInt32(dataPoolChnk.pData + cdotTbl.pDataOffsets[mpindex]);
	cData.pData = dataPoolChnk.pData + cdotTbl.pDataOffsets[mpindex] + sizeof(cData.nLen);

	dataLookupKey.nLookupKey = nLookupKey;
	dataLookupKey.nSecondaryLookupKey = nSecLookupKey;
	dataLookupKey.nTableId = tblId;
	cdftLookupKey = (uintptr_t)&cdefTbl.pCntDef[mpindex];
	result = Acdb_Voice_DM_Instance_Ioctl((uint32_t) ACDB_SET_DATA,
		persistData,
		deltaFileIndex,
		(AcdbDataLookupCVDKeyType*) &dataLookupKey,
		cdftLookupKey,
		(uint32_t*) &pid,
		pInBuff,
		nInBuffLen,
		(const uint8_t *)cData.pData,
		cData.nLen,
		NULL,
		0,
		NULL,
		NULL,
		pIndices,
		nIdxCount
		);

	if(result != ACDB_SUCCESS)
	{
		ACDB_DEBUG_LOG("Failed to do set operation\n");
		return result;
	}

	return ACDB_SUCCESS;
}
int32_t SetMidPidCalibData(uint32_t persistData, uint32_t deltaFileIndex, uint32_t tblId,uintptr_t nLookupKey, uint8_t *pIndices,uint32_t nIdxCount, ContentDefTblType cdefTbl,ContentDataOffsetsTblType cdotTbl,
	AcdbDataInfo dataPoolChnk, uint32_t mid,uint32_t pid,uint8_t *pInBuff, uint32_t nInBuffLen)
{
	uint32_t result = ACDB_SUCCESS;
	uint32_t mpindex = 0;
	AcdbDataType cData = { 0, NULL };
	AcdbDataLookupKeyType dataLookupKey = { 0, 0 };
	uintptr_t cdftLookupKey = 0;
	if(cdefTbl.nLen != cdotTbl.nLen)
	{
		ACDB_DEBUG_LOG("The no of entries in CDEF and CDOT tables are not matching\n");
		return ACDB_ERROR;
	}

	if(dataPoolChnk.pData == NULL || cdefTbl.pCntDef == NULL || cdotTbl.pDataOffsets == NULL)
	{
		ACDB_DEBUG_LOG("Invalid tables provided to retrieve data\n");
		return ACDB_ERROR;
	}

	// Now find the (mid,pid) index from the CDef table
	if(ACDB_SUCCESS != GetMidPidIndex(cdefTbl,mid,pid,&mpindex))
	{
		ACDB_DEBUG_LOG("mid %08X and pid %08X not found\n",mid,pid);
		return ACDB_ERROR;
	}

	if(mpindex >= dataPoolChnk.nDataLen)
	{
		ACDB_DEBUG_LOG("Invalid dataoffset provided to retrieve the data from datapool table\n");
		return ACDB_ERROR;
	}

	// Now get the datalen and data pointer
	cData.nLen = READ_UInt32(dataPoolChnk.pData + cdotTbl.pDataOffsets[mpindex]);
	cData.pData = dataPoolChnk.pData + cdotTbl.pDataOffsets[mpindex] + sizeof(cData.nLen);
	cdftLookupKey = (uintptr_t)&cdefTbl.pCntDef[mpindex];
	dataLookupKey.nLookupKey = nLookupKey;
	dataLookupKey.nTableId = tblId;
	result = Acdb_DM_Instance_Ioctl((uint32_t) ACDB_SET_DATA,
		persistData,
		deltaFileIndex,
		(AcdbDataLookupKeyType*) &dataLookupKey,
		cdftLookupKey,
		(uint32_t*) &pid,
		pInBuff,
		nInBuffLen,
		(const uint8_t *)cData.pData,
		cData.nLen,
		NULL,
		0,
		NULL,
		NULL,
		pIndices,
		nIdxCount
		);

	if(result != ACDB_SUCCESS)
	{
		ACDB_DEBUG_LOG("Failed to do set operation\n");
		return result;
	}

	return ACDB_SUCCESS;
}
int32_t GetMidPidCalibTable(uint32_t tblId,uintptr_t nLookupKey,ContentDefTblType cdefTbl,ContentDataOffsetsTblType cdotTbl,AcdbDataInfo dataPoolChnk,
	uint8_t *pDstBuff, uint32_t nDstBuffLen,uint32_t *pDstBytesUsed)
{
	uint32_t nMemBytesLeft=0;
	AcdbDataType cData = { 0, NULL };
	uint32_t i=0;
	uint32_t offset = 0;
	AcdbDataLookupKeyType dataLookupKey = { 0, 0 };
	AcdbDynamicUniqueDataType *pDataNode = NULL;
	uintptr_t cdftLookupKey = 0;

	dataLookupKey.nLookupKey = nLookupKey;
	dataLookupKey.nTableId = tblId;

	if(pDstBuff == NULL || pDstBytesUsed == NULL)
	{
		ACDB_DEBUG_LOG("Recieved pDstBuff or pDstBytesUsed pointer with NULL value for tblid = %d\n",tblId);
		return ACDB_BADPARM;
	}

	if(cdefTbl.nLen != cdotTbl.nLen)
	{
		ACDB_DEBUG_LOG("The no of entries in CDEF and CDOT tables are not matching\n");
		return ACDB_ERROR;
	}


	nMemBytesLeft = nDstBuffLen;
	
	for(i=0 ;i< cdefTbl.nLen;i++)
	{
		uint32_t nMemBytesRequired = 0;
		uint32_t nPaddedBytes = 0;
		DspCalHdrFormatType hdr = { 0, 0, 0, 0 };
		uint32_t pid = cdefTbl.pCntDef[i].nPid;
		uint32_t result = 0;

		cdftLookupKey = (uintptr_t)&cdefTbl.pCntDef[i];
		// Now get the datalen and data pointer
		result = Acdb_DM_Instance_Ioctl((uint32_t) ACDB_GET_DATA,
			FALSE,
			0,
			(AcdbDataLookupKeyType*) &dataLookupKey,
			cdftLookupKey,
			(uint32_t*) &pid,
			NULL,
			0,
			NULL,
			0,
			NULL,
			0,
			NULL,
			(AcdbDynamicUniqueDataType**) &pDataNode,
			NULL,
			0
			);

		if(result == ACDB_SUCCESS)
		{
			if(pDataNode == NULL)
			{
				ACDB_DEBUG_LOG("Issue with heap, Unable to retrieve data from Heap\n");
				return ACDB_ERROR;
			}
			cData.nLen = pDataNode->ulDataLen;
			cData.pData = pDataNode->ulDataBuf;
		}
		else
		{
			// Now get the datalen and data pointer
			cData.nLen = READ_UInt32(dataPoolChnk.pData + cdotTbl.pDataOffsets[i]);
			cData.pData = dataPoolChnk.pData + cdotTbl.pDataOffsets[i] + sizeof(cData.nLen);
		}

		if(cData.nLen%4)
		{
			nPaddedBytes = 4-cData.nLen%4;
		}

		nMemBytesRequired = (uint32_t)(sizeof(DspCalHdrFormatType) + cData.nLen + nPaddedBytes);

		if(nMemBytesLeft < nMemBytesRequired)
		{
			ACDB_DEBUG_LOG("insufficient memory provided to copy the requested data\n");
			return ACDB_INSUFFICIENTMEMORY;
		}

		hdr.nModuleId = cdefTbl.pCntDef[i].nMid;
		hdr.nParamId = cdefTbl.pCntDef[i].nPid;
		hdr.nParamSize = (uint16_t)(cData.nLen + nPaddedBytes);
		hdr.nReserved = 0;

		memset((void *)(pDstBuff+offset),0,nMemBytesRequired);
		ACDB_MEM_CPY ((void *) (pDstBuff+offset),nMemBytesLeft,(const void *)&hdr,sizeof(hdr));
		offset += (uint32_t)sizeof(hdr);
		ACDB_MEM_CPY ((void *) (pDstBuff+offset),(nMemBytesLeft-(sizeof(hdr))),(const void *)cData.pData,cData.nLen);
		offset += cData.nLen + nPaddedBytes;
		nMemBytesLeft -= nMemBytesRequired;
	}
	*pDstBytesUsed = nDstBuffLen - nMemBytesLeft;
	return ACDB_SUCCESS;
}

int32_t GetMidPidCalibHeapDataEx(uint32_t tblId,uintptr_t nLookupKey,uintptr_t nSecLookupKey,uintptr_t cdftLookup,uint32_t pid,AcdbDataType *cData)
{
	AcdbDataLookupCVDKeyType dataLookupKey = { 0, 0, 0 };
	AcdbDynamicUniqueDataType *pDataNode = NULL;
	uint32_t result =0;
	dataLookupKey.nLookupKey = nLookupKey;
	dataLookupKey.nSecondaryLookupKey = nSecLookupKey;
	dataLookupKey.nTableId = tblId;

	if(cData == NULL)
	{
		return ACDB_BADPARM;
	}

	// Now get the datalen and data pointer
	result = Acdb_Voice_DM_Instance_Ioctl((uint32_t) ACDB_GET_DATA,
		FALSE,
		0,
		(AcdbDataLookupCVDKeyType*) &dataLookupKey,
		cdftLookup,
		(uint32_t*) &pid,
		NULL,
		0,
		NULL,
		0,
		NULL,
		0,
		NULL,
		(AcdbDynamicUniqueDataType**) &pDataNode,
		NULL,
		0
		);

	if(result == ACDB_SUCCESS)
	{
		if(pDataNode == NULL)
		{
			ACDB_DEBUG_LOG("Issue with heap, Unable to retrieve data from Heap\n");
			return ACDB_ERROR;
		}
		cData->nLen = pDataNode->ulDataLen;
		cData->pData = pDataNode->ulDataBuf;
		return ACDB_SUCCESS;
	}
	else
	{
		return ACDB_ERROR;
	}
}

int32_t GetMidPidCalibHeapData(uint32_t tblId,uintptr_t nLookupKey,uintptr_t ncdftLookupKey,uint32_t pid,AcdbDataType *cData)
{
	AcdbDataLookupKeyType dataLookupKey = { 0, 0 };
	AcdbDynamicUniqueDataType *pDataNode = NULL;
	uint32_t result =0;
	dataLookupKey.nLookupKey = nLookupKey;
	dataLookupKey.nTableId = tblId;
	
	if(cData == NULL)
	{
		return ACDB_BADPARM;
	}

	// Now get the datalen and data pointer
	result = Acdb_DM_Instance_Ioctl((uint32_t) ACDB_GET_DATA,
		FALSE,
		0,
		(AcdbDataLookupKeyType*) &dataLookupKey,
		ncdftLookupKey,
		(uint32_t*) &pid,
		NULL,
		0,
		NULL,
		0,
		NULL,
		0,
		NULL,
		(AcdbDynamicUniqueDataType**) &pDataNode,
		NULL,
		0
		);

	if(result == ACDB_SUCCESS)
	{
		if(pDataNode == NULL)
		{
			ACDB_DEBUG_LOG("Issue with heap, Unable to retrieve data from Heap\n");
			return ACDB_ERROR;
		}
		cData->nLen = pDataNode->ulDataLen;
		cData->pData = pDataNode->ulDataBuf;
		return ACDB_SUCCESS;
	}
	else
	{
		return ACDB_ERROR;
	}
}

int32_t GetMidPidCalibTableSize(uint32_t tblId,uintptr_t nLookupKey,ContentDefTblType cdefTbl,ContentDataOffsetsTblType cdotTbl,AcdbDataInfo dataPoolChnk,
	uint32_t *pSize)
{
	AcdbDataType cData = { 0, NULL };
	uint32_t i=0;
	uint32_t nSizeRequired=0;
	AcdbDataLookupKeyType dataLookupKey = { 0, 0 };
	AcdbDynamicUniqueDataType *pDataNode = NULL;
	uintptr_t cdftLookupKey = 0;
	uint32_t result = 0;
	dataLookupKey.nLookupKey = nLookupKey;
	dataLookupKey.nTableId = tblId;

	if(pSize == NULL)
	{
		ACDB_DEBUG_LOG("Recieved pSize pointer with NULL value for tblid = %d\n",tblId);
		return ACDB_BADPARM;
	}

	if(cdefTbl.nLen != cdotTbl.nLen)
	{
		ACDB_DEBUG_LOG("The no of entries in CDEF and CDOT tables are not matching\n");
		return ACDB_ERROR;
	}
	for(i=0 ;i< cdefTbl.nLen;i++)
	{
		uint32_t nPaddedBytes = 0;
		// Now get the datalen and data pointer
		uint32_t pid = cdefTbl.pCntDef[i].nPid;

		cdftLookupKey = (uintptr_t)&cdefTbl.pCntDef[i];
		result = Acdb_DM_Instance_Ioctl((uint32_t) ACDB_GET_DATA,
			FALSE,
			0,
			(AcdbDataLookupKeyType*) &dataLookupKey,
			cdftLookupKey,
			(uint32_t*) &pid,
			NULL,
			0,
			NULL,
			0,
			NULL,
			0,
			NULL,
			(AcdbDynamicUniqueDataType**) &pDataNode,
			NULL,
			0
			);

		if(result == ACDB_SUCCESS)
		{
			if(pDataNode == NULL)
			{
				ACDB_DEBUG_LOG("Issue with heap, Unable to retrieve data from Heap\n");
				return ACDB_ERROR;
			}
			cData.nLen = pDataNode->ulDataLen;
		}
		else
		{
			cData.nLen = READ_UInt32(dataPoolChnk.pData + cdotTbl.pDataOffsets[i]);
		}

		if(cData.nLen%4)
		{
			nPaddedBytes = 4-cData.nLen%4;
		}

		nSizeRequired += (uint32_t)(sizeof(DspCalHdrFormatType) + cData.nLen + nPaddedBytes);
	}
	*pSize = nSizeRequired;
	return ACDB_SUCCESS;
}

int32_t GetNoOfTblEntriesOnHeap(uint8_t *pCmd,uint32_t nCmdSize ,uint8_t *pRsp,uint32_t nRspSizse)
{
	int32_t result = ACDB_SUCCESS;
	if (pCmd == NULL || pRsp == NULL || nCmdSize == 0 || nRspSizse == 0)
	{
		ACDB_DEBUG_LOG("[ACDB Command]->Invalid NULL value parameters are provided to get tbl entries\n");
		return ACDB_ERROR;
	}
		result = Acdb_DM_Instance_Ioctl(ACDB_GET_NO_OF_TBL_ENTRIES_ON_HEAP, FALSE, 0, NULL,
		0,NULL,pCmd,nCmdSize,NULL,0,pRsp,nRspSizse,NULL,NULL, NULL, 0);

	return result;
}

int32_t GetTblEntriesOnHeap(uint8_t *pCmd,uint32_t nCmdSize ,uint8_t *pRsp,uint32_t nRspSizse)
{
	int32_t result = ACDB_SUCCESS;
	if (pCmd == NULL || pRsp == NULL || nCmdSize == 0 || nRspSizse == 0)
	{
		ACDB_DEBUG_LOG("[ACDB Command]->Invalid NULL value parameters are provided to get tbl entries\n");
		return ACDB_ERROR;
	}
	
	result = Acdb_DM_Instance_Ioctl(ACDB_GET_TBL_ENTRIES_ON_HEAP,FALSE, 0 ,NULL,
		0,NULL,pCmd,nCmdSize,NULL,0,pRsp,nRspSizse,NULL,NULL,NULL,0);
	

	return result;
}

int32_t ACDBHeapReset(void)
{
	int32_t result = ACDB_SUCCESS;
	
		result = Acdb_DM_Instance_Ioctl(ACDB_SYS_RESET, FALSE, 0, NULL,
		0,NULL,NULL,0,
		NULL,0,NULL,0,NULL,NULL, NULL, 0);
	

	return result;
}

int32_t GetMidPidCalibTableSizeWithIid(uint32_t tblId,ContentDefWithInstanceTblType cdefTbl,ContentDataOffsetsTblType cdotTbl,AcdbDataInfo dataPoolChnk,
	uint32_t *pSize)
{
	AcdbDataType cData = { 0, NULL };
	uint32_t i=0;
	uint32_t nSizeRequired=0;
	(void)tblId;
	if(pSize == NULL)
	{
		ACDB_DEBUG_LOG("Recieved pSize pointer with NULL value for tblid = %d\n",tblId);
		return ACDB_BADPARM;
	}

	if(cdefTbl.nLen != cdotTbl.nLen)
	{
		ACDB_DEBUG_LOG("The no of entries in CDEF and CDOT tables are not matching\n");
		return ACDB_ERROR;
	}
	for(i=0 ;i< cdefTbl.nLen;i++)
	{
		uint32_t nPaddedBytes = 0;
		cData.nLen = READ_UInt32(dataPoolChnk.pData + cdotTbl.pDataOffsets[i]);
		if(cData.nLen%4)
		{
			nPaddedBytes = 4-cData.nLen%4;
		}
		nSizeRequired += (cData.nLen + nPaddedBytes);
	}
	*pSize = nSizeRequired;
	return ACDB_SUCCESS;
}

int32_t GetMidPidCalibTableWithIid(uint32_t tblId,ContentDefWithInstanceTblType cdefTbl,ContentDataOffsetsTblType cdotTbl,AcdbDataInfo dataPoolChnk,
	uint8_t *pDstBuff, uint32_t nDstBuffLen,uint32_t *pDstBytesUsed)
{
	uint32_t nMemBytesLeft=0;
	AcdbDataType cData = { 0, NULL };
	uint32_t i=0;
	uint32_t offset = 0;
	(void)tblId;
	if(pDstBuff == NULL || pDstBytesUsed == NULL)
	{
		ACDB_DEBUG_LOG("Recieved pDstBuff or pDstBytesUsed pointer with NULL value for tblid = %d\n",tblId);
		return ACDB_BADPARM;
	}

	if(cdefTbl.nLen != cdotTbl.nLen)
	{
		ACDB_DEBUG_LOG("The no of entries in CDEF and CDOT tables are not matching\n");
		return ACDB_ERROR;
	}

	nMemBytesLeft = nDstBuffLen;
	for(i=0 ;i< cdefTbl.nLen;i++)
	{
		uint32_t nMemBytesRequired = 0;
		uint32_t nPaddedBytes = 0;
		// Now get the datalen and data pointer
		cData.nLen = READ_UInt32(dataPoolChnk.pData + cdotTbl.pDataOffsets[i]);
		cData.pData = dataPoolChnk.pData + cdotTbl.pDataOffsets[i] + sizeof(cData.nLen);

		if(cData.nLen%4)
		{
			nPaddedBytes = 4-cData.nLen%4;
		}

		nMemBytesRequired = cData.nLen + nPaddedBytes;

		if(nMemBytesLeft < nMemBytesRequired)
		{
			ACDB_DEBUG_LOG("insufficient memory provided to copy the requested data\n");
			return ACDB_INSUFFICIENTMEMORY;
		}

		memset((void *)(pDstBuff+offset),0,nMemBytesRequired);
		ACDB_MEM_CPY ((void *) (pDstBuff+offset),nMemBytesLeft,(const void *)cData.pData,cData.nLen);
		offset += cData.nLen + nPaddedBytes;
		nMemBytesLeft -= nMemBytesRequired;
	}
	*pDstBytesUsed = nDstBuffLen - nMemBytesLeft;
	return ACDB_SUCCESS;
}

int32_t GetModuleMetaInfoWithoutPadding(uint32_t tblId,ContentDefWithOnlyParamTblType cdefTbl,ContentDataOffsetsTblType cdotTbl,AcdbDataInfo dataPoolChnk,
	uint8_t *pDstBuff, uint32_t nDstBuffLen,uint32_t *pDstBytesUsed)
{
	uint32_t nMemBytesLeft=0;
	AcdbDataType cData = { 0, NULL };
	uint32_t i=0;
	uint32_t offset = 0;
	(void)tblId;
	if(pDstBuff == NULL || pDstBytesUsed == NULL)
	{
		ACDB_DEBUG_LOG("Recieved pDstBuff or pDstBytesUsed pointer with NULL value for tblid = %d\n",tblId);
		return ACDB_BADPARM;
	}

	if(cdefTbl.nLen != cdotTbl.nLen)
	{
		ACDB_DEBUG_LOG("The no of entries in CDEF and CDOT tables are not matching\n");
		return ACDB_ERROR;
	}

	nMemBytesLeft = nDstBuffLen;
	for(i=0 ;i< cdefTbl.nLen;i++)
	{
		uint32_t nMemBytesRequired = 0;
		// Now get the datalen and data pointer
			cData.nLen = READ_UInt32(dataPoolChnk.pData + cdotTbl.pDataOffsets[i]);
			cData.pData = dataPoolChnk.pData + cdotTbl.pDataOffsets[i] + sizeof(cData.nLen);


		nMemBytesRequired = cData.nLen;

		if(nMemBytesLeft < nMemBytesRequired)
		{
			ACDB_DEBUG_LOG("insufficient memory provided to copy the requested data\n");
			return ACDB_INSUFFICIENTMEMORY;
		}

		memset((void *)(pDstBuff+offset),0,nMemBytesRequired);

		ACDB_MEM_CPY ((void *) (pDstBuff+offset),nMemBytesLeft,(const void *)cData.pData,cData.nLen);
		offset += cData.nLen;
		nMemBytesLeft -= nMemBytesRequired;
	}
	*pDstBytesUsed = nDstBuffLen - nMemBytesLeft;
	return ACDB_SUCCESS;
}

int32_t GetModuleMetaInfoSizeWithoutPadding(uint32_t tblId,ContentDefWithOnlyParamTblType cdefTbl,ContentDataOffsetsTblType cdotTbl,AcdbDataInfo dataPoolChnk,
	uint32_t *pSize)
{
	AcdbDataType cData = { 0, NULL };
	uint32_t i=0;
	uint32_t nSizeRequired=0;
	(void)tblId;
	if(pSize == NULL)
	{
		ACDB_DEBUG_LOG("Recieved pSize pointer with NULL value for tblid = %d\n",tblId);
		return ACDB_BADPARM;
	}

	if(cdefTbl.nLen != cdotTbl.nLen)
	{
		ACDB_DEBUG_LOG("The no of entries in CDEF and CDOT tables are not matching\n");
		return ACDB_ERROR;
	}
	for(i=0 ;i< cdefTbl.nLen;i++)
	{
		//uint32_t nPaddedBytes = 0;
		cData.nLen = READ_UInt32(dataPoolChnk.pData + cdotTbl.pDataOffsets[i]);
		/*if(cData.nLen%4)
		{
			nPaddedBytes = 4-cData.nLen%4;
		}*/
		nSizeRequired += cData.nLen;
	}
	*pSize = nSizeRequired;
	return ACDB_SUCCESS;
}

int32_t GetIdcalibTable(uint32_t tblId,uintptr_t nLookupKey,ContentDefWithOnlyParamTblType cdefTbl,ContentDataOffsetsTblType cdotTbl,AcdbDataInfo dataPoolChnk,
	uint8_t *pDstBuff, uint32_t nDstBuffLen,uint32_t *pDstBytesUsed)
{
	uint32_t nMemBytesLeft=0;
	AcdbDataType cData = { 0, NULL };
	uint32_t i=0;
	uint32_t offset = 0;
	
	WdspModuleCalibDataLookupType *lookUp = (WdspModuleCalibDataLookupType *)nLookupKey;
	(void)tblId;
	if(pDstBuff == NULL || pDstBytesUsed == NULL)
	{
		ACDB_DEBUG_LOG("Recieved pDstBuff or pDstBytesUsed pointer with NULL value for tblid = %d\n", tblId);
		return ACDB_BADPARM;
	}

	if(cdefTbl.nLen != cdotTbl.nLen)
	{
		ACDB_DEBUG_LOG("The no of entries in CDEF and CDOT tables are not matching\n");
		return ACDB_ERROR;
	}

	nMemBytesLeft = nDstBuffLen;
	for(i=0 ;i< cdefTbl.nLen;i++)
	{
		uint32_t nMemBytesRequired = 0;
		uint32_t nPaddedBytes = 0;
		WdspCalHdrWithInstanceIDFormatType hdr = { 0, 0, 0, 0, 0, 0 };

		// Now get the datalen and data pointer
		cData.nLen = READ_UInt32(dataPoolChnk.pData + cdotTbl.pDataOffsets[i]);
		cData.pData = dataPoolChnk.pData + cdotTbl.pDataOffsets[i] + sizeof(cData.nLen);
		if(cData.nLen%4)
		{
			nPaddedBytes = 4-cData.nLen%4;
		}

		nMemBytesRequired = (uint32_t)(sizeof(WdspCalHdrWithInstanceIDFormatType) + cData.nLen + nPaddedBytes);

		if(nMemBytesLeft < nMemBytesRequired)
		{
			ACDB_DEBUG_LOG("insufficient memory provided to copy the requested data\n");
			return ACDB_INSUFFICIENTMEMORY;
		}

		hdr.nModuleId = lookUp->nMId;
		hdr.nInstanceId = (uint16_t)lookUp->nIId;
		hdr.nReserved1 = 0;
		hdr.nParamId = cdefTbl.pCntDef[i].nID;
		hdr.nParamSize = (uint16_t)(cData.nLen + nPaddedBytes);
		hdr.nReserved2 = 0;

		memset((void *)(pDstBuff+offset),0,nMemBytesRequired);
		ACDB_MEM_CPY ((void *) (pDstBuff+offset),nMemBytesLeft,(const void *)&hdr,sizeof(hdr));
		offset += (uint32_t)sizeof(hdr);
		ACDB_MEM_CPY ((void *) (pDstBuff+offset),(nMemBytesLeft-(sizeof(hdr))),(const void *)cData.pData,cData.nLen);
		offset += cData.nLen + nPaddedBytes;
		nMemBytesLeft -= nMemBytesRequired;
	}
	*pDstBytesUsed = nDstBuffLen - nMemBytesLeft;
	return ACDB_SUCCESS;
}

int32_t GetIdcalibTableSize(uint32_t tblId,ContentDefWithOnlyParamTblType cdefTbl,ContentDataOffsetsTblType cdotTbl,AcdbDataInfo dataPoolChnk,
	uint32_t *pSize)
{
	AcdbDataType cData = { 0, NULL };
	uint32_t i=0;
	uint32_t nSizeRequired=0;
	(void)tblId;
	if(pSize == NULL)
	{
		ACDB_DEBUG_LOG("Recieved pSize pointer with NULL value for tblid = %d\n", tblId);
		return ACDB_BADPARM;
	}

	if(cdefTbl.nLen != cdotTbl.nLen)
	{
		ACDB_DEBUG_LOG("The no of entries in CDEF and CDOT tables are not matching\n");
		return ACDB_ERROR;
	}
	for(i=0 ;i< cdefTbl.nLen;i++)
	{
		uint32_t nPaddedBytes = 0;
		// Now get the datalen and data pointer
			cData.nLen = READ_UInt32(dataPoolChnk.pData + cdotTbl.pDataOffsets[i]);
		if(cData.nLen%4)
		{
			nPaddedBytes = 4-cData.nLen%4;
		}
		nSizeRequired += (uint32_t)(sizeof(WdspCalHdrWithInstanceIDFormatType) + cData.nLen + nPaddedBytes);
	}
	*pSize = nSizeRequired;
	return ACDB_SUCCESS;
}

int32_t GetMidIidPidCalibTable(uint32_t tblId,uintptr_t nLookupKey,ContentDefWithInstanceTblType cdefTbl,ContentDataOffsetsTblType cdotTbl,AcdbDataInfo dataPoolChnk,
	uint8_t *pDstBuff, uint32_t nDstBuffLen,uint32_t *pDstBytesUsed)
{
	uint32_t nMemBytesLeft=0;
	AcdbDataType cData = { 0, NULL };
	uint32_t i=0;
	uint32_t offset = 0;
	AcdbDataLookupKeyType dataLookupKey = { 0, 0 };
	AcdbDynamicUniqueDataType *pDataNode = NULL;

	dataLookupKey.nLookupKey = nLookupKey;
	dataLookupKey.nTableId = tblId;

	if(pDstBuff == NULL || pDstBytesUsed == NULL)
	{
		ACDB_DEBUG_LOG("Recieved pDstBuff or pDstBytesUsed pointer with NULL value for tblid = %d\n",tblId);
		return ACDB_BADPARM;
	}

	if(cdefTbl.nLen != cdotTbl.nLen)
	{
		ACDB_DEBUG_LOG("The no of entries in CDEF and CDOT tables are not matching\n");
		return ACDB_ERROR;
	}

	nMemBytesLeft = nDstBuffLen;
	for(i=0 ;i< cdefTbl.nLen;i++)
	{
		uint32_t nMemBytesRequired = 0;
		uint32_t nPaddedBytes = 0;
		AdspCalHdrWithInstanceIDFormatType hdr = { 0, 0, 0, 0, 0 };
		uintptr_t cdftLookupKey = (uintptr_t)&cdefTbl.pCntDef[i];
		uint32_t pid = cdefTbl.pCntDef[i].nPid;

		//uintptr_t cdftPtr = (uintptr_t) &cdefTbl.pCntDef[i];
		// Now get the datalen and data pointer
		uint32_t result = Acdb_DM_Instance_Ioctl((uint32_t) ACDB_GET_DATA,
			FALSE,
			0,
			(AcdbDataLookupKeyType*) &dataLookupKey,
			cdftLookupKey,
			(uint32_t*) &pid,
			NULL,
			0,
			NULL,
			0,
			NULL,
			0,
			NULL,
			(AcdbDynamicUniqueDataType**) &pDataNode,
			NULL,
			0
			);

		if(result == ACDB_SUCCESS)
		{
			if(pDataNode == NULL)
			{
				ACDB_DEBUG_LOG("Issue with heap, Unable to retrieve data from Heap\n");
				return ACDB_ERROR;
			}
			cData.nLen = pDataNode->ulDataLen;
			cData.pData = pDataNode->ulDataBuf;
		}
		else
		{
			// Now get the datalen and data pointer
			cData.nLen = READ_UInt32(dataPoolChnk.pData + cdotTbl.pDataOffsets[i]);
			cData.pData = dataPoolChnk.pData + cdotTbl.pDataOffsets[i] + sizeof(cData.nLen);
		}

		if(cData.nLen%4)
		{
			nPaddedBytes = 4-cData.nLen%4;
		}

		nMemBytesRequired = (uint32_t)(sizeof(AdspCalHdrWithInstanceIDFormatType) + cData.nLen + nPaddedBytes);

		if(nMemBytesLeft < nMemBytesRequired)
		{
			ACDB_DEBUG_LOG("insufficient memory provided to copy the requested data\n");
			return ACDB_INSUFFICIENTMEMORY;
		}

		hdr.nModuleId = cdefTbl.pCntDef[i].nMid;
		hdr.nInstanceId = GetUint16IID(cdefTbl.pCntDef[i].nIid);
		hdr.nReserved1 = 0;
		hdr.nParamId = cdefTbl.pCntDef[i].nPid;
		hdr.nParamSize = (cData.nLen + nPaddedBytes);

		memset((void *)(pDstBuff+offset),0,nMemBytesRequired);
		ACDB_MEM_CPY ((void *) (pDstBuff+offset),nMemBytesLeft,(const void *)&hdr,sizeof(hdr));
		offset += (uint32_t)sizeof(hdr);
		ACDB_MEM_CPY ((void *) (pDstBuff+offset),(nMemBytesLeft-(sizeof(hdr))),(const void *)cData.pData,cData.nLen);
		offset += cData.nLen + nPaddedBytes;
		nMemBytesLeft -= nMemBytesRequired;
	}
	*pDstBytesUsed = nDstBuffLen - nMemBytesLeft;
	return ACDB_SUCCESS;
}

int32_t GetMidIidPidCalibTableSize(uint32_t tblId,uintptr_t nLookupKey,ContentDefWithInstanceTblType cdefTbl,ContentDataOffsetsTblType cdotTbl,AcdbDataInfo dataPoolChnk,
	uint32_t *pSize)
{
	AcdbDataType cData = { 0, NULL };
	uint32_t i=0;
	uint32_t nSizeRequired=0;
	AcdbDataLookupKeyType dataLookupKey = { 0, 0 };
	AcdbDynamicUniqueDataType *pDataNode = NULL;

	dataLookupKey.nLookupKey = nLookupKey;
	dataLookupKey.nTableId = tblId;

	if(pSize == NULL)
	{
		ACDB_DEBUG_LOG("Recieved pSize pointer with NULL value for tblid = %d\n",tblId);
		return ACDB_BADPARM;
	}

	if(cdefTbl.nLen != cdotTbl.nLen)
	{
		ACDB_DEBUG_LOG("The no of entries in CDEF and CDOT tables are not matching\n");
		return ACDB_ERROR;
	}
	for(i=0 ;i< cdefTbl.nLen;i++)
	{
		uint32_t nPaddedBytes = 0;
		// Now get the datalen and data pointer
		uintptr_t cdftLookupKey = (uintptr_t)&cdefTbl.pCntDef[i];
		uint32_t pid = cdefTbl.pCntDef[i].nPid;

		uint32_t result = Acdb_DM_Instance_Ioctl((uint32_t) ACDB_GET_DATA,
			FALSE,
			0,
			(AcdbDataLookupKeyType*) &dataLookupKey,
			cdftLookupKey,
			(uint32_t*) &pid,
			NULL,
			0,
			NULL,
			0,
			NULL,
			0,
			NULL,
			(AcdbDynamicUniqueDataType**) &pDataNode,
			NULL,
			0
			);

		if(result == ACDB_SUCCESS)
		{
			if(pDataNode == NULL)
			{
				ACDB_DEBUG_LOG("Issue with heap, Unable to retrieve data from Heap\n");
				return ACDB_ERROR;
			}
			cData.nLen = pDataNode->ulDataLen;
		}
		else
		{
			cData.nLen = READ_UInt32(dataPoolChnk.pData + cdotTbl.pDataOffsets[i]);
		}

		if(cData.nLen%4)
		{
			nPaddedBytes = 4-cData.nLen%4;
		}

		nSizeRequired += (uint32_t)(sizeof(AdspCalHdrWithInstanceIDFormatType) + cData.nLen + nPaddedBytes);
	}
	*pSize = nSizeRequired;
	return ACDB_SUCCESS;
}

int32_t GetMidIidPidIndex(ContentDefWithInstanceTblType cdefTbl,uint32_t mid,uint32_t iid, uint32_t pid,uint32_t *index)
{
	uint32_t i = 0;
	uint8_t blnFound = 0;
	int32_t result = ACDB_ERROR;
	if(index == NULL)
	{
		return ACDB_BADPARM;
	}
	for(i=0;i<cdefTbl.nLen;i++)
	{
		if(cdefTbl.pCntDef[i].nMid == mid &&
			cdefTbl.pCntDef[i].nIid == iid &&
			cdefTbl.pCntDef[i].nPid == pid)
		{
			blnFound = 1;
			*index = i;
			break;
		}
	}
	if(blnFound == 1)
		result = ACDB_SUCCESS;
	return result;
}

int32_t GetMidIidPidCalibData(uint32_t tblId,uintptr_t nLookupKey,ContentDefWithInstanceTblType cdefTbl,ContentDataOffsetsTblType cdotTbl,AcdbDataInfo dataPoolChnk,
	uint32_t mid,uint32_t iid, uint32_t pid,uint8_t *pDstBuff, uint32_t nDstBuffLen,
	uint32_t *pDstBytesUsed)
{
	uint32_t mpindex = 0;
	AcdbDataType cData = { 0, NULL };
	uint32_t result = ACDB_SUCCESS;
	AcdbDataLookupKeyType dataLookupKey = { 0, 0 };
	uintptr_t cdftLookupKey = 0;
	AcdbDynamicUniqueDataType *pDataNode = NULL;
	if(pDstBuff == NULL || pDstBytesUsed == NULL)
	{
		ACDB_DEBUG_LOG("Recieved pDstBuff or pDstBytesUsed buffer pointer with NULL value for tblid = %d\n",tblId);
		return ACDB_BADPARM;
	}

	if(cdefTbl.nLen != cdotTbl.nLen)
	{
		ACDB_DEBUG_LOG("The no of entries in CDEF and CDOT tables are not matching\n");
		return ACDB_ERROR;
	}

	if(dataPoolChnk.pData == NULL || cdefTbl.pCntDef == NULL || cdotTbl.pDataOffsets == NULL)
	{
		ACDB_DEBUG_LOG("Invalid tables provided to retrieve data\n");
		return ACDB_ERROR;
	}

	// Now find the (mid,pid) index from the CDef table
	if(ACDB_SUCCESS != GetMidIidPidIndex(cdefTbl,mid, iid, pid, &mpindex))
	{
		ACDB_DEBUG_LOG("mid %08X and pid %08X not found\n",mid,pid);
		return ACDB_PARMNOTFOUND;
	}

	if(mpindex >= dataPoolChnk.nDataLen)
	{
		ACDB_DEBUG_LOG("Invalid dataoffset provided to retrieve the data from datapool table\n");
		return ACDB_ERROR;
	}
	cdftLookupKey = (uintptr_t)&cdefTbl.pCntDef[mpindex];
	dataLookupKey.nTableId = tblId;
	dataLookupKey.nLookupKey = nLookupKey;
	result = Acdb_DM_Instance_Ioctl((uint32_t) ACDB_GET_DATA,
		FALSE,
		0,
		(AcdbDataLookupKeyType*) &dataLookupKey,
		cdftLookupKey,
		(uint32_t*) &pid,
		NULL,
		0,
		NULL,
		0,
		NULL,
		0,
		NULL,
		(AcdbDynamicUniqueDataType**) &pDataNode,
		NULL,
		0
		);

	if(result == ACDB_SUCCESS)
	{
		if(pDataNode == NULL)
		{
			ACDB_DEBUG_LOG("Issue with heap, Unable to retrieve data from Heap\n");
			return ACDB_ERROR;
		}
		cData.nLen = pDataNode->ulDataLen;
		cData.pData = pDataNode->ulDataBuf;
	}
	else
	{
		// Now get the datalen and data pointer
		cData.nLen = READ_UInt32(dataPoolChnk.pData + cdotTbl.pDataOffsets[mpindex]);
		cData.pData = dataPoolChnk.pData + cdotTbl.pDataOffsets[mpindex] + sizeof(cData.nLen);
	}

	if(nDstBuffLen < cData.nLen)
	{
		ACDB_DEBUG_LOG("insufficient memory provided to copy the requested data\n");
		return ACDB_INSUFFICIENTMEMORY;
	}

	ACDB_MEM_CPY ((void *) pDstBuff,nDstBuffLen,(const void *)cData.pData,cData.nLen);
	*pDstBytesUsed = cData.nLen;

	return ACDB_SUCCESS;
}

int32_t GetMidIidPidCalibHeapDataEx(uint32_t tblId,uintptr_t nLookupKey,uintptr_t nSecLookupKey,uintptr_t cdftLookup,uint32_t pid,AcdbDataType *cData)
{
	AcdbDataLookupCVDKeyType dataLookupKey = { 0, 0, 0 };
	AcdbDynamicUniqueDataType *pDataNode = NULL;
	uint32_t result =0;
	dataLookupKey.nLookupKey = nLookupKey;
	dataLookupKey.nSecondaryLookupKey = nSecLookupKey;
	dataLookupKey.nTableId = tblId;

	if(cData == NULL)
	{
		return ACDB_BADPARM;
	}

	// Now get the datalen and data pointer
	result = Acdb_Voice_DM_Instance_Ioctl((uint32_t) ACDB_GET_DATA,
		FALSE,
		0,
		(AcdbDataLookupCVDKeyType*) &dataLookupKey,
		cdftLookup,
		(uint32_t*) &pid,
		NULL,
		0,
		NULL,
		0,
		NULL,
		0,
		NULL,
		(AcdbDynamicUniqueDataType**) &pDataNode,
		NULL,
		0
		);

	if(result == ACDB_SUCCESS)
	{
		if(pDataNode == NULL)
		{
			ACDB_DEBUG_LOG("Issue with heap, Unable to retrieve data from Heap\n");
			return ACDB_ERROR;
		}
		cData->nLen = pDataNode->ulDataLen;
		cData->pData = pDataNode->ulDataBuf;
		return ACDB_SUCCESS;
	}
	else
	{
		return ACDB_ERROR;
	}
}

int32_t GetMidIidPidCalibHeapData(uint32_t tblId,uintptr_t nLookupKey,uintptr_t cdftLookup, uint32_t pid,AcdbDataType *cData)
{
	AcdbDataLookupKeyType dataLookupKey = { 0, 0 };
	AcdbDynamicUniqueDataType *pDataNode = NULL;
	uint32_t result =0;
	dataLookupKey.nLookupKey = nLookupKey;
	dataLookupKey.nTableId = tblId;
	if(cData == NULL)
	{
		return ACDB_BADPARM;
	}

	// Now get the datalen and data pointer
	result = Acdb_DM_Instance_Ioctl((uint32_t) ACDB_GET_DATA,
		FALSE,
		0,
		(AcdbDataLookupKeyType*) &dataLookupKey,
		cdftLookup,
		(uint32_t*) &pid,
		NULL,
		0,
		NULL,
		0,
		NULL,
		0,
		NULL,
		(AcdbDynamicUniqueDataType**) &pDataNode,
		NULL,
		0
		);

	if(result == ACDB_SUCCESS)
	{
		if(pDataNode == NULL)
		{
			ACDB_DEBUG_LOG("Issue with heap, Unable to retrieve data from Heap\n");
			return ACDB_ERROR;
		}
		cData->nLen = pDataNode->ulDataLen;
		cData->pData = pDataNode->ulDataBuf;
		return ACDB_SUCCESS;
	}
	else
	{
		return ACDB_ERROR;
	}
}

int32_t SetMidIidPidCalibCVDData(uint32_t persistData, uint32_t deltaFileIndex, uint32_t tblId,uintptr_t nLookupKey,uintptr_t nSecLookupKey, uint8_t *pIndices,uint32_t nIdxCount, ContentDefWithInstanceTblType cdefTbl,ContentDataOffsetsTblType cdotTbl,
	AcdbDataInfo dataPoolChnk, uint32_t mid,uint32_t pid, uint32_t iid, uint8_t *pInBuff, uint32_t nInBuffLen)
{
	uint32_t result = ACDB_SUCCESS;
	uint32_t mpindex = 0;
	AcdbDataType cData = { 0, NULL };
	AcdbDataLookupCVDKeyType dataLookupKey = { 0, 0, 0 };
	uintptr_t cdftLookupKey = 0;
	if(cdefTbl.nLen != cdotTbl.nLen)
	{
		ACDB_DEBUG_LOG("The no of entries in CDEF and CDOT tables are not matching\n");
		return ACDB_ERROR;
	}

	if(dataPoolChnk.pData == NULL || cdefTbl.pCntDef == NULL || cdotTbl.pDataOffsets == NULL)
	{
		ACDB_DEBUG_LOG("Invalid tables provided to retrieve data\n");
		return ACDB_ERROR;
	}

	// Now find the (mid,pid) index from the CDef table
	if(ACDB_SUCCESS != GetMidIidPidIndex(cdefTbl,mid,iid, pid, &mpindex))
	{
		ACDB_DEBUG_LOG("pid %08X not found\n",pid);
		return ACDB_ERROR;
	}

	if(mpindex >= dataPoolChnk.nDataLen)
	{
		ACDB_DEBUG_LOG("Invalid dataoffset provided to retrieve the data from datapool table\n");
		return ACDB_ERROR;
	}

	// Now get the datalen and data pointer
	cData.nLen = READ_UInt32(dataPoolChnk.pData + cdotTbl.pDataOffsets[mpindex]);
	cData.pData = dataPoolChnk.pData + cdotTbl.pDataOffsets[mpindex] + sizeof(cData.nLen);
	cdftLookupKey = (uintptr_t)&cdefTbl.pCntDef[mpindex];
	dataLookupKey.nLookupKey = nLookupKey;
	dataLookupKey.nSecondaryLookupKey = nSecLookupKey;
	dataLookupKey.nTableId = tblId;
	result = Acdb_Voice_DM_Instance_Ioctl((uint32_t) ACDB_SET_DATA,
		persistData,
		deltaFileIndex,
		(AcdbDataLookupCVDKeyType*) &dataLookupKey,
		cdftLookupKey,
		(uint32_t*) &pid,
		pInBuff,
		nInBuffLen,
		(const uint8_t *)cData.pData,
		cData.nLen,
		NULL,
		0,
		NULL,
		NULL,
		pIndices,
		nIdxCount
		);

	if(result != ACDB_SUCCESS)
	{
		ACDB_DEBUG_LOG("Failed to do set operation\n");
		return result;
	}

	return ACDB_SUCCESS;
}

int32_t GetMidIidPidCalibCVDData(uint32_t tblId,uintptr_t nLookupKey,uintptr_t nSecLookupKey,ContentDefWithInstanceTblType cdefTbl,ContentDataOffsetsTblType cdotTbl,AcdbDataInfo dataPoolChnk,
	uint32_t mid,uint32_t iid,uint32_t pid,uint8_t *pDstBuff, uint32_t nDstBuffLen,
	uint32_t *pDstBytesUsed)
{
	uint32_t mpindex = 0;
	AcdbDataType cData = { 0, NULL };
	uint32_t result = ACDB_SUCCESS;
	AcdbDataLookupCVDKeyType dataLookupKey = { 0, 0, 0 };
	uintptr_t cdftLookupKey = 0;
	AcdbDynamicUniqueDataType *pDataNode = NULL;
	if(pDstBuff == NULL || pDstBytesUsed == NULL)
	{
		return ACDB_BADPARM;
	}
	if(cdefTbl.nLen != cdotTbl.nLen)
	{
		ACDB_DEBUG_LOG("The no of entries in CDEF and CDOT tables are not matching\n");
		return ACDB_ERROR;
	}

	if(dataPoolChnk.pData == NULL || cdefTbl.pCntDef == NULL || cdotTbl.pDataOffsets == NULL)
	{
		ACDB_DEBUG_LOG("Invalid tables provided to retrieve data\n");
		return ACDB_ERROR;
	}

	// Now find the (mid,pid) index from the CDef table
	if(ACDB_SUCCESS != GetMidIidPidIndex(cdefTbl,mid,iid, pid,&mpindex))
	{
		ACDB_DEBUG_LOG("mid %08X and pid %08X not found\n",mid,pid);
		return ACDB_PARMNOTFOUND;
	}

	if(mpindex >= dataPoolChnk.nDataLen)
	{
		ACDB_DEBUG_LOG("Invalid dataoffset provided to retrieve the data from datapool table\n");
		return ACDB_ERROR;
	}

	dataLookupKey.nTableId = tblId;
	cdftLookupKey = (uintptr_t)&cdefTbl.pCntDef[mpindex];
	dataLookupKey.nLookupKey = nLookupKey;
	dataLookupKey.nSecondaryLookupKey = nSecLookupKey;
	result = Acdb_Voice_DM_Instance_Ioctl((uint32_t) ACDB_GET_DATA,
		FALSE,
		0,
		(AcdbDataLookupCVDKeyType*) &dataLookupKey,
		cdftLookupKey,
		(uint32_t*) &pid,
		NULL,
		0,
		NULL,
		0,
		NULL,
		0,
		NULL,
		(AcdbDynamicUniqueDataType**) &pDataNode,
		NULL,
		0
		);

	if(result == ACDB_SUCCESS)
	{
		if(pDataNode == NULL)
		{
			ACDB_DEBUG_LOG("Issue with heap, Unable to retrieve data from Heap\n");
			return ACDB_ERROR;
		}
		cData.nLen = pDataNode->ulDataLen;
		cData.pData = pDataNode->ulDataBuf;
	}
	else
	{
		// Now get the datalen and data pointer
		cData.nLen = READ_UInt32(dataPoolChnk.pData + cdotTbl.pDataOffsets[mpindex]);
		cData.pData = dataPoolChnk.pData + cdotTbl.pDataOffsets[mpindex] + sizeof(cData.nLen);
	}

	if(nDstBuffLen < cData.nLen)
	{
		ACDB_DEBUG_LOG("insufficient memory provided to copy the requested data\n");
		return ACDB_INSUFFICIENTMEMORY;
	}

	ACDB_MEM_CPY ((void *) pDstBuff,nDstBuffLen,(const void *)cData.pData,cData.nLen);
	*pDstBytesUsed = cData.nLen;

	return ACDB_SUCCESS;
}

int32_t SetMidIidPidCalibData(uint32_t persistData, uint32_t deltaFileIndex, uint32_t tblId,uintptr_t nLookupKey, uint8_t *pIndices,uint32_t nIdxCount, ContentDefWithInstanceTblType cdefTbl,ContentDataOffsetsTblType cdotTbl,
	AcdbDataInfo dataPoolChnk, uint32_t mid,uint32_t iid, uint32_t pid,uint8_t *pInBuff, uint32_t nInBuffLen)
{
	uint32_t result = ACDB_SUCCESS;
	uint32_t mpindex = 0;
	AcdbDataType cData = { 0, NULL };
	AcdbDataLookupKeyType dataLookupKey = { 0, 0 };
	uintptr_t cdftLookupKey = 0;
	if(cdefTbl.nLen != cdotTbl.nLen)
	{
		ACDB_DEBUG_LOG("The no of entries in CDEF and CDOT tables are not matching\n");
		return ACDB_ERROR;
	}

	if(dataPoolChnk.pData == NULL || cdefTbl.pCntDef == NULL || cdotTbl.pDataOffsets == NULL)
	{
		ACDB_DEBUG_LOG("Invalid tables provided to retrieve data\n");
		return ACDB_ERROR;
	}

	// Now find the (mid,pid) index from the CDef table
	if(ACDB_SUCCESS != GetMidIidPidIndex(cdefTbl,mid,iid,pid,&mpindex))
	{
		ACDB_DEBUG_LOG("mid %08X and pid %08X not found\n",mid,pid);
		return ACDB_ERROR;
	}

	if(mpindex >= dataPoolChnk.nDataLen)
	{
		ACDB_DEBUG_LOG("Invalid dataoffset provided to retrieve the data from datapool table\n");
		return ACDB_ERROR;
	}

	// Now get the datalen and data pointer
	cData.nLen = READ_UInt32(dataPoolChnk.pData + cdotTbl.pDataOffsets[mpindex]);
	cData.pData = dataPoolChnk.pData + cdotTbl.pDataOffsets[mpindex] + sizeof(cData.nLen);

	dataLookupKey.nLookupKey = nLookupKey;
	cdftLookupKey = (uintptr_t)&cdefTbl.pCntDef[mpindex];
	dataLookupKey.nTableId = tblId;
	result = Acdb_DM_Instance_Ioctl((uint32_t) ACDB_SET_DATA,
		persistData,
		deltaFileIndex,
		(AcdbDataLookupKeyType*) &dataLookupKey,
		cdftLookupKey,
		(uint32_t*) &pid,
		pInBuff,
		nInBuffLen,
		(const uint8_t *)cData.pData,
		cData.nLen,
		NULL,
		0,
		NULL,
		NULL,
		pIndices,
		nIdxCount
		);

	if(result != ACDB_SUCCESS)
	{
		ACDB_DEBUG_LOG("Failed to do set operation\n");
		return result;
	}

	return ACDB_SUCCESS;
}

/* \brief
*      Verifies whether a PID exists in the Offloaded Parameter Data Global
*      Property list. The format of the list is:
*
*          OffloadedParameterList = numPidEntries PIDEntry+
*          PIDEntry = parameterID
*
*      Offloaded parameters are parameters whoes actual data does not
*      reside in the ACDB Data files. This data is loaded from a file
*      on the target system and stitched into the calibration blob
*
* \param[in] paramID : description
*
* \sa ACDB_GBL_PROPID_OFFLOADED_PARAM_INFO
* \return status
*/
int32_t IsOffloadedParameter(uint32_t paramID)
{
    int32_t status = ACDB_SUCCESS;
    uint32_t i = 0;
    uint32_t offset = 0;
    uint32_t num_entries;
    AcdbGlbalPropInfo prop_info = { 0,{ 0, NULL } };
    memset(&prop_info, 0, sizeof(prop_info));
    prop_info.pId = ACDB_GBL_PROPID_OFFLOADED_PARAM_INFO;

    status = acdbdata_ioctl(
        ACDBDATACMD_GET_GLOBAL_PROP, (uint8_t *)&prop_info,
        sizeof(AcdbGlbalPropInfo), (uint8_t *)NULL, 0);
    if (status != ACDB_SUCCESS)
    {
        switch (status)
        {
        case ACDB_ERROR:
            ACDB_DEBUG_LOG("[Data Manager] -> Global acdb file"
                "not loaded to fetch data\n");
            break;

        case ACDB_BADPARM:
            ACDB_DEBUG_LOG("[Data Manager] -> Received NULL input for"
                " AcdbDataGetGlobalPropData\n");
            break;

        case ACDB_DATA_NOT_FOUND:
            ACDB_DEBUG_LOG("[Data Manager] -> Failed to fetch the "
                "property info for pid %08X \n", prop_info.pId);
            break;

        default:
            ACDB_DEBUG_LOG("[Data Manager] -> Invalid command\n");
            break;
        }

        return status;
    }

    num_entries = READ_UInt32(prop_info.dataInfo.pData);
    offset += sizeof(uint32_t);

    if (SEARCH_SUCCESS ==
        AcdbDataBinarySearch(prop_info.dataInfo.pData + offset,
            num_entries, 1, &paramID, 1, &i))
    {
        return ACDB_SUCCESS;
    }

    return ACDB_DATA_NOT_FOUND;
}

/* \brief
*      Reads the offloaded param data stored on the file system into the
*      calibration buffer. It performs data alignment by adding padding to
*      the start of the file data and sets the starting offset of the data.
*
*      Parse the offloaded parameter data format.:
*
*          [ 4 Bytes        | 4 Bytes     | File Size Bytes ]
*          [ Data Alignment | Data Offset | Data Blob       ]
*
*      Offloaded parameters are parameters whoes actual data does not
*      reside in the ACDB Data files. This data is loaded from a file
*      on the target system and stitched into the calibration blob
*
* \param[in] calData : The ACDB File cal data that contains the file path
*                      to the parameter data file on the file system
* \param[in/out] paramSize : The size of the offloaded parameter which is
*                            sizeof(AcdbOffloadedParamHeader) + padding + data_size;
* \param[in/out] pDstBuff : The buffer to write to
* \param[in] nDstBuffLen : The remaining length of the buffer
* \param[in/out] pDstBuffOffset : The current point to write to in the buffer
*
* \sa AcdbOffloadedParamHeader
* \sa ACDB_GBL_PROPID_OFFLOADED_PARAM_INFO
* \return status
*/
int32_t GetOffloadedParameterData(
    AcdbDataType *calData, uint32_t *paramSize,
    uint8_t *pDstBuff, uint32_t nDstBuffLen, uint32_t *pDstBuffOffset)
{
    int32_t status = ACDB_SUCCESS;
    uint32_t offset = 0;
    AcdbOffloadedParamHeader header = { 0 };
    uint32_t data_size = 0;
    uint32_t padding = 0;
    uint32_t path_length;
    char* path = NULL;

    if (calData == NULL || paramSize == NULL ||
        calData->nLen <= 0 || calData->pData == NULL)
    {
        ACDB_DEBUG_LOG("[Data Manager Error:%d]-> Invalid input "
            "parameter\n", ACDB_BADPARM);
        return ACDB_BADPARM;
    }

    //Parse AcdbOffloadedParamHeader from parameter payload
    ACDB_MEM_CPY(&header, sizeof(header), calData->pData, sizeof(header));
    offset += sizeof(header);

    //Parse data: Should contain path length and path
    path_length = READ_UInt32(calData->pData + offset);
    if(path_length > 0)
        path = (char*)calData->pData + offset + sizeof(uint32_t);

    //perform alignment before writing data
    if ((*pDstBuffOffset + offset) % header.alignment)
    {
        padding = header.alignment
            - (*pDstBuffOffset + offset) % header.alignment;
        header.data_offset = padding;

        if (NULL != pDstBuff)
        {
            if (offset > nDstBuffLen)
            {
                ACDB_DEBUG_LOG("[Data Manager Error:%d]-> Buffer is not large enough "
                    "to set offloaded parameter padding.\n", ACDB_INSUFFICIENTMEMORY);
                return ACDB_INSUFFICIENTMEMORY;
            }
            memset((void*)(pDstBuff + offset), 0, padding);
        }
    }

    //Get Size
    if (ACDB_UTILITY_INIT_SUCCESS !=
        AcdbReadFile(path, NULL, nDstBuffLen, &data_size))
    {
        if(!pDstBuff)
            ACDB_DEBUG_LOG("[Data Manager Error:%d]-> Unable to read file size for "
            "offloaded parameter. Does the file exist?\n", ACDB_DATA_NOT_FOUND);
        data_size = 0;
    }

    //Update Parameter Size
    *paramSize = sizeof(header) + padding + data_size;
    header.data_size = data_size;

    //4 byte aligned padding
    padding = 0;
    if (*paramSize % 4)
    {
        padding = 4 - *paramSize % 4;
        *paramSize += padding;
    }

    //Write Data
    if (NULL == pDstBuff)
        return ACDB_UTILITY_INIT_SUCCESS;

    if (sizeof(header) > nDstBuffLen)
    {
        ACDB_DEBUG_LOG("[Data Manager Error:%d]-> Buffer is not large enough "
            "to copy offloaded parameter header.\n", ACDB_INSUFFICIENTMEMORY);
        return ACDB_INSUFFICIENTMEMORY;
    }

    offset = 0;
    //Write Header
    ACDB_MEM_CPY(pDstBuff, sizeof(header), &header, sizeof(header));
    offset += sizeof(header);
    *pDstBuffOffset += sizeof(header);

    if (data_size > nDstBuffLen - offset)
    {
        ACDB_DEBUG_LOG("[Data Manager Error:%d]-> Buffer is not large enough "
            "to copy offloaded parameter data.\n", ACDB_INSUFFICIENTMEMORY);
        return ACDB_INSUFFICIENTMEMORY;
    }

    if (data_size > 0 && ACDB_UTILITY_INIT_SUCCESS ==
        AcdbReadFile(path, pDstBuff + offset + header.data_offset, nDstBuffLen, &data_size))
    {
        *pDstBuffOffset += header.data_offset + data_size;
        offset += header.data_offset + data_size;

        //Write 4 byte aligned padding
        memset((void*)(pDstBuff + offset), 0, padding);
        *pDstBuffOffset += padding;
    }

    return status;
}

int32_t GetPersistentMidIidPidCalibTable(
    uint32_t tblId, uintptr_t nLookupKey,
    ContentDefWithInstanceTblType cdefTbl,
    ContentDataOffsetsTblType cdotTbl,
    AcdbDataInfo dataPoolChnk,
    uint8_t *pDstBuff, uint32_t nDstBuffLen, uint32_t *pDstBytesUsed)
{
    uint32_t nMemBytesLeft = 0;
    AcdbDataType cData = { 0, NULL };
    uint32_t i = 0;
    uint32_t offset = 0;
    uint32_t header_offset = 0;
    int32_t result = 0;
    AcdbDataLookupKeyType dataLookupKey = { 0, 0 };
    AcdbDynamicUniqueDataType *pDataNode = NULL;

    dataLookupKey.nLookupKey = nLookupKey;
    dataLookupKey.nTableId = tblId;

    if (pDstBuff == NULL || pDstBytesUsed == NULL)
    {
        ACDB_DEBUG_LOG("Recieved pDstBuff or pDstBytesUsed pointer with NULL value for tblid = %d\n", tblId);
        return ACDB_BADPARM;
    }

    if (cdefTbl.nLen != cdotTbl.nLen)
    {
        ACDB_DEBUG_LOG("The no of entries in CDEF and CDOT tables are not matching\n");
        return ACDB_ERROR;
    }

    nMemBytesLeft = nDstBuffLen;
    for (i = 0; i< cdefTbl.nLen; i++)
    {
        uint32_t nMemBytesRequired = 0;
        uint32_t nPaddedBytes = 0;
        AdspCalHdrWithInstanceIDFormatType hdr = { 0, 0, 0, 0, 0 };
        uintptr_t cdftLookupKey = (uintptr_t)&cdefTbl.pCntDef[i];
        uint32_t pid = cdefTbl.pCntDef[i].nPid;

        result = IsOffloadedParameter(pid);
        if (ACDB_SUCCESS != result)
        {
            continue;
        }

        //uintptr_t cdftPtr = (uintptr_t) &cdefTbl.pCntDef[i];
        // Now get the datalen and data pointer
        result = Acdb_DM_Instance_Ioctl((uint32_t)ACDB_GET_DATA,
            FALSE,
            0,
            (AcdbDataLookupKeyType*)&dataLookupKey,
            cdftLookupKey,
            (uint32_t*)&pid,
            NULL,
            0,
            NULL,
            0,
            NULL,
            0,
            NULL,
            (AcdbDynamicUniqueDataType**)&pDataNode,
            NULL,
            0
        );

        if (result == ACDB_SUCCESS)
        {
            if (pDataNode == NULL)
            {
                ACDB_DEBUG_LOG("Issue with heap, Unable to retrieve data from Heap\n");
                return ACDB_ERROR;
            }
            cData.nLen = pDataNode->ulDataLen;
            cData.pData = pDataNode->ulDataBuf;
        }
        else
        {
            // Now get the datalen and data pointer
            cData.nLen = READ_UInt32(dataPoolChnk.pData + cdotTbl.pDataOffsets[i]);
            cData.pData = dataPoolChnk.pData + cdotTbl.pDataOffsets[i] + sizeof(cData.nLen);
        }

        nMemBytesRequired = (uint32_t)(
            sizeof(AdspCalHdrWithInstanceIDFormatType));
        if (nMemBytesLeft < nMemBytesRequired)
        {
            ACDB_DEBUG_LOG("insufficient memory provided to copy the requested data\n");
            return ACDB_INSUFFICIENTMEMORY;
        }

        //make space to write module header offset later
        header_offset = offset;
        offset += sizeof(AdspCalHdrWithInstanceIDFormatType);

        //Get/Write Offloaded Parameter Data
        result = GetOffloadedParameterData(&cData, &hdr.nParamSize,
            pDstBuff + offset, nMemBytesLeft, &offset);
        if (ACDB_SUCCESS != result)
        {
            //log error
            return result;
        }

        //TODO: this alignment is already handled in GetOffloadedParameterData
        //if (cData.nLen % 4)
        //{
        //    nPaddedBytes = 4 - cData.nLen % 4;
        //}

        nMemBytesRequired = (uint32_t)(
            sizeof(AdspCalHdrWithInstanceIDFormatType)
            + hdr.nParamSize + nPaddedBytes);

        if (nMemBytesLeft < nMemBytesRequired)
        {
            ACDB_DEBUG_LOG("insufficient memory provided to copy the requested data\n");
            return ACDB_INSUFFICIENTMEMORY;
        }

        //Write Header
        hdr.nModuleId = cdefTbl.pCntDef[i].nMid;
        hdr.nInstanceId = GetUint16IID(cdefTbl.pCntDef[i].nIid);
        hdr.nReserved1 = 0;
        hdr.nParamId = cdefTbl.pCntDef[i].nPid;
        //hdr.nParamSize = (cData.nLen + nPaddedBytes);

        memset((void *)(pDstBuff + header_offset),
            0, sizeof(AdspCalHdrWithInstanceIDFormatType));
        ACDB_MEM_CPY((void *)(pDstBuff + header_offset), nMemBytesLeft,
            (const void *)&hdr, sizeof(hdr));
        //offset += (uint32_t)sizeof(hdr) + nMemBytesRequired;

        nMemBytesLeft -= nMemBytesRequired;
    }

    if (nDstBuffLen == nMemBytesLeft)
    {
        ACDB_DEBUG_LOG("[Data Manager]-> No data found\n");
        return ACDB_DATA_NOT_FOUND;
    }

    *pDstBytesUsed = nDstBuffLen - nMemBytesLeft;
    return ACDB_SUCCESS;
}

int32_t GetPersistentMidIidPidCalibTableSize(
    uint32_t tblId, uintptr_t nLookupKey,
    ContentDefWithInstanceTblType cdefTbl,
    ContentDataOffsetsTblType cdotTbl,
    AcdbDataInfo dataPoolChnk, uint32_t *pSize)
{
    AcdbDataType cData = { 0, NULL };
    uint32_t i = 0;
    uint32_t offset = 0;
    uint32_t nSizeRequired = 0;
    uint32_t result = 0;
    AcdbDataLookupKeyType dataLookupKey = { 0, 0 };
    AcdbDynamicUniqueDataType *pDataNode = NULL;

    dataLookupKey.nLookupKey = nLookupKey;
    dataLookupKey.nTableId = tblId;

    if (pSize == NULL)
    {
        ACDB_DEBUG_LOG("Recieved pSize pointer with NULL value for tblid = %d\n", tblId);
        return ACDB_BADPARM;
    }

    if (cdefTbl.nLen != cdotTbl.nLen)
    {
        ACDB_DEBUG_LOG("The no of entries in CDEF and CDOT tables are not matching\n");
        return ACDB_ERROR;
    }
    for (i = 0; i< cdefTbl.nLen; i++)
    {
        //uint32_t nPaddedBytes = 0;
        // Now get the datalen and data pointer
        uintptr_t cdftLookupKey = (uintptr_t)&cdefTbl.pCntDef[i];
        uint32_t pid = cdefTbl.pCntDef[i].nPid;
        uint32_t pid_size = 0;

        result = IsOffloadedParameter(pid);
        if (ACDB_SUCCESS != result)
        {
            continue;
        }

        result = Acdb_DM_Instance_Ioctl((uint32_t)ACDB_GET_DATA,
            FALSE,
            0,
            (AcdbDataLookupKeyType*)&dataLookupKey,
            cdftLookupKey,
            (uint32_t*)&pid,
            NULL,
            0,
            NULL,
            0,
            NULL,
            0,
            NULL,
            (AcdbDynamicUniqueDataType**)&pDataNode,
            NULL,
            0
        );

        if (result == ACDB_SUCCESS)
        {
            if (pDataNode == NULL)
            {
                ACDB_DEBUG_LOG("Issue with heap, Unable to retrieve data from Heap\n");
                return ACDB_ERROR;
            }
            cData.nLen = pDataNode->ulDataLen;
        }
        else
        {
            cData.nLen = READ_UInt32(dataPoolChnk.pData + cdotTbl.pDataOffsets[i]);
            cData.pData = dataPoolChnk.pData + cdotTbl.pDataOffsets[i] + sizeof(cData.nLen);
            pid_size = cData.nLen;
        }

        offset += sizeof(AdspCalHdrWithInstanceIDFormatType);

        //Get/Write Offloaded Parameter Data
        result = GetOffloadedParameterData(&cData, &pid_size,
            NULL, 0, &offset);
        if (ACDB_SUCCESS != result)
        {
            //log error

            return result;
        }

        nSizeRequired += sizeof(AdspCalHdrWithInstanceIDFormatType) + pid_size;
        offset += pid_size;
    }

    if (0 == nSizeRequired)
    {
        ACDB_DEBUG_LOG("[Data Manager]-> No data found\n");
        return ACDB_DATA_NOT_FOUND;
    }

    *pSize = nSizeRequired;
    return ACDB_SUCCESS;
}

int32_t GetPersistentMidIidPidCalibData(
    uint32_t tblId, uintptr_t nLookupKey,
    ContentDefWithInstanceTblType cdefTbl,
    ContentDataOffsetsTblType cdotTbl,
    AcdbDataInfo dataPoolChnk,
    uint32_t mid, uint32_t iid, uint32_t pid,
    uint8_t *pDstBuff, uint32_t nDstBuffLen,
    uint32_t *pDstBytesUsed)
{
    uint32_t mpindex = 0;
    uint32_t pid_size = 0;
    uint32_t offset = 0;
    AcdbDataType cData = { 0, NULL };
    uint32_t result = ACDB_SUCCESS;
    AcdbDataLookupKeyType dataLookupKey = { 0, 0 };
    uintptr_t cdftLookupKey = 0;
    AcdbDynamicUniqueDataType *pDataNode = NULL;
    if (pDstBuff == NULL || pDstBytesUsed == NULL)
    {
        ACDB_DEBUG_LOG("Recieved pDstBuff or pDstBytesUsed buffer pointer with NULL value for tblid = %d\n", tblId);
        return ACDB_BADPARM;
    }

    if (cdefTbl.nLen != cdotTbl.nLen)
    {
        ACDB_DEBUG_LOG("The no of entries in CDEF and CDOT tables are not matching\n");
        return ACDB_ERROR;
    }

    if (dataPoolChnk.pData == NULL || cdefTbl.pCntDef == NULL || cdotTbl.pDataOffsets == NULL)
    {
        ACDB_DEBUG_LOG("Invalid tables provided to retrieve data\n");
        return ACDB_ERROR;
    }

    result = IsOffloadedParameter(pid);
    if (ACDB_SUCCESS != result)
    {
        if(result == ACDB_PARMNOTFOUND)
            ACDB_DEBUG_LOG("Offloaded parameter list does "
                "not contain Param ID(0x%x)\n", pid);
        return result;
    }

    // Now find the (mid,pid) index from the CDef table
    if (ACDB_SUCCESS != GetMidIidPidIndex(cdefTbl, mid, iid, pid, &mpindex))
    {
        ACDB_DEBUG_LOG("mid %08X and pid %08X not found\n", mid, pid);
        return ACDB_PARMNOTFOUND;
    }

    if (mpindex >= dataPoolChnk.nDataLen)
    {
        ACDB_DEBUG_LOG("Invalid dataoffset provided to retrieve the data from datapool table\n");
        return ACDB_ERROR;
    }
    cdftLookupKey = (uintptr_t)&cdefTbl.pCntDef[mpindex];
    dataLookupKey.nTableId = tblId;
    dataLookupKey.nLookupKey = nLookupKey;
    result = Acdb_DM_Instance_Ioctl((uint32_t)ACDB_GET_DATA,
        FALSE,
        0,
        (AcdbDataLookupKeyType*)&dataLookupKey,
        cdftLookupKey,
        (uint32_t*)&pid,
        NULL,
        0,
        NULL,
        0,
        NULL,
        0,
        NULL,
        (AcdbDynamicUniqueDataType**)&pDataNode,
        NULL,
        0
    );

    if (result == ACDB_SUCCESS)
    {
        if (pDataNode == NULL)
        {
            ACDB_DEBUG_LOG("Issue with heap, Unable to retrieve data from Heap\n");
            return ACDB_ERROR;
        }
        cData.nLen = pDataNode->ulDataLen;
        cData.pData = pDataNode->ulDataBuf;
    }
    else
    {
        // Now get the datalen and data pointer
        cData.nLen = READ_UInt32(dataPoolChnk.pData + cdotTbl.pDataOffsets[mpindex]);
        cData.pData = dataPoolChnk.pData + cdotTbl.pDataOffsets[mpindex] + sizeof(cData.nLen);
    }

    if (nDstBuffLen < cData.nLen)
    {
        ACDB_DEBUG_LOG("insufficient memory provided to copy the requested data\n");
        return ACDB_INSUFFICIENTMEMORY;
    }

    result = GetOffloadedParameterData(&cData, &pid_size,
        pDstBuff, nDstBuffLen, &offset);
    if (ACDB_SUCCESS != result)
    {
        //log error

        return result;
    }

    //ACDB_MEM_CPY((void *)pDstBuff, nDstBuffLen, (const void *)cData.pData, cData.nLen);
    //*pDstBytesUsed = cData.nLen;
    *pDstBytesUsed = pid_size;

    return ACDB_SUCCESS;
}


