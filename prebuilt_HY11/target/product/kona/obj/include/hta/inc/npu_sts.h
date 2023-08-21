/*========================================================================*/
/**
\file npu_sts.h

    Copyright (c) 2018-2019 Qualcomm Technologies, Inc.
    All rights reserved.
    Confidential and Proprietary - Qualcomm Technologies, Inc.
*/
/*========================================================================*/

#ifndef NPU_STS_H
#define NPU_STS_H

#include <stdint.h>

/*========================================================================
 Error Codes
 ========================================================================*/

#define NPU_STS_SUCCESS                       0
#define NPU_STS_FAILED                       -1000
#define NPU_STS_INVALID_PARAM                -1002
#define NPU_STS_TIMEOUT                      -1003
#define NPU_STS_OUT_OF_RESOURCE              -1004
#define NPU_STS_NOT_SUPPORTED                -1005
#define NPU_STS_INTERNAL_ERROR               -1006
#define NPU_STS_REQUEST_PENDING              -1007
#define NPU_STS_RESOURCE_INUSE               -1008
#define NPU_STS_BAD_STATE                    -1009
#define NPU_STS_LAYER_STATS_NOT_SUPPORTED    -1010


/*========================================================================
 Data Types
 ========================================================================*/
typedef int32_t npu_sts_t;

#endif // NPU_STS_H
