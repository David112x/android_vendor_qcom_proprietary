/*
 * Copyright (c) 2018 Qualcomm Technologies, Inc.
 * All Rights Reserved.
 * Confidential and Proprietary - Qualcomm Technologies, Inc.
 */

#include "stdlib.h"
#include <dlfcn.h>
#include <utils/Log.h>
#include <sys/stat.h>
#include "remote.h"

/*Funtion pointer declarations to the open , close and invoke methods*/
typedef int (*remote_handle_open_t)(const char* name, remote_handle *ph);
typedef int (*remote_handle_close_t)(remote_handle h);
typedef int (*remote_handle_invoke_t)(remote_handle h, uint32_t dwScalars, remote_arg *pra);

/*sns_remote_handles contains handle for open, close and invoke methods*/
typedef struct
{
  remote_handle_open_t open;
  remote_handle_close_t close;
  remote_handle_invoke_t invoke;
}sns_remote_handles;

/*Sets a variable within fastRPC util if this call comes from system parition*/
void set_system_parition(bool is_system);

/*returns the libarry name which supposed to load at run time*/
const char *get_lib();

/**
 * @brief init_rpc_symbols, intialise the fastRPC symbols by loading proper
 *  library and storing handles.
 *
 * @param[i] pointer to input where this fucntion updates the remote handles like
 * open, close and invoke handles.
 *
 * @param[o] returns -1 in case of failure and 0 incase of succes.
 */
int init_rpc_symbols(sns_remote_handles *remote_handles);

/**
 * @brief deInit_rpc_symbols, Will close the used library which was opend by
 *  init_rpc_symbols
 */
void deInit_rpc_symbols();


