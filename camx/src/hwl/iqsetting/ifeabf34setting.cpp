// NOWHINE NC009 <- Shared file with system team so uses non-CamX file naming
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2017-2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  ifeabf34setting.cpp
/// @brief IFE ABF34 setting calculation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#include "ifeabf34setting.h"

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// IFEABF34Setting::GenerateNoiseStandardLUT
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void IFEABF34Setting::GenerateNoiseStandardLUT(
    FLOAT*  pLUTTable,
    UINT32* pLUT0,
    INT32   length,
    BOOL    absoluteValueFlag)
{
    INT32   x;
    INT32   y;
    INT32   LUTdelta;
    INT32   i;
    INT32   nbits = ABF34_NBITS;

    for (i = 0; i < length; i++)
    {

        x = IQSettingUtils::ClampUINT32(static_cast<UINT32>(IQSettingUtils::RoundFLOAT(pLUTTable[i])),
                                        ABF34_NOISESTDLUT_BASE_MIN,
                                        ABF34_NOISESTDLUT_BASE_MAX);

        y = IQSettingUtils::ClampUINT32(static_cast<UINT32>(IQSettingUtils::RoundFLOAT(pLUTTable[i + 1])),
                                        ABF34_NOISESTDLUT_BASE_MIN,
                                        ABF34_NOISESTDLUT_BASE_MAX);

        LUTdelta = y - x;
        if (TRUE == absoluteValueFlag)
        {
            if (LUTdelta > 0)
            {
                /// @todo (CAMX-1812) Need to add logging for Common library
                /// CAMX_LOG_ERROR(CamxLogGroupIQMod,
                ///                "Titan: ABF34: Noise Std LUT Should be Monotonically increasing lut[%d]:%d lut[%d]:%d",
                ///                i, x, i + 1, y);
            }
            LUTdelta = abs(LUTdelta);
        }
        LUTdelta = IQSettingUtils::ClampINT32(LUTdelta, ABF34_NOISESTDLUT_DELTA_MIN, ABF34_NOISESTDLUT_DELTA_MAX);

        pLUT0[i] = static_cast<UINT32>(x | (LUTdelta * (1 << nbits)));
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// IFEABF34Setting::CalculateHWSetting
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL IFEABF34Setting::CalculateHWSetting(
    const ABF34InputData*                                 pInput,
    abf_3_4_0::abf34_rgn_dataType*                        pData,
    abf_3_4_0::chromatix_abf34Type::enable_sectionStruct* pModuleEnable,
    abf_3_4_0::chromatix_abf34_reserveType*               pReserveType,
    VOID*                                                 pOutput)
{
    BOOL   result = TRUE;
    FLOAT  min;
    FLOAT  max;
    INT16  rSquareShift;
    INT16  noiseShiftVal;
    UINT32 radialPoint;
    UINT32 channel;
    UINT32 level;
    UINT32 noiseStandardVal;
    INT    distance;
    DOUBLE tempNoiseStandardVal0;
    INT32  tempNoiseStandardVal00;
    INT16  anchorDifference;
    INT16  baseDifference;
    UINT16 anchorTable[ABF34_RADIAL_POINTS];
    UINT16 baseTable[2][ABF34_RADIAL_POINTS];
    UINT16 noise_prsv_hi[2];
    UINT32 noise_offset_max_val_chromatix = IQSettingUtils::MAXUINTBITFIELD(9);
    FLOAT  calculatedNoiseStdlutLevel[ABFV34_NOISE_STD_LENGTH];


    if ((NULL != pInput) && (NULL != pData) && (NULL != pOutput) && (NULL != pReserveType))
    {
        ABF34UnpackedField* pUnpackedField = static_cast<ABF34UnpackedField*>(pOutput);
        INT16 subgridHOffset = static_cast<INT16>(pInput->sensorOffsetX);
        INT16 subgridVOffset = static_cast<INT16>(pInput->sensorOffsetY);
        INT32 width          = static_cast<INT32>(pInput->CAMIFWidth);
        INT32 height         = static_cast<INT32>(pInput->CAMIFHeight);

        /// @todo (CAMX-1812) Need to check clamping part and any other check need for enable this
        pUnpackedField->enable       =
            static_cast<UINT16>(IQSettingUtils::ClampUINT32(pModuleEnable->abf_enable,
                                                            ABF34_DISABLE,
                                                            ABF34_ENABLE));
        pUnpackedField->lut_bank_sel = pInput->LUTBankSel;

        /// @todo (CAMX-1812) Need to check clamping
        pUnpackedField->cross_process_en   =
            static_cast<UINT16>(IQSettingUtils::ClampUINT32(pReserveType->cross_plane_en,
                                                            ABF34_CP_DISABLE,
                                                            ABF34_CP_ENABLE));

        pUnpackedField->bpc_bls =
            static_cast<UINT16>(IQSettingUtils::ClampUINT32(pInput->blackResidueOffset,
                                                            ABF34_BPC_BLS_MIN,
                                                            ABF34_BPC_BLS_MAX));

        pUnpackedField->bx       = static_cast<INT16>(IQSettingUtils::ClampINT32(subgridHOffset - (width / 2),
                                                                                 ABF34_BX_MIN,
                                                                                 ABF34_BX_MAX));
        pUnpackedField->by       = static_cast<INT16>(IQSettingUtils::ClampINT32(subgridVOffset - (height / 2),
                                                                                 ABF34_BY_MIN,
                                                                                 ABF34_BY_MAX));

        pUnpackedField->r_square_init  = pUnpackedField->bx * pUnpackedField->bx + pUnpackedField->by * pUnpackedField->by;
        pUnpackedField->r_square_init  = IQSettingUtils::ClampUINT32(pUnpackedField->r_square_init,
                                                                     ABF34_R_SQUARE_INIT_MIN,
                                                                     ABF34_R_SQUARE_INIT_MAX);
        pUnpackedField->r_square_shft  = 0;

        pUnpackedField->single_bpc_en  = static_cast<UINT16>(pModuleEnable->sbpc_enable);
        // moduleEnable is the result of Filter_en for Dynamic Trigger
        pUnpackedField->filter_en      = static_cast<UINT16>((pModuleEnable->abf_enable) & pInput->moduleEnable);
        pUnpackedField->enable         = pUnpackedField->single_bpc_en | pUnpackedField->filter_en;

        for (rSquareShift = 0; rSquareShift <= ABF34_R_SQUARE_SHFT_MAX; rSquareShift++)
        {
            if ((pUnpackedField->r_square_init >> rSquareShift) <= (static_cast<UINT32>(ABF34_MAX_RSQUARE_VAL )))
            {
                pUnpackedField->r_square_shft = rSquareShift;
                break;
            }
        }
        pUnpackedField->r_square_shft  = static_cast<UINT16>(IQSettingUtils::ClampUINT32(pUnpackedField->r_square_shft,
                                                                                         ABF34_R_SQUARE_SHFT_MIN,
                                                                                         ABF34_R_SQUARE_SHFT_MAX));

        /// @todo (CAMX-1812) Need to check edge softness for unpacked data
        memset(pUnpackedField->edge_softness, 0, sizeof(pUnpackedField->edge_softness));

        for (radialPoint = 0; radialPoint < ABF34_RADIAL_POINTS; radialPoint++)
        {
            baseTable[0][radialPoint] =
                static_cast<UINT16>(IQSettingUtils::ClampUINT32(IQSettingUtils::RoundFLOAT(
                                    pData->radial_gain_tab.radial_gain[(0 * (ABF34_BASETABLE_ENTRIES_RB_G + 1)) + radialPoint]
                                        * (1 << ABF34_BASE_TABLE_Q_FACTOR)),
                                    ABF34_BASE_TABLE_MIN,
                                    ABF34_BASE_TABLE_MAX));
            baseTable[1][radialPoint] =
                static_cast<UINT16>(IQSettingUtils::ClampUINT32(IQSettingUtils::RoundFLOAT(
                                    pData->radial_gain_tab.radial_gain[(1 * (ABF34_BASETABLE_ENTRIES_RB_G + 1)) + radialPoint]
                                        * (1 << ABF34_BASE_TABLE_Q_FACTOR)),
                                    ABF34_BASE_TABLE_MIN,
                                    ABF34_BASE_TABLE_MAX));
        }

        // from 5-entry table in header to 4-entry table in cfg
        for (radialPoint = 0; radialPoint < ABF34_RADIAL_POINTS - 1; radialPoint++)
        {
            anchorTable[radialPoint + 1] =
                static_cast<UINT16>(pow(pReserveType->radial_anchor_tab.radial_anchor[radialPoint + 1] /
                    pReserveType->radial_anchor_tab.radial_anchor[4], 2.0f) * 4095);

            anchorTable[radialPoint] =
                static_cast<UINT16>(pow(pReserveType->radial_anchor_tab.radial_anchor[radialPoint] /
                    pReserveType->radial_anchor_tab.radial_anchor[4], 2.0f) * 4095);

            anchorDifference =
                static_cast<INT16>(IQSettingUtils::ClampINT32(anchorTable[radialPoint + 1] - anchorTable[radialPoint],
                                                              ABF34_ANCHOR_TABLE_MIN + 1,
                                                              ABF34_ANCHOR_TABLE_MAX));

            baseDifference =
                static_cast<INT16>(IQSettingUtils::ClampINT32(baseTable[0][radialPoint] - baseTable[0][radialPoint + 1],
                                                              ABF34_BASE_TABLE_MIN+1,
                                                              ABF34_BASE_TABLE_MAX));

            if (0 == baseDifference)  // Divide by Zero check
            {
                pUnpackedField->shift_table[0][radialPoint] = 0;
            }
            else
            {
                pUnpackedField->shift_table[0][radialPoint] = static_cast<UINT16>(static_cast<FLOAT>(
                    (log(static_cast<FLOAT>(anchorDifference)) + log(255.0f) - log(static_cast<FLOAT>(baseDifference))) /
                     log(2.0f)));
            }


            pUnpackedField->slope_table[0][radialPoint] =
                static_cast<UINT16>(((baseDifference << (pUnpackedField->shift_table[0][radialPoint])) / anchorDifference));
            pUnpackedField->shift_table[0][radialPoint] =
                static_cast<UINT16>(IQSettingUtils::ClampUINT32(pUnpackedField->shift_table[0][radialPoint],
                                                                ABF34_SHIFT_TABLE_MIN,
                                                                ABF34_SHIFT_TABLE_MAX));

            pUnpackedField->slope_table[0][radialPoint] =
                static_cast<UINT16>(IQSettingUtils::ClampUINT32(pUnpackedField->slope_table[0][radialPoint],
                                                                ABF34_SLOPE_TABLE_MIN,
                                                                ABF34_SLOPE_TABLE_MAX));

            baseDifference =
                static_cast<INT16>(IQSettingUtils::ClampINT32(baseTable[1][radialPoint] - baseTable[1][radialPoint + 1],
                                                              ABF34_BASE_TABLE_MIN + 1,
                                                              ABF34_BASE_TABLE_MAX));

            if (0 == baseDifference)  // Divide by zero check
            {
                pUnpackedField->shift_table[0][radialPoint] = 0;
            }
            else
            {
                pUnpackedField->shift_table[1][radialPoint] = static_cast<UINT16>(static_cast<FLOAT>(
                    (log(static_cast<FLOAT>(anchorDifference)) + log(255.0f) - log(static_cast<FLOAT>(baseDifference))) /
                     log(2.0f)));
            }

            pUnpackedField->slope_table[1][radialPoint] =
                static_cast<UINT16>(((baseDifference << (pUnpackedField->shift_table[1][radialPoint])) / anchorDifference));
            pUnpackedField->shift_table[1][radialPoint] =
                static_cast<UINT16>(IQSettingUtils::ClampUINT32(pUnpackedField->shift_table[1][radialPoint],
                                                                ABF34_SHIFT_TABLE_MIN,
                                                                ABF34_SHIFT_TABLE_MAX));

            pUnpackedField->slope_table[1][radialPoint] =
                static_cast<UINT16>(IQSettingUtils::ClampUINT32(pUnpackedField->slope_table[1][radialPoint],
                                                                ABF34_SLOPE_TABLE_MIN,
                                                                ABF34_SLOPE_TABLE_MAX));
        }

        for (radialPoint = 0; radialPoint < ABF34_RADIAL_POINTS - 1; radialPoint++)
        {
            pUnpackedField->base_table[0][radialPoint] =
                static_cast<UINT16>(IQSettingUtils::ClampUINT32(IQSettingUtils::RoundFLOAT(
                                    pData->radial_gain_tab.radial_gain[(0 * (ABF34_BASETABLE_ENTRIES_RB_G + 1) +radialPoint)]
                                        * (1 << ABF34_BASE_TABLE_Q_FACTOR)),
                                    ABF34_BASE_TABLE_MIN,
                                    ABF34_BASE_TABLE_MAX));

            pUnpackedField->base_table[1][radialPoint] =
                static_cast<UINT16>(IQSettingUtils::ClampUINT32(IQSettingUtils::RoundFLOAT(
                                    pData->radial_gain_tab.radial_gain[(1 * (ABF34_BASETABLE_ENTRIES_RB_G + 1) +radialPoint)]
                                        * (1 << ABF34_BASE_TABLE_Q_FACTOR)),
                                    ABF34_BASE_TABLE_MIN,
                                    ABF34_BASE_TABLE_MAX));

            pUnpackedField->anchor_table[radialPoint]  =
                static_cast<UINT16>(IQSettingUtils::ClampUINT32(
                                    anchorTable[radialPoint] ,
                                    ABF34_ANCHOR_TABLE_MIN,
                                    ABF34_ANCHOR_TABLE_MAX));
        }

        pUnpackedField->bpc_fmax = static_cast<UINT16>(
            IQSettingUtils::ClampFLOAT(static_cast<FLOAT>(
                IQSettingUtils::RoundFLOAT(pData->bpc_fmax)), ABF34_BPC_FMAX_MIN, ABF34_BPC_FMAX_MAX));
        pUnpackedField->bpc_fmin = static_cast<UINT16>(
            IQSettingUtils::ClampFLOAT(static_cast<FLOAT>(
                IQSettingUtils::RoundFLOAT(pData->bpc_fmin)), ABF34_BPC_FMIN_MIN, ABF34_BPC_FMIN_MAX));
        pUnpackedField->bpc_maxshft = static_cast<UINT16>(
            IQSettingUtils::ClampFLOAT(static_cast<FLOAT>(
                IQSettingUtils::RoundFLOAT(pData->bpc_maxshft)), ABF34_BPC_MAXSHFT_MIN, ABF34_BPC_MAXSHFT_MAX));
        pUnpackedField->bpc_minshft = static_cast<UINT16>(
            IQSettingUtils::ClampFLOAT(static_cast<FLOAT>(
                IQSettingUtils::RoundFLOAT(pData->bpc_minshft)), ABF34_BPC_MAXSHFT_MIN, ABF34_BPC_MINSHFT_MAX));
        pUnpackedField->bpc_offset = static_cast<UINT16>(
            IQSettingUtils::ClampFLOAT(static_cast<FLOAT>(
                IQSettingUtils::RoundFLOAT(pData->bpc_offset)), ABF34_BPC_OFFSET_MIN, ABF34_BPC_OFFSET_MAX));
        pUnpackedField->blk_pix_matching_rb = static_cast<UINT16>(
            IQSettingUtils::ClampFLOAT(static_cast<FLOAT>(
                IQSettingUtils::RoundFLOAT(pData->blk_pix_matching_rb)), ABF34_BLKPIX_LEV_MIN, ABF34_BLKPIX_LEV_MAX));
        pUnpackedField->blk_pix_matching_g  = static_cast<UINT16>(
            IQSettingUtils::ClampFLOAT(static_cast<FLOAT>(
                IQSettingUtils::RoundFLOAT(pData->blk_pix_matching_g)), ABF34_BLKPIX_LEV_MIN, ABF34_BLKPIX_LEV_MAX));

        for (channel = 0; channel < ABF3_CHANNEL_MAX; channel++)
        {
            pUnpackedField->curve_offset[channel] = static_cast<UINT16>(
                IQSettingUtils::ClampFLOAT(static_cast<FLOAT>(
                    IQSettingUtils::RoundFLOAT(pData->curve_offset_tab.curve_offset[channel])),
                                               ABF34_CURVE_OFFSET_MIN,
                                               ABF34_CURVE_OFFSET_MAX));
        }

        for (noiseStandardVal = 0; noiseStandardVal < ABFV34_NOISE_STD_LENGTH; noiseStandardVal++)
        {
            tempNoiseStandardVal0  = static_cast<DOUBLE>(pData->noise_stdlut_level_tab.noise_stdlut_level[noiseStandardVal]);
            tempNoiseStandardVal0  = IQSettingUtils::ClampDOUBLE(tempNoiseStandardVal0,
                                                                 0.0,
                                                                 static_cast<DOUBLE>(noise_offset_max_val_chromatix));
            /// @todo (CAMX-1812) // 10u // 1/2 is sqrt(1/4)
            tempNoiseStandardVal0  = tempNoiseStandardVal0*(pData->edge_softness);
            tempNoiseStandardVal00 = static_cast<INT32>(IQSettingUtils::RoundDOUBLE(
                ((0.0f == tempNoiseStandardVal0 ) ? static_cast<DOUBLE>((1 << ABF34_NOISE_STD_QVAL)) :
                                                   static_cast<DOUBLE>((1 << ABF34_NOISE_STD_QVAL)) / tempNoiseStandardVal0)));

            calculatedNoiseStdlutLevel[noiseStandardVal] = static_cast<FLOAT>(tempNoiseStandardVal00);
        }

        pUnpackedField->noise_prsv_anchor_lo = static_cast<UINT16>(
            IQSettingUtils::ClampFLOAT(static_cast<FLOAT>(
                IQSettingUtils::RoundFLOAT(pData->noise_prsv_anchor_lo)),
                                           ABF34_NOISE_PRSV_ANCHOR_LO_MIN,
                                           ABF34_NOISE_PRSV_ANCHOR_LO_MAX)); // in 10u

        if (pData->noise_prsv_anchor_hi == pData->noise_prsv_anchor_lo)
        {
            pData->noise_prsv_anchor_hi = pData->noise_prsv_anchor_lo + 1;
        }

        // in 10u
        pUnpackedField->noise_prsv_anchor_gap = static_cast<UINT16>(
            IQSettingUtils::ClampFLOAT(static_cast<FLOAT>(
                IQSettingUtils::RoundFLOAT(pData->noise_prsv_anchor_hi - pData->noise_prsv_anchor_lo)),
                                           static_cast<FLOAT>(ABF34_NOISE_PRSV_ANCHOR_GAP_MIN),
                                           static_cast<FLOAT>(ABF34_NOISE_PRSV_ANCHOR_GAP_MAX)));


        pUnpackedField->noise_prsv_lo[0] = static_cast<UINT16>(IQSettingUtils::ClampINT32(
            IQSettingUtils::RoundFLOAT(static_cast<FLOAT>(1 << 8) * pData->noise_prsv_lo_tab.noise_prsv_lo[0]),
                                        ABF34_NOISE_PRSV_LO_MIN,
                                        ABF34_NOISE_PRSV_LO_MAX));

        pUnpackedField->noise_prsv_lo[1] = static_cast<UINT16>(IQSettingUtils::ClampINT32(
            IQSettingUtils::RoundFLOAT(static_cast<FLOAT>(1 << 8) * pData->noise_prsv_lo_tab.noise_prsv_lo[1]),
                                        ABF34_NOISE_PRSV_LO_MIN,
                                        ABF34_NOISE_PRSV_LO_MAX));

        noise_prsv_hi[0] = static_cast<UINT16>(IQSettingUtils::ClampINT32(
            IQSettingUtils::RoundFLOAT(static_cast<FLOAT>(1 << 8) * pData->noise_prsv_hi_tab.noise_prsv_hi[0]),
                                        ABF34_NOISE_PRSV_LO_MIN,
                                        ABF34_NOISE_PRSV_LO_MAX));

        noise_prsv_hi[1] = static_cast<UINT16>(IQSettingUtils::ClampINT32(
            IQSettingUtils::RoundFLOAT(static_cast<FLOAT>(1 << 8) * pData->noise_prsv_hi_tab.noise_prsv_hi[1]),
                                        ABF34_NOISE_PRSV_LO_MIN,
                                        ABF34_NOISE_PRSV_LO_MAX));

        /// @todo (CAMX-1812) Add description for each loop
        if (noise_prsv_hi[0] >= pUnpackedField->noise_prsv_lo[0])
        {
            for (noiseShiftVal = ABF34_NOISE_PRSV_SHFT_MAX; noiseShiftVal >= ABF34_NOISE_PRSV_SHFT_MIN; noiseShiftVal--)
            {
                if ((((noise_prsv_hi[0] - pUnpackedField->noise_prsv_lo[0]) << noiseShiftVal) /
                       pUnpackedField->noise_prsv_anchor_gap) <= ABF34_NOISE_PRSV_SLOPE_MAX)
                {
                    pUnpackedField->noise_prsv_shft[0]  = noiseShiftVal;
                    pUnpackedField->noise_prsv_slope[0] = static_cast<INT16>(IQSettingUtils::RoundFLOAT(
                        static_cast<FLOAT>((noise_prsv_hi[0] - pUnpackedField->noise_prsv_lo[0]) << noiseShiftVal) /
                        static_cast<FLOAT>(pUnpackedField->noise_prsv_anchor_gap)));
                    break;
                }
            }
        }
        else
        {
            for (noiseShiftVal = ABF34_NOISE_PRSV_SHFT_MAX; noiseShiftVal >= ABF34_NOISE_PRSV_SHFT_MIN; noiseShiftVal--)
            {
                if ((((noise_prsv_hi[0] - pUnpackedField->noise_prsv_lo[0]) << noiseShiftVal) /
                       pUnpackedField->noise_prsv_anchor_gap) >= ABF34_NOISE_PRSV_SLOPE_MIN)
                {
                    pUnpackedField->noise_prsv_shft[0]  = noiseShiftVal;
                    pUnpackedField->noise_prsv_slope[0] = static_cast<INT16>(IQSettingUtils::RoundFLOAT(static_cast<FLOAT>
                        ((noise_prsv_hi[0] - pUnpackedField->noise_prsv_lo[0]) << noiseShiftVal) /
                        static_cast<FLOAT>(pUnpackedField->noise_prsv_anchor_gap)));
                    break;
                }
            }
        }

        /// @todo (CAMX-1812) Add description for each loop and ifs
        if (noise_prsv_hi[1] >= pUnpackedField->noise_prsv_lo[1])
        {
            for (noiseShiftVal = ABF34_NOISE_PRSV_SHFT_MAX; noiseShiftVal >= ABF34_NOISE_PRSV_SHFT_MIN; noiseShiftVal--)
            {
                if ((((noise_prsv_hi[1] - pUnpackedField->noise_prsv_lo[1]) << noiseShiftVal) /
                       pUnpackedField->noise_prsv_anchor_gap) <= ABF34_NOISE_PRSV_SLOPE_MAX)
                {
                    pUnpackedField->noise_prsv_shft[1]  = noiseShiftVal;
                    pUnpackedField->noise_prsv_slope[1] = static_cast<INT16>(IQSettingUtils::RoundFLOAT(static_cast<FLOAT>
                        ((noise_prsv_hi[1] - pUnpackedField->noise_prsv_lo[1]) << noiseShiftVal) /
                        static_cast<FLOAT>(pUnpackedField->noise_prsv_anchor_gap)));
                    break;
                }
            }
        }
        else
        {
            for (noiseShiftVal = ABF34_NOISE_PRSV_SHFT_MAX; noiseShiftVal >= ABF34_NOISE_PRSV_SHFT_MIN; noiseShiftVal--)
            {
                if ((((noise_prsv_hi[1] - pUnpackedField->noise_prsv_lo[1]) << noiseShiftVal) /
                       pUnpackedField->noise_prsv_anchor_gap) >= ABF34_NOISE_PRSV_SLOPE_MIN)
                {
                    pUnpackedField->noise_prsv_shft[1]  = noiseShiftVal;
                    pUnpackedField->noise_prsv_slope[1] = static_cast<INT16>(IQSettingUtils::RoundFLOAT(static_cast<FLOAT>
                        ((noise_prsv_hi[1] - pUnpackedField->noise_prsv_lo[1]) << noiseShiftVal) /
                        static_cast<FLOAT>(pUnpackedField->noise_prsv_anchor_gap)));
                    break;
                }
            }
        }

        // noise_prsv_shft --> [1] - Gr / Gb, [0] - R / B
        for (level = 0; level < ABF3_LEVEL_MAX; level++)
        {
            pUnpackedField->noise_prsv_shft[level]  =
                static_cast<UINT16>(IQSettingUtils::ClampUINT32(pUnpackedField->noise_prsv_shft[level],
                                                                ABF34_NOISE_PRSV_SHFT_MIN,
                                                                ABF34_NOISE_PRSV_SHFT_MAX));
            pUnpackedField->noise_prsv_slope[level] =
                static_cast<INT16>(IQSettingUtils::ClampINT32(pUnpackedField->noise_prsv_slope[level],
                                                              ABF34_NOISE_PRSV_SLOPE_MIN,
                                                              ABF34_NOISE_PRSV_SLOPE_MAX));
        }

        // [0][] - Gr/Gb, [1][] - R/B, [][0] range: 1-4, [][1] range 0-2,
        // [][2] range: 0 - 2; non - zero n mean 1 << (n - 1)
        for (level = 0; level < ABF3_LEVEL_MAX; level++)
        {
            for (distance = 0; distance < 3; distance++)
            {
                switch (distance)
                {
                    case 0:
                        min = ABF34_DISTANCE_KER_0_MIN;
                        max = ABF34_DISTANCE_KER_0_MAX;
                        break;
                    case 1:
                        min = ABF34_DISTANCE_KER_1_MIN;
                        max = ABF34_DISTANCE_KER_1_MAX;
                        break;
                    default:
                        min = ABF34_DISTANCE_KER_2_MIN;
                        max = ABF34_DISTANCE_KER_2_MAX;
                        break;
                }
                pUnpackedField->distance_ker[level][distance] =
                    static_cast<UINT16>(
                        IQSettingUtils::ClampFLOAT(pData->distance_ker_tab.distance_ker[(level * 3) + distance], min, max));
            }
        }

        GenerateNoiseStandardLUT(calculatedNoiseStdlutLevel,
                                 pUnpackedField->noise_std_lut_level0[pInput->LUTBankSel],
                                 DMIRAM_ABF34_NOISESTD_LENGTH,
                                 TRUE);
    }
    else
    {
        result = FALSE;
    }
    return result;
}
