// NOWHINE NC009 <- Shared file with system team so uses non-CamX file naming
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2017 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  ifecc12setting.cpp
/// @brief IFE cc12 setting calculation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#include "ifecc12setting.h"

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// IFECC12Setting::CalculateHWSetting
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL IFECC12Setting::CalculateHWSetting(
    const CC12InputData*                  pInput,
    cc_1_2_0::cc12_rgn_dataType*          pData,
    cc_1_2_0::chromatix_cc12_reserveType* pReserveType,
    VOID*                                 pOutput)
{
    BOOL   result  = TRUE;
    UINT32 qFactor = 0;

    if ((NULL != pInput) && (NULL != pData) && (NULL != pOutput) && (NULL != pReserveType))
    {
        CC12UnpackedField* pUnpackedField = static_cast<CC12UnpackedField*>(pOutput);

        qFactor = pReserveType->q_factor;

        pUnpackedField->c0_l = static_cast<INT16>(IQSettingUtils::RoundFLOAT(
           pData->c_tab.c[4] * (1 << (CC12_Q_FACTOR + qFactor))));
        pUnpackedField->c1_l = static_cast<INT16>(IQSettingUtils::RoundFLOAT(
           pData->c_tab.c[5] * (1 << (CC12_Q_FACTOR + qFactor))));
        pUnpackedField->c2_l = static_cast<INT16>(IQSettingUtils::RoundFLOAT(
           pData->c_tab.c[3] * (1 << (CC12_Q_FACTOR + qFactor))));
        pUnpackedField->c3_l = static_cast<INT16>(IQSettingUtils::RoundFLOAT(
           pData->c_tab.c[7] * (1 << (CC12_Q_FACTOR + qFactor))));
        pUnpackedField->c4_l = static_cast<INT16>(IQSettingUtils::RoundFLOAT(
           pData->c_tab.c[8] * (1 << (CC12_Q_FACTOR + qFactor))));
        pUnpackedField->c5_l = static_cast<INT16>(IQSettingUtils::RoundFLOAT(
           pData->c_tab.c[6] * (1 << (CC12_Q_FACTOR + qFactor))));
        pUnpackedField->c6_l = static_cast<INT16>(IQSettingUtils::RoundFLOAT(
           pData->c_tab.c[1] * (1 << (CC12_Q_FACTOR + qFactor))));
        pUnpackedField->c7_l = static_cast<INT16>(IQSettingUtils::RoundFLOAT(
           pData->c_tab.c[2] * (1 << (CC12_Q_FACTOR + qFactor))));
        pUnpackedField->c8_l = static_cast<INT16>(IQSettingUtils::RoundFLOAT(
           pData->c_tab.c[0] * (1 << (CC12_Q_FACTOR + qFactor))));

        pUnpackedField->k0_l = static_cast<INT16>(IQSettingUtils::RoundFLOAT(pData->k_tab.k[1]));
        pUnpackedField->k1_l = static_cast<INT16>(IQSettingUtils::RoundFLOAT(pData->k_tab.k[2]));
        pUnpackedField->k2_l = static_cast<INT16>(IQSettingUtils::RoundFLOAT(pData->k_tab.k[0]));
        pUnpackedField->c0_r = pUnpackedField->c0_l;
        pUnpackedField->c1_r = pUnpackedField->c1_l;
        pUnpackedField->c2_r = pUnpackedField->c2_l;
        pUnpackedField->c3_r = pUnpackedField->c3_l;
        pUnpackedField->c4_r = pUnpackedField->c4_l;
        pUnpackedField->c5_r = pUnpackedField->c5_l;
        pUnpackedField->c6_r = pUnpackedField->c6_l;
        pUnpackedField->c7_r = pUnpackedField->c7_l;
        pUnpackedField->c8_r = pUnpackedField->c8_l;
        pUnpackedField->k0_r = pUnpackedField->k0_l;
        pUnpackedField->k1_r = pUnpackedField->k1_l;
        pUnpackedField->k2_r = pUnpackedField->k2_l;

        pUnpackedField->qfactor_l = static_cast<INT16>(qFactor);
        pUnpackedField->qfactor_r = pUnpackedField->qfactor_l;
    }
    else
    {
        /// @todo (CAMX-1812) Need to add logging for Common library
        result = FALSE;
    }

    return result;
}
