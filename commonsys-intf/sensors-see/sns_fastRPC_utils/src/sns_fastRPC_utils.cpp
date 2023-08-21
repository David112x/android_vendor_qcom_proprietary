/*
 * Copyright (c) 2018-2019 Qualcomm Technologies, Inc.
 * All Rights Reserved.
 * Confidential and Proprietary - Qualcomm Technologies, Inc.
 */

extern "C" {
  #include "sns_fastRPC_utils.h"
}

static bool is_system;
static void *g_dlhandler = NULL;
static int dlopen_count = 0;

void set_system_parition(bool isSystem){
	is_system = isSystem;
}

const char *get_lib() {
  /*check for slpi or adsp*/
  struct stat sb;
  if(!stat("/sys/kernel/boot_slpi", &sb)){
    if(true == is_system)
      return "libsdsprpc_system.so";
    else
      return "libsdsprpc.so";
  }
  else {
    if(true == is_system)
      return "libadsprpc_system.so";
    else
      return "libadsprpc.so";
  }
}


int init_rpc_symbols(sns_remote_handles *remote_handles) {
  void *dlhandler = NULL;
  if(NULL == (dlhandler = dlopen(get_lib(), RTLD_NOW))) {
    ALOGE("%s: load stub(%s) failed !", __FUNCTION__, get_lib());
    return -1;
  }

  ALOGE("%s: dlopen(%s) done !", __FUNCTION__, get_lib());
  remote_handles->open = (remote_handle_open_t)dlsym(dlhandler,"remote_handle_open");
  remote_handles->close = (remote_handle_close_t)dlsym(dlhandler,"remote_handle_close");
  remote_handles->invoke = (remote_handle_invoke_t)dlsym(dlhandler,"remote_handle_invoke");

  if ((NULL == remote_handles->open) ||
     (NULL == remote_handles->close) ||
     (NULL == remote_handles->invoke) )
  {
    ALOGE("%s: dlsym failed !", __FUNCTION__);
    return -1;
  }
	g_dlhandler = dlhandler;
	dlopen_count++;
  return 0;
}

void deInit_rpc_symbols()
{
  dlopen_count--;
  if(dlopen_count == 0 && g_dlhandler !=  NULL)
  {
    dlclose(g_dlhandler);
    g_dlhandler = NULL;
  }
}

