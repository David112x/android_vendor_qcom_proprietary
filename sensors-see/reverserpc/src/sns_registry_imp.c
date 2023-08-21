/*==============================================================================
  @file sns_registry_imp.c

  interface/Skeliton implementation for reverse rpc communication used for
  registry operations

  Copyright (c) 2019 Qualcomm Technologies, Inc.
  All Rights Reserved.
  Confidential and Proprietary - Qualcomm Technologies, Inc.
==============================================================================*/
#include <stdio.h>
#include <string.h>
#include <cutils/properties.h>
#include <utils/Log.h>
#include "reverserpc_arm.h"
#include "verify.h"

/**
 * This function is exposed via fastrpc skel utility and can be called from Q6
 * gets the prop_name and returns the prop value
 *
 * @param[i] input property name to get the value
 * @param[i] buffer to store the prop value to return value back
 * @param[i] max buffer length to store prop_value
 *
 * @return 0 success and -1 for failure
 */
int sns_registry_get_property(char* prop_name,
        char* prop_value, uint32_t prop_valueLen)
{
  char p_val[PROPERTY_VALUE_MAX] = "";

  if(NULL == prop_value)
  {
    ALOGE("sns_registry_get_property:invalid buffer to place requested property(%s) value",
            prop_name);
    return -1;
  }

  if(prop_valueLen < PROPERTY_VALUE_MAX)
  {
    ALOGE("sns_registry_get_property:requested property(%s) cannot fitin by size:%d",
            prop_name, prop_valueLen);
    return -1;
  }

  memset(prop_value, '\0', prop_valueLen);
  property_get(prop_name, p_val, "");
  strlcpy(prop_value, p_val, prop_valueLen);

  ALOGI("sns_registry_get_property:prop_name/prop_value : ('%s')/('%s')",
            prop_name, prop_value);

  return 0;
}
