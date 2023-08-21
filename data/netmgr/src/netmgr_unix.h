/******************************************************************************

                        N E T M G R _ U N I X . H

******************************************************************************/

/******************************************************************************

  @file    netmgr_unix.h
  @brief   Network Manager UNIX domain sockets header file

  DESCRIPTION
  Header file for NetMgr UNIX module.

******************************************************************************/
/*===========================================================================

Copyright (c) 2015, 2020 Qualcomm Technologies, Inc.  All Rights Reserved.
Qualcomm Technologies Proprietary and Confidential.

===========================================================================*/

/*===========================================================================
                     GLOBAL DEFINITIONS AND DECLARATIONS
===========================================================================*/

/*---------------------------------------------------------------------------
   Constant representing integer value returned for errors occurring in
   system calls
---------------------------------------------------------------------------*/
#define NETMGR_UNIX_API_ERROR (-1)

/*---------------------------------------------------------------------------
   Constant representing maximum number of clients that can connect to
   Netmgr before they are dropped.
---------------------------------------------------------------------------*/
#define NETMGR_UNIX_MAX_CLIENTS (20)

/*---------------------------------------------------------------------------
   Constant representing maximum size of a message to be used for
   communication.
---------------------------------------------------------------------------*/
#define NETMGR_UNIX_MAX_MESSAGE_SIZE (2048)

/*---------------------------------------------------------------------------
   UNIX Socket path locations for Netmgr client to connect and talk to Netmgr
---------------------------------------------------------------------------*/
#if defined (NETMGR_OFFTARGET)
#define NETMGR_UNIX_CLIENT_CONN_SOCK_PATH "/tmp/data/netmgr_connect_socket"
#define NETMGR_UNIX_PRIORITY_SOCK_PATH "/tmp/data/netmgr_priority_socket"
#define NETMGR_UNIX_PRIORITY_NOTIFY_SOCK_PATH "/tmp/data/netmgr_priority_notify_socket"
#define UNIX_PATH_MAX  (108)
#elif defined (FEATURE_DS_LINUX_ANDROID)
#define NETMGR_UNIX_CLIENT_CONN_SOCK_PATH "/dev/socket/netmgr/netmgr_connect_socket"
#define NETMGR_UNIX_PRIORITY_SOCK_PATH "/dev/socket/netmgr/netmgr_priority_socket"
#define NETMGR_UNIX_PRIORITY_NOTIFY_SOCK_PATH "/dev/socket/netmgr/netmgr_priority_notify_socket"
#else
#define UNIX_PATH_MAX  (108)
#define NETMGR_UNIX_CLIENT_CONN_SOCK_PATH "/var/netmgr_connect_socket"
#define NETMGR_UNIX_PRIORITY_SOCK_PATH "/var/netmgr_priority_socket"
#define NETMGR_UNIX_PRIORITY_NOTIFY_SOCK_PATH "/var/netmgr_priority_notify_socket"
#endif

/*===========================================================================
  FUNCTION  netmgr_unix_ipc_send_msg
===========================================================================*/
/*!
@brief
  Function to send unicast NETLINK messages to netmgr and to receive the
  responses from netmgr. This is interface for clients of netmgr to
  communicate with netmgr

@return
  int - NETMGR_SUCCESS on successful operation, NETMGR_FAILURE otherwise

@note

  - Dependencies
    - None

  - Side Effects
    - None
*/
/*=========================================================================*/
int
netmgr_unix_ipc_send_msg
(
  char           *req_buffer,
  int            req_bufflen,
  char           **resp_buffer,
  unsigned int   *resp_bufflen,
  int            resp_timeout
);
