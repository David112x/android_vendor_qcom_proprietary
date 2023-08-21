// NOWHINE NC009 <- Shared file with system team so uses non-CamX file naming
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  dsx10setting.cpp
/// @brief IFE DSX10 setting calculation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#include "NcLibWarp.h"
#include "Process_DSX.h"
#include "dsx10setting.h"
#include "ds4to1_1_1_0.h"

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// DSX10Setting::CalculateHWSetting
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL DSX10Setting::CalculateHWSetting(
    const DSX10InputData*                                 pInput,
    dsx_1_0_0::dsx10_rgn_dataType*                        pData,
    const dsx_1_0_0::chromatix_dsx10_reserveType*         pReserveData,
    const ds4to1_1_1_0::mod_ds4to1v11_pass_reserve_dataType::pass_dataStruct* pDS4to1Data,
    VOID*                                                 pOutput)
{
    BOOL result = TRUE;
    INT32 intRet = 0;
    if ((NULL != pInput) &&
        (NULL != pData) &&
        (NULL != pReserveData) &&
        (NULL != pDS4to1Data) &&
        (NULL != pOutput))
    {
        DSX10UnpackedField*    pUnpackedField = static_cast<DSX10UnpackedField*>(pOutput);
        DSX_Chromatix*         pDSXChromatix = static_cast<DSX_Chromatix*>(pInput->pDSXChromatix);
        DS4to1_Chromatix       DS4to1Chromatix;
        ICA_Chromatix          icaChromatix; // @todo (CAMX-1812): NEEDS TO GET THIS INFO for (param_calc_mode == 0) !!!
        NCLIB_CONTEXT_DSX      ncLibCtxtDSX;
        DSX_REG                DSXRegData;

        DSX_ProcessNcLibBuffer buffer;

        MapChromatix2DSXChromatix(pReserveData,
            pData,
            pDSXChromatix);

        MapChromatix2DS4to1Chromatix(pDS4to1Data, &DS4to1Chromatix);

        ncLibCtxtDSX.inW       = pInput->inW;
        ncLibCtxtDSX.inH       = pInput->inH;
        ncLibCtxtDSX.outW      = pInput->outW;
        ncLibCtxtDSX.outH      = pInput->outH;
        ncLibCtxtDSX.downscaleX = pInput->downscale;
        ncLibCtxtDSX.downscaleY = pInput->downscale;
        ncLibCtxtDSX.offsetX   = pInput->offsetX;
        ncLibCtxtDSX.offsetY   = pInput->offsetY;

        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "ncLibCtxtDSX in [ %d x %d]  out [%d x %d]  DS x, y %f, %f OffsetX x Y %d x %d ",
            ncLibCtxtDSX.inW,
            ncLibCtxtDSX.inH,
            ncLibCtxtDSX.outW,
            ncLibCtxtDSX.outH,
            ncLibCtxtDSX.downscaleX,
            ncLibCtxtDSX.downscaleY,
            ncLibCtxtDSX.offsetX,
            ncLibCtxtDSX.offsetY);


        pDSXChromatix->param_calc_mode = 1;         // @todo (CAMX-1812): REMOVE THIS WHEN ICA DATA IS AVAILABLE!

        intRet = DSX_ProcessNcLib(pDSXChromatix, &DS4to1Chromatix, &icaChromatix, &ncLibCtxtDSX, &buffer, &DSXRegData);

        if (NC_LIB_SUCCESS != intRet)
        {
            result = FALSE;
            CAMX_LOG_ERROR(CamxLogGroupIQMod, "DSX_ProcessNcLib failed %d intRet %d ", result, intRet);
        }
        else
        {
            CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "DSX10Setting::CalculateHWSetting success");
            pUnpackedField->dsxData = DSXRegData;
        }
    }
    else
    {
        CAMX_LOG_ERROR(CamxLogGroupIQMod, "pInput=%p, pData=%p, pOutput=%p", pInput, pData, pOutput);
        result = FALSE;
    }

    return result;
}

VOID DSX10Setting::MapChromatix2DSXChromatix(
    const dsx_1_0_0::chromatix_dsx10_reserveType* pReserveData,
    dsx_1_0_0::dsx10_rgn_dataType*          pUnpackedData,
    DSX_Chromatix*                          pDSXChromatix)
{
    UINT32 i;

    pDSXChromatix->general.luma_padding_weights_en = pUnpackedData->luma_padding_weights_en;
    pDSXChromatix->general.chroma_padding_weights_en = pUnpackedData->chroma_padding_weights_en;

    for (i = 0; i < DSX_LUMA_PADDING_WEIGHTS; i++)
    {
        pDSXChromatix->general.luma_padding_weights_top[i]   =
                IQSettingUtils::RoundFLOAT(pUnpackedData->luma_padding_weights_top_tab.luma_padding_weights_top[i]);
        pDSXChromatix->general.luma_padding_weights_bot[i]   =
                IQSettingUtils::RoundFLOAT(pUnpackedData->luma_padding_weights_bot_tab.luma_padding_weights_bot[i]);
        pDSXChromatix->general.luma_padding_weights_left[i]  =
                IQSettingUtils::RoundFLOAT(pUnpackedData->luma_padding_weights_left_tab.luma_padding_weights_left[i]);
        pDSXChromatix->general.luma_padding_weights_right[i] =
                IQSettingUtils::RoundFLOAT(pUnpackedData->luma_padding_weights_right_tab.luma_padding_weights_right[i]);
    }

    for (i = 0; i < DSX_CHROMA_PADDING_WEIGHTS; i++)
    {
        pDSXChromatix->general.chroma_padding_weights_top[i]   =
                IQSettingUtils::RoundFLOAT(pUnpackedData->chroma_padding_weights_top_tab.chroma_padding_weights_top[i]);
        pDSXChromatix->general.chroma_padding_weights_bot[i]   =
                IQSettingUtils::RoundFLOAT(pUnpackedData->chroma_padding_weights_bot_tab.chroma_padding_weights_bot[i]);
        pDSXChromatix->general.chroma_padding_weights_left[i]  =
                IQSettingUtils::RoundFLOAT(pUnpackedData->chroma_padding_weights_left_tab.chroma_padding_weights_left[i]);
        pDSXChromatix->general.chroma_padding_weights_right[i] =
                IQSettingUtils::RoundFLOAT(pUnpackedData->chroma_padding_weights_right_tab.chroma_padding_weights_right[i]);
    }

    pDSXChromatix->param_calc_mode = pReserveData->param_calc_mode;

    for (i = 0; i < DSX_LUMA_KERNAL_WEIGHTS; i++)
    {
        pDSXChromatix->luma_kernel_weights_unpacked_horiz[i] = IQSettingUtils::RoundFLOAT(
            pUnpackedData->luma_kernel_weights_unpacked_horiz_tab.luma_kernel_weights_unpacked_horiz[i]);
        pDSXChromatix->luma_kernel_weights_unpacked_vert[i]  = IQSettingUtils::RoundFLOAT(
            pUnpackedData->luma_kernel_weights_unpacked_vert_tab.luma_kernel_weights_unpacked_vert[i]);
    }

    for (i = 0; i < DSX_CHROMA_KERNAL_WEIGHTS; i++)
    {
        pDSXChromatix->chroma_kernel_weights_unpacked_horiz[i] = IQSettingUtils::RoundFLOAT(
            pUnpackedData->chroma_kernel_weights_unpacked_horiz_tab.chroma_kernel_weights_unpacked_horiz[i]);
        pDSXChromatix->chroma_kernel_weights_unpacked_vert[i]  = IQSettingUtils::RoundFLOAT(
            pUnpackedData->chroma_kernel_weights_unpacked_vert_tab.chroma_kernel_weights_unpacked_vert[i]);
    }

    return;
}

VOID DSX10Setting::MapChromatix2DS4to1Chromatix(
    const ds4to1_1_1_0::mod_ds4to1v11_pass_reserve_dataType::pass_dataStruct* pDS4to1Data,
    DS4to1_Chromatix*          pDSXChromatix)
{
    pDSXChromatix->general.coeff_07 = pDS4to1Data->coeff_07;
    pDSXChromatix->general.coeff_16 = pDS4to1Data->coeff_16;
    pDSXChromatix->general.coeff_25 = pDS4to1Data->coeff_25;

    return;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// DSX10Setting::GetInitializationData
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL DSX10Setting::GetInitializationData(
    struct DSXNcLibOutputData* pData)
{
    pData->DSX10ChromatixSize  = (sizeof(DSX_Chromatix));
    pData->DSX10OutputDataSize = (sizeof(DSX10UnpackedField));

    return TRUE;
}
