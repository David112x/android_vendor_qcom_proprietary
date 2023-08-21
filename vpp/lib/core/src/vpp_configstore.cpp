/*!
 * @file vpp_configstore.cpp
 *
 * @cr
 * Copyright (c) 2019 Qualcomm Technologies, Inc.
 * All Rights Reserved.
 * Confidential and Proprietary - Qualcomm Technologies, Inc.

 * @services
 */

#include <sys/types.h>
#include <pthread.h>
#include <string.h>

#include <vendor/qti/hardware/capabilityconfigstore/1.0/types.h>
#include <vendor/qti/hardware/capabilityconfigstore/1.0/ICapabilityConfigStore.h>

#include "vpp.h"
#include "vpp_def.h"
#include "vpp_dbg.h"

#include "vpp_configstore.h"

using android::sp;
using vendor::qti::hardware::capabilityconfigstore::V1_0::ICapabilityConfigStore;

/************************************************************************
 * Local definitions
 ***********************************************************************/
static struct {
    sp<ICapabilityConfigStore> pService = NULL;
    pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
} stConfigStore;

static const uint32_t MAX_CONFIG_STORE_STR_SIZE = 512;

/************************************************************************
 * Local static variables
 ***********************************************************************/

/************************************************************************
 * Forward Declarations
 ***********************************************************************/

/************************************************************************
 * Local functions
 ***********************************************************************/
static void vVppConfigStore_LoadIfNecessary()
{
    pthread_mutex_lock(&stConfigStore.mutex);
    if (stConfigStore.pService == NULL)
    {
        stConfigStore.pService = ICapabilityConfigStore::tryGetService();
    }
    pthread_mutex_unlock(&stConfigStore.mutex);

    LOGE_IF(stConfigStore.pService == NULL, "failed to acquire configstore service");
}

/************************************************************************
 * Global functions
 ***********************************************************************/
uint32_t u32VppConfigStore_GetString(const char *pcArea,
                                     const char *pcConfig,
                                     t_StConfigStoreStr *pstRes)
{
    VPP_RET_IF_NULL(pcArea, VPP_ERR_PARAM);
    VPP_RET_IF_NULL(pcConfig, VPP_ERR_PARAM);
    VPP_RET_IF_NULL(pstRes, VPP_ERR_PARAM);

    using vendor::qti::hardware::capabilityconfigstore::V1_0::CommandResult;
    using vendor::qti::hardware::capabilityconfigstore::V1_0::Result;

    vVppConfigStore_LoadIfNecessary();
    if (stConfigStore.pService == NULL)
    {
        LOGE("unable to acquire configstore service");
        return VPP_ERR;
    }

    CommandResult result = {
        .result_type = Result::NOT_FOUND,
    };
    stConfigStore.pService->getConfig(pcArea, pcConfig, [&] (const CommandResult &res) {
        result = res;
    });

    LOGI("configstore: area=%s, config=%s, res=%u, val=%s",
         pcArea, pcConfig, result.result_type, result.value.c_str());

    if (result.result_type != Result::SUCCESS)
    {
        LOGE("failed to find in configstore: '%s' - '%s'", pcArea, pcConfig);
        return VPP_ERR;
    }

    strlcpy(pstRes->pc, result.value.c_str(), pstRes->u32Len);

    if (result.value.size() < pstRes->u32Len)
    {
        LOGI("configstore: updating result size from %u to %zu",
             pstRes->u32Len, result.value.size());
        pstRes->u32Len = result.value.size();
    }

    return VPP_OK;
}

uint32_t bVppConfigStore_GetBool(const char *pcArea,
                                 const char *pcConfig,
                                 uint32_t bDefault)
{
    static const char * acTrueStr[] = {
        "1", "t", "y", "yes", "true"
    };

    uint32_t i;
    uint32_t bResult = bDefault;
    char buf[MAX_CONFIG_STORE_STR_SIZE];
    t_StConfigStoreStr stStr;
    stStr.u32Len = MAX_CONFIG_STORE_STR_SIZE;
    stStr.pc = buf;

    VPP_RET_IF_NULL(pcArea, bResult);
    VPP_RET_IF_NULL(pcConfig, bResult);

    if (u32VppConfigStore_GetString(pcArea, pcConfig, &stStr) != VPP_OK)
    {
        LOGE("configstore: unable to find area=%s, config=%s, returning default: %u",
             pcArea, pcConfig, bResult);
        return bResult;
    }

    // If it was found in the configstore, then the result should no longer be
    // the default, but reflect the value that was pulled.
    bResult = VPP_FALSE;
    for (i = 0; i < sizeof(acTrueStr) / sizeof(const char *); i++)
    {
        const char *pcTrue = acTrueStr[i];

        if (strlen(pcTrue) != stStr.u32Len)
        {
            continue;
        }

        if (strncasecmp(pcTrue, stStr.pc, stStr.u32Len) == 0)
        {
            bResult = VPP_TRUE;
            break;
        }
    }

    LOGI("configstore (bool): area=%s, config=%s, result=%u",
         pcArea, pcConfig, bResult);
    return bResult;
}

uint32_t u32VppConfigStore_GetUnsignedInt(const char *pcArea,
                                          const char *pcConfig,
                                          uint32_t u32Default)
{
    char buf[MAX_CONFIG_STORE_STR_SIZE];
    t_StConfigStoreStr stStr;
    stStr.u32Len = MAX_CONFIG_STORE_STR_SIZE;
    stStr.pc = buf;

    uint32_t u32Ret = u32Default;

    VPP_RET_IF_NULL(pcArea, u32Ret);
    VPP_RET_IF_NULL(pcConfig, u32Ret);

    if (u32VppConfigStore_GetString(pcArea, pcConfig, &stStr) != VPP_OK)
    {
        LOGE("configstore: unable to find area=%s, config=%s, returning default=%u",
             pcArea, pcConfig, u32Ret);
        return u32Ret;
    }

    u32Ret = strtoul(stStr.pc, NULL, 0);

    LOGI("configstore (int): area=%s, config=%s, result=%u",
         pcArea, pcConfig, u32Ret);
    return u32Ret;
}
