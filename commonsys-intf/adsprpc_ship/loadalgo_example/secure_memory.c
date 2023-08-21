/**=============================================================================

@file
   secure_memory.c

@brief
   Secure memory management implementation.

Copyright (c) 2019 Qualcomm Technologies, Inc.
All Rights Reserved.
Confidential and Proprietary - Qualcomm Technologies, Inc.

=============================================================================**/

//==============================================================================
// Include Files
//==============================================================================
#include "secure_memory.h"
#include <utils/Log.h>

#ifdef LOG_TAG
#undef LOG_TAG
#endif
#define LOG_TAG "SecureMemory"

int secure_mem_free(mem_handle *pIon)
{
   int32_t ret = 0;

   if (pIon == NULL)
   {
      return SECUREMEMORY_INVALIDHANDLE;
   }

   if (pIon->addr != 0)
   {
      ret = munmap((void*)(uint64_t)pIon->addr, pIon->size);
      if (ret)
      {
         ALOGD("Error: Unmapping ION Buffer failed with ret = %d", ret);
      }
      pIon->addr = 0;
   }

   if (pIon->fd)
   {
      close(pIon->fd);
   }
   pIon->fd = -1;

   if (pIon->ion_fd >= 0)
   {
      ion_close(pIon->ion_fd);
      pIon->ion_fd = -1;
   }

   pIon->size = 0;

   return 0;
}

int secure_mem_alloc(mem_handle *pIon, uint32_t nBytes, uint32_t nByteAlignment, boolean secure)
{
   int32_t retAlloc = 0;
   uint32_t alignment = 0;
   int map_fd;
   unsigned int heap_id_mask = ION_HEAP(ION_SYSTEM_HEAP_ID);
   unsigned int flags = ION_FLAG_CACHED;

   if (pIon == NULL)
   {
      return SECUREMEMORY_INVALIDHANDLE;
   }
   if (nBytes == 0)
   {
      return SECUREMEMORY_INVALIDSIZE;
   }

   pIon->ion_fd = -1;
   pIon->fd = -1;
   pIon->size = 0;
   pIon->addr = 0;

   if (secure || nByteAlignment>0)
   {
      uint32_t alignMin1 = nByteAlignment - 1;
      pIon->size = (nBytes + alignMin1) & (~alignMin1);
      alignment = nByteAlignment;
   }
   else
   {
      pIon->size = nBytes;
      alignment = 0;
   }

   pIon->ion_fd = ion_open();
   if (pIon->ion_fd < 0)
   {
      ALOGD("Error: ion_open return %d", pIon->ion_fd);
      return SECUREMEMORY_OPENFAILED;
   }

   if (secure)
   {
      heap_id_mask = ION_HEAP(ION_SECURE_CARVEOUT_HEAP_ID);
      flags = ION_FLAG_CP_CDSP | ION_FLAG_SECURE;
   }
   else
   {
      ALOGD("Using non-secure memory from System heap, cached");
      //This is a heap that is contiguous and non-dma.(For TA mapping purpose)
      heap_id_mask = ION_HEAP(ION_QSECOM_HEAP_ID);
      flags = ION_FLAG_CACHED;
   }

   retAlloc = ion_alloc_fd(pIon->ion_fd, pIon->size, alignment, heap_id_mask, flags, &map_fd);
   if (retAlloc)
   {
      ALOGD("Error: ion_alloc_fd returned failure %d", retAlloc);
      goto alloc_fail;
   }

   if (map_fd)
   {
      pIon->fd = map_fd;
   }
   else
   {
      ALOGD("Error: map_fd is NULL");
      goto alloc_fail;
   }

   if (secure)
      pIon->addr = 0;
   else
   {
      pIon->addr = (uint64_t)mmap(NULL, pIon->size, PROT_READ | PROT_WRITE, MAP_SHARED, map_fd, 0);
      if (pIon->addr == 0 || pIon->addr == (uint32_t)(-1))
      {
         ALOGD("Error: mmap return NULL");
         goto ioctl_fail;
      }
   }

   return 0;

ioctl_fail:
   if (map_fd >= 0)
   {
      close(map_fd);
   }

alloc_fail:
   if (pIon->ion_fd >= 0)
   {
      ion_close(pIon->ion_fd);
   }
   return SECUREMEMORY_ALLOCFAILED;
}

