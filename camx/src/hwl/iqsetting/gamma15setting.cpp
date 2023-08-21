// NOWHINE NC009 <- Shared file with system team so uses non-CamX file naming
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2017-2018 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  gamma15setting.cpp
/// @brief gamma15 setting calculation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "gamma15setting.h"

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Gamma15Setting::CalculateHWSetting
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL Gamma15Setting::CalculateHWSetting(
    const Gamma15InputData*                                   pInput,
    gamma_1_5_0::mod_gamma15_cct_dataType::cct_dataStruct*    pData,
    gamma_1_5_0::chromatix_gamma15Type::enable_sectionStruct* pModuleEnable,
    VOID*                                                     pOutput)
{
    BOOL result = TRUE;

    if ((NULL != pInput)        &&
        (NULL != pData)         &&
        (NULL != pModuleEnable) &&
        (NULL != pOutput))
    {
        Gamma15UnpackedField* pUnpackedField = static_cast<Gamma15UnpackedField*>(pOutput);

        pUnpackedField->enable = static_cast<UINT16>(pModuleEnable->gamma_enable);

        if (pInput->contrastLevel == Gamma15DefaultContrastLevel)
        {
            ApplyManualContrastToGammaCurve(pInput->contrastLevel,
                pData->mod_gamma15_channel_data[GammaLUTChannel0].gamma15_rgn_data.table);
            ApplyManualContrastToGammaCurve(pInput->contrastLevel,
                pData->mod_gamma15_channel_data[GammaLUTChannel1].gamma15_rgn_data.table);
            ApplyManualContrastToGammaCurve(pInput->contrastLevel,
                pData->mod_gamma15_channel_data[GammaLUTChannel2].gamma15_rgn_data.table);
        }

        GenerateGammaLUT(pData->mod_gamma15_channel_data[GammaLUTChannel0].gamma15_rgn_data.table,
                         pUnpackedField->gLUTInConfig.pGammaTable);
        GenerateGammaLUT(pData->mod_gamma15_channel_data[GammaLUTChannel1].gamma15_rgn_data.table,
                         pUnpackedField->bLUTInConfig.pGammaTable);
        GenerateGammaLUT(pData->mod_gamma15_channel_data[GammaLUTChannel2].gamma15_rgn_data.table,
                         pUnpackedField->rLUTInConfig.pGammaTable);
    }
    else
    {
        /// @todo (CAMX-1812) Need to add logging for Common library
        result = FALSE;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Gamma15Setting::GenerateGammaLUT
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID Gamma15Setting::GenerateGammaLUT(
    FLOAT*  pGammaTable,
    UINT32* pLUT)
{
    UINT16  x;
    UINT16  y;
    INT32   deltaLUT;
    UINT32  index;
    UINT32  tmp;

    const INT32  maxDeltaEntry = MAX_INT10;
    const INT32  minDeltaEntry = MIN_INT10;
    const UINT16 mask          = NMAX10 - 1;

    for (index = 0; index < Gamma15LUTNumEntriesPerChannel; index++)
    {
        tmp = static_cast<UINT32>(IQSettingUtils::RoundFLOAT(pGammaTable[index]));
        x   = static_cast<UINT16>(IQSettingUtils::ClampUINT32(tmp, 0, (NMAX10 - 1)));
        tmp = static_cast<UINT32>(IQSettingUtils::RoundFLOAT(pGammaTable[index +1]));
        if (index < Gamma15LUTNumEntriesPerChannel - 1)
        {
            y = static_cast<UINT16>(IQSettingUtils::ClampUINT32(tmp, 0, (NMAX10 - 1)));
        }
        else
        {
            y = static_cast<UINT16>(IQSettingUtils::ClampUINT32(tmp, 0, NMAX10));
        }

        deltaLUT    = y - x;
        deltaLUT    = IQSettingUtils::ClampINT32(deltaLUT, minDeltaEntry, maxDeltaEntry);
        deltaLUT    = (deltaLUT & mask);  ///< this target has packing format = "10bit delta | 10bit entry"
        pLUT[index] = (x) | (deltaLUT << 10);
    }

    return;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Gamma15Setting::ApplyManualContrastToGammaCurve
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID Gamma15Setting::ApplyManualContrastToGammaCurve(
    const UINT8 contrastLevel,
    FLOAT*      pGammaTable)
{
    CAMX_LOG_VERBOSE(CamxLogGroupPProc, "contrastLevel = %d", contrastLevel);

    if (contrastLevel != 5)
    {
        FLOAT sigmoid = 0.0f;
        switch (contrastLevel)
        {
            case 10:
                sigmoid = 2.4f;
                break;
            case 9:
                sigmoid = 2.1f;
                break;
            case 8:
                sigmoid = 1.8f;
                break;
            case 7:
                sigmoid = 1.5f;
                break;
            case 6:
                sigmoid = 1.2f;
                break;
            case 4:
                sigmoid = 0.9f;
                break;
            case 3:
                sigmoid = 0.8f;
                break;
            case 2:
                sigmoid = 0.7f;
                break;
            case 1:
                sigmoid = 0.6f;
                break;
            case 0:
                sigmoid = 0.5f;
                break;
            default:
                CAMX_LOG_WARN(CamxLogGroupPProc, "Wrong Manual Contrast Level %d", contrastLevel);
                break;
        }

        if (contrastLevel <= 10)
        {
            DOUBLE mid_pt     = pow(2, 9);
            DOUBLE multiplier = pow(mid_pt, 1.0 - sigmoid);
            UINT32 k;

            for (k = 0; k < Gamma15LUTNumEntriesPerChannel; k++)
            {
                if (pGammaTable[k] < mid_pt)
                {
                    pGammaTable[k] = static_cast<FLOAT>(multiplier * pow(pGammaTable[k], sigmoid));
                }
                else
                {
                    pGammaTable[k] = static_cast<FLOAT>(2 * mid_pt - (multiplier * pow(2 * mid_pt - pGammaTable[k], sigmoid)));
                }
            }
        }
    }
}