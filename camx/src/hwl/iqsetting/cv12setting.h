// NOWHINE NC009 <- Shared file with system team so uses non-CamX file naming
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2017, 2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  cv12setting.h
/// @brief CV12 module setting calculation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef CV12SETTING_H
#define CV12SETTING_H
#include "cv_1_2_0.h"
#include "iqcommondefs.h"
#include "iqsettingutil.h"

/// @brief CV12 Module Unpacked Data Structure
// NOWHINE NC004c: Share code with system team
struct CV12UnpackedField
{
    UINT16 enable;    ///< Module enable flag
    FLOAT rToY;       ///< 12 bit
    FLOAT gToY;       ///< 12 bit
    FLOAT bToY;       ///< 12 bit
    FLOAT yOffset;    ///< 11 bit
    FLOAT ap;         ///< 12 bit
    FLOAT am;         ///< 12 bit
    FLOAT bp;         ///< 12 bit
    FLOAT bm;         ///< 12 bit
    FLOAT cp;         ///< 12 bit
    FLOAT cm;         ///< 12 bit
    FLOAT dp;         ///< 12 bit
    FLOAT dm;         ///< 12 bit
    FLOAT kcr;        ///< 13 bit
    FLOAT kcb;        ///< 13 bit
};

static const INT32   BITWIDTH_FACTOR                = 2;
static const UINT32  CV12_RGB2Y_DATA_Q_FACTOR       = 8;
static const INT32   CV12_RGB2Y_RTOY_MIN            = MIN_INT12;
static const INT32   CV12_RGB2Y_RTOY_MAX            = MAX_INT12;
static const INT32   CV12_RGB2Y_GTOY_MIN            = MIN_INT12;
static const INT32   CV12_RGB2Y_GTOY_MAX            = MAX_INT12;
static const INT32   CV12_RGB2Y_BTOY_MIN            = MIN_INT12;
static const INT32   CV12_RGB2Y_BTOY_MAX            = MAX_INT12;
static const INT32   CV12_RGB2Y_OFFSET_MIN          = MIN_INT11;
static const INT32   CV12_RGB2Y_OFFSET_MAX          = MAX_INT11;
static const UINT32  CV12_CHROMAPROC_DATA_Q_FACTOR  = 8;
static const INT32   CV12_CHROMAPROC_AP_MIN         = MIN_INT12;
static const INT32   CV12_CHROMAPROC_AP_MAX         = MAX_INT12;
static const INT32   CV12_CHROMAPROC_AM_MIN         = MIN_INT12;
static const INT32   CV12_CHROMAPROC_AM_MAX         = MAX_INT12;
static const INT32   CV12_CHROMAPROC_BP_MIN         = MIN_INT12;
static const INT32   CV12_CHROMAPROC_BP_MAX         = MAX_INT12;
static const INT32   CV12_CHROMAPROC_BM_MIN         = MIN_INT12;
static const INT32   CV12_CHROMAPROC_BM_MAX         = MAX_INT12;
static const INT32   CV12_CHROMAPROC_CP_MIN         = MIN_INT12;
static const INT32   CV12_CHROMAPROC_CP_MAX         = MAX_INT12;
static const INT32   CV12_CHROMAPROC_CM_MIN         = MIN_INT12;
static const INT32   CV12_CHROMAPROC_CM_MAX         = MAX_INT12;
static const INT32   CV12_CHROMAPROC_DP_MIN         = MIN_INT12;
static const INT32   CV12_CHROMAPROC_DP_MAX         = MAX_INT12;
static const INT32   CV12_CHROMAPROC_DM_MIN         = MIN_INT12;
static const INT32   CV12_CHROMAPROC_DM_MAX         = MAX_INT12;
static const INT32   CV12_CHROMAPROC_KCR_MIN        = IQSettingUtils::MININT32BITFIELD(11 + BITWIDTH_FACTOR);
static const INT32   CV12_CHROMAPROC_KCR_MAX        = IQSettingUtils::MAXINT32BITFIELD(11 + BITWIDTH_FACTOR);
static const INT32   CV12_CHROMAPROC_KCB_MIN        = IQSettingUtils::MININT32BITFIELD(11 + BITWIDTH_FACTOR);
static const INT32   CV12_CHROMAPROC_KCB_MAX        = IQSettingUtils::MAXINT32BITFIELD(11 + BITWIDTH_FACTOR);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief Class that implements CV12 module setting calculation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// NOWHINE NC004c: Share code with system team
class CV12Setting
{
public:
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// CalculateHWSetting
    ///
    /// @brief  Calculate the unpacked register value
    ///
    /// @param  pInput         Pointer to the input data
    /// @param  pData          Pointer to the intepolation result
    /// @param  pModuleEnable  Pointer to the variable(s) to enable this module
    /// @param  pUnpackedField Pointer to the unpacked register result
    ///
    /// @return TRUE for success
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static BOOL CalculateHWSetting(
        const CV12InputData*                                pInput,
        cv_1_2_0::cv12_rgn_dataType*                        pData,
        cv_1_2_0::chromatix_cv12Type::enable_sectionStruct* pModuleEnable,
        VOID*                                               pUnpackedField);
};

#endif // CV12SETTING_H
