#ifndef __ACDB_UTILITY_H__
#define __ACDB_UTILITY_H__
/*===========================================================================
    @file   acdb_utility.h

    The interface to the ACDBINIT utility functions.

    This file will provide API access to OS-specific init utility functions.

                    Copyright (c) 2010-2018 Qualcomm Technologies, Inc.
                    All Rights Reserved.
                    Confidential and Proprietary - Qualcomm Technologies, Inc.
========================================================================== */
/* $Header: //source/qcom/qct/multimedia2/Audio/audcal4/acdb_sw/main/latest/acdb/src/acdb_utility.h#6 $ */

/* ---------------------------------------------------------------------------
 * Include Files
 *--------------------------------------------------------------------------- */
#include "acdb_os_includes.h"

/* ---------------------------------------------------------------------------
 * Preprocessor Definitions and Constants
 *--------------------------------------------------------------------------- */

/* ---------------------------------------------------------------------------
 * Type Declarations
 *--------------------------------------------------------------------------- */

/* ---------------------------------------------------------------------------
* Class Definitions
*--------------------------------------------------------------------------- */

/* ---------------------------------------------------------------------------
 * Function Declarations and Documentation
 *--------------------------------------------------------------------------- */

#define SEARCH_SUCCESS 0
#define SEARCH_ERROR -1
#define NON_INSTANCE_CDFT_INDICES_COUNT 2
#define INSTANCE_CDFT_INDICES_COUNT 3

int32_t AcdbDataBinarySearch(void *voidLookUpArray, int32_t max,int32_t indexCount,
            void *pCmd, int32_t nNoOfIndsCount,uint32_t *index);

uint16_t GetUint16IID(uint32_t iid);

uint32_t GetUint32IID(uint16_t iid);

int32_t GetNoOfCdftIndices(uint32_t tblId, uint32_t* noOfCdftIndices);

int32_t Get_table_indices_count(uint32_t tblId,
	uint32_t *noOfTableIndices,
	uint32_t *nonModuleTblFound,
	uint32_t *noOfCdftIndices,
	uint32_t *noOfCmdIndices);

#endif /* __ACDB_UTILITY_H__ */
