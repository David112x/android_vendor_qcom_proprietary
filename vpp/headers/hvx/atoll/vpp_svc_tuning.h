/**
 Copyright (c) 2019 Qualcomm Technologies, Inc.
 All Rights Reserved.
 Confidential and Proprietary - Qualcomm Technologies, Inc.
**/

#include <stdlib.h>

#ifndef VPP_SVC_TUNING_H
#define VPP_SVC_TUNING_H

#define VPP_SVC_TUNING_DEF(_nm, _type, _count, _min, _max) \
{ \
    .pcId = #_nm, \
    .u32Id = _nm, \
    .u32Count = _count, \
    .eType = VPP_SVC_TUNING_TYPE_##_type, \
    .min._type = _min, \
    .max._type = _max, \
}

#define VPP_SVC_TUNING_MAX_CHAR 200

#define fixed16p16_to_float(i_fValue) ((float) i_fValue * 0.0000152587890625f) /* i_fValue / (1 << 16) */
#define float_to_fixed16p16(i_fValue) ((FIXED16P16_t) ((float) i_fValue * 65536.0f)) /* i_fValue * (1 << 16) */

typedef int32_t FIXED16P16_t;

typedef enum {
    VPP_SVC_TUNING_TYPE_FIXED16P16,
    VPP_SVC_TUNING_TYPE_U32,
    VPP_SVC_TUNING_TYPE_S32,
} t_EVppSvcTuningType;

typedef union {
    uint32_t U32;
    int32_t S32;
    FIXED16P16_t FIXED16P16;
} t_UVppSvcTuningValue;

typedef struct {
    const char pcId[VPP_SVC_TUNING_MAX_CHAR]; // String ID (matches name of your enum)
    uint32_t u32Id; // your enum value
    t_EVppSvcTuningType eType; // defined above
    t_UVppSvcTuningValue min; // min value of each element
    t_UVppSvcTuningValue max; // max value of each element
    uint32_t u32Count; // number of elements needed
} t_StVppSvcTuningDef;

#endif //VPP_SVC_TUNING_H
