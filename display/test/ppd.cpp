/*
 * Copyright (c) 2017, 2019 Qualcomm Technologies, Inc.
 * All Rights Reserved.
 * Confidential and Proprietary - Qualcomm Technologies, Inc.
 */

#include "ppd.h"
#include <cutils/sockets.h>
#include <log/log.h>
#include <unistd.h>

bool postprocTester::sendDppsCommand(char *cmd) {
  const char *kDaemonSocketName = "pps";
  int daemonSocket = -1, ret = 0;
  char reply[64] = {0};

  daemonSocket =
      socket_local_client(kDaemonSocketName, ANDROID_SOCKET_NAMESPACE_RESERVED, SOCK_STREAM);
  if (daemonSocket >= 0) {
    ret = write(daemonSocket, cmd, strlen(cmd));
    if (ret > 0) {
      ret = read(daemonSocket, reply, sizeof(reply) - 1);
      if (ret < 0) {
        ALOGE("%s: Failed to get data over socket %s\n", strerror(errno), kDaemonSocketName);
      }
    } else {
      ALOGE("%s: Failed to send data over socket %s\n", strerror(errno), kDaemonSocketName);
    }
    close(daemonSocket);
  } else {
    ALOGE("Connecting to socket failed: %s\n", strerror(errno));
  }

  log_info("Response from sendDPPSCommand: ");
  log_info(reply);
  return (ret >= 0) ? true : false;
}

int main(int32_t argc, char **argv) {
  char *cmd = NULL;

  if (argc < 2) {
    log_error("ppd needs a string argument.\n");
    return 0;
  }
  if (argc > 2) {
    log_error("ppd needs only one string argument.\n");
    return 0;
  }
  cmd = argv[1];
  if (cmd == NULL) {
    log_error("Invalid Command.\n");
    return 0;
  }

  postprocTester *tester = new postprocTester();

  if (tester->sendDppsCommand(cmd))
    log_info("Command Success \n");
  else
    log_info("Command Failed \n");

  delete tester;
  return 0;
}
