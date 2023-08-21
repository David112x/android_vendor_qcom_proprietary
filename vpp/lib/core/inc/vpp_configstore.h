/*!
 * @file vpp_configstore.h
 *
 * @cr
 * Copyright (c) 2019 Qualcomm Technologies, Inc.
 * All Rights Reserved.
 * Confidential and Proprietary - Qualcomm Technologies, Inc.

 * @services
 */

#ifndef _VPP_CONFIGSTORE_H_
#define _VPP_CONFIGSTORE_H_

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    char *pc;
    uint32_t u32Len;
} t_StConfigStoreStr;

/***************************************************************************
 * Function Prototypes
 ***************************************************************************/
uint32_t u32VppConfigStore_GetString(const char *pcArea,
                                     const char *pcConfig,
                                     t_StConfigStoreStr *pstRes);

uint32_t bVppConfigStore_GetBool(const char *pcArea,
                                 const char *pcConfig,
                                 uint32_t bDefault);

uint32_t u32VppConfigStore_GetUnsignedInt(const char *pcArea,
                                          const char *pcConfig,
                                          uint32_t u32Default);
#ifdef __cplusplus
}
#endif

#endif /* _VPP_CONFIGSTORE_H_ */
