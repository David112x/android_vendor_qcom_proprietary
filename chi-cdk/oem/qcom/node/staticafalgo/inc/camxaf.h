////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2017 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  camxaf.h
/// @brief Entry to AF algorithm. Implements C style Algorithm interface.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef CAMXAF_H
#define CAMXAF_H

#include "chiafinterface.h"
#include "camxdefs.h"

CAMX_NAMESPACE_BEGIN

/// @brief  Represents AF internal data
struct AFInternalDataType
{
    CHIAFAlgorithm  algorithmOps;      ///< AF Algorithm operations that can be performed
    VOID*           pAFCoreAlgorithm;  ///< AF Algorithm handle
};

CAMX_NAMESPACE_END

#endif // CAMXAF_H
