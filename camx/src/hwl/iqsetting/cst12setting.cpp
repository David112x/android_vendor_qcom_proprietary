// NOWHINE NC009 <- Shared file with system team so uses non-CamX file naming
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2017 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  cst12setting.cpp
/// @brief CST12 setting calculation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#include "cst12setting.h"

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CST12Setting::CalculateHWSetting
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL CST12Setting::CalculateHWSetting(
    const CST12InputData*                                 pInput,
    cst_1_2_0::chromatix_cst12_reserveType*               pReserveType,
    cst_1_2_0::chromatix_cst12Type::enable_sectionStruct* pModuleEnable,
    VOID*                                                 pOutput)
{
    CAMX_UNREFERENCED_PARAM(pInput);

    BOOL result = TRUE;

    if ((NULL != pInput)        &&
        (NULL != pReserveType)  &&
        (NULL != pModuleEnable) &&
        (NULL != pOutput))
    {
        CST12UnpackedField* pUnpackedField = static_cast<CST12UnpackedField*>(pOutput);

        pUnpackedField->enable = static_cast<UINT16>(pModuleEnable->cst_enable);
        pUnpackedField->c00    = static_cast<UINT16>(IQSettingUtils::ClampUINT32(pReserveType->c_x0_tab.c_x0[0],
                                                                                 CST12_C00_MIN,
                                                                                 CST12_C00_MAX));
        pUnpackedField->c10    = static_cast<UINT16>(IQSettingUtils::ClampUINT32(pReserveType->c_x0_tab.c_x0[1],
                                                                                 CST12_C10_MIN,
                                                                                 CST12_C10_MAX));
        pUnpackedField->c20    = static_cast<UINT16>(IQSettingUtils::ClampUINT32(pReserveType->c_x0_tab.c_x0[2],
                                                                                 CST12_C20_MIN,
                                                                                 CST12_C20_MAX));
        pUnpackedField->c01    = static_cast<UINT16>(IQSettingUtils::ClampUINT32(pReserveType->c_x1_tab.c_x1[0],
                                                                                 CST12_C01_MIN,
                                                                                 CST12_C01_MAX));
        pUnpackedField->c11    = static_cast<UINT16>(IQSettingUtils::ClampUINT32(pReserveType->c_x1_tab.c_x1[1],
                                                                                 CST12_C11_MIN,
                                                                                 CST12_C11_MAX));
        pUnpackedField->c21    = static_cast<UINT16>(IQSettingUtils::ClampUINT32(pReserveType->c_x1_tab.c_x1[2],
                                                                                 CST12_C21_MIN,
                                                                                 CST12_C21_MAX));
        pUnpackedField->s0     = static_cast<INT16>(IQSettingUtils::ClampINT32(pReserveType->s_tab.s[0],
                                                                               CST12_S_MIN,
                                                                               CST12_S_MAX));
        pUnpackedField->s1     = static_cast<INT16>(IQSettingUtils::ClampINT32(pReserveType->s_tab.s[1],
                                                                               CST12_S_MIN,
                                                                               CST12_S_MAX));
        pUnpackedField->s2     = static_cast<INT16>(IQSettingUtils::ClampINT32(pReserveType->s_tab.s[2],
                                                                               CST12_S_MIN,
                                                                               CST12_S_MAX));
        pUnpackedField->o0     = static_cast<INT16>(IQSettingUtils::ClampINT32(pReserveType->o_tab.o[0],
                                                                               CST12_O_MIN,
                                                                               CST12_O_MAX));
        pUnpackedField->o1     = static_cast<INT16>(IQSettingUtils::ClampINT32(pReserveType->o_tab.o[1],
                                                                               CST12_O_MIN,
                                                                               CST12_O_MAX));
        pUnpackedField->o2     = static_cast<INT16>(IQSettingUtils::ClampINT32(pReserveType->o_tab.o[2],
                                                                               CST12_O_MIN,
                                                                               CST12_O_MAX));
        pUnpackedField->m00    = static_cast<INT16>(IQSettingUtils::ClampINT32(
                                     IQSettingUtils::FloatToQNumber(pReserveType->m_tab.m[0], CST12_M00_QFACTOR),
                                     CST12_M00_MIN,
                                     CST12_M00_MAX));
        pUnpackedField->m01    = static_cast<INT16>(IQSettingUtils::ClampINT32(
                                     IQSettingUtils::FloatToQNumber(pReserveType->m_tab.m[1], CST12_M01_QFACTOR),
                                     CST12_M01_MIN,
                                     CST12_M01_MAX));
        pUnpackedField->m02    = static_cast<INT16>(IQSettingUtils::ClampINT32(
                                     IQSettingUtils::FloatToQNumber(pReserveType->m_tab.m[2], CST12_M02_QFACTOR),
                                     CST12_M02_MIN,
                                     CST12_M02_MAX));
        pUnpackedField->m10    = static_cast<INT16>(IQSettingUtils::ClampINT32(
                                     IQSettingUtils::FloatToQNumber(pReserveType->m_tab.m[3], CST12_M10_QFACTOR),
                                     CST12_M10_MIN,
                                     CST12_M10_MAX));
        pUnpackedField->m11    = static_cast<INT16>(IQSettingUtils::ClampINT32(
                                     IQSettingUtils::FloatToQNumber(pReserveType->m_tab.m[4], CST12_M11_QFACTOR),
                                     CST12_M11_MIN,
                                     CST12_M11_MAX));
        pUnpackedField->m12    = static_cast<INT16>(IQSettingUtils::ClampINT32(
                                     IQSettingUtils::FloatToQNumber(pReserveType->m_tab.m[5], CST12_M12_QFACTOR),
                                     CST12_M12_MIN,
                                     CST12_M12_MAX));
        pUnpackedField->m20    = static_cast<INT16>(IQSettingUtils::ClampINT32(
                                     IQSettingUtils::FloatToQNumber(pReserveType->m_tab.m[6], CST12_M20_QFACTOR),
                                     CST12_M20_MIN,
                                     CST12_M20_MAX));
        pUnpackedField->m21    = static_cast<INT16>(IQSettingUtils::ClampINT32(
                                     IQSettingUtils::FloatToQNumber(pReserveType->m_tab.m[7], CST12_M21_QFACTOR),
                                     CST12_M21_MIN,
                                     CST12_M21_MAX));
        pUnpackedField->m22    = static_cast<INT16>(IQSettingUtils::ClampINT32(
                                     IQSettingUtils::FloatToQNumber(pReserveType->m_tab.m[8], CST12_M22_QFACTOR),
                                     CST12_M22_MIN,
                                     CST12_M22_MAX));
    }
    else
    {
        /// @todo (CAMX-1812) Need to add logging for Common library
        result = FALSE;
    }

    return result;
}
