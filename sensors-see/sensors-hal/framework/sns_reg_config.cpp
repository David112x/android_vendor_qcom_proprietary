/*=============================================================================
  @file sns_reg_config.cpp

  Creates files on the LA filesystem corresponding to android properties
  specified in a registry control file.

  Additionally, will remove/clear the registry's list of parsed configuration
  files upon notification/detection of an OTA update.  This will force the
  registry to reparse and apply all configuration files, including any new
  files or modifications made via the OTA update.

  Copyright (c) 2018-2019 Qualcomm Technologies, Inc.
  All Rights Reserved.
  Confidential and Proprietary - Qualcomm Technologies, Inc.
  ===========================================================================*/

/*=============================================================================
  Include Files
  ===========================================================================*/

#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <sys/types.h>
#include <dirent.h>
#include "sensors_log.h"
#include <cutils/properties.h>
#include <string.h>
#include <iostream>
#include <string>
#include <fstream>
#ifdef USE_GLIB
#include <glib.h>
#define strlcpy g_strlcpy
#endif

/*=============================================================================
  Macros
  ===========================================================================*/

#define START_ENTRY "BEG:AP\n"
#define END_ENTRY   "END:AP\n"

/*=============================================================================
  Static Data
  ===========================================================================*/

/* file format */
/*
   <type>=<type_name>=<type_value>
   file=soc_id=/sys/devices/soc0/soc_id
   file=hw_platform_path=/sys/devices/soc0/hw_platform
   property=persist.vendor.sensor.<propertyname>=/persist/sensors/registry/registry/<filename>
   */
/*persist.vendor.sensor.<propertyname>  - complete string needs to be added to .json file*/

/*=============================================================================
  EXTERNAL FILES
  ===========================================================================*/

/* Customers can do OTA of this file to update property fields or out put the
 * registry file path.  e.g from Android P registry files path is changed to
 * /mnt/vendor/persist from /persist */
static const char* registry_config_file = "/vendor/etc/sensors/sns_reg_config";

/*=============================================================================
  INTERNAL FILES - For Registry Use Only
  ===========================================================================*/

/*=============================================================================
  Static Function Definitions
  ===========================================================================*/

/**
 * Parse sns_reg_config for the list of android properties, and create
 * the required output files which will subsequently be used by the SSC to
 * control which registry configuration files are loaded/parsed.
 *
 * @param[i] config Contents of the configuration file
 * @param[i] errors error count during parsing
 *
 * @return Number of valid lines within the config string
 */
static int
parse_device_hw_info(char *config, int* errors)
{
  char *token = NULL, *sub_token = NULL, *saveptr1 = NULL;
  char *saveptr2 = NULL, *str2 = NULL;
  int token_id = 0;
  size_t write_len = 0;

  sns_logi("##sen_reg_config file parsing start\n");
  for(token_id = 0 ;; config = NULL)
  {
    int subtoken_id = 0;
    char property_name[PROPERTY_VALUE_MAX];

    token = strtok_r(config, "\n", &saveptr1);
    if(NULL == token)
      break;

    sns_logd("%s \n", token);
    for(str2 = token;; str2 = NULL)
    {
      sub_token = strtok_r(str2, "=", &saveptr2);
      if(NULL == sub_token)
      {
        sns_logd("parsing end for subtoken");
        break;
      }

      sns_logd("\t subtoken_id:%d entry#:%s \n", subtoken_id, sub_token);
      if(0 == subtoken_id)
      {
        if(strncmp(sub_token, "property", sizeof("property")))
        {
          /* No need to parse other than property */
          break;
        }
      }

      /* Reached here means it is for property */
      if(1 == subtoken_id)
      {
        strlcpy(property_name, sub_token, PROPERTY_VALUE_MAX);
      }
      if(2 == subtoken_id)
      {
        FILE *t_fp = fopen(sub_token, "w+");

        sns_logd("fopen on %s for property_name %s", sub_token, property_name);
        if(NULL == t_fp)
        {
          (*errors)++;
          sns_loge("fopen(w) failed for %s", sub_token);
        }
        else
        {
          char sns_prop[PROPERTY_VALUE_MAX];
          int len;

          len = property_get(property_name, sns_prop, "");
          if(PROPERTY_VALUE_MAX == len)
          {
            (*errors)++;
            sns_loge("max length reached");
            fclose(t_fp);
            break;
          }

          sns_prop[len] = '\n';
          if(0 == len)
          {
            sns_loge("property:'%s' not set\n", property_name);
            fclose(t_fp);
          }
          else
          {
            write_len = fwrite(sns_prop, (len + 1), 1, t_fp);
            if(0 != write_len)
            {
              sns_logi("('%s'):%s to %s", property_name, sns_prop, sub_token);
              fclose(t_fp);
            }
            else
            {
              (*errors)++;
              sns_loge("writing %s failed for %s", sns_prop, sub_token);
              fclose(t_fp);
            }
          }
        }
      }
      subtoken_id++;
    }
    token_id++;
  }

  return token_id;
}

static int
load_device_hw_info(void)
{
  int no_of_entries = 0;
  size_t read_len = 0;
  struct stat stat_buf;
  FILE *fp = NULL;
  int errors = 0;

    if(0 == stat(registry_config_file, &stat_buf))
    {
      fp = fopen(registry_config_file, "r");
      if(NULL != fp)
      {
        char *str1 = (char*)malloc(stat_buf.st_size);
        if(NULL != str1)
        {
          read_len = fread(str1, sizeof(uint8_t), stat_buf.st_size, fp);
          if(0 != read_len)
          {
            str1[read_len - 1] = '\0';
            sns_logd("reading fine'%s", registry_config_file);
          }
          no_of_entries = parse_device_hw_info(str1, &errors);
          sns_logi("##sen_reg_config file parsing end, total entries %d\n",
              no_of_entries);

          free(str1);
          str1 = NULL;
        }
        else
        {
          errors++;
          sns_loge("malloc(str1) failed");
        }
      }
      else
      {
        errors++;
        sns_loge("fopen failed for %s\n", registry_config_file);
      }
    }
    else
    {
      errors++;
      sns_loge("stat failed for %s", registry_config_file);
    }

  if(NULL != fp)
    fclose(fp);

  return errors;
}

/*=============================================================================
  Public Function Definitions
  ===========================================================================*/

int
handle_new_registry_update(void)
{
  return load_device_hw_info();
}
