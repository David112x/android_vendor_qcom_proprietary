// NOWHINE NC009 <- Shared file with system team so uses non-CamX file naming
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2017 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  cst12setting.h
/// @brief CST12 module setting calculation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef CST12SETTING_H
#define CST12SETTING_H
#include "cst_1_2_0.h"
#include "iqcommondefs.h"
#include "iqsettingutil.h"

/// @brief CST12 Module Common library output: Unpacked Data
// NOWHINE NC004c: Share code with system team
struct CST12UnpackedField
{
    UINT16 enable;    ///< Module enable flag
    UINT16 c00;       ///< C00
    UINT16 c10;       ///< C10
    UINT16 c20;       ///< C20
    UINT16 c01;       ///< C01
    UINT16 c11;       ///< C11
    UINT16 c21;       ///< C21
    INT16  m00;       ///< M00
    INT16  m01;       ///< M01
    INT16  m02;       ///< M02
    INT16  m10;       ///< M10
    INT16  m11;       ///< M11
    INT16  m12;       ///< M12
    INT16  m20;       ///< M20
    INT16  m21;       ///< M21
    INT16  m22;       ///< M22
    INT16  o0;        ///< O0
    INT16  o1;        ///< O1
    INT16  o2;        ///< O2
    INT16  s0;        ///< S0
    INT16  s1;        ///< S1
    INT16  s2;        ///< S2
};

/// @todo (CAMX-1771) Should this constant be in the auto-generated system header file
static const INT32  QFACTOR           = 10;
static const INT32  CST12_S_MIN       = IQSettingUtils::MININT32BITFIELD(11);
static const INT32  CST12_S_MAX       = IQSettingUtils::MAXINT32BITFIELD(11);
static const INT32  CST12_O_MIN       = IQSettingUtils::MINUINTBITFIELD(10);
static const INT32  CST12_O_MAX       = IQSettingUtils::MAXUINTBITFIELD(10);
static const INT32  CST12_M00_QFACTOR = QFACTOR;
static const INT32  CST12_M00_MIN     = IQSettingUtils::MININT32BITFIELD(13);
static const INT32  CST12_M00_MAX     = IQSettingUtils::MAXINT32BITFIELD(13);
static const INT32  CST12_M01_QFACTOR = QFACTOR;
static const INT32  CST12_M01_MIN     = IQSettingUtils::MININT32BITFIELD(13);
static const INT32  CST12_M01_MAX     = IQSettingUtils::MAXINT32BITFIELD(13);
static const INT32  CST12_M02_QFACTOR = QFACTOR;
static const INT32  CST12_M02_MIN     = IQSettingUtils::MININT32BITFIELD(13);
static const INT32  CST12_M02_MAX     = IQSettingUtils::MAXINT32BITFIELD(13);
static const INT32  CST12_M10_QFACTOR = QFACTOR;
static const INT32  CST12_M10_MIN     = IQSettingUtils::MININT32BITFIELD(13);
static const INT32  CST12_M10_MAX     = IQSettingUtils::MAXINT32BITFIELD(13);
static const INT32  CST12_M11_QFACTOR = QFACTOR;
static const INT32  CST12_M11_MIN     = IQSettingUtils::MININT32BITFIELD(13);
static const INT32  CST12_M11_MAX     = IQSettingUtils::MAXINT32BITFIELD(13);
static const INT32  CST12_M12_QFACTOR = QFACTOR;
static const INT32  CST12_M12_MIN     = IQSettingUtils::MININT32BITFIELD(13);
static const INT32  CST12_M12_MAX     = IQSettingUtils::MAXINT32BITFIELD(13);
static const INT32  CST12_M20_QFACTOR = QFACTOR;
static const INT32  CST12_M20_MIN     = IQSettingUtils::MININT32BITFIELD(13);
static const INT32  CST12_M20_MAX     = IQSettingUtils::MAXINT32BITFIELD(13);
static const INT32  CST12_M21_QFACTOR = QFACTOR;
static const INT32  CST12_M21_MIN     = IQSettingUtils::MININT32BITFIELD(13);
static const INT32  CST12_M21_MAX     = IQSettingUtils::MAXINT32BITFIELD(13);
static const INT32  CST12_M22_QFACTOR = QFACTOR;
static const INT32  CST12_M22_MIN     = IQSettingUtils::MININT32BITFIELD(13);
static const INT32  CST12_M22_MAX     = IQSettingUtils::MAXINT32BITFIELD(13);
static const UINT32 CST12_C00_MIN     = IQSettingUtils::MINUINTBITFIELD(10);
static const UINT32 CST12_C00_MAX     = IQSettingUtils::MAXUINTBITFIELD(10);
static const UINT32 CST12_C10_MIN     = IQSettingUtils::MINUINTBITFIELD(10);
static const UINT32 CST12_C10_MAX     = IQSettingUtils::MAXUINTBITFIELD(10);
static const UINT32 CST12_C20_MIN     = IQSettingUtils::MINUINTBITFIELD(10);
static const UINT32 CST12_C20_MAX     = IQSettingUtils::MAXUINTBITFIELD(10);
static const UINT32 CST12_C01_MIN     = IQSettingUtils::MINUINTBITFIELD(10);
static const UINT32 CST12_C01_MAX     = IQSettingUtils::MAXUINTBITFIELD(10);
static const UINT32 CST12_C11_MIN     = IQSettingUtils::MINUINTBITFIELD(10);
static const UINT32 CST12_C11_MAX     = IQSettingUtils::MAXUINTBITFIELD(10);
static const UINT32 CST12_C21_MIN     = IQSettingUtils::MINUINTBITFIELD(10);
static const UINT32 CST12_C21_MAX     = IQSettingUtils::MAXUINTBITFIELD(10);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief Class that implements CST12 module setting calculation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// NOWHINE NC004c: Share code with system team
class CST12Setting
{
public:
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// CalculateHWSetting
    ///
    /// @brief  Calculate the unpacked register value
    ///
    /// @param  pInput        Pointer to the Input Data Structure
    /// @param  pReserveType  Pointer to the reserveType of this module
    /// @param  pModuleEnable Pointer to the variable(s) to enable this module
    /// @param  pRegCmd       Pointer to the Unpacked Data Structure
    ///
    /// @return TRUE for success
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static BOOL CalculateHWSetting(
        const CST12InputData*                                 pInput,
        cst_1_2_0::chromatix_cst12_reserveType*               pReserveType,
        cst_1_2_0::chromatix_cst12Type::enable_sectionStruct* pModuleEnable,
        VOID*                                                 pRegCmd);
};
#endif // CST12SETTING_H
