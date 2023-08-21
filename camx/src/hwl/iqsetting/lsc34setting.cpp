// NOWHINE ENTIRE FILE
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2017-2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// @file  lsc34setting.cpp
// @brief lsc34 setting calculation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "lsc34setting.h"

/// @todo (CAMX-833) Update ISP input data from the port
// referance from camxifenode.cpp

INT32 LSC34Setting::s_meshHorizontal = 1;
INT32 LSC34Setting::s_meshVertical   = 1;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// LSC34Setting::CalculateTintlessSetting
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL LSC34Setting::CalculateTintlessSetting(
    LSC34InputData*                          pInput,
    lsc_3_4_0::lsc34_rgn_dataType*           pData,
    tintless_2_0_0::tintless20_rgn_dataType* pTintlessData,
    lsc_3_4_0::lsc34_rgn_dataType*           pOutput,
    UINT32                                   subGridWidth,
    UINT32                                   subGridHeight,
    UINT32                                   interpolateFactor,
    UINT32                                   meshHorizontalOffset,
    UINT32                                   meshVerticalOffset)
{
    CDKResult cdkResult = CDKResultEFailed;
    BOOL      result    = FALSE;

    if (NULL != pInput                     &&
        NULL != pData                      &&
        NULL != pOutput                    &&
        NULL != pInput->pTintlessConfig    &&
        NULL != pInput->pTintlessChromatix &&
        NULL != pInput->pTintlessStats     &&
        NULL != pInput->pTintlessAlgo)
    {
        TintlessRolloffTable rollOffTableInput;
        TintlessRolloffTable rollOffTableOutput;

        pInput->pTintlessConfig->rolloffConfig.LSCSubgridHeight           = subGridHeight;
        pInput->pTintlessConfig->rolloffConfig.LSCSubgridWidth            = subGridWidth;
        pInput->pTintlessConfig->rolloffConfig.LSCSubgridHorizontalOffset = meshHorizontalOffset;
        pInput->pTintlessConfig->rolloffConfig.LSCSubgridVerticalOffset   = meshVerticalOffset;
        pInput->pTintlessConfig->rolloffConfig.LSCTableHeight             = ROLLOFF_MESH_PT_V_V34;
        pInput->pTintlessConfig->rolloffConfig.LSCTableWidth              = ROLLOFF_MESH_PT_H_V34;
        pInput->pTintlessConfig->rolloffConfig.numberOfLSCSubgrids        = (1 << interpolateFactor);

        pInput->pTintlessConfig->tintlessParamConfig.centerWeight               = pTintlessData->center_weight;
        pInput->pTintlessConfig->tintlessParamConfig.cornerWeight               = pTintlessData->corner_weight;
        pInput->pTintlessConfig->tintlessParamConfig.tintAcuuracy               =
            static_cast<UINT8>(pTintlessData->tintless_high_accuracy_mode);
        pInput->pTintlessConfig->tintlessParamConfig.tracePercentage            = pTintlessData->tintless_trace_percentage;
        pInput->pTintlessConfig->tintlessParamConfig.updateDelay                =
            static_cast<UINT8>(pTintlessData->tintless_update_delay);
        pInput->pTintlessConfig->tintlessParamConfig.tintlessCorrectionStrength =
            static_cast<UINT8>(pTintlessData->tintless_threshold_tab.tintless_threshold[0]);


        for (UINT index = 0; index < 16; index++)
        {
            pInput->pTintlessConfig->tintlessParamConfig.tintlessThreshold[index] =
                static_cast<UINT8>(IQSettingUtils::RoundFLOAT(pTintlessData->tintless_threshold_tab.tintless_threshold[index]));
        }

        rollOffTableInput.TintlessRoloffTableSize  = ROLLOFF_SIZE;
        rollOffTableOutput.TintlessRoloffTableSize = ROLLOFF_SIZE;

        rollOffTableInput.RGain   = pData->r_gain_tab.r_gain;
        rollOffTableInput.BGain   = pData->b_gain_tab.b_gain;
        rollOffTableInput.GRGain  = pData->gr_gain_tab.gr_gain;
        rollOffTableInput.GBGain  = pData->gb_gain_tab.gb_gain;

        rollOffTableOutput.RGain  = pOutput->r_gain_tab.r_gain;
        rollOffTableOutput.BGain  = pOutput->b_gain_tab.b_gain;
        rollOffTableOutput.GRGain = pOutput->gr_gain_tab.gr_gain;
        rollOffTableOutput.GBGain = pOutput->gb_gain_tab.gb_gain;

        if ((pInput->pTintlessStats->GetChannelGainThreshold(ChannelIndexR)  == 0) ||
            (pInput->pTintlessStats->GetChannelGainThreshold(ChannelIndexB)  == 0) ||
            (pInput->pTintlessStats->GetChannelGainThreshold(ChannelIndexGR) == 0) ||
            (pInput->pTintlessStats->GetChannelGainThreshold(ChannelIndexGB) == 0))
        {
            CAMX_LOG_WARN(CamxLogGroupISP, "Prevent saturationLimit (%d, %d, %d, %d) from reaching negative",
                          pInput->pTintlessStats->GetChannelGainThreshold(ChannelIndexR),
                          pInput->pTintlessStats->GetChannelGainThreshold(ChannelIndexB),
                          pInput->pTintlessStats->GetChannelGainThreshold(ChannelIndexGR),
                          pInput->pTintlessStats->GetChannelGainThreshold(ChannelIndexGB));
        }

        // Convert Stats to 8Bit for Tintless Algo Optimization
        pInput->pTintlessConfig->statsConfig.saturationLimit[ChannelIndexR]  =
            (pInput->pTintlessStats->GetChannelGainThreshold(ChannelIndexR) >= 1) ?
             pInput->pTintlessStats->GetChannelGainThreshold(ChannelIndexR) - 1 : 0;
        pInput->pTintlessConfig->statsConfig.saturationLimit[ChannelIndexB]  =
            (pInput->pTintlessStats->GetChannelGainThreshold(ChannelIndexB) >= 1) ?
             pInput->pTintlessStats->GetChannelGainThreshold(ChannelIndexB) - 1 : 0;
        pInput->pTintlessConfig->statsConfig.saturationLimit[ChannelIndexGR] =
            (pInput->pTintlessStats->GetChannelGainThreshold(ChannelIndexGR) >= 1) ?
             pInput->pTintlessStats->GetChannelGainThreshold(ChannelIndexGR) - 1 : 0;
        pInput->pTintlessConfig->statsConfig.saturationLimit[ChannelIndexGB] =
            (pInput->pTintlessStats->GetChannelGainThreshold(ChannelIndexGB) >= 1) ?
             pInput->pTintlessStats->GetChannelGainThreshold(ChannelIndexGB) - 1 : 0;

        CAMX_LOG_VERBOSE(CamxLogGroupISP, "saturationLimit %d, %d, %d, %d.",
                         pInput->pTintlessConfig->statsConfig.saturationLimit[ChannelIndexR],
                         pInput->pTintlessConfig->statsConfig.saturationLimit[ChannelIndexB],
                         pInput->pTintlessConfig->statsConfig.saturationLimit[ChannelIndexGR],
                         pInput->pTintlessConfig->statsConfig.saturationLimit[ChannelIndexGB]);

        cdkResult = pInput->pTintlessAlgo->TintlessAlgorithmProcess(pInput->pTintlessAlgo,
                                                                    pInput->pTintlessConfig,
                                                                    pInput->pTintlessStats,
                                                                    &rollOffTableInput,
                                                                    &rollOffTableOutput);

        CAMX_LOG_VERBOSE(CamxLogGroupISP, "Tintless algo result %d", cdkResult);

        if (CDKResultSuccess == cdkResult)
        {
            result = TRUE;
        }
    }
    else
    {
        CAMX_LOG_INFO(CamxLogGroupISP, "input:%p pData:%p  pOutput:%p Config:%p Chromatix:%p Stats:%p Algo:%p",
                       pInput, pData , pOutput,
                       NULL != pInput ? pInput->pTintlessConfig : NULL,
                       NULL != pInput ? pInput->pTintlessChromatix : NULL,
                       NULL != pInput ? pInput->pTintlessStats : NULL,
                       NULL != pInput ? pInput->pTintlessAlgo : NULL)
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// LSC34Setting::CalculateHWSetting
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL LSC34Setting::CalculateHWSetting(
    LSC34InputData*                                       pInput,
    lsc_3_4_0::lsc34_rgn_dataType*                        pData,
    lsc_3_4_0::chromatix_lsc34Type::enable_sectionStruct* pModuleEnable,
    tintless_2_0_0::tintless20_rgn_dataType*              pTintlessData,
    VOID*                                                 pOutput)
{
    BOOL    result = TRUE;
    INT32   vCount;
    INT32   hCount;
    INT32   index;
    INT32   meshH;
    INT32   meshV;
    INT32   tempScale;
    INT32   tempDH;
    INT32   tempDV;
    INT32   tempSGH;
    INT32   tempSGV;
    UINT16  value;
    UINT32  streamInWidth;
    UINT32  streamInHeight;
    INT32   xStart;
    INT32   yStart;
    INT32   sGx;
    INT32   sGy;
    UINT32  horizontalOffset = 0;
    UINT32  verticalOffset   = 0;
    UINT32  meshGridWidth    = 0;
    UINT32  meshGridHeight   = 0;
    FLOAT*  RGain            = NULL;
    FLOAT*  BGain            = NULL;
    FLOAT*  GBGain           = NULL;
    FLOAT*  GRGain           = NULL;
    BOOL    tintlessResult   = FALSE;

    lsc_3_4_0::lsc34_rgn_dataType tintlessOuput;

    if ((NULL != pInput)        &&
        (NULL != pData)         &&
        (NULL != pModuleEnable) &&
        (NULL != pOutput))
    {
        LSC34UnpackedField* pUnpackedField = static_cast<LSC34UnpackedField*>(pOutput);

        streamInWidth                  = pInput->imageWidth;
        streamInHeight                 = pInput->imageHeight;
        pUnpackedField->enable         = static_cast<UINT16>(pModuleEnable->rolloff_enable);
        pUnpackedField->enable_3d      = 0;
        pUnpackedField->pixel_offset   = 0;
        pUnpackedField->left_img_wd    = static_cast<UINT16>(pInput->imageWidth /2);
        pUnpackedField->bank_sel       = pInput->bankSelect;

        scaleRolloffTable(pInput, pData);
        meshH = s_meshHorizontal + 1;
        meshV = s_meshVertical + 1;

        InterpGridOptimization(streamInWidth,
            streamInHeight,
            &tempScale,
            &tempDH,
            &tempDV,
            &tempSGH,
            &tempSGV,
            &meshH,
            &meshV);

        pUnpackedField->num_meshgain_h = meshH - 1;
        pUnpackedField->num_meshgain_v = meshV - 1;

        pUnpackedField->intp_factor_l = static_cast<UINT32>((log(static_cast<FLOAT>(tempScale)) / log(2.0) + 0.5f));
        pUnpackedField->intp_factor_r = static_cast<UINT32>((log(static_cast<FLOAT>(tempScale)) / log(2.0) + 0.5f));

        pUnpackedField->bwidth_l = IQSettingUtils::ClampUINT32(tempSGH - 1,
                                                               LSC34_BWIDTH_MIN,
                                                               LSC34_BWIDTH_MAX);  // Bayer Bw is SG width, 10u
        pUnpackedField->bwidth_r = IQSettingUtils::ClampUINT32(tempSGH - 1,
                                                               LSC34_BWIDTH_MIN,
                                                               LSC34_BWIDTH_MAX);  // Bayer Bw is SG width, 10u
        pUnpackedField->bheight_l = IQSettingUtils::ClampUINT32(tempSGV - 1,
                                                                LSC34_BHEIGHT_MIN,
                                                                LSC34_BHEIGHT_MAX);  // Bayer Bh is SG height, 10u
        pUnpackedField->bheight_r = IQSettingUtils::ClampUINT32(tempSGV - 1,
                                                                LSC34_BHEIGHT_MIN,
                                                                LSC34_BHEIGHT_MAX);  // Bayer Bh is SG height, 10u
        pUnpackedField->meshGridBwidth_l = IQSettingUtils::ClampUINT32(tempSGH*tempScale - 1,
                                                                       LSC34_MESHGRIDBWIDTH_MIN,
                                                                       LSC34_MESHGRIDBWIDTH_MAX);  // Bayer block width, 10u
        pUnpackedField->meshGridBwidth_r = IQSettingUtils::ClampUINT32(tempSGH*tempScale - 1,
                                                                       LSC34_MESHGRIDBWIDTH_MIN,
                                                                       LSC34_MESHGRIDBWIDTH_MAX);  // Bayer block width, 10u
        pUnpackedField->meshGridBheight_l = IQSettingUtils::ClampUINT32(tempSGV*tempScale - 1,
                                                                        LSC34_MESHGRIDBHEIGHT_MIN,
                                                                        LSC34_MESHGRIDBHEIGHT_MAX);  // Bayer block height
        pUnpackedField->meshGridBheight_r = IQSettingUtils::ClampUINT32(tempSGV*tempScale - 1,
                                                                        LSC34_MESHGRIDBHEIGHT_MIN,
                                                                        LSC34_MESHGRIDBHEIGHT_MAX);  // Bayer block height

        pUnpackedField->x_delta_l = (1 << LSC34_DELTA_Q_FACTOR) / (pUnpackedField->bwidth_l + 1);   // 17uQ20
        pUnpackedField->x_delta_r = (1 << LSC34_DELTA_Q_FACTOR) / (pUnpackedField->bwidth_r + 1);   // 17uQ20
        pUnpackedField->y_delta_l = (1 << LSC34_DELTA_Q_FACTOR) / (pUnpackedField->bheight_l + 1);  // 17uQ20
        pUnpackedField->y_delta_r = (1 << LSC34_DELTA_Q_FACTOR) / (pUnpackedField->bheight_r + 1);  // 17uQ20

        pUnpackedField->x_delta_l = IQSettingUtils::ClampUINT32(pUnpackedField->x_delta_l,
                                                                LSC34_INV_SUBBLOCK_WIDTH_MIN,
                                                                LSC34_INV_SUBBLOCK_WIDTH_MAX);
        pUnpackedField->x_delta_r = IQSettingUtils::ClampUINT32(pUnpackedField->x_delta_r,
                                                                LSC34_INV_SUBBLOCK_WIDTH_MIN,
                                                                LSC34_INV_SUBBLOCK_WIDTH_MAX);
        pUnpackedField->y_delta_l = IQSettingUtils::ClampUINT32(pUnpackedField->y_delta_l,
                                                                LSC34_INV_SUBBLOCK_HEIGHT_MIN,
                                                                LSC34_INV_SUBBLOCK_HEIGHT_MAX);
        pUnpackedField->y_delta_r = IQSettingUtils::ClampUINT32(pUnpackedField->y_delta_r,
                                                                LSC34_INV_SUBBLOCK_HEIGHT_MIN,
                                                                LSC34_INV_SUBBLOCK_HEIGHT_MAX);

        xStart = tempDH * 2;
        yStart = tempDV * 2;
        sGx = (xStart >> 1) / (pUnpackedField->bwidth_l + 1);   // Starting SG x index
        sGy = (yStart >> 1) / (pUnpackedField->bheight_l + 1);  // Starting SG y index

        pUnpackedField->lx_start_l = IQSettingUtils::ClampUINT32(IQSettingUtils::RoundFLOAT(
                                                                 static_cast<FLOAT>(sGx) / static_cast<FLOAT>(tempScale)),
                                                                 LSC34_MIN_BLOCK_X,
                                                                 LSC34_MAX_BLOCK_X); // 6u
        pUnpackedField->lx_start_r = IQSettingUtils::ClampUINT32(IQSettingUtils::RoundFLOAT(
                                                                 static_cast<FLOAT>(sGx) / static_cast<FLOAT>(tempScale)),
                                                                 LSC34_MIN_BLOCK_X,
                                                                 LSC34_MAX_BLOCK_X); // 6u
        pUnpackedField->ly_start_l = IQSettingUtils::ClampUINT32(IQSettingUtils::RoundFLOAT(
                                                                 static_cast<FLOAT>(sGy) / static_cast<FLOAT>(tempScale)),
                                                                 LSC34_MIN_BLOCK_Y,
                                                                 LSC34_MAX_BLOCK_Y); // 6u
        pUnpackedField->ly_start_r = IQSettingUtils::ClampUINT32(IQSettingUtils::RoundFLOAT(
                                                                 static_cast<FLOAT>(sGy) / static_cast<FLOAT>(tempScale)),
                                                                 LSC34_MIN_BLOCK_Y,
                                                                 LSC34_MAX_BLOCK_Y); // 6u

        pUnpackedField->bx_start_l = IQSettingUtils::ClampUINT32(sGx & (tempScale - 1),
                                                                 LSC34_MIN_SUBGRID_X,
                                                                 LSC34_MAX_SUBGRID_X); // 3u
        pUnpackedField->bx_start_r = IQSettingUtils::ClampUINT32(sGx & (tempScale - 1),
                                                                 LSC34_MIN_SUBGRID_X,
                                                                 LSC34_MAX_SUBGRID_X); // 3u
        pUnpackedField->by_start_l = IQSettingUtils::ClampUINT32(sGy & (tempScale - 1),
                                                                 LSC34_MIN_SUBGRID_Y,
                                                                 LSC34_MAX_SUBGRID_Y); // 3u
        pUnpackedField->by_start_r = IQSettingUtils::ClampUINT32(sGy & (tempScale - 1),
                                                                 LSC34_MIN_SUBGRID_Y,
                                                                 LSC34_MAX_SUBGRID_Y); // 3u
        pUnpackedField->bx_d1_l = IQSettingUtils::ClampUINT32((xStart >> 1) % (pUnpackedField->bwidth_l + 1),
                                                              LSC34_MIN_PIXEL_INDEX_X,
                                                              LSC34_MAX_PIXEL_INDEX_X);  // 11u
        pUnpackedField->bx_d1_r = IQSettingUtils::ClampUINT32((xStart >> 1) % (pUnpackedField->bwidth_r + 1),
                                                              LSC34_MIN_PIXEL_INDEX_X,
                                                              LSC34_MAX_PIXEL_INDEX_X);  // 11u
        pUnpackedField->by_e1_l = IQSettingUtils::ClampUINT32((yStart >> 1) % (pUnpackedField->bheight_l + 1),
                                                              LSC34_MIN_PIXEL_INDEX_Y,
                                                              LSC34_MAX_PIXEL_INDEX_Y);  // 10u
        pUnpackedField->by_e1_r = IQSettingUtils::ClampUINT32((yStart >> 1) % (pUnpackedField->bheight_r + 1),
                                                              LSC34_MIN_PIXEL_INDEX_Y,
                                                              LSC34_MAX_PIXEL_INDEX_Y);  // 10u
        pUnpackedField->by_init_e1_l = IQSettingUtils::ClampUINT32(pUnpackedField->by_e1_l * pUnpackedField->y_delta_l,
                                                                   LSC34_MIN_E_INIT,
                                                                   LSC34_MAX_E_INIT); // Q20
        pUnpackedField->by_init_e1_r = IQSettingUtils::ClampUINT32(pUnpackedField->by_e1_r * pUnpackedField->y_delta_r,
                                                                   LSC34_MIN_E_INIT,
                                                                   LSC34_MAX_E_INIT); // Q20

        horizontalOffset = (pUnpackedField->lx_start_l * 2 * (pUnpackedField->meshGridBwidth_l + 1)) +
            (pUnpackedField->bx_start_l * 2 * (pUnpackedField->bwidth_l + 1)) + (pUnpackedField->bx_d1_l * 2);
        verticalOffset   = (pUnpackedField->ly_start_l * 2 * (pUnpackedField->meshGridBheight_l + 1)) +
            (pUnpackedField->by_start_l * 2 * (pUnpackedField->bheight_l + 1)) + (pUnpackedField->by_e1_l * 2);
        meshGridHeight = (pUnpackedField->bheight_l + 1) * 2;
        meshGridWidth  = (pUnpackedField->bwidth_r + 1) * 2;

        if((NULL != pTintlessData) && (TRUE == pInput->enableTintless))
        {
            tintlessResult = CalculateTintlessSetting(pInput,
                                                      pData,
                                                      pTintlessData,
                                                      &tintlessOuput,
                                                      meshGridWidth,
                                                      meshGridHeight,
                                                      pUnpackedField->intp_factor_l,
                                                      horizontalOffset,
                                                      verticalOffset);
        }

        if ((TRUE == pInput->enableTintless) && (TRUE == tintlessResult))
        {
            if (NULL != pInput->pTintlessRatio)
            {
                if ((NULL != pInput->pTintlessRatio->pRGain)  &&
                    (NULL != pInput->pTintlessRatio->pBGain)  &&
                    (NULL != pInput->pTintlessRatio->pGbGain) &&
                    (NULL != pInput->pTintlessRatio->pGrGain))
                {
                    for (UINT32 k = 0; k < (ROLLOFF_MESH_PT_V_V34 * ROLLOFF_MESH_PT_H_V34); k++)
                    {
                        pInput->pTintlessRatio->pRGain[k]  = tintlessOuput.r_gain_tab.r_gain[k]   / pData->r_gain_tab.r_gain[k];
                        pInput->pTintlessRatio->pBGain[k]  = tintlessOuput.b_gain_tab.b_gain[k]   / pData->b_gain_tab.b_gain[k];
                        pInput->pTintlessRatio->pGrGain[k] = tintlessOuput.gr_gain_tab.gr_gain[k] / pData->gr_gain_tab.gr_gain[k];
                        pInput->pTintlessRatio->pGbGain[k] = tintlessOuput.gb_gain_tab.gb_gain[k] / pData->gb_gain_tab.gb_gain[k];
                    }
                    pInput->pTintlessRatio->isValid = TRUE;
                }
            }
        }

        if ((FALSE == pInput->enableTintless) && (TRUE == pInput->pTintlessRatio->isValid))
        {
            for (UINT32 k = 0; k < (ROLLOFF_MESH_PT_V_V34 * ROLLOFF_MESH_PT_H_V34); k++)
            {
                tintlessOuput.r_gain_tab.r_gain[k]   = pInput->pTintlessRatio->pRGain[k]  * pData->r_gain_tab.r_gain[k];
                tintlessOuput.b_gain_tab.b_gain[k]   = pInput->pTintlessRatio->pBGain[k]  * pData->b_gain_tab.b_gain[k];
                tintlessOuput.gr_gain_tab.gr_gain[k] = pInput->pTintlessRatio->pGrGain[k] * pData->gr_gain_tab.gr_gain[k];
                tintlessOuput.gb_gain_tab.gb_gain[k] = pInput->pTintlessRatio->pGbGain[k] * pData->gb_gain_tab.gb_gain[k];
            }

            RGain  = tintlessOuput.r_gain_tab.r_gain;
            BGain  = tintlessOuput.b_gain_tab.b_gain;
            GRGain = tintlessOuput.gr_gain_tab.gr_gain;
            GBGain = tintlessOuput.gb_gain_tab.gb_gain;
        }
        else if (FALSE == tintlessResult)
        {
            RGain  = pData->r_gain_tab.r_gain;
            BGain  = pData->b_gain_tab.b_gain;
            GRGain = pData->gr_gain_tab.gr_gain;
            GBGain = pData->gb_gain_tab.gb_gain;
        }
        else
        {
            RGain  = tintlessOuput.r_gain_tab.r_gain;
            BGain  = tintlessOuput.b_gain_tab.b_gain;
            GRGain = tintlessOuput.gr_gain_tab.gr_gain;
            GBGain = tintlessOuput.gb_gain_tab.gb_gain;
        }

        memset(pUnpackedField->mesh_table_l, 0, sizeof(pUnpackedField->mesh_table_l));
        memset(pUnpackedField->mesh_table_r, 0, sizeof(pUnpackedField->mesh_table_r));

        meshH = s_meshHorizontal + 1;
        meshV = s_meshVertical + 1;

        for (vCount = 0; vCount < meshV; vCount++)
        {
            for (hCount = 0; hCount < meshH; hCount++)
            {
                index = vCount*meshH + hCount;

                value = static_cast<UINT16>(
                    IQSettingUtils::ClampUINT32(IQSettingUtils::FloatToQNumber(RGain[index],
                                                LSC34_Q_FACTOR),
                                                LSC34_GAIN_MIN,
                                                LSC34_GAIN_MAX)); // 13uQ10;

                pUnpackedField->mesh_table_l[pUnpackedField->bank_sel][0][vCount][hCount] = value;
                pUnpackedField->mesh_table_r[pUnpackedField->bank_sel][0][vCount][hCount] = value;

                value = static_cast<UINT16>(
                    IQSettingUtils::ClampUINT32(IQSettingUtils::FloatToQNumber(GRGain[index],
                                                LSC34_Q_FACTOR),
                                                LSC34_GAIN_MIN,
                                                LSC34_GAIN_MAX)); // 13uQ10

                pUnpackedField->mesh_table_l[pUnpackedField->bank_sel][1][vCount][hCount] = value;
                pUnpackedField->mesh_table_r[pUnpackedField->bank_sel][1][vCount][hCount] = value;

                value = static_cast<UINT16>(
                    IQSettingUtils::ClampUINT32(IQSettingUtils::FloatToQNumber(GBGain[index],
                                                LSC34_Q_FACTOR),
                                                LSC34_GAIN_MIN,
                                                LSC34_GAIN_MAX)); // 13uQ10

                pUnpackedField->mesh_table_l[pUnpackedField->bank_sel][2][vCount][hCount] = value;
                pUnpackedField->mesh_table_r[pUnpackedField->bank_sel][2][vCount][hCount] = value;

                value = static_cast<UINT16>(
                    IQSettingUtils::ClampUINT32(IQSettingUtils::FloatToQNumber(BGain[index],
                                                LSC34_Q_FACTOR),
                                                LSC34_GAIN_MIN,
                                                LSC34_GAIN_MAX)); // 13uQ10

                pUnpackedField->mesh_table_l[pUnpackedField->bank_sel][3][vCount][hCount] = value;
                pUnpackedField->mesh_table_r[pUnpackedField->bank_sel][3][vCount][hCount] = value;
            }
        }
    }
    else
    {
        /// @todo (CAMX-1812) Need to add logging for Common library
        result = FALSE;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// LSC34Setting::MeshRolloffScaleRolloff
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID LSC34Setting::MeshRolloffScaleRolloff(
    const FLOAT* pMeshIn,
    FLOAT*       pMeshOut,
    INT          fullWidth,
    INT          fullHeight,
    INT          outputWidth,
    INT          outputHeight,
    INT          offsetX,
    INT          offsetY,
    INT          scaleX,
    INT          scaleY,
    INT*         pMeshH,
    INT*         pMeshV)
{
    FLOAT cxm;
    FLOAT cx0;   ///< Coefficint 0 in x direction for bicubic interpolation
    FLOAT cx1;   ///< Coefficint 1 in x direction for bicubic interpolation
    FLOAT cx2;   ///< Coefficint 2 in x direction for bicubic interpolation
    FLOAT cym;
    FLOAT cy0;   ///< Coefficint 0 in x direction for bicubic interpolation
    FLOAT cy1;   ///< Coefficint 1 in Y direction for bicubic interpolation
    FLOAT cy2;   ///< Coefficint 2 in Y direction for bicubic interpolation
    FLOAT am;
    FLOAT a0;
    FLOAT a1;
    FLOAT a2;
    FLOAT bm;
    FLOAT b0;
    FLOAT b1;
    FLOAT b2;
    FLOAT tx;
    FLOAT ty;
    INT   ix;
    INT   iy;
    INT   vMeshNum;
    INT   hMeshNum;
    INT   scale;
    INT   channelWidth;
    INT   channelHeight;
    INT   sgh;
    INT   sgv;
    INT   gh;
    INT   gv;
    INT   dh;
    INT   dv;
    INT   gh_up;
    INT   gv_up;
    INT   dh_up;
    INT   dv_up;
    FLOAT gh_full;
    FLOAT gv_full;
    INT   meshHMax;
    INT   meshVMax;
    FLOAT extendMesh[(ROLLOFF_MESH_H_V34 + 3)*(ROLLOFF_MESH_V_V34 + 3)];

    InterpGridOptimization(outputWidth, outputHeight, &scale, &dh, &dv, &sgh, &sgv, pMeshH, pMeshV);
    gh = sgh * scale;
    gv = sgv * scale;

    //  Upsampling the output roll-off mesh grid by scale factor
    gh_up = gh * scaleX;
    gv_up = gv * scaleY;
    dh_up = dh * scaleX;
    dv_up = dv * scaleY;

    // Calculate the rolloff grid at the full resolution
    meshHMax      = ROLLOFF_MESH_H_V34;
    meshVMax      = ROLLOFF_MESH_V_V34;
    channelWidth  = fullWidth >> 1;  // per-channel image width
    channelHeight = fullHeight >> 1;  // per-channel image height

    gh_full = (channelWidth - 1) / static_cast<FLOAT>(meshHMax);
    gv_full = (channelHeight - 1) / static_cast<FLOAT>(meshVMax);

    // outer extend the mesh data 1 block by keeping the same slope
    MeshExtend1block(pMeshIn, extendMesh, meshHMax + 1, meshVMax + 1);

    //  resample Extended Mesh data onto the roll-off mesh grid
    for (vMeshNum = 0; vMeshNum < (*pMeshV + 1); vMeshNum++)
    {
        for (hMeshNum = 0; hMeshNum < (*pMeshH + 1); hMeshNum++)
        {
            tx = static_cast<FLOAT>(hMeshNum*gh_up - dh_up + offsetX / 2 + gh_full) / gh_full;
            ix = static_cast<INT>(floor(tx));
            tx = tx - static_cast<FLOAT>(ix);

            ty = static_cast<FLOAT>(vMeshNum*gv_up - dv_up + offsetY / 2 + gv_full) / gv_full;
            iy = static_cast<INT>(floor(ty));
            ty = ty - static_cast<FLOAT>(iy);

            if ((vMeshNum == 0) || (hMeshNum == 0) || (vMeshNum == *pMeshV) || (hMeshNum == *pMeshH))
            {
                // for boundary points, use bilinear interpolation
                b1 = (1 - tx)* extendMesh[iy * (meshHMax + 3) + ix] +
                     tx      * extendMesh[iy * (meshHMax + 3) + ix + 1];
                b2 = (1 - tx)* extendMesh[(iy + 1)*(meshHMax + 3) + ix] +
                     tx      * extendMesh[(iy + 1)*(meshHMax + 3) + ix + 1];

                pMeshOut[(vMeshNum * (*pMeshH + 1)) + hMeshNum] = ((1.0f - ty)*b1 + ty*b2);
                pMeshOut[(vMeshNum * (*pMeshH + 1)) + hMeshNum] =
                    IQSettingUtils::MinFLOAT(IQSettingUtils::MaxFLOAT(pMeshOut[(vMeshNum * (*pMeshH + 1)) + hMeshNum],
                                             LSC34_MIN_MESH_VAL),
                                             LSC34_MAX_MESH_VAL);
            }
            else
            {
                // for nonboundary points, use bicubic interpolation

                // get x direction coeff and y direction coeff
                BicubicF(tx, &cxm, &cx0, &cx1, &cx2);
                BicubicF(ty, &cym, &cy0, &cy1, &cy2);

                am = extendMesh[(iy - 1) * (meshHMax + 3) + (ix - 1)];
                a0 = extendMesh[(iy - 1) * (meshHMax + 3) + (ix)];
                a1 = extendMesh[(iy - 1) * (meshHMax + 3) + (ix + 1)];
                a2 = extendMesh[(iy - 1) * (meshHMax + 3) + (ix + 2)];
                bm = ((cxm * am) + (cx0 * a0) + (cx1 * a1) + (cx2 * a2)) * 10;

                am = extendMesh[(iy) * (meshHMax + 3) + (ix - 1)];
                a0 = extendMesh[(iy) * (meshHMax + 3) + (ix)];
                a1 = extendMesh[(iy) * (meshHMax + 3) + (ix + 1)];
                a2 = extendMesh[(iy) * (meshHMax + 3) + (ix + 2)];
                b0 = ((cxm * am) + (cx0 * a0) + (cx1 * a1) + (cx2 * a2)) * 10;

                am = extendMesh[(iy + 1) * (meshHMax + 3) + (ix - 1)];
                a0 = extendMesh[(iy + 1) * (meshHMax + 3) + (ix)];
                a1 = extendMesh[(iy + 1) * (meshHMax + 3) + (ix + 1)];
                a2 = extendMesh[(iy + 1) * (meshHMax + 3) + (ix + 2)];
                b1 = ((cxm * am) + (cx0 * a0) + (cx1 * a1) + (cx2 * a2)) * 10;

                am = extendMesh[(iy + 2) * (meshHMax + 3) + (ix - 1)];
                a0 = extendMesh[(iy + 2) * (meshHMax + 3) + (ix)];
                a1 = extendMesh[(iy + 2) * (meshHMax + 3) + (ix + 1)];
                a2 = extendMesh[(iy + 2) * (meshHMax + 3) + (ix + 2)];
                b2 = ((cxm * am) + (cx0 * a0) + (cx1 * a1) + (cx2 * a2)) * 10;

                pMeshOut[(vMeshNum * (*pMeshH + 1)) + hMeshNum] =
                    ((cym * bm) + (cy0 * b0) + (cy1 * b1) + (cy2 * b2)) * 0.025f;
                pMeshOut[(vMeshNum * (*pMeshH + 1)) + hMeshNum] =
                    IQSettingUtils::MinFLOAT(IQSettingUtils::MaxFLOAT(pMeshOut[(vMeshNum * (*pMeshH + 1)) + hMeshNum],
                                             LSC34_MIN_MESH_VAL),
                                             LSC34_MAX_MESH_VAL);
            }
        }
    }

    return;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// LSC34Setting::BicubicF
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID LSC34Setting::BicubicF(
    FLOAT fs,
    FLOAT* pFc0,
    FLOAT* pFc1,
    FLOAT* pFc2,
    FLOAT* pFc3)
{
    FLOAT fs3;
    FLOAT fs2;

    fs2 = fs * fs;
    fs3 = fs * fs2;

    *pFc0 = (-fs3 + 2.0f * fs2 - fs);
    *pFc1 = (3.0f * fs3 - 5.0f * fs2 + 2.0f);
    *pFc2 = (-3 * fs3 + 4.0f * fs2 + fs);
    *pFc3 = (fs3 - fs2);

    return;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// LSC34Setting::scaleRolloffTable
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID LSC34Setting::scaleRolloffTable(
    const LSC34InputData*          pInput,
    lsc_3_4_0::lsc34_rgn_dataType* pData)
{
    UINT32 streamInWidth;
    UINT32 streamInHeight;
    UINT32 fullResWidth;
    UINT32 fullResHeight;
    FLOAT  rGain[MESH_ROLLOFF_SIZE];
    FLOAT  grGain[MESH_ROLLOFF_SIZE];
    FLOAT  gbGain[MESH_ROLLOFF_SIZE];
    FLOAT  bGain[MESH_ROLLOFF_SIZE];

    streamInWidth  = pInput->imageWidth;
    streamInHeight = pInput->imageHeight;
    fullResWidth   = pInput->fullResWidth;
    fullResHeight  = pInput->fullResHeight;

    /// @todo (CAMX-1812) sensorstreamwidth/height = raw width/height?

    MeshRolloffScaleRolloff(pData->r_gain_tab.r_gain,
                            rGain,
                            fullResWidth,
                            fullResHeight,
                            streamInWidth,
                            streamInHeight,
                            pInput->offsetX,
                            pInput->offsetY,
                            pInput->scalingFactor,
                            pInput->scalingFactor,
                            &s_meshHorizontal,
                            &s_meshVertical);

    MeshRolloffScaleRolloff(pData->gr_gain_tab.gr_gain,
                            grGain,
                            fullResWidth,
                            fullResHeight,
                            streamInWidth,
                            streamInHeight,
                            pInput->offsetX,
                            pInput->offsetY,
                            pInput->scalingFactor,
                            pInput->scalingFactor,
                            &s_meshHorizontal,
                            &s_meshVertical);

    MeshRolloffScaleRolloff(pData->gb_gain_tab.gb_gain,
                            gbGain,
                            fullResWidth,
                            fullResHeight,
                            streamInWidth,
                            streamInHeight,
                            pInput->offsetX,
                            pInput->offsetY,
                            pInput->scalingFactor,
                            pInput->scalingFactor,
                            &s_meshHorizontal,
                            &s_meshVertical);

    MeshRolloffScaleRolloff(pData->b_gain_tab.b_gain,
                            bGain,
                            fullResWidth,
                            fullResHeight,
                            streamInWidth,
                            streamInHeight,
                            pInput->offsetX,
                            pInput->offsetY,
                            pInput->scalingFactor,
                            pInput->scalingFactor,
                            &s_meshHorizontal,
                            &s_meshVertical);

    memcpy(pData->r_gain_tab.r_gain, rGain, sizeof(FLOAT)* MESH_ROLLOFF_SIZE);
    memcpy(pData->gr_gain_tab.gr_gain, grGain, sizeof(FLOAT) * MESH_ROLLOFF_SIZE);
    memcpy(pData->gb_gain_tab.gb_gain, gbGain, sizeof(FLOAT) * MESH_ROLLOFF_SIZE);
    memcpy(pData->b_gain_tab.b_gain, bGain, sizeof(FLOAT) * MESH_ROLLOFF_SIZE);

    return;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// LSC34Setting::MeshExtend1block
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID LSC34Setting::MeshExtend1block(
    const FLOAT* pMeshIn,
    FLOAT*       pMeshOut,
    INT          nx,
    INT          ny)
{
    INT hMeshNum;
    INT vMeshNum;

    for (vMeshNum = 1; vMeshNum < ny + 1; vMeshNum++)
    {
        for (hMeshNum = 1; hMeshNum < nx + 1; hMeshNum++)
        {
            pMeshOut[vMeshNum*(nx + 2) + hMeshNum] = pMeshIn[(vMeshNum - 1)*nx + hMeshNum - 1];
        }
    }

    /// @todo (CAMX-1812) - Remove magic numbers

    pMeshOut[0 * (nx + 2) + 0]           = pMeshOut[1 * (nx + 2) + 1] * 2   - pMeshOut[2 * (nx + 2) + 2];
    pMeshOut[(ny + 1)*(nx + 2) + 0]      = pMeshOut[(ny)*(nx + 2) + 1] * 2  - pMeshOut[(ny - 1)*(nx + 2) + 2];
    pMeshOut[(ny + 1)*(nx + 2) + nx + 1] = pMeshOut[(ny)*(nx + 2) + nx] * 2 - pMeshOut[(ny - 1)*(nx + 2) + nx - 1];
    pMeshOut[0 * (nx + 2) + nx + 1]      = pMeshOut[1 * (nx + 2) + nx] * 2  - pMeshOut[2 * (nx + 2) + nx - 1];

    for (vMeshNum = 1; vMeshNum<ny + 1; vMeshNum++)
    {
        pMeshOut[vMeshNum*(nx + 2) + 0]      = pMeshOut[vMeshNum*(nx + 2) + 1] * 2 - pMeshOut[vMeshNum*(nx + 2) + 2];
        pMeshOut[vMeshNum*(nx + 2) + nx + 1] = pMeshOut[vMeshNum*(nx + 2) + nx] * 2 - pMeshOut[vMeshNum*(nx + 2) + nx - 1];
    }
    for (hMeshNum = 1; hMeshNum<nx + 1; hMeshNum++)
    {
        pMeshOut[0 * (nx + 2) + hMeshNum]      = pMeshOut[1 * (nx + 2) + hMeshNum] * 2 - pMeshOut[2 * (nx + 2) + hMeshNum];
        pMeshOut[(ny + 1)*(nx + 2) + hMeshNum] =
            pMeshOut[(ny)*(nx + 2) + hMeshNum] * 2 - pMeshOut[(ny - 1)*(nx + 2) + hMeshNum];
    }

    return;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// LSC34Setting::InterpGridOptimization
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID LSC34Setting::InterpGridOptimization(
    INT  rawWidth,
    INT  rawHeight,
    INT* pScaleCubic,
    INT* pDeltaH,
    INT* pDeltaV,
    INT* pSubgridH,
    INT* pSubgridV,
    INT* pNH,
    INT* pNV)
{
    UINT32 level;
    UINT32 numGridH;
    UINT32 numGridV;
    INT    nWidth;
    INT    nHeight;
    INT    blockWidth;
    INT    blockHeight;
    INT    sGwidth        = 0;
    INT    sGHeight       = 0;
    INT    meshOverWidth  = 0;
    INT    meshOverHeight = 0;

    numGridH = ROLLOFF_MESH_H_V34;
    numGridV = ROLLOFF_MESH_V_V34;

    nWidth  = rawWidth >> 1;   // per-channel image width
    nHeight = rawHeight >> 1;  // per-channel image height

    /// @todo (CAMX-1812) - Remove magic numbers

    level = 4; // Initial bicubic level level as 1 more than maximum 3

    do
    {
        if ((level == 0) &&
            ((numGridH <= LSC34_MESH_ROLLOFF_HORIZONTAL_GRIDS) ||
             (numGridV <= LSC34_MESH_ROLLOFF_VERTICAL_GRIDS)))
        {
            /// @todo (CAMX-1812) Add log Image size is too small and not supported
            break;
        }
        if (level > 0)
        {
            level--;
        }
        else if ((numGridH > LSC34_MESH_ROLLOFF_HORIZONTAL_GRIDS) && (numGridV > LSC34_MESH_ROLLOFF_VERTICAL_GRIDS))
        {
            numGridH -= 4;
            numGridV -= 3;
            level     = 3;
        }

        sGwidth       = (nWidth + numGridH - 1) / numGridH;       // Ceil
        sGwidth       = (sGwidth + (1 << level) - 1) >> level;    // Ceil
        blockWidth    = sGwidth << level;                         // Bayer grid width
        meshOverWidth = blockWidth * numGridH - nWidth;           // two-side overhead

        sGHeight        = (nHeight + numGridV - 1) / numGridV;    // Ceil
        sGHeight        = (sGHeight + (1 << level) - 1) >> level; // Ceil
        blockHeight     = sGHeight << level;                      // Bayer grid height
        meshOverHeight  = blockHeight * numGridV - nHeight;       // two-side overhead

    } while ((blockWidth < 18)                        ||
            (blockHeight < 9)                         ||
            (sGwidth < 9)                             ||
            (sGHeight < 9)                            ||
            (meshOverWidth >= blockWidth)             ||
            (meshOverHeight >= blockHeight)           ||
            (blockWidth - (meshOverWidth + 1) / 2<18) ||
            (sGwidth - (((meshOverWidth + 1) / 2) % sGwidth)<9)); // latest setting

    *pScaleCubic = 1 << level;
    *pDeltaH     = (meshOverWidth + 1) >> 1;
    *pDeltaV     = (meshOverHeight + 1) >> 1;
    *pSubgridH   = sGwidth;
    *pSubgridV   = sGHeight;
    *pNH         = numGridH;
    *pNV         = numGridV;

    return;
}
