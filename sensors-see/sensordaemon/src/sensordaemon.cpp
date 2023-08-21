/*=============================================================================
  @file sensordaemon.cpp

  Background daemon process on LA to serve sensors needs.

  - Initializes and manages DIAG settings for SLPI subsystem
  - Initializes "Always On Test Sensors"
  - Provides android property access to the registry service

  Copyright (c) 2018 - 2019 Qualcomm Technologies, Inc.
  All Rights Reserved.
  Confidential and Proprietary - Qualcomm Technologies, Inc.
  ===========================================================================*/

/*=============================================================================
  Include Files
  ===========================================================================*/

#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <unordered_map>
#include <cutils/properties.h>
#include <poll.h>
#include "verify.h"
#include <diag_lsm.h>

#ifdef USE_GLIB
#include <glib.h>
#define strlcpy g_strlcpy
#endif
#include "worker.h"
#include "sensors_log.h"
#include "aont.h"
#include <sys/inotify.h>

#define BUF_LEN     4096
#define CMD_MAX_LEN 4
#define MICRO_SECOND 1000000
#define POLL_TIMEOUT -1

using namespace std;
using namespace sensors_log;

/*=============================================================================
  Macros
  ===========================================================================*/

/* "Sensors" DIAG peripheral ID.  Seems to usually refer to SLPI Root PD */
#define SNS_DIAG_PERIPHERAL_ID1         3
/* "Sensors User PD" DIAG peripheral ID */
#define SNS_DIAG_PERIPHERAL_ID2         9

#define SNS_DIAG_TX_MODE_STREAMING      0
#define SNS_DIAG_TX_MODE_CIRCULAR_BUFF  2
#define SNS_DIAG_LOW_WM_VALUE           20
#define SNS_DIAG_HIGH_WM_VALUE          70

/*=============================================================================
  Static Data
  ===========================================================================*/

static const char SENSORS_DAEMON_PROP_DEBUG[] = "persist.vendor.sensors.debug.daemon";
static const char SENSORS_AONT_ENABLE[] = "persist.vendor.sensors.debug.enable.aon_sensor";
static const char SENSORS_DIAG_BUFFER_ENABLE[] = "persist.vendor.sensors.diag_buffer_mode";
static const char SENSORS_ODL_ADSP_ENABLE[] = "persist.vendor.sensors.odl.adsp";

/* Named pipe for external entities to request a DIAG buffer flush */
static const char *pipe_name = "/mnt/vendor/persist/sensors/sns_diag_flush";
/* Location for DIAG configuration; if not found on-device logging disabled */
static const char *diag_cfg_name = "/mnt/vendor/persist/sensors/Diag.cfg";
static const char *diag_odl_file = "/mnt/vendor/persist/sensors/file1";

static worker _worker_diag;
static worker _worker_registry;
static worker _worker_aon;
static worker _worker_diag_mdlog;

static string prev_command=" ";

/* map debug property value to log_level */
static const unordered_map<char, sensors_log::log_level> log_level_map = {
  { '0', sensors_log::SILENT },
  { '1', sensors_log::INFO },
  { 'e', sensors_log::ERROR },
  { 'E', sensors_log::ERROR },
  { 'i', sensors_log::INFO },
  { 'I', sensors_log::INFO },
  { 'd', sensors_log::DEBUG },
  { 'D', sensors_log::DEBUG },
  { 'v', sensors_log::VERBOSE },
  { 'V', sensors_log::VERBOSE },
};

/*=============================================================================
  External Function Declarations
  ===========================================================================*/

/*=============================================================================
  Static Function Definitions
  ===========================================================================*/

/**
 * Determine whether DIAG buffering mode is enabled for Sensors.
 * Defaults to enabled.
 **/

static bool
diag_buffering_enabled(void)
{
  char debug_prop[PROPERTY_VALUE_MAX];
  int len = property_get(SENSORS_DIAG_BUFFER_ENABLE, debug_prop, "false");
  bool rv = true;

  if((len > 0) && (0 == strncmp(debug_prop, "false", strlen("false"))))
    rv = false;

  sns_logi("DIAG Buffering mode val: %d", rv);
  return rv;
}
/**
 * Determine whether ODL for Sensors ADSP is enabled.
 * Defaults to disabled.
 **/
static bool
is_odl_sensors_adsp(void)
{
  char debug_prop[PROPERTY_VALUE_MAX];
  int len = property_get(SENSORS_ODL_ADSP_ENABLE , debug_prop, "false");
  bool rv = true;

  if((len > 0) && (0 == strncmp(debug_prop, "false", strlen("false"))))
    rv = false;

  sns_logi("ODL sensors ADSP: %d", rv);
  return rv;
}

/**
 * Generates folder name for On Device Logging
 **/

static string
get_folder_name(void){
    time_t rawtime;
    tm* timeinfo;
    char buffer [80];
    time(&rawtime);
    timeinfo = localtime(&rawtime);
    strftime(buffer,80,"%Y-%m-%d-%H-%M-%S",timeinfo);
    string time_str(buffer);
    replace( time_str.begin(), time_str.end(), '-', '_' );
    return ("odl_sns_"+time_str);
}

/*
 Reads the data from diag_odl_file
 Starts or Stops the On Device Logging Instance
*/
static int
handle_events(int fd) {
  char buf[BUF_LEN];
  memset(buf, '\0', BUF_LEN);
  const struct inotify_event *event;
  ssize_t len;
  char *ptr;
  char flush[] = "1";
  int pipe_fd = open(pipe_name, O_WRONLY);
  if (pipe_fd < 0) {
    sns_loge(" Unable to open the pipe : %s", pipe_name);
    return -1;
  }
  FILE * pFile = fopen(diag_odl_file, "r+");
  if (NULL == pFile) {
    sns_loge(" Unable to open the file : %s", diag_odl_file);
    return -1;
  }
  len = read(fd, buf, sizeof buf);
  if (len == -1 && errno != EAGAIN) {
    sns_loge(" perror read : %i ", errno);
    return -1;
  }
  if (len <= 0)
    return -1;
  /* Loop over all events in the buffer */
  for (ptr = buf; ptr < buf + len;
      ptr += sizeof(struct inotify_event) + event->len) {
    event = (const struct inotify_event *) ptr;
    if (NULL != event) {
      /* Print event type */
      if (event->mask & IN_MODIFY) {
        sns_logi("handle_event : IN_MODIFY");
        fseek(pFile, 0, SEEK_SET);
        char cmd[CMD_MAX_LEN+1];
        fscanf(pFile, "%4s", cmd);
        if (0 == strncmp("stop", cmd, CMD_MAX_LEN)) {
         string temp_cmd(cmd);
         if(prev_command!=temp_cmd){
            write(pipe_fd, flush, strlen(flush));
            sns_logi("Daig : Killing current ODL instance ");
            if(true == is_odl_sensors_adsp()){
                system("/vendor/bin/diag_mdlog -k -p 4");
                system("/vendor/bin/diag_mdlog -k -g 16384 ");
            }
            else
                system("/vendor/bin/diag_mdlog -k -p 16");

            prev_command=temp_cmd;
            usleep(5 * MICRO_SECOND);
         }
        }
        if (0 == strncmp("star", cmd, CMD_MAX_LEN)) {
          if (0 == access(diag_cfg_name, F_OK)) {
            sns_logi("Daig : Diag.cfg present: starting diag_mdlog");
            string dir = "/data/vendor/sensors/"+get_folder_name();
            char * cdir = new char [dir.length()+1];
            strlcpy (cdir, dir.c_str(),(dir.length()+1));
            string temp_cmd(cmd);
            if(prev_command!=temp_cmd){
                if( (mkdir(cdir, S_IRWXU | S_IRWXG | S_IRWXO )) !=0 )
                    sns_loge("mkdir failed errno:%d - strerror: %s\n",errno,strerror(errno));
                else{
                    string command;
                    string command2;
                    if(true == is_odl_sensors_adsp()) {
                         command = "/vendor/bin/diag_mdlog -g 16384 -f /mnt/vendor/persist/sensors/Diag.cfg -w "+dir+" -o "+dir+" & ";
                         command2 = "/vendor/bin/diag_mdlog -p 4 -f /mnt/vendor/persist/sensors/Diag.cfg -w "+dir+" -o "+dir+" & ";
                    }
                    else{
                         command = "/vendor/bin/diag_mdlog -p 16 -f /mnt/vendor/persist/sensors/Diag.cfg -w "+dir+" -o "+dir+" & ";
                    }

                    char * c_command = new char [command.length()+1];
                    strlcpy (c_command, command.c_str(),(command.length()+1));
                    system(c_command);

                    if(true == is_odl_sensors_adsp()) {
                         char * c_command2 = new char [command2.length()+1];
                         strlcpy (c_command2, command2.c_str(),(command2.length()+1));
                         system(c_command2);

                    }

                    prev_command=temp_cmd;
                    usleep(3 * MICRO_SECOND);
                }
             }
          }
        }
      }
      if (event->len) {
        sns_logi("event : %s ", event->name);
      }
    }
    else{
        break;
    }
  }
  fclose(pFile);
  close(pipe_fd);
  return 1;
}

/*
 Creates file watcher on diag_odl_file and waits on modify event
*/
static int
diag_mdlog_capture(void) {
  sns_logi(" in diag_mdlog_test ");
  FILE *pFile = fopen(diag_odl_file, "w");
  usleep(2 * MICRO_SECOND);
  if (NULL != pFile) {
    fclose(pFile);
  }
  int fd, poll_num;
  int wd;
  nfds_t nfds;
  struct pollfd fds[1];

  fd = inotify_init1(0);
  if (fd == -1) {
    sns_loge("inotify_init1 failed ");
    return -1;
  }

  wd = inotify_add_watch(fd, diag_odl_file, IN_MODIFY | IN_CREATE);
  nfds = 1;
  fds[0].fd = fd;
  fds[0].events = POLLIN;
  while (1) {
    sns_logi(" before poll - for modify event");
    poll_num = poll(fds, nfds, POLL_TIMEOUT);
    if (poll_num == -1) {
      if (errno == EINTR)
        continue;
      sns_loge("poll perror: %i", errno);
      return -1;
    }
    if (poll_num > 0) {
      if (fds[0].revents & POLLIN) {
        int ret = handle_events(fd);
        if (-1 == ret) {
          return -1;
        }
      }
    }
  }
  inotify_rm_watch(fd, wd);
  close(fd);
  return 1;
}

/**
 * Wait for and handle requests to flush the DIAG buffer.
 */
static void
wait_handle_flush(void)
{
  struct pollfd fds[1];

  if(0 != mkfifo(pipe_name, 0666) && EEXIST != errno)
  {
    sns_loge("mkfifo error: %i", errno);
  }
  else
  {
    int pipe_fd = open(pipe_name, O_RDONLY);
    if(pipe_fd < 0)
    {
      sns_loge("open error: %i", errno);
    }
    else
    {
      fds[0].fd = pipe_fd;
      fds[0].events = POLLIN;

      for(;;)
      {
        if(0 < poll(fds, 1, POLL_TIMEOUT) && fds[0].revents & POLLIN)
        {
          char read_data[10];
          int read_len;
          read_len = read(fds[0].fd, read_data, sizeof(read_data));

          sns_logi("sns_diag_flush read len: %i", read_len);

          if(1 != diag_peripheral_buffering_drain_immediate(SNS_DIAG_PERIPHERAL_ID1) ||
             1 != diag_peripheral_buffering_drain_immediate(SNS_DIAG_PERIPHERAL_ID2))
            sns_loge("diag_peripheral_buffering_drain_immediate error");
        }
      }

      close(pipe_fd);
    }
  }
}

/**
 * Initialize the DIAG module, and configure appropriate
 * buffering/streaming mode.
 */
static void
handle_diag_settings(void)
{
  int err = 0;
  boolean diag_init = Diag_LSM_Init(NULL);

  if(!diag_init)
  {
    sns_loge("Diag_LSM_Init failed!");
  }
  else
  {
    uint32_t buffer_mode;
    usleep(6000000);
    if(diag_buffering_enabled())
    {
      sns_logi("Setting SNS TX MODE to CIRCULAR BUFF");
      buffer_mode = SNS_DIAG_TX_MODE_CIRCULAR_BUFF;
    }
    else
    {
      sns_logi("Setting SNS TX MODE to STREAMING");
      buffer_mode = SNS_DIAG_TX_MODE_STREAMING;
    }

    err = diag_configure_peripheral_buffering_tx_mode(SNS_DIAG_PERIPHERAL_ID1,
        buffer_mode, SNS_DIAG_LOW_WM_VALUE, SNS_DIAG_HIGH_WM_VALUE);

    err = diag_configure_peripheral_buffering_tx_mode(SNS_DIAG_PERIPHERAL_ID2,
        buffer_mode, SNS_DIAG_LOW_WM_VALUE, SNS_DIAG_HIGH_WM_VALUE);

    /* 1 is success and 0 failure */
    if(1 == err)
      sns_logi("Set SNS TX MODE success and return val: %d", err);
    else
      sns_loge("Set SNS TX MODE failed and return val: %d", err);
  }

  if(0 == access(diag_cfg_name, F_OK))
  {
    sns_logi(" Diag.cfg present: starting diag_mdlog ");
    if( true == is_odl_sensors_adsp()){
        system("/vendor/bin/diag_mdlog -g 16384 -f /mnt/vendor/persist/sensors/Diag.cfg -w /data/vendor/sensors -o /data/vendor/sensors & ");
        system("/vendor/bin/diag_mdlog -p 4 -f /mnt/vendor/persist/sensors/Diag.cfg -w /data/vendor/sensors -o /data/vendor/sensors & ");
    }
    else
        system("/vendor/bin/diag_mdlog -p 16 -f /mnt/vendor/persist/sensors/Diag.cfg -w /data/vendor/sensors -o /data/vendor/sensors & ");

    FILE *pFile = fopen(diag_odl_file, "w");
    sns_logi(" after opening file ");
    if(NULL != pFile){
        char boot_odl_started[]="start\n";
        size_t write_len = fwrite(boot_odl_started, sizeof(char), sizeof(boot_odl_started), pFile);
        sns_logi(" written %zu",write_len);
        if(write_len != sizeof(boot_odl_started)){
            sns_loge(" write error to diag_odl_file");
        }
        fclose(pFile);
    }
    else{
        sns_loge(" error opening diag_odl_file");
    }
  }

  wait_handle_flush();
}

/**
 * Set level of debug messages which will be delivered to logcat.
 */
static void
set_log_level(void)
{
  char debug_prop[PROPERTY_VALUE_MAX];
  int len;
  log_level loglevel;
  sensors_log::set_tag("sensors.qti");

  len = property_get(SENSORS_DAEMON_PROP_DEBUG, debug_prop, "i");
  if(len > 0)
  {
    if(log_level_map.find(debug_prop[0]) != log_level_map.end())
    {
      loglevel = log_level_map.at(debug_prop[0]);
      sns_logi("log_level set: %d", loglevel);
      sensors_log::set_level(loglevel);
    }
  }
}

/**
 * Checks whether the "Always On Test Sensor" should be enabled based on
 * Android configuration.
 *
 * PEND: What does AONT do?
 */
static void
handle_aont(aont *aontobj)
{
  char debug_prop[PROPERTY_VALUE_MAX];
  int len = property_get(SENSORS_AONT_ENABLE, debug_prop, "false");

  if((len > 0) && (0 == strncmp(debug_prop, "true", 4)))
  {
    sns_logi("sending enable aont request");
    aontobj->enable();
    sns_logi("sent enable aont request");
  }
  else
  {
    sns_logd("aont disabled");
  }
}

/*=============================================================================
  Public Function Definitions
  ===========================================================================*/

int main()
{

  set_log_level();
  aont aontobj;

  sns_logi("sensors daemon starting");

  _worker_diag.add_task(NULL, [] {
      handle_diag_settings();
      });

  _worker_aon.add_task(NULL, [&aontobj] {
      handle_aont(&aontobj);
      });

  _worker_diag_mdlog.add_task(NULL, [] {
     diag_mdlog_capture();
     });

  sleep(INT_MAX);
  sns_loge("sensors daemon exiting!");

  return 0;
}
