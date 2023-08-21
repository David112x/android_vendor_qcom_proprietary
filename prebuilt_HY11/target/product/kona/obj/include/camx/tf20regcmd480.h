// NOWHINE ENTIRE FILE
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  tf20regcmd480.h
/// @brief camxipetfregcmd structure definition
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef TF20REGCMD480_H
#define TF20REGCMD480_H

#include "titan480_ipe.h"
#include "tfbaseregcmd.h"

CAMX_BEGIN_PACKED

struct IPETFRegCmd480 : public IPETFRegCmd
{
    IPE_IPE_0_NPS_CLC_TF_TF_LM_CONFIG_0                   lmConfig0;              ///< LMC configuration params
    IPE_IPE_0_NPS_CLC_TF_TF_LM_CONFIG_1                   lmConfig1;              ///< LMC configuration params
    IPE_IPE_0_NPS_CLC_TF_TF_LM_CONFIG_2                   lmConfig2;              ///< LMC configuration params
    IPE_IPE_0_NPS_CLC_TF_TF_LM_CROP_VER                   lmCropVer;              ///< LMC configuration params
    IPE_IPE_0_NPS_CLC_TF_TF_LM_CROP_HOR                   lmCropHor;              ///< LMC configuration params
} CAMX_PACKED;

CAMX_END_PACKED

#endif // TF20REGCMD480_H
