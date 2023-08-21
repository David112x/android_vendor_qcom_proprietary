#ifndef ACDB_NEW_LINKED_LIST_H
#define ACDB_NEW_LINKED_LIST_H
/*==============================================================================

FILE: acdb_linked_list.h

DESCRIPTION: Functions and definitions to access the ACDB data structure.

PUBLIC CLASSES: Not Applicable

INITIALIZATION AND SEQUENCING REQUIREMENTS: N/A

Copyright (c) 2018 Qualcomm Technologies, Inc.
All Rights Reserved.
Confidential and Proprietary - Qualcomm Technologies, Inc.
==============================================================================*/
/* $Header: //source/qcom/qct/multimedia2/Audio/audcal4/acdb_sw/main/latest/acdb/src/acdb_new_linked_list.h#1 $ */
/*===========================================================================

EDIT HISTORY FOR FILE

This section contains comments describing changes made to this file.
Notice that changes are listed in reverse chronological order.

when who what, where, why
-------- --- ----------------------------------------------------------
05/28/14 mh SW migration from 32-bit to 64-bit architecture
07/23/10 ernanl Introduce new heap optimization APIs
07/06/10 ernanl Initial revision.

===========================================================================*/

/* ---------------------------------------------------------------------------
* Include Files
*--------------------------------------------------------------------------- */
#include "acdb_os_includes.h"

/* ---------------------------------------------------------------------------
* Preprocessor Definitions and Constants
*--------------------------------------------------------------------------- */


/*------------------------------------------------------------------------------
Target specific definitions
------------------------------------------------------------------------------*/

/* ---------------------------------------------------------------------------
* Type Declarations
*--------------------------------------------------------------------------- */
typedef struct AcdbInstanceVoiceTblStruct AcdbInstanceVoiceTblType;
#include "acdb_begin_pack.h"
struct AcdbInstanceVoiceTblStruct{
	AcdbDataLookupTblKeyType *pKey;
	struct AcdbInstancePrimaryKeyNodeStruct *pPrimaryKeyNode;
	struct AcdbInstanceVoiceTblStruct *pNext;
}
#include "acdb_end_pack.h"
;

typedef struct AcdbInstanceVoiceTblNodeStruct AcdbInstanceVoiceTblNodeType;
#include "acdb_begin_pack.h"
struct AcdbInstanceVoiceTblNodeStruct{
	AcdbInstanceVoiceTblType *pTblHead;
	AcdbInstanceVoiceTblType *pTblTail;
}
#include "acdb_end_pack.h"
;

typedef struct AcdbInstancePrimaryKeyStruct AcdbInstancePrimaryKeyType;
#include "acdb_begin_pack.h"
struct AcdbInstancePrimaryKeyStruct{
	AcdbDataLookupPrimaryKeyType *pKey;
	struct AcdbInstanceSecondaryKeyNodeStruct *pSecondaryNode;
	struct AcdbInstancePrimaryKeyStruct *pNext;
}
#include "acdb_end_pack.h"
;

typedef struct AcdbInstancePrimaryKeyNodeStruct AcdbInstancePrimaryKeyNodeType;
#include "acdb_begin_pack.h"
struct AcdbInstancePrimaryKeyNodeStruct{
	AcdbInstancePrimaryKeyType *pTblHead;
	AcdbInstancePrimaryKeyType *pTblTail;
}
#include "acdb_end_pack.h"
;

typedef struct AcdbInstanceSecondaryKeyStruct AcdbInstanceSecondaryKeyType;
#include "acdb_begin_pack.h"
struct AcdbInstanceSecondaryKeyStruct{
	AcdbDataLookupSecondaryKeyType *pKey;
	struct AcdbInstanceTopologyNodeStruct *pTopologyNode;
	struct AcdbInstanceSecondaryKeyStruct *pNext;
}
#include "acdb_end_pack.h"
;

typedef struct AcdbInstanceSecondaryKeyNodeStruct AcdbInstanceSecondaryKeyNodeType;
#include "acdb_begin_pack.h"
struct AcdbInstanceSecondaryKeyNodeStruct{
	AcdbInstanceSecondaryKeyType *pTblHead;
	AcdbInstanceSecondaryKeyType *pTblTail;
}
#include "acdb_end_pack.h"
;

typedef struct AcdbInstanceTopologyStruct AcdbInstanceTopologyType;
#include "acdb_begin_pack.h"
struct AcdbInstanceTopologyStruct{
	uintptr_t nCDFTLookupKey;
	AcdbDynamicUniqueDataType *pDataNode;
	struct AcdbInstanceTopologyStruct *pNext;
}
#include "acdb_end_pack.h"
;

typedef struct AcdbInstanceTopologyNodeStruct AcdbInstanceTopologyNodeType;
#include "acdb_begin_pack.h"
struct AcdbInstanceTopologyNodeStruct{
	AcdbInstanceTopologyType *pTopHead;
	AcdbInstanceTopologyType *pTopTail;
}
#include "acdb_end_pack.h"
;

typedef struct AcdbInstanceAudioTblStruct AcdbInstanceAudioTblType;
#include "acdb_begin_pack.h"
struct AcdbInstanceAudioTblStruct{
	AcdbDataLookupKeyType *pKey;
	struct AcdbInstanceTopologyNodeStruct *pTopologyNode;
	struct AcdbInstanceAudioTblStruct *pNext;
}
#include "acdb_end_pack.h"
;

typedef struct AcdbInstanceAudioTblNodeStruct AcdbInstanceAudioTblNodeType;
#include "acdb_begin_pack.h"
struct AcdbInstanceAudioTblNodeStruct{
	AcdbInstanceAudioTblType *pTblHead;
	AcdbInstanceAudioTblType *pTblTail;
}
#include "acdb_end_pack.h"
;

/**
@struct AcdbDeltaDataKeyType
@brief The lookup key structure for delta file data.

@param nLookupKey: An intermediate lookup key

This structure provides an lookup key that is used
to locate data between several calls to the data base.
*/
typedef struct _AcdbDeltaInstanceDataKeyType AcdbDeltaInstanceDataKeyType;
#include "acdb_begin_pack.h"
struct _AcdbDeltaInstanceDataKeyType {
	uint32_t nTableId;
	uint32_t nIndicesCount;
	uint32_t mid;
	uint32_t iid;
	uint32_t pid;
	uint8_t *pIndices;
}
#include "acdb_end_pack.h"
;

typedef struct AcdbDeltaInstanceStruct AcdbDeltaInstanceType;
#include "acdb_begin_pack.h"
struct AcdbDeltaInstanceStruct{
	AcdbDeltaInstanceDataKeyType *pKey;
	AcdbDynamicUniqueDataType *pDataNode;
	struct AcdbDeltaInstanceStruct *pNext;
}
#include "acdb_end_pack.h"
;

typedef struct AcdbDeltaFileInstanceDataStruct AcdbDeltaFileInstanceDataType;
#include "acdb_begin_pack.h"
struct AcdbDeltaFileInstanceDataStruct{
	AcdbDeltaInstanceType *pFileHead;
	AcdbDeltaInstanceType *pFileTail;
}
#include "acdb_end_pack.h"
;

int32_t CreateAudioTableNodeOnHeap(AcdbDataLookupKeyType *pKey,
	AcdbInstanceAudioTblNodeType *pTblOnHeap,
	AcdbInstanceAudioTblType **pTblNode
	);
int32_t CreateVoiceTableNodeOnHeap(AcdbDataLookupCVDKeyType *pKey,
	AcdbInstanceVoiceTblNodeType *pTblOnHeap,
	AcdbInstanceVoiceTblType **pTblNode
	);
int32_t CreateAudioTopologyNodeOnHeap(uintptr_t cdft,
	uint32_t *pParamId,
	AcdbDynamicUniqueDataType *pDataNode,
	AcdbInstanceAudioTblType *pTbNodelOnHeap
	);
int32_t CreateVoiceTopologyNodeOnHeap(uintptr_t cdft,
	uint32_t *pParamId,
	AcdbDynamicUniqueDataType *pDataNode,
	AcdbInstanceSecondaryKeyType *pSecondaryKeyNodeOnHeap
	);
int32_t CreatePrimaryKeyNodeOnHeap(AcdbDataLookupCVDKeyType *pKey,
	AcdbInstanceVoiceTblType *pTblNode,
	AcdbInstancePrimaryKeyType **ppPrimaryKeyNode
	);
int32_t CreateSecondaryKeyNodeOnHeap(AcdbDataLookupCVDKeyType *pKey,
	AcdbInstancePrimaryKeyType *pPrimaryKeyNode,
	AcdbInstanceSecondaryKeyType **ppSecondaryKeyNode
	);
int32_t FreeAudioTableNode(AcdbDataLookupKeyType *pKey,
	AcdbInstanceAudioTblNodeType *pTblOnHeap
	);
int32_t FreeVoiceTableNode(AcdbDataLookupCVDKeyType *pCVDKey,
	AcdbInstanceVoiceTblNodeType *pTblOnHeap
	);
int32_t FreeInstancePrimaryNode(AcdbDataLookupCVDKeyType *pCVDKey,
	AcdbInstancePrimaryKeyNodeType *pPrimaryNodeOnHeap
	);
int32_t FreeInstanceSecondaryNode(AcdbDataLookupCVDKeyType *pKey,
	AcdbInstanceSecondaryKeyNodeType *pSecondaryNodeOnHeap
	);
int32_t FreeAudioTopologyNode(uintptr_t cdft,
	uint32_t *pParamId,
	AcdbInstanceAudioTblType *pTblNode,
	uint32_t *fReeTblResult
	);
int32_t FreeVoiceTopologyNode(uintptr_t cdft,
	uint32_t cdftIndices,
	uint32_t *pParamId,
	AcdbInstanceSecondaryKeyType *pSecondaryKeyNode,
	uint32_t *fReeTblResult
	);
int32_t FindAudioTableNodeOnHeap(AcdbDataLookupKeyType *pKey,
	AcdbInstanceAudioTblNodeType *pTblOnHeap,
	AcdbInstanceAudioTblType **ppTblNode
	);
int32_t FindVoiceTableNodeOnHeap(AcdbDataLookupCVDKeyType *pKey,
	AcdbInstanceVoiceTblNodeType *pTblOnHeap,
	AcdbInstanceVoiceTblType **ppTblNode
	);
int32_t FindInstancePrimaryKeyNodeOnHeap(AcdbDataLookupCVDKeyType *pKey,
	AcdbInstancePrimaryKeyNodeType *pPrimaryKeyOnHeap,
	AcdbInstancePrimaryKeyType **ppPrimaryNode
	);
int32_t FindInstanceSecondaryKeyNodeOnHeap(AcdbDataLookupCVDKeyType *pKey,
	AcdbInstanceSecondaryKeyNodeType *pSecondaryKeyOnHeap,
	AcdbInstanceSecondaryKeyType **ppSecondaryNode
	);
int32_t FindInstanceSecondaryKeyNodeFromCVDKeyOnHeap(AcdbDataLookupCVDKeyType *pKey,
   AcdbInstanceVoiceTblNodeType *pTblOnHeap,
   AcdbInstanceSecondaryKeyType **ppSecondaryNode
   );
int32_t FindAudioTopologyNodeOnHeap(uintptr_t cdft,
	uint32_t *pParamId,
	AcdbInstanceAudioTblType *pTblNode,
	AcdbInstanceTopologyType **ppTopNode
	);
int32_t FindVoiceTopologyNodeOnHeap(uintptr_t cdft,
	uint32_t cdftIndices,
	uint32_t *pParamId,
	AcdbInstanceSecondaryKeyType *pSecondaryKeyNode,
	AcdbInstanceTopologyType **ppTopNode
	);
int32_t GetInstanceSecondaryNodeFromCVDKeyOnHeap(AcdbDataLookupCVDKeyType *pKey,
	AcdbInstanceVoiceTblNodeType *pTblOnHeap,
	AcdbInstanceSecondaryKeyType **ppSecondaryKeyNode
	);

#endif//ACDB_NEW_LINKED_LIST_H