/**********************************************************************
* Copyright (c) 2015-2019 Qualcomm Technologies, Inc.
* All Rights Reserved.
* Confidential and Proprietary - Qualcomm Technologies, Inc.
**********************************************************************/
#ifndef VERIFY_PRINT_ERROR
#define VERIFY_PRINT_ERROR
#endif //VERIFY_PRINT_ERROR

#include "rpcmem.h"
#include "remote.h"
#include "log/log.h"

extern void remote_register_buf(void* buf, int size, int fd);
#pragma weak  remote_register_buf
extern void remote_register_buf_attr(void* buf, int size, int fd, int attrs);
#pragma weak  remote_register_buf_attr


static remote_handle remote_handle_fd;

/*Funtion pointer declarations to the open , close and invoke methods*/
 typedef int (*remote_handle_open_t)(const char* name, remote_handle *ph);
 typedef int (*remote_handle_close_t)(remote_handle h);
 typedef int (*remote_handle_invoke_t)(remote_handle h, uint32_t dwScalars, remote_arg *pra);


remote_handle_open_t remote_handle_open_rpcmem;
remote_handle_close_t remote_handle_close_rpcmem;
remote_handle_invoke_t remote_handle_invoke_rpcmem;

/*sns_remote_handles contains handle for open, close and invoke methods*/
typedef struct
{
   remote_handle_open_t open;
  remote_handle_close_t close;
  remote_handle_invoke_t invoke;
}_sns_remote_handlers;

void sns_init_rpc_remote_handlers(_sns_remote_handlers handlers) {
  remote_handle_open_rpcmem = handlers.open;
  remote_handle_close_rpcmem = handlers.close;
  remote_handle_invoke_rpcmem = handlers.invoke;
}

void sns_create_staticpd_channel() {
  if (!remote_handle_fd) {
     if (remote_handle_open_rpcmem(ITRANSPORT_PREFIX "createstaticpd:sensorspd", &remote_handle_fd)){
        ALOGE("Failed to open remote handle for sensorspd");
     } else {
        ALOGI("opened remote handle for sensorspd");
        remote_handle remote_handle_fd = 1;
     }
  } else {
     ALOGI("using opened remote handle for sensorspd");
  }
}

void sns_close_staticpd_channel() {
  ALOGI("remote_handle_close_rpcmem(remote_handle_fd: %d)", remote_handle_fd);
  remote_handle_close_rpcmem(remote_handle_fd);
}
void register_buf(void* buf, int size, int fd, int attr) {
   int nErr = 0;
   ALOGI("rpcmem:edge:remote_register_buf: %p remote_handle_fd %d", remote_register_buf, remote_handle_fd);
      if(remote_register_buf_attr) {
         remote_register_buf_attr(buf, size, fd, attr);
      } else if(remote_register_buf) {
         remote_register_buf(buf, size, fd);
      }
   }
