/*!
 * @file vpp_tunings.h
 *
 * @cr
 * Copyright (c) 2019 Qualcomm Technologies, Inc.
 * All Rights Reserved.
 * Confidential and Proprietary - Qualcomm Technologies, Inc.

 * @services
 */

#ifndef _VPP_TUNINGS_H_
#define _VPP_TUNINGS_H_

#include <pthread.h>

#define VPP_TUNINGS_MAX_FILE_NAME_SIZE 256
#define VPP_TUNINGS_MAX_TUNING_NAME_SIZE 200

#define IS_IN_RANGE(val, min, max)      ((val) >= (min) && (val) <= (max))
#define RET_ERR_IF_U32_OUT_OF_RANGE(pst, idx, min, max) \
    if (!IS_IN_RANGE((pst)->puVal[(idx)].U32, (min), (max))) \
    { \
        LOGE("Tuning %s is out of range. idx=%u, val=%u, min=%u, max=%u", \
             (pst)->pstDef->acId, (idx), (pst)->puVal[(idx)].U32, \
             (min), (max)); \
        return VPP_ERR; \
    }

#define TUNING_DEF(_nm, _type, _count, _min, _max, _val) \
{ \
    .acId = #_nm, \
    .u32Id = _nm, \
    .u32Count = _count, \
    .eType = TUNING_TYPE_##_type, \
    .min._type = _min, \
    .max._type = _max, \
    .pfVal = _val, \
}

#define RET_ERR_IF_NULL_TUNING(pst) \
    if (!(pst) || !(pst)->pstDef || !(pst)->puVal) \
        return VPP_ERR;

#define RET_ERR_IF_WRONG_TUNING(pst, id) \
    if ((pst)->pstDef->u32Id != (id)) \
        return VPP_ERR;


struct StTuning;
typedef uint32_t (*val_fnc)(struct StTuning *pstTuning);

typedef enum {
    TUNING_TYPE_FLOAT,
    TUNING_TYPE_U32,
    TUNING_TYPE_S32,
} t_ETuningType;

typedef union {
    uint32_t U32;
    int32_t S32;
    float FLOAT;
} t_UTuningValue;

typedef struct {
    char acId[VPP_TUNINGS_MAX_TUNING_NAME_SIZE];
    uint32_t u32Id;
    t_ETuningType eType;
    t_UTuningValue min;
    t_UTuningValue max;
    uint32_t u32Count;
    val_fnc pfVal;
} t_StTuningDef;

typedef struct StTuning {
    struct StTuning *pstNext;
    const t_StTuningDef *pstDef;
    uint32_t u32ExternalCnt; // number of values that were loaded from external
    uint32_t bValid;
    t_UTuningValue *puVal;
} t_StTuning;

typedef struct StTuningBlock {
    t_StTuningDef *pstTuningDef;
    uint32_t u32TuningDefCnt;
    char acFileName[VPP_TUNINGS_MAX_FILE_NAME_SIZE];
    t_StTuning *pstTuningList;
    uint32_t u32TuningListCnt; // Total list size
    uint32_t u32TuningListValidCnt; // Number of valid entries (bValid is true)
} t_StTuningBlock;

/***************************************************************************
 * Function Prototypes
 ***************************************************************************/
void *vpVppTunings_Init(const char *pcFileName,
                        const t_StTuningDef *pstTuningDefSrc,
                        uint32_t u32TuningDefCnt);
void vVppTunings_Term(void *pstCb);
uint32_t u32VppTunings_GetValidTuningsCount(void *pstCb);
t_StTuning *pstVppTunings_GetTuningByIndex(void *pstCb, uint32_t u32Idx);
t_StTuning *pstVppTunings_GetTuningById(void *pstCb, uint32_t u32TuningId);
uint32_t u32VppTunings_GetTuningCount(t_StTuning *pstTuning);
uint32_t u32VppTunings_GetTuningCountById(void *pstCb, uint32_t u32TuningId);
uint32_t u32VppTunings_GetTuningCountByIndex(void *pstCb, uint32_t u32Idx);
uint32_t u32VppTunings_GetTuningValues(t_StTuning *pstTuning,
                                       t_UTuningValue *puTuning,
                                       uint32_t u32Len);
uint32_t u32VppTunings_GetTuningValuesByIndex(void *pstCb, uint32_t u32Idx,
                                              t_UTuningValue *puTuning,
                                              uint32_t u32Len);
uint32_t u32VppTunings_GetTuningValuesById(void *pstCb, uint32_t u32TuningId,
                                           t_UTuningValue *puTuning,
                                           uint32_t u32Len);

#endif /* _VPP_TUNINGS_H_ */
