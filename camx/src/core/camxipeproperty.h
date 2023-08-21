
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file camxipeproperty.h
/// @brief Define ipe properties per usecase and per frame
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef CAMXIPEPROPERTY_H
#define CAMXIPEPROPERTY_H

#include "camxdefs.h"
#include "camxtypes.h"


CAMX_NAMESPACE_BEGIN

static const UINT32 IPE_MAX_PASSES                  = 4; // Number of multi pass
/// @ brief scratch buffer data
struct ScratchBufferData
{
    UINT32 scratchBufferSize;           ///< scratch Buffer size
    UINT32 pdiOffset[IPE_MAX_PASSES];   ///< pdi offset
    UINT32 pdiSize [IPE_MAX_PASSES];    ///< pdi size
    UINT32 tfiOffset[IPE_MAX_PASSES];   ///< tfi offset
    UINT32 tfiSize[IPE_MAX_PASSES];     ///< tfi size
    UINT32 tfrOffset[IPE_MAX_PASSES];   ///< tfr offset
    UINT32 tfrSize[IPE_MAX_PASSES];     ///< tfr size
    UINT32 lmciOffset[IPE_MAX_PASSES];  ///< lmci offset
    UINT32 lmciSize[IPE_MAX_PASSES];    ///< lmci size
};

CAMX_NAMESPACE_END

#endif // CAMXIPEPROPERTY_H
