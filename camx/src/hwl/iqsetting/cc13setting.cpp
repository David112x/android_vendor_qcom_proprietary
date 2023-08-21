// NOWHINE NC009 <- Shared file with system team so uses non-CamX file naming
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2017 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  cc13setting.cpp
/// @brief BPs cc13 setting calculation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#include "cc13setting.h"

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CC13Setting::CalculateHWSetting
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL CC13Setting::CalculateHWSetting(
    const CC13InputData*                                pInput,
    cc_1_3_0::cc13_rgn_dataType*                        pData,
    cc_1_3_0::chromatix_cc13_reserveType*               pReserveType,
    cc_1_3_0::chromatix_cc13Type::enable_sectionStruct* pModuleEnable,
    VOID*                                               pOutput)
{
    BOOL result = TRUE;

    if ((NULL != pInput)           &&
        (NULL != pData)            &&
        (NULL != pReserveType)     &&
        (NULL != pModuleEnable)    &&
        (NULL != pOutput))
    {
        CC13UnpackedField* pUnpackedField = static_cast<CC13UnpackedField*>(pOutput);
        UINT32             qFactor        = pReserveType->q_factor;

        pUnpackedField->qfactor = static_cast<INT16>(qFactor);

        pUnpackedField->enable  = static_cast<UINT16>(pModuleEnable->cc_enable);
        pUnpackedField->c0      =
            static_cast<INT16>(IQSettingUtils::FloatToQNumber(pData->c_tab.c[4], (CC13_Q_FACTOR + qFactor))); // G
        pUnpackedField->c1      =
            static_cast<INT16>(IQSettingUtils::FloatToQNumber(pData->c_tab.c[5], (CC13_Q_FACTOR + qFactor))); // B
        pUnpackedField->c2      =
            static_cast<INT16>(IQSettingUtils::FloatToQNumber(pData->c_tab.c[3], (CC13_Q_FACTOR + qFactor))); // R
        pUnpackedField->c3      =
            static_cast<INT16>(IQSettingUtils::FloatToQNumber(pData->c_tab.c[7], (CC13_Q_FACTOR + qFactor))); // G
        pUnpackedField->c4      =
            static_cast<INT16>(IQSettingUtils::FloatToQNumber(pData->c_tab.c[8], (CC13_Q_FACTOR + qFactor))); // B
        pUnpackedField->c5      =
            static_cast<INT16>(IQSettingUtils::FloatToQNumber(pData->c_tab.c[6], (CC13_Q_FACTOR + qFactor))); // R
        pUnpackedField->c6      =
            static_cast<INT16>(IQSettingUtils::FloatToQNumber(pData->c_tab.c[1], (CC13_Q_FACTOR + qFactor))); // G
        pUnpackedField->c7      =
            static_cast<INT16>(IQSettingUtils::FloatToQNumber(pData->c_tab.c[2], (CC13_Q_FACTOR + qFactor))); // B
        pUnpackedField->c8      =
            static_cast<INT16>(IQSettingUtils::FloatToQNumber(pData->c_tab.c[0], (CC13_Q_FACTOR + qFactor))); // R

       // Need to update in relation to frame rate
        pUnpackedField->k0      = static_cast<INT16>(IQSettingUtils::RoundFLOAT(pData->k_tab.k[1]));  // G
        pUnpackedField->k1      = static_cast<INT16>(IQSettingUtils::RoundFLOAT(pData->k_tab.k[2]));  // B
        pUnpackedField->k2      = static_cast<INT16>(IQSettingUtils::RoundFLOAT(pData->k_tab.k[0]));  // R
    }
    else
    {
        /// @todo (CAMX-1812) Need to add logging for Common library
        result = FALSE;
    }

    return result;
}
