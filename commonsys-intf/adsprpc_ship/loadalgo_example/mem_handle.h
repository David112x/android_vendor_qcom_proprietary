/**=============================================================================

@file
   mem_handle.h

@brief
   Header file for secure memory management.

Copyright (c) 2019 Qualcomm Technologies, Inc.
All Rights Reserved.
Confidential and Proprietary - Qualcomm Technologies, Inc.

=============================================================================**/

#ifndef MEM_MEMORY_H
#define MEM_MEMORY_H

typedef struct {
   int32_t  fd;
   uint32_t size;
   uint64_t addr;
   int32_t  ion_fd;
} __attribute__ ((packed)) mem_handle;

#endif
