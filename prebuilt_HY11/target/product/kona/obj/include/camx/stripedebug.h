#ifndef _STRIPEDEBUG_H_
#define _STRIPEDEBUG_H_

//-------------------------------------------------------------------------
// Copyright (c) 2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.

//------------------------------------------------------------------------
// @file  stripedebug.h
// @brief Add userArgs for IPE and BPS
//------------------------------------------------------------------------

/*------------------------------------------------------------------------
*       Include Files
* ----------------------------------------------------------------------- */

#ifdef __cplusplus
extern "C" {
#endif

/*------------------------------------------------------------------------
*       Type Declarations
* ----------------------------------------------------------------------- */

typedef struct {
    uint32_t        realTime;
    uint32_t        processingType;
    uint32_t        profileID;
    uint32_t        frameNumber;
    uint32_t        instance;
    uint32_t        dumpEnable;
    uint32_t        fileOverwriteEnable;
    const char     *FileDumpPath;
} userIPEArgs;

typedef struct {
    uint32_t        realTime;
    uint32_t        processingType;
    uint32_t        profileID;
    uint32_t        frameNumber;
    uint32_t        instance;
    uint32_t        dumpEnable;
    uint32_t        fileOverwriteEnable;
    const char      *FileDumpPath;
} userBPSArgs;

#ifdef __cplusplus
}
#endif

#endif /* _STRIPEDEBUG_H_ */
