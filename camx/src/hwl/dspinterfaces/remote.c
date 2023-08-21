// NOWHINE // NOWHINE ENTIRE FILE - Auto Gen by IDL Hexagon sdk \
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2017 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "remote.h"
#ifdef __cplusplus
extern "C" {
#endif

#define VERIFY_EPRINTF(format, ...) (void)0

int remote_mmap64(int fd, uint32_t flags, uintptr_t vaddrin, int64_t size, uintptr_t* vaddrout) {
   VERIFY_EPRINTF("Invoking stubbed routine - Return failure");
   return -1;
}

int remote_munmap64(uintptr_t vaddrout, int64_t size) {
   VERIFY_EPRINTF("Invoking stubbed routine - Return failure");
   return -1;
}

int remote_mmap(int fd, uint32_t flags, uint32_t vaddrin, int size, uint32_t* vaddrout) {
   VERIFY_EPRINTF("Invoking stubbed routine - Return failure");
   return -1;
}

int remote_munmap(uint32_t vaddrout, int size) {
   VERIFY_EPRINTF("Invoking stubbed routine - Return failure");
   return -1;
}

int remote_handle_open(const char* name, remote_handle *ph) {
   VERIFY_EPRINTF("Invoking stubbed routine - Return failure");
   return -1;
}

int remote_handle_invoke(remote_handle h, uint32_t dwScalars, remote_arg *pra) {
   VERIFY_EPRINTF("Invoking stubbed routine - Return failure");
   return -1;
}

int remote_handle_close(remote_handle h) {
   VERIFY_EPRINTF("Invoking stubbed routine - Return failure");
   return -1;
}

void remote_register_buf(void* buf, int size, int fd) {
}

int remote_register_dma_handle(int fd, uint32_t len) {
   return 0;
}

int remote_set_mode(uint32_t mode) {
   VERIFY_EPRINTF("Invoking stubbed routine - Return failure");
   return -1;
}

int remote_handle64_open(const char* name, remote_handle64 *ph) {
   VERIFY_EPRINTF("Invoking stubbed routine - Return failure");
   return -1;
}

int remote_handle64_invoke(remote_handle64 h, uint32_t dwScalars, remote_arg *pra) {
   VERIFY_EPRINTF("Invoking stubbed routine - Return failure");
   return -1;
}

int remote_handle64_close(remote_handle64 h) {
   VERIFY_EPRINTF("Invoking stubbed routine - Return failure");
   return -1;
}
#ifdef __cplusplus
}
#endif

