// NOWHINE NC009 <- Shared file with system team so uses non-CamX file naming
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  ltm14setting.cpp
/// @brief IPE LTM14 setting calculation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#include "ltm14setting.h"

using namespace CamX;

static FLOAT inverseGammaIn_v14[LTM14_GAMMA_LUT_SIZE + 1] =
{
    0, 16, 32, 47, 63, 86, 110, 135,
    163, 191, 223, 255, 290, 326, 364, 403,
    443, 486, 529, 575, 622, 670, 720, 771,
    824, 879, 934, 992, 1050, 1110, 1172, 1235,
    1299, 1365, 1433, 1501, 1571, 1643, 1716, 1790,
    1866, 1943, 2021, 2101, 2183, 2265, 2349, 2435,
    2521, 2610, 2699, 2790, 2882, 2976, 3071, 3167,
    3265, 3364, 3464, 3566, 3669, 3774, 3879, 3987,
    4095
};

static FLOAT inverseGammaInNormalized_v14[LTM14_GAMMA_LUT_SIZE + 1] =
{
    0.0002442f, 0.0041514f, 0.0080586f, 0.0117216f, 0.0156288f, 0.0212454f, 0.0271062f, 0.0332112f,
    0.0400488f, 0.0468864f, 0.0547008f, 0.0625152f, 0.0710622f, 0.0798534f, 0.0891330f, 0.0986568f,
    0.1084249f, 0.1189255f, 0.1294261f, 0.1406593f, 0.1521367f, 0.1638583f, 0.1760683f, 0.1885225f,
    0.2014652f, 0.2148962f, 0.2283272f, 0.2424908f, 0.2566544f, 0.2713064f, 0.2864468f, 0.3018315f,
    0.3174603f, 0.3335775f, 0.3501831f, 0.3667887f, 0.3838827f, 0.4014652f, 0.4192918f, 0.4373626f,
    0.4559218f, 0.4747252f, 0.4937728f, 0.5133089f, 0.5333333f, 0.5533577f, 0.5738705f, 0.5948717f,
    0.6158730f, 0.6376068f, 0.6593406f, 0.6815628f, 0.7040293f, 0.7269841f, 0.7501831f, 0.7736263f,
    0.7975579f, 0.8217338f, 0.8461538f, 0.8710622f, 0.8962148f, 0.9218559f, 0.9474969f, 0.9738705f,
    1.0000000f
};


static FLOAT ltmFixedGamma_v14[LTM14_GAMMA_LUT_SIZE + 1] =
{
    0, 259, 432, 577, 705, 822, 930, 1031,
    1127, 1218, 1304, 1387, 1468, 1545, 1620, 1692,
    1763, 1832, 1899, 1964, 2028, 2091, 2152, 2212,
    2271, 2329, 2386, 2442, 2497, 2551, 2604, 2657,
    2708, 2759, 2810, 2859, 2909, 2957, 3005, 3052,
    3099, 3145, 3191, 3236, 3281, 3325, 3369, 3412,
    3455, 3498, 3540, 3582, 3624, 3665, 3705, 3746,
    3786, 3825, 3865, 3904, 3943, 3981, 4019, 4057,
    4095
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief Both LCE_scale curves are 16-entry signed LUT
/// Pack1D16EntrySignedLUT
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static void Pack1D16EntrySignedLUT(
    INT16* pTableIn,
    INT32* pTableOut,
    UINT16 e)
{
    UINT32 index = 0;
    UINT16 msk;
    INT32  diff;
    UINT32 base;

    for (index = 0; index < (LTM14_LCE_SCALE_LUT_SIZE - 1); index++)
    {
        pTableIn[index + 1] = IQSettingUtils::MinINT16(pTableIn[index + 1], (1 << (9 + e)) - 1);
        pTableIn[index]     = IQSettingUtils::MinINT16(pTableIn[index], (1 << (9 + e)) - 1);
        diff                = pTableIn[index + 1] - pTableIn[index];
        base                = pTableIn[index];

        if (1 <= (base >> 15))
        {
            base |= (1 << (9 + e));
        }

        if (1 <= (diff >> 15))
        {
            diff |= (1 << (9 + e));
        }
        msk              = ((1 << (10 + e)) - 1);
        pTableOut[index] = ((diff & msk) << (10 + e)) | (base & msk);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief LTM_scale is 64-entry signed LUT
/// Pack1D64EntrySignedLUT
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static void Pack1D64EntrySignedLUT(
    INT16* pTableIn,
    INT32* pTableOut,
    UINT16 e)
{
    UINT32 index = 0;
    UINT16 msk;
    INT32  diff;
    UINT32 base;

    for (index = 0; index < (LTM14_SCALE_LUT_SIZE - 1); index++)
    {
        pTableIn[index + 1] = IQSettingUtils::MinINT16(pTableIn[index + 1], (1 << (9 + e)) - 1);
        pTableIn[index]     = IQSettingUtils::MinINT16(pTableIn[index], (1 << (9 + e)) - 1);
        diff                = pTableIn[index + 1] - pTableIn[index];
        base                = pTableIn[index];

        if (0 != (base >> 15))
        {
            base |= (1 << (9 + e));
        }

        if (0 != (diff >> 15))
        {
            diff |= (1 << (9 + e));
        }
        msk              = ((1 << (10 + e)) - 1);
        pTableOut[index] = ((diff & msk) << (10 + e)) | (base & msk);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief LTM curve, MASK rectification curve and LA curve are of 64-entry unsigned 1d LUT
/// Pack1D64EntryUnsignedLUT
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static void Pack1D64EntryUnsignedLUT(
    INT32* pTableIn,
    INT32* pTableOut,
    UINT16 e)
{
    UINT32 index = 0;
    UINT16 msk;
    INT32  diff;
    UINT32 base;

    for (index = 0; index < (LTM14_CURVE_LUT_SIZE - 1); index++)
    {
        pTableIn[index + 1] = IQSettingUtils::MinINT32(pTableIn[index + 1], (1 << (10 + e)) - 1);
        pTableIn[index]     = IQSettingUtils::MinINT32(pTableIn[index], (1 << (10 + e)) - 1);
        diff                = pTableIn[index + 1] - pTableIn[index];
        base                = pTableIn[index];

        if (0 != (diff >> 15))
        {
            diff |= (1 << (9 + e));
        }
        msk              = ((1 << (10 + e)) - 1);
        pTableOut[index] = ((diff & msk) << (10 + e)) | (base & msk);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief Reverse Gamma of 64-entry unsigned 1d LUT
/// Pack1D64EntryUnsignedHLUT
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static void Pack1D64EntryUnsignedHLUT(
    INT32* pTableIn,
    INT32* pTableOut,
    UINT16 e)
{
    UINT32 index = 0;
    UINT16 msk;
    INT32  diff;
    UINT32 base;

    for (index = 0; index < LTM14_GAMMA_LUT_SIZE; index++)
    {
        pTableIn[index + 1] = IQSettingUtils::MinINT32(pTableIn[index + 1], (1 << (12 + e)) - 1);
        pTableIn[index]     = IQSettingUtils::MinINT32(pTableIn[index], (1 << (12 + e)) - 1);
        diff                = pTableIn[index + 1] - pTableIn[index];
        base                = pTableIn[index];

        if (0 != (diff >> 15))
        {
            diff |= (1 << (11 + e));
        }
        msk = ((1 << (12 + e)) - 1);
        pTableOut[index] = ((diff & msk) << (12 + e)) | (base & msk);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// LTM14Setting::InverseGamma
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static VOID InverseGamma(
    INT32*                pIn,
    INT32*                pOut,
    const LTM14InputData* pInput)
{
    const UINT32 e               = 2; // (Word16u)(bitwidthOut - bitwidth); Variable shift factor
    const FLOAT  gammaMaxValue8  = LTM14_GAMMA_MAX_VALUE * 8;
    const UINT32 innerIterations = 1 << (4 + e);
    const UINT32 outerIterations = 4096 >> (4 + e);
    const UINT32 e2Shift         = 2 + e;

    INT32  gammaHex[LTM14_GAMMA_LUT_SIZE];
    INT32  igamma[LTM14_GAMMA_LUT_SIZE + 1];
    INT32  gammaTemp[LTM14_GAMMA_LUT_SIZE + 1];
    INT32  minCmp;
    INT32  diffMin;
    INT32  entry;
    INT32  base;
    INT32  delta;
    INT32  deltak; // delta * k
    INT32  diff32;
    UINT32 i;
    UINT32 j;
    UINT32 k;
    UINT32 jInnerIterations; // j * innerIterations
    UINT32 indexMin = 0;

    // If the input gamma is the same as the previous request, don't recalculate, just return the previous inverse gamma
    if (IQSettingUtils::Memcmp(pInput->pGammaPrev, pIn, sizeof(INT32) * LTM14_GAMMA_LUT_SIZE) == 0)
    {
        IQSettingUtils::Memcpy(pOut, pInput->pIGammaPrev, sizeof(INT32) * LTM14_GAMMA_LUT_SIZE);
    }
    else
    {
        // Save the current input gamma so the next request can check if the same input gamma is used
        IQSettingUtils::Memcpy(pInput->pGammaPrev, pIn, sizeof(INT32) * LTM14_GAMMA_LUT_SIZE);

        for (i = 0; i < LTM14_GAMMA_LUT_SIZE; i++)
        {
            // Convert to [0, GAMMA_MAX] range
            gammaTemp[i] =
                static_cast<INT32>(IQSettingUtils::ClampINT32(pIn[i] * 16,
                                                              static_cast<INT32>(LTM14_GAMMA_MIN_VALUE),
                                                              static_cast<INT32>(LTM14_GAMMA_MAX_VALUE * 4)));
        }
        gammaTemp[LTM14_GAMMA_LUT_SIZE] = LTM14_GAMMA_MAX;

        Pack1D64EntryUnsignedHLUT(gammaTemp, gammaHex, e);

        for (i = 0; i < LTM14_GAMMA_LUT_SIZE + 1; i++)
        {
            minCmp           = IQSettingUtils::MinINT32(i * 512, static_cast<INT32>(gammaMaxValue8));
            diffMin          = 32768; // 4096 * 8
            jInnerIterations = 0;

            // The previous 4096 iteration inner loop is being replaced by two nested 64 iteration loops
            // This allows us to pull almost half of the operations out of the innermost loop
            for (j = 0; j < outerIterations; j++)
            {
                entry  = gammaHex[j];
                base   = static_cast<INT16>(entry & ((1 << (12 + e)) - 1));
                delta  = static_cast<INT16>((entry >> (12 + e)) & ((1 << (12 + e)) - 1));
                delta  = static_cast<INT16>(static_cast<UINT16>(delta) << (4 - e)) >> (4 - e);  // 11sQ6: duplicate sign bits
                base   = base << 1;  // Cshift_u(g_base, 12+e, 1, 13+e, &cp->shift_u[0]);
                deltak = 0;

                for (k = 0; k < innerIterations; k++)
                {
                    // (INT32)Cmult_su(delta, (12+e), offset, 4+e, 16+2*e, &cp->mult_su[0]);
                    // Cshift_s(tmp, 16+2*e, -(2+e), 14+e, &cp->shift_s[0]);
                    // Cadd_su(tmp, 14+e, 1, 2, 14+e, &cp->add_su[0]);
                    // Cshift_s(tmp, 14+e, -1, 13+e, &cp->shift_s[0]);
                    // (UINT16)Cadd_su((INT32)tmp, 13+e, (INT32)g_base, 13+e, 13+e, &cp->add_su[0]);
                    diff32 = IQSettingUtils::AbsoluteINT(static_cast<INT32>((((deltak >> e2Shift) + 1) >> 1) + base) - minCmp);

                    if (diff32 < diffMin)
                    {
                        diffMin  = diff32;
                        indexMin = jInnerIterations + k;
                    }

                    deltak += delta;
                }
                jInnerIterations += innerIterations;
            }
            igamma[i] = indexMin * 4; // 14 in
        }

        Pack1D64EntryUnsignedHLUT(igamma, pOut, e);

        // Save the inverse gamma to reuse if the next request has the same input gamma
        IQSettingUtils::Memcpy(pInput->pIGammaPrev, pOut, sizeof(INT32) * LTM14_GAMMA_LUT_SIZE);
    }
}

// temporary function to check the gamma received when reverse gamma is enable
static VOID PrintGamma(INT32 gamma[], INT32 size)
{
    CAMX_UNREFERENCED_PARAM(size);

    CAMX_LOG_VERBOSE(CamxLogGroupPProc, "The gamma input for LTM is:");
    CAMX_LOG_VERBOSE(CamxLogGroupPProc, "0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x",
        gamma[0], gamma[1], gamma[2], gamma[3], gamma[4], gamma[5], gamma[6], gamma[7]);
    CAMX_LOG_VERBOSE(CamxLogGroupPProc, "0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x",
        gamma[8], gamma[9], gamma[10], gamma[11], gamma[12], gamma[13], gamma[14], gamma[15]);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// AttenuateCurves
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static VOID AttenuateCurves(
    ltm_1_4_0::ltm14_rgn_dataType* pOutput,
    const LTM14InputData*          pInputData)
{
    FLOAT deltaIndex = 0.0f;
    FLOAT exposureRatio = 1.0f;

    deltaIndex = IQSettingUtils::AbsoluteFLOAT(pInputData->exposureIndex - pInputData->prevExposureIndex);

    if (pOutput->exp_atten_end < deltaIndex)
    {
        deltaIndex = pOutput->exp_atten_end;
    }

    if (pOutput->exp_atten_start > deltaIndex)
    {
        deltaIndex = pOutput->exp_atten_start;
    }

    if (pOutput->exp_atten_end != pOutput->exp_atten_start)
    {
        exposureRatio = (pOutput->exp_atten_end - deltaIndex) / (pOutput->exp_atten_end - pOutput->exp_atten_start);
        exposureRatio = IQSettingUtils::ClampFLOAT(exposureRatio, 0.0f, 1.0f);
    }

    for (UINT index = 0; index < LTM14_SCALE_LUT_SIZE; index++)
    {
        pOutput->ltm_scale_tab.ltm_scale[index] = pOutput->ltm_scale_tab.ltm_scale[index] * exposureRatio;
    }

    for (UINT index = 0; index < LTM14_LCE_SCALE_LUT_SIZE; index++)
    {
        pOutput->lce_scale_pos_tab.lce_scale_pos[index] = pOutput->lce_scale_pos_tab.lce_scale_pos[index] * exposureRatio;
        pOutput->lce_scale_neg_tab.lce_scale_neg[index] = pOutput->lce_scale_neg_tab.lce_scale_neg[index] * exposureRatio;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// DarkBrightRegionTreatment
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static VOID DarkBrightRegionTreatment(ltm_1_4_0::ltm14_rgn_dataType * pOutput)
{
    LTM14DarkBrightRegionState state = LTM14DarkBrightRegionState::LTM14RegionIndexInit;
    UINT32                   darkIndex = 0;
    UINT32                   brightIndex = 0;

    if (pOutput->ltm_scale_tab.ltm_scale[0] >= 0.0f)
    {
        darkIndex = LTM14_SCALE_LUT_SIZE;
        brightIndex = LTM14_SCALE_LUT_SIZE;
    }

    for (UINT index = 0; index < LTM14_SCALE_LUT_SIZE; index++)
    {
        if (LTM14DarkBrightRegionState::LTM14BrightRegion == state)
        {
            break;
        }
        switch (state)
        {
            case LTM14DarkBrightRegionState::LTM14RegionIndexInit:
                if (0.0f == pOutput->ltm_scale_tab.ltm_scale[index])
                {
                    darkIndex = index;
                    state = LTM14DarkBrightRegionState::LTM14DarkBoostRegion;
                }
                break;
            case LTM14DarkBrightRegionState::LTM14DarkBoostRegion:
                if (0.0f != pOutput->ltm_scale_tab.ltm_scale[index])
                {
                    brightIndex = index;
                    state = LTM14DarkBrightRegionState::LTM14BrightRegion;
                }
                break;
            default:
                break;
        }
    }

    for (UINT index = 0; index < LTM14_SCALE_LUT_SIZE; index++)
    {
        if (darkIndex > index)
        {
            pOutput->ltm_scale_tab.ltm_scale[index] = pOutput->ltm_scale_tab.ltm_scale[index] * pOutput->dark_boost;
        }
        else if (brightIndex <= index)
        {
            pOutput->ltm_scale_tab.ltm_scale[index] = pOutput->ltm_scale_tab.ltm_scale[index] * pOutput->bright_suppress;
        }
    }
}

VOID AutoManualCalculate(
    ltm_1_4_0::ltm14_rgn_dataType*          pOutput,
    const LTM14InputData*                   pInputData,
    ltm_1_4_0::chromatix_ltm14_reserveType* pReserveData)
{
    FLOAT darkRange   = 0.0f;
    FLOAT brightRange = 0.0f;
    FLOAT darkMax     = 0.0f;
    FLOAT brightMax   = 0.0f;
    FLOAT darkCap     = 0.0f;
    FLOAT brightCap   = 0.0f;
    FLOAT darkGamma   = 0.0f;
    FLOAT brightGamma = 0.0f;

    if (pReserveData->ltm_scale_manual_curve_enable)
    {
        DarkBrightRegionTreatment(pOutput);
    }
    else
    {
        darkGamma   = IQSettingUtils::ClampFLOAT(pOutput->dark_gamma, 1.0f, 5.0f);
        brightGamma = IQSettingUtils::ClampFLOAT(pOutput->bright_gamma, 1.0f, 5.0f);
        darkRange   = IQSettingUtils::ClampFLOAT(pOutput->dark_range, 0.01f, 1.0f);
        brightRange = IQSettingUtils::ClampFLOAT(pOutput->bright_range, 0.01f, 1.0f);
        darkMax     = IQSettingUtils::ClampFLOAT(pOutput->dark_max, 0.0f, 8.0f) * 1024.0f;
        brightMax   = IQSettingUtils::ClampFLOAT(pOutput->bright_max, 0.0f, 4.0f) * 1024.0f;

        darkCap     = 0.9f * darkMax;
        brightCap   = 0.9f * brightMax;
        darkRange   = (LTM14_SCALE_LUT_SIZE - 1) * darkRange;
        brightRange = (LTM14_SCALE_LUT_SIZE - 1) * brightRange;

        for (UINT index = 0; index < LTM14_SCALE_LUT_SIZE; index++)
        {
            if (darkRange >= index)
            {
                pOutput->ltm_scale_tab.ltm_scale[index] =
                    darkMax * static_cast<FLOAT>(pow((darkRange - static_cast<FLOAT>(index)) / darkRange, darkGamma));
            }
            else if ((LTM14_SCALE_LUT_SIZE - brightRange) <= index)
            {
                pOutput->ltm_scale_tab.ltm_scale[index] =
                    -(brightMax *
                    static_cast<FLOAT>(pow((brightRange - static_cast<FLOAT>(LTM14_SCALE_LUT_SIZE - index)) / brightRange,
                                           brightGamma)));
            }
            else
            {
                pOutput->ltm_scale_tab.ltm_scale[index] = 0.0f;
            }

            if (pOutput->ltm_scale_tab.ltm_scale[index] > darkCap)
            {
                pOutput->ltm_scale_tab.ltm_scale[index] = darkCap;
            }

            if (pOutput->ltm_scale_tab.ltm_scale[index] < -brightCap)
            {
                pOutput->ltm_scale_tab.ltm_scale[index] = -brightCap;
            }
        }
    }

    for (UINT index = 0; index < LTM14_SCALE_LUT_SIZE; index++)
    {
        pOutput->ltm_scale_tab.ltm_scale[index] = pOutput->ltm_scale_tab.ltm_scale[index] * pOutput->ltm_strength;
    }

    for (UINT index = 0; index < LTM14_LCE_SCALE_LUT_SIZE; index++)
    {
        pOutput->lce_scale_pos_tab.lce_scale_pos[index] =
            pOutput->lce_scale_pos_tab.lce_scale_pos[index] * pOutput->lce_strength;
        pOutput->lce_scale_neg_tab.lce_scale_neg[index] =
            pOutput->lce_scale_neg_tab.lce_scale_neg[index] * pOutput->lce_strength;
    }
    AttenuateCurves(pOutput, pInputData);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// LTM14Setting::CalculateHWSetting
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL LTM14Setting::CalculateHWSetting(
    LTM14InputData*                                       pInput,
    ltm_1_4_0::ltm14_rgn_dataType*                        pData,
    ltm_1_4_0::chromatix_ltm14_reserveType*               pReserveData,
    ltm_1_4_0::chromatix_ltm14Type::enable_sectionStruct* pModuleEnable,
    VOID*                                                 pOutput)
{
    BOOL  result = TRUE;

    if ((NULL != pInput) &&
        (NULL != pData)  &&
        (NULL != pReserveData)  &&
        (NULL != pModuleEnable) &&
        (NULL != pOutput))
    {
        UINT16 e      = 2;  // Variable shift factor as per LTM HPG
        UINT16 t      = static_cast<UINT16>((e <= 2) ? 0 : 2);
        UINT16 fac    = 0;
        FLOAT  hRatio = 0;
        FLOAT  vRatio = 0;
        INT32  gamma[LTM14_GAMMA_LUT_SIZE];
        UINT16 i;
        UINT16 width;
        UINT16 height;
        UINT16 dsWidth;
        UINT16 dsHeight;
        UINT8  q2;
        UINT16 hSkip;
        UINT16 vSkip;
        UINT16 tmp16;
        INT16  ltmScale[LTM14_SCALE_LUT_SIZE];
        INT16  lceScalePositive[LTM14_LCE_SCALE_LUT_SIZE];
        INT16  lceScaleNegative[LTM14_LCE_SCALE_LUT_SIZE];
        INT32  laCurve[LTM14_CURVE_LUT_SIZE];
        INT32  ltmCurve[LTM14_CURVE_LUT_SIZE];
        INT32  tmp;
        float denominator = 1.0f;

        LTM14UnpackedField* pUnpackedField      = static_cast<LTM14UnpackedField*>(pOutput);

        AutoManualCalculate(pData, pInput, pReserveData);

        pUnpackedField->enable                  = static_cast<UINT16>(pModuleEnable->ltm_enable);

        pUnpackedField->c1                      = static_cast<UINT16>(IQSettingUtils::ClampINT32(
                                                                      IQSettingUtils::RoundFLOAT(pReserveData->c1 * (1 << 6)),
                                                                      LTM14_RGB2Y_CFG_1_MIN,
                                                                      LTM14_RGB2Y_CFG_1_MAX));
        pUnpackedField->c2                      = static_cast<UINT16>(IQSettingUtils::ClampINT32(
                                                                      IQSettingUtils::RoundFLOAT(pReserveData->c2 * (1 << 6)),
                                                                      LTM14_RGB2Y_CFG_1_MIN,
                                                                      LTM14_RGB2Y_CFG_1_MAX));
        pUnpackedField->c3                      = static_cast<UINT16>(IQSettingUtils::ClampINT32(
                                                                      IQSettingUtils::RoundFLOAT(pReserveData->c3 * (1 << 6)),
                                                                      LTM14_RGB2Y_CFG_1_MIN,
                                                                      LTM14_RGB2Y_CFG_1_MAX));
        pUnpackedField->c4                      = static_cast<UINT16>(IQSettingUtils::ClampINT32(
                                                                      IQSettingUtils::RoundFLOAT(pReserveData->c4 * (1 << 6)),
                                                                      LTM14_RGB2Y_CFG_2_MIN,
                                                                      LTM14_RGB2Y_CFG_2_MAX));
        pUnpackedField->bin_init_cnt            = 0;
        // Enable Data Collection always or else striping libraray will fail
        pUnpackedField->data_collect_en         =
            static_cast<UINT16>(IQSettingUtils::ClampUINT32(pReserveData->data_collect_en,
                                                            LTM14_ONE_BIT_MIN, LTM14_ONE_BIT_MAX));
        pUnpackedField->img_process_en          =
            static_cast<UINT16>(IQSettingUtils::ClampUINT32(pReserveData->img_process_en,
                                                            LTM14_ONE_BIT_MIN, LTM14_ONE_BIT_MAX));
        pUnpackedField->dc_3dtable_avg_pong_sel = 0;
        pUnpackedField->dc_3d_sum_clear         = 1;
        pUnpackedField->dc_init_cellnum_x       = 0;
        pUnpackedField->dc_init_dx              = 0;
        pUnpackedField->debug_out_sel           = 0;
        pUnpackedField->y_ratio_max             = static_cast<UINT16>(IQSettingUtils::ClampINT32(
                                                                      static_cast<INT32>(pData->y_ratio_max),
                                                                      LTM14_Y_RATIO_MAX_MIN,
                                                                      LTM14_Y_RATIO_MAX_MAX));

        width                                   = static_cast<UINT16>(pInput->imageWidth);
        height                                  = static_cast<UINT16>(pInput->imageHeight);
        pUnpackedField->ds_horizontal_skip_cnt  = 0;
        pUnpackedField->ds_vertical_skip_cnt    = 0;
        hSkip                                   = 0;
        vSkip                                   = 0;
        // 0 - ds_size=288x216, cell_w/h=24; 1 - ds_size=144x108, cell_w/h=12; 2 - ds_size=72x54, cell_w/h=6;
        // Down scaling factor
        pUnpackedField->ds_fac                  = 2;

        for (fac = 0; fac < 3; fac++)
        {
            if ((static_cast<UINT32>(width - hSkip)  >= static_cast<UINT32>(LTM_IMG_WIDTH_DS_V14 >> fac)) &&
                (static_cast<UINT32>(height - vSkip) >= static_cast<UINT32>(LTM_IMG_HEIGHT_DS_V14 >> fac)))
            {
                pUnpackedField->ds_fac = fac;
                break;
            }
        }

        // Striping  requires DC to be 1 when module enable.
        if ((width < (LTM_IMG_WIDTH_DS_V14 >> pUnpackedField->ds_fac)) ||
            (height < (LTM_IMG_HEIGHT_DS_V14 >> pUnpackedField->ds_fac)))
        {
            CAMX_LOG_WARN(CamxLogGroupIQMod, "Assumming full frame, input image is too small for LTM_DC. Turning off DC/IP");
            // pUnpackedField->data_collect_en = 0;
            pUnpackedField->img_process_en = 0;
        }
        // down-scaling
        dsWidth                                 = (LTM_IMG_WIDTH_DS_V14 >> pUnpackedField->ds_fac);
        dsHeight                                = (LTM_IMG_HEIGHT_DS_V14 >> pUnpackedField->ds_fac);

        // n means n+1 from 8994, ds_cropped_width w/o skip. aka ds_input_width
        pUnpackedField->scale_in_cfg.input_h    = static_cast<UINT16>(width - hSkip - 1);
        // n means n+1 from 8994, ds_cropped_height w/o skip. aka ds_input_height
        pUnpackedField->scale_in_cfg.input_v    = static_cast<UINT16>(height - vSkip - 1);

        pUnpackedField->scale_in_cfg.pre_crop_en = 0;  // HW doesn't have pre-crop

        /// @todo (CAMX-1234) to get the pre-crop parameters
        pUnpackedField->scale_in_cfg.first_pixel = static_cast<UINT16>(pInput->first_pixel);
        pUnpackedField->scale_in_cfg.last_pixel  = static_cast<UINT16>(pInput->last_pixel);
        pUnpackedField->scale_in_cfg.first_line  = static_cast<UINT16>(pInput->first_line);
        pUnpackedField->scale_in_cfg.last_line   = static_cast<UINT16>(pInput->last_line);

        pUnpackedField->scale_in_cfg.drop_first_output_h = 0; // @todo (CAMX-1234)
        pUnpackedField->scale_in_cfg.drop_first_output_v = 0; // @todo (CAMX-1234)

        if (0 != dsWidth)
        {
            hRatio = static_cast<FLOAT>(width - hSkip) / static_cast<FLOAT>(dsWidth);
        }

        if (hRatio > LTM14_RATIO_64)
        {
            q2 = 0;
        }
        else if (hRatio > LTM14_RATIO_32)
        {
            q2 = 1;
        }
        else if (hRatio > LTM14_RATIO_16)
        {
            q2 = 2;
        }
        else
        {
            q2 = 3;
        }

        pUnpackedField->scale_in_cfg.horizontal_interp_reso = q2;
        // phase_step in DS2.0. aka ds_horizontal_mult_factor
        pUnpackedField->scale_in_cfg.phase_step_h           = static_cast<UINT32>(
                                                              static_cast<FLOAT>(1 << (LTM_SCALER_PHASE_ACCUM_Q_BITS_V14)) *
                                                              hRatio);
        pUnpackedField->scale_in_cfg.phase_init_h           = 0;        // aka ds_horizontal_phase_init
        pUnpackedField->scale_in_cfg.early_termination_h    = 0;        // not in use in DS2.0, borrowed as early_termination
        pUnpackedField->scale_in_cfg.h_en                   = static_cast<UINT16>(
            (static_cast<INT32>(width - hSkip) > static_cast<INT32>(dsWidth)) ? 1 : 0);
        pUnpackedField->scale_in_cfg.rounding_option_h      = 0;

        if (0 != dsHeight)
        {
            vRatio = static_cast<FLOAT>(height - vSkip) / static_cast<FLOAT>(dsHeight);
        }

        if (vRatio > LTM14_RATIO_64)
        {
            q2 = 0;
        }
        else if (vRatio > LTM14_RATIO_32)
        {
            q2 = 1;
        }
        else if (vRatio > LTM14_RATIO_16)
        {
            q2 = 2;
        }
        else
        {
            q2 = 3;
        }

        pUnpackedField->scale_in_cfg.vertical_interp_reso = q2;
        // phase_step in DS2.0.
        pUnpackedField->scale_in_cfg.phase_step_v         = static_cast<UINT32>(
                                                            static_cast<FLOAT>(1 << (LTM_SCALER_PHASE_ACCUM_Q_BITS_V14)) *
                                                            vRatio);
        pUnpackedField->scale_in_cfg.phase_init_v         = 0;
        pUnpackedField->scale_in_cfg.early_termination_v  = 0;          // not in use in DS 2.0, borrowed as early_termination
        pUnpackedField->scale_in_cfg.v_en                 = static_cast<UINT16>(
            (static_cast<INT32>(height - vSkip) > static_cast<INT32>(dsHeight)) ? 1 : 0);
        pUnpackedField->scale_in_cfg.rounding_option_v    = 0;
        pUnpackedField->scale_in_cfg.enable               = pUnpackedField->scale_in_cfg.h_en |
                                                            pUnpackedField->scale_in_cfg.v_en;
        pUnpackedField->dc_xstart                         = 0;              // dc post-cropping
        pUnpackedField->dc_xend                           = dsWidth - 1;    // dc post-cropping

        // ip: default cell width, the last cell col may have different width
        pUnpackedField->ip_cellwidth                      = static_cast<UINT16>(
                                                            (IQSettingUtils::MaxUINT16(width, static_cast<UINT16>(320)) +
                                                                             LTM_LUT3D_W_V14 - 2) / (LTM_LUT3D_W_V14 - 1));
        // default cell height, the last cell row may have different height
        pUnpackedField->ip_cellheight                     = static_cast<UINT16>(
                                                            (IQSettingUtils::MaxUINT16(height, static_cast<UINT16>(240)) +
                                                                             LTM_LUT3D_H_V14 - 2) / (LTM_LUT3D_H_V14 - 1));

        UINT16 stripeStart = 0; // don't need the information for target code.
        if (0 != pUnpackedField->ip_cellwidth)
        {
            pUnpackedField->ip_inv_cellwidth  = (((1 << (16 + t)) / pUnpackedField->ip_cellwidth) & 0x3fff);
            pUnpackedField->ip_init_cellnum_x = stripeStart / pUnpackedField->ip_cellwidth;
            pUnpackedField->ip_init_dx        = stripeStart % pUnpackedField->ip_cellwidth;
        }

        if (0 != pUnpackedField->ip_cellheight)
        {
            pUnpackedField->ip_inv_cellheight = (((1 << (16 + t)) / pUnpackedField->ip_cellheight) & 0x3fff);
            pUnpackedField->ip_init_cellnum_y = stripeStart / pUnpackedField->ip_cellheight;
            pUnpackedField->ip_init_dy        = stripeStart % pUnpackedField->ip_cellheight;
        }

        pUnpackedField->ip_init_px                        = pUnpackedField->ip_init_dx*pUnpackedField->ip_inv_cellwidth;

        pUnpackedField->ip_init_py                        = pUnpackedField->ip_init_dy*pUnpackedField->ip_inv_cellheight;
        pUnpackedField->ip_3dtable_avg_pong_sel           = 0;  // Only valid upon SW override


        pUnpackedField->dc_conv_start_cell_x              = 0;  // convolution start cell, 0-12
        pUnpackedField->dc_conv_end_cell_x                = 12; // convolution end cell, 0-12

        pUnpackedField->igamma_en                         = static_cast<UINT16>(
                                                            IQSettingUtils::ClampUINT32(pReserveData->igamma_en,
                                                                                        LTM14_ONE_BIT_MIN,
                                                                                        LTM14_ONE_BIT_MAX));
        pUnpackedField->la_en                             = static_cast<UINT16>(
                                                            IQSettingUtils::ClampUINT32(pReserveData->la_en,
                                                                                        LTM14_ONE_BIT_MIN,
                                                                                        LTM14_ONE_BIT_MAX));

        INT32 wtArrayIndex = 0;
        INT32 wtArraySize  = CAMX_ARRAY_SIZE(pReserveData->wt_tab.wt);

        for (i = 0; i < LTM14_WEIGHT_LUT_SIZE; i++)
        {
            wtArrayIndex = (i - (i % 2)) * 2 + (1 - (i % 2));

            if ((0 <= wtArrayIndex) && (wtArrayIndex < (wtArraySize - 2)))
            {
                UINT16 wtLSB = static_cast<UINT16>(pReserveData->wt_tab.wt[wtArrayIndex]);
                UINT16 wtMSB = static_cast<UINT16>(pReserveData->wt_tab.wt[wtArrayIndex + 2]);

                pUnpackedField->wt.pLUTTable[i] = (wtMSB << 8) | wtLSB;
            }
        }

        for (i = 0; i < MASK_FILTER_KERNEL_SIZE_V14; i++)
        {
            tmp = pReserveData->mask_filter_kernel_tab.mask_filter_kernel[i];   // 4u, 5x5 mask filtering
            pUnpackedField->mask_filter_kernel[i]         = static_cast<UINT16>(
                                                            IQSettingUtils::ClampINT32(tmp,
                                                                                       LTM14_MASK_FILTER_KERNEL_MIN,
                                                                                       LTM14_MASK_FILTER_KERNEL_MAX));
        }
        pUnpackedField->mask_filter_kernel[0]             = IQSettingUtils::ClampUINT16(pUnpackedField->mask_filter_kernel[0],
                                                                                        1,
                                                                                        6);
        pUnpackedField->mask_filter_kernel[1]             = IQSettingUtils::ClampUINT16(pUnpackedField->mask_filter_kernel[1],
                                                                                        0,
                                                                                        5);
        pUnpackedField->mask_filter_kernel[2]             = IQSettingUtils::ClampUINT16(pUnpackedField->mask_filter_kernel[2],
                                                                                        0,
                                                                                        4);
        pUnpackedField->mask_filter_kernel[3]             = IQSettingUtils::ClampUINT16(pUnpackedField->mask_filter_kernel[3],
                                                                                        0,
                                                                                        3);
        pUnpackedField->mask_filter_kernel[4]             = IQSettingUtils::ClampUINT16(pUnpackedField->mask_filter_kernel[4],
                                                                                        0,
                                                                                        2);
        pUnpackedField->mask_filter_kernel[5]             = IQSettingUtils::ClampUINT16(pUnpackedField->mask_filter_kernel[5],
                                                                                        0,
                                                                                        1);
        pUnpackedField->mask_filter_scale                 = 0;
        pUnpackedField->mask_filter_shift                 = 0;
        tmp16                                             = (1 << (pUnpackedField->mask_filter_kernel[0] - 1))
                                                            + 4 * (1 << (pUnpackedField->mask_filter_kernel[1] - 1))
                                                            + 4 * (1 << (pUnpackedField->mask_filter_kernel[2] - 1))
                                                            + 4 * (1 << (pUnpackedField->mask_filter_kernel[3] - 1))
                                                            + 8 * (1 << (pUnpackedField->mask_filter_kernel[4] - 1))
                                                            + 4 * (1 << (pUnpackedField->mask_filter_kernel[5] - 1));
        for (INT16 j = 16; j >= 0; j--)
        {
            pUnpackedField->mask_filter_scale = (1 << j) / tmp16;   // could start from 19 to increase precision
            pUnpackedField->mask_filter_shift = static_cast<UINT16>(j);

            if (pUnpackedField->mask_filter_scale < ((1 << 12) - 1))
            {
                break;
            }
        }
        pUnpackedField->mask_filter_scale                 = static_cast<UINT16>(
                                                            IQSettingUtils::ClampUINT32(pUnpackedField->mask_filter_scale,
                                                                                        LTM14_MASK_FILTER_SCALE_MIN,
                                                                                        LTM14_MASK_FILTER_SCALE_MAX));
        pUnpackedField->mask_filter_shift                 = static_cast<UINT16>(
                                                            IQSettingUtils::ClampUINT32(pUnpackedField->mask_filter_shift,
                                                                                        LTM14_MASK_FILTER_SHIFT_MIN,
                                                                                        LTM14_MASK_FILTER_SHIFT_MAX));

        pUnpackedField->mask_filter_en                    = static_cast<UINT16>(
                                                            IQSettingUtils::ClampUINT32(pReserveData->mask_filter_en,
                                                                                        LTM14_ONE_BIT_MIN,
                                                                                        LTM14_ONE_BIT_MAX));
        for (i = 0; i < LTM14_SCALE_LUT_SIZE; i++)
        {
            ltmScale[i] = static_cast<INT16>(IQSettingUtils::RoundFLOAT(pData->ltm_scale_tab.ltm_scale[i] / 4.0f));

            laCurve[i]  = IQSettingUtils::RoundFLOAT(pData->la_curve_tab.la_curve[i]);
            ltmCurve[i] = pReserveData->ltm_curve_tab.ltm_curve[i];
        }

        for (i = 0; i < LTM14_LCE_SCALE_LUT_SIZE; i++)
        {
            lceScalePositive[i] = static_cast<INT16>(IQSettingUtils::ClampINT32(
                                                     IQSettingUtils::RoundFLOAT(pData->lce_scale_pos_tab.lce_scale_pos[i]/4.0f),
                                                     LTM14_LCE_SCALE_POS_MIN,
                                                     LTM14_LCE_SCALE_POS_MAX));
            lceScaleNegative[i] = static_cast<INT16>(IQSettingUtils::ClampINT32(
                                                     IQSettingUtils::RoundFLOAT(pData->lce_scale_neg_tab.lce_scale_neg[i]/4.0f),
                                                     LTM14_LCE_SCALE_NEG_MIN,
                                                     LTM14_LCE_SCALE_NEG_MAX));
        }

        // Inverse to the input IFE/BPS gamma
        for (i = 0; i < LTM14_GAMMA_LUT_SIZE; i++)
        {
            gamma[i] = IQSettingUtils::RoundFLOAT(pInput->gammaOutput[i]);
        }

        PrintGamma(gamma, 64);

        InverseGamma(gamma, pUnpackedField->igamma64.pLUTTable, pInput);

        ///< if ADRC enable, override the LTM Curve.
        if (pInput->pAdrcInputData &&
            pInput->pAdrcInputData->ltmEnable)
        {

            FLOAT tmcOutLtmGainCurve[LTM14_LUT_SIZE + 1];
            FLOAT tmcOutLtmCurve[LTM14_LUT_SIZE + 1];
            FLOAT tmcOutLtmScale[LTM14_LUT_SIZE + 1];

            for (i = 0; i < LTM14_GAMMA_LUT_SIZE; i++)
            {
                inverseGammaIn_v14[i] = static_cast<FLOAT>((pUnpackedField->igamma64.pLUTTable[i] & 0x3FFF) / 4);
            }
            inverseGammaIn_v14[LTM14_GAMMA_LUT_SIZE] = 4095.0f;
            FLOAT adrc_ltm_mix_rate                  = 0.0f;

            if ((SWTMCVersion::TMC11 == pInput->pAdrcInputData->version) ||
                (SWTMCVersion::TMC12 == pInput->pAdrcInputData->version))
            {
                FLOAT* pKneeX;
                FLOAT* pKneeY;

                if (SWTMCVersion::TMC11 == pInput->pAdrcInputData->version)
                {
                    pKneeX = &pInput->pAdrcInputData->kneePoints.KneePointsTMC11.kneeX[0];
                    pKneeY = &pInput->pAdrcInputData->kneePoints.KneePointsTMC11.kneeY[0];
                }
                else
                {
                    // For (SWTMCVersion::TMC12 == pInput->pAdrcInputData->version)
                    pKneeX = &pInput->pAdrcInputData->kneePoints.KneePointsTMC12.kneeX[0];
                    pKneeY = &pInput->pAdrcInputData->kneePoints.KneePointsTMC12.kneeY[0];
                }

                adrc_ltm_mix_rate = IQSettingUtils::ClampFLOAT(
                                          2.0f - (pKneeY[1] /
                                                  pKneeX[1]),
                                          0.0f,
                                          1.0f);

                for (i = 0; i < LTM14_GAMMA_LUT_SIZE; i++)
                {
                    inverseGammaInNormalized_v14[i] = (inverseGammaIn_v14[i] + 1) / inverseGammaIn_v14[LTM14_GAMMA_LUT_SIZE];
                }

                inverseGammaInNormalized_v14[LTM14_GAMMA_LUT_SIZE-1] =
                    (inverseGammaInNormalized_v14[LTM14_GAMMA_LUT_SIZE-1] > 1.0f)?
                    1.0f:
                    inverseGammaInNormalized_v14[LTM14_GAMMA_LUT_SIZE-1];

                if (pInput->pAdrcInputData->curveModel == 2)
                {
                    // PCHIP curve
                    IQSettingUtils::PCHIPCurve(pKneeX,
                                               pKneeY,
                                               &pInput->pAdrcInputData->pchipCoeffficient[0],
                                               inverseGammaInNormalized_v14,
                                               tmcOutLtmGainCurve,
                                               LTM14_LUT_SIZE);
                }
                else
                {
                    // Bezier curve
                    IQSettingUtils::BezierCurve(pKneeX,
                                                pKneeY,
                                                inverseGammaInNormalized_v14,
                                                tmcOutLtmGainCurve,
                                                LTM14_LUT_SIZE);
                }

                IQSettingUtils::EnhanceGlobalContrast(inverseGammaInNormalized_v14,
                                                        tmcOutLtmGainCurve,
                                                        pInput->pAdrcInputData,
                                                        pInput->pAdrcInputData->ltmPercentage,
                                                        LTM14_SCALE_Q,
                                                        LTM14_CLAMP_Q);
            }
            else
            {
                // For (SWTMCVersion::TMC10 == pInput->pAdrcInputData->version)
                adrc_ltm_mix_rate  = IQSettingUtils::ClampFLOAT(2.0f - pInput->pAdrcInputData->drcGainDark, 0.0f, 1.0f);
                IQSettingUtils::GainCurveSampling(pInput->pAdrcInputData->kneePoints.KneePointsTMC10.kneeX,
                                                  pInput->pAdrcInputData->kneePoints.KneePointsTMC10.kneeY,
                                                  pInput->pAdrcInputData->coefficient,
                                                  inverseGammaIn_v14,
                                                  tmcOutLtmGainCurve,
                                                  LTM14_LUT_SIZE,
                                                  pInput->pAdrcInputData->ltmPercentage,
                                                  pInput->pAdrcInputData->drcGainDark,
                                                  LTM14_SCALE_Q);
            }

            GenerateAdrcLtmCurves(inverseGammaIn_v14, tmcOutLtmGainCurve, pInput->gammaOutput, tmcOutLtmScale, tmcOutLtmCurve);

            for (i = 0; i < LTM14_LUT_SIZE + 1; i++)
            {
                ltmCurve[i] = IQSettingUtils::RoundFLOAT(
                    IQSettingUtils::InterpolationFloatBilinear((tmcOutLtmCurve[i]),
                                                               static_cast<FLOAT>(pReserveData->ltm_curve_tab.ltm_curve[i]),
                                                               adrc_ltm_mix_rate));

                ltmScale[i] = static_cast<INT16>(IQSettingUtils::MaxFLOAT(ltmScale[i] * adrc_ltm_mix_rate,
                    (tmcOutLtmScale[i])));
            }
        }
        Pack1D64EntryUnsignedLUT(ltmCurve, pUnpackedField->ltm_curve.pLUTTable, e);
        Pack1D64EntrySignedLUT(ltmScale, pUnpackedField->ltm_scale.pLUTTable, e);
        Pack1D16EntrySignedLUT(lceScalePositive, pUnpackedField->lce_scale_pos.pLUTTable, e);
        Pack1D16EntrySignedLUT(lceScaleNegative, pUnpackedField->lce_scale_neg.pLUTTable, e);
        Pack1D64EntryUnsignedLUT(pReserveData->mask_rect_curve_tab.mask_rect_curve,
                                 pUnpackedField->mask_rect_curve.pLUTTable,
                                 e);
        Pack1D64EntryUnsignedLUT(laCurve, pUnpackedField->la_curve.pLUTTable, e);

        pUnpackedField->lce_thd = static_cast<UINT16>(IQSettingUtils::ClampUINT32(pReserveData->lce_thd,
                                                                                  LTM14_LCE_THD_MIN,
                                                                                  LTM14_LCE_THD_MAX));

        for (i = 0; i < static_cast<UINT16>(LTM_LUT3D_H_V14 * LTM_LUT3D_W_V14); i++)
        {
            memset(&pUnpackedField->LUT25b[i].valsum[0], 0, sizeof(UINT16) * LTM_LUT3D_Z_V14);
            memset(&pUnpackedField->LUT25b[i].pixcnt[0], 0, sizeof(UINT16) * LTM_LUT3D_Z_V14);
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
/// LTM14Setting::GenerateAdrcLtmCurves
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void LTM14Setting::GenerateAdrcLtmCurves(
    FLOAT*        pInEqvlnt,
    FLOAT*        pGain,
    FLOAT*        pGamma,
    FLOAT*        pLtmScale,
    FLOAT*        pLtmCurve)
{
    UINT  x1              = 0;
    UINT  x2              = 0;
    FLOAT drcGain         = pGain[0] / (1 << LTM14_SCALE_Q);;
    FLOAT drcGainLTMCurve = 1;
    INT   flatRange       = 0;
    INT   maxIdx          = 0;
    INT   span            = 0;
    FLOAT ltmCurveXStep   = static_cast<FLOAT>((1 << LTM14_BIT_DEPTH) / LTM14_GAMMA_LUT_SIZE);
    FLOAT val1;
    FLOAT val2;
    FLOAT curX;
    FLOAT curY;
    FLOAT ratio;
    FLOAT max;

    FLOAT out_indx[LTM14_LUT_SIZE + 1] = { 0 };
    FLOAT gammaOutEqvlnt = 0;
    FLOAT tempLtmScale[LTM14_SCALE_LUT_SIZE];

    for (INT i = 0; i < LTM14_LUT_SIZE + 1; i++)
    {
        ltmFixedGamma_v14[i] = pGamma[i] * 4;
    }

    for (INT i = 0; i < (LTM14_LUT_SIZE + 1); i++)
    {
        out_indx[i] =
            IQSettingUtils::ClampFLOAT(((pGain[i] / (1 << LTM14_SCALE_Q)) * pInEqvlnt[i]) * LTM14_LUT_SIZE / LTM14_MAX_VALUE,
            0.0f, 64.0f);
    }

    tempLtmScale[0] = (1 << LTM14_SCALE_Q);

    for (INT i = 1; i < (LTM14_LUT_SIZE + 1); i++)
    {
        curX  = ltmCurveXStep * i; ///< ltm index value

        x1    = out_indx[i] * (LTM14_GAMMA_LUT_SIZE / LTM14_LUT_SIZE) >= (LTM14_GAMMA_LUT_SIZE + 1) ?
            LTM14_GAMMA_LUT_SIZE : static_cast<UINT>(out_indx[i]); ///< eqlvnt compensated ltm indx
        x2    = x1 + 1;
        val1  = ltmFixedGamma_v14[x1];
        val2  = static_cast<FLOAT>(x2 >= (LTM14_GAMMA_LUT_SIZE) ? (1 << LTM14_BIT_DEPTH) : (ltmFixedGamma_v14[x2]));
        ratio = (out_indx[i] - x1) / static_cast<FLOAT>(x2 - x1);

        gammaOutEqvlnt = IQSettingUtils::ClampFLOAT(IQSettingUtils::InterpolationFloatBilinear(val1, val2, ratio),
                                                    LTM14_MIN_VALUE,
                                                    LTM14_MAX_VALUE);
        curY = gammaOutEqvlnt;
        tempLtmScale[i] = IQSettingUtils::ClampFLOAT(((1 << LTM14_SCALE_Q) * (curY / curX - 1.0f)),
            LTM14_MIN_VALUE, LTM14_SCALE_MAX_VALUE);
    }

    tempLtmScale[0]  = tempLtmScale[1];
    tempLtmScale[64] = tempLtmScale[63];

    ///< flat at the beginning of MS
    flatRange = static_cast<INT> (floor(LTM14_LUT_SIZE / (drcGain * 3)));

    max = tempLtmScale[LTM14_LUT_SIZE]; // start with last entry
    for (INT i = LTM14_LUT_SIZE; i >= 0; i--)
    {
        if (tempLtmScale[i] < max)
        {
            tempLtmScale[i] = max;
        }
        else
        {
            max = tempLtmScale[i];
        }
    }

    ///< smooth out MS
    for (INT i = 0; i < LTM14_LUT_SIZE / 3; i++)
    {
        if (tempLtmScale[i] == max && i > maxIdx)
        {
            maxIdx = i;
        }
    }

    maxIdx = (maxIdx <= 1) ? 1 : maxIdx;
    for (INT j = 0; j < 3; j++)
    {
        for (INT i = maxIdx + 1; i < LTM14_LUT_SIZE; i++)
        {
            tempLtmScale[i] = (tempLtmScale[i - 1] + tempLtmScale[i] + tempLtmScale[i + 1]) / 3;
        }
    }

    ///< MC
    for (INT i = 0; i < (LTM14_LUT_SIZE + 1); i++)
    {
        pLtmScale[i] = tempLtmScale[i];
        pLtmCurve[i] = LTM14_LUT_SIZE * i * drcGainLTMCurve;
    }

    span = LTM14_LUT_SIZE - flatRange;
    for (INT i = 0; i < span + 1; i++)
    {
        if (span != 0)
        {
            pLtmCurve[i + flatRange] *= (static_cast<FLOAT>(span - i) / static_cast<FLOAT>(span));
        }
        else
        {
            pLtmCurve[i + flatRange] *= 0;
        }
    }

    ///< smooth out MC
    for (INT j = 0; j < 4; j++)
    {
        for (INT i = 1; i < LTM14_LUT_SIZE; i++)
        {
            pLtmCurve[i] = (pLtmCurve[i - 1] + pLtmCurve[i] + pLtmCurve[i + 1]) / 3;
        }
    }

    return;
}
