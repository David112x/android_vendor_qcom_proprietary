// NOWHINE NC009 <- Shared file with system team so uses non-CamX file naming
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2018 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  upscale12setting.cpp
/// @brief IPE Upscale12 setting calculation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "upscale12setting.h"

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Upscale12Setting::CalculateHWSetting
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL Upscale12Setting::CalculateHWSetting(
    const Upscale12InputData*                                     pInput,
    VOID*                                                         pOutput)
{
    BOOL result = FALSE;

    if ((NULL != pInput)        &&
        (NULL != pOutput))
    {
        Upscale12UnpackedField* pUnpackedField = static_cast<Upscale12UnpackedField*>(pOutput);
        CAMX_ASSERT(NULL != pUnpackedField);

        pUnpackedField->lumaVScaleFirAlgorithm      = pInput->lumaVScaleFirAlgorithm;
        pUnpackedField->lumaHScaleFirAlgorithm      = pInput->lumaHScaleFirAlgorithm;
        pUnpackedField->lumaInputDitheringDisable   = pInput->lumaInputDitheringDisable;
        pUnpackedField->lumaInputDitheringMode      = pInput->lumaInputDitheringMode;
        pUnpackedField->chromaVScaleFirAlgorithm    = pInput->chromaVScaleFirAlgorithm;
        pUnpackedField->chromaHScaleFirAlgorithm    = pInput->chromaHScaleFirAlgorithm;
        pUnpackedField->chromaInputDitheringDisable = pInput->chromaInputDitheringDisable;
        pUnpackedField->chromaInputDitheringMode    = pInput->chromaInputDitheringMode;
        pUnpackedField->chromaRoundingModeV         = pInput->chromaRoundingModeV;
        pUnpackedField->chromaRoundingModeH         = pInput->chromaRoundingModeH;
        result                                      = TRUE;
    }

    if (FALSE == result)
    {
        CAMX_LOG_ERROR(CamxLogGroupISP, "Upscale12 pInput/pOutput is NULL");
    }

    return result;
}
