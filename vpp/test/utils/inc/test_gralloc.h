/*!
 * @file test_gralloc.h
 *
 * @cr
 * Copyright (c) 2018 Qualcomm Technologies, Inc.
 * All Rights Reserved.
 * Confidential and Proprietary - Qualcomm Technologies, Inc.

 * @services
 */
#ifndef _TEST_GRALLOC_H_
#define _TEST_GRALLOC_H_

#ifdef __cplusplus
extern "C" {
#endif

/***************************************************************************
 * Function Prototypes
 ***************************************************************************/
void* vpVppTestGralloc_Alloc(int fdPx, int fdMeta);
uint32_t u32VppTestGralloc_Free(void *pvGralloc);

#ifdef __cplusplus
 }
#endif

#endif /* _TEST_GRALLOC_H_ */
