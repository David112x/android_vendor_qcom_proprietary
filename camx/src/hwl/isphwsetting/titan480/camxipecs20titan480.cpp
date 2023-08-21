////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  camxipecs20titan480.cpp
/// @brief IPECS20Titan480 class implementation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "titan480_ipe.h"
#include "ipecs20setting.h"
#include "camxutils.h"
#include "camxipecs20titan480.h"
#include "camxipechromasuppression20.h"

CAMX_NAMESPACE_BEGIN

CAMX_BEGIN_PACKED

/// @brief: This structure contains chroma enhancement knee point related registers
struct IPEChromaSuppressionKneePoint
{
    IPE_IPE_0_PPS_CLC_CHROMA_SUP_CS_Y_KNEE_PT_LUT_CFG_0 cfg0;  ///< Luma kneepoints 0 and 1 in the luma
    IPE_IPE_0_PPS_CLC_CHROMA_SUP_CS_Y_KNEE_PT_LUT_CFG_1 cfg1;  ///< Luma kneepoints 2 and 3 in the luma
    IPE_IPE_0_PPS_CLC_CHROMA_SUP_CS_Y_KNEE_PT_LUT_CFG_2 cfg2;  ///< Luma kneepoints 4 and 5 in the luma
    IPE_IPE_0_PPS_CLC_CHROMA_SUP_CS_Y_KNEE_PT_LUT_CFG_3 cfg3;  ///< Luma kneepoints 6 and 7 in the luma
    IPE_IPE_0_PPS_CLC_CHROMA_SUP_CS_Y_KNEE_PT_LUT_CFG_4 cfg4;  ///< Luma kneepoints 8 and 9 in the luma
    IPE_IPE_0_PPS_CLC_CHROMA_SUP_CS_Y_KNEE_PT_LUT_CFG_5 cfg5;  ///< Luma kneepoints 10 and 11 in the luma
    IPE_IPE_0_PPS_CLC_CHROMA_SUP_CS_Y_KNEE_PT_LUT_CFG_6 cfg6;  ///< Luma kneepoints 12 and 13 in the luma
    IPE_IPE_0_PPS_CLC_CHROMA_SUP_CS_Y_KNEE_PT_LUT_CFG_7 cfg7;  ///< Luma kneepoints 14 and 15 in the luma
} CAMX_PACKED;

/// @brief: This structure contains chroma enhancement weight related registers
struct IPEChromaSuppressionWeight
{
    IPE_IPE_0_PPS_CLC_CHROMA_SUP_CS_Y_WEIGHT_LUT_CFG_0  cfg0;    ///< Luma suppression ratio weights 0 and 1 in the luma
    IPE_IPE_0_PPS_CLC_CHROMA_SUP_CS_Y_WEIGHT_LUT_CFG_1  cfg1;    ///< Luma suppression ratio weights 2 and 3 in the luma
    IPE_IPE_0_PPS_CLC_CHROMA_SUP_CS_Y_WEIGHT_LUT_CFG_2  cfg2;    ///< Luma suppression ratio weights 4 and 5 in the luma
    IPE_IPE_0_PPS_CLC_CHROMA_SUP_CS_Y_WEIGHT_LUT_CFG_3  cfg3;    ///< Luma suppression ratio weights 6 and 7 in the luma
    IPE_IPE_0_PPS_CLC_CHROMA_SUP_CS_Y_WEIGHT_LUT_CFG_4  cfg4;    ///< Luma suppression ratio weights 8 and 9 in the luma
    IPE_IPE_0_PPS_CLC_CHROMA_SUP_CS_Y_WEIGHT_LUT_CFG_5  cfg5;    ///< Luma suppression ratio weights 10 and 11 in the luma
    IPE_IPE_0_PPS_CLC_CHROMA_SUP_CS_Y_WEIGHT_LUT_CFG_6  cfg6;    ///< Luma suppression ratio weights 12 and 13 in the luma
    IPE_IPE_0_PPS_CLC_CHROMA_SUP_CS_Y_WEIGHT_LUT_CFG_7  cfg7;    ///< Luma suppression ratio weights 14 and 15 in the luma
} CAMX_PACKED;

/// @brief: This structure contains chroma enhancement threshold related registers
struct IPEChromaSuppressionThreshold
{
    IPE_IPE_0_PPS_CLC_CHROMA_SUP_CS_CHROMA_TH_LUT_CFG_0 cfg0;  ///< Chroma threshold 0 and 1 values
    IPE_IPE_0_PPS_CLC_CHROMA_SUP_CS_CHROMA_TH_LUT_CFG_1 cfg1;  ///< Chroma threshold 2 and 3 values
    IPE_IPE_0_PPS_CLC_CHROMA_SUP_CS_CHROMA_TH_LUT_CFG_2 cfg2;  ///< Chroma threshold 4 and 5 values
    IPE_IPE_0_PPS_CLC_CHROMA_SUP_CS_CHROMA_TH_LUT_CFG_3 cfg3;  ///< Chroma threshold 6 and 7 values
    IPE_IPE_0_PPS_CLC_CHROMA_SUP_CS_CHROMA_TH_LUT_CFG_4 cfg4;  ///< Chroma threshold 8 and 9 values
    IPE_IPE_0_PPS_CLC_CHROMA_SUP_CS_CHROMA_TH_LUT_CFG_5 cfg5;  ///< Chroma threshold 10 and 11 values
    IPE_IPE_0_PPS_CLC_CHROMA_SUP_CS_CHROMA_TH_LUT_CFG_6 cfg6;  ///< Chroma threshold 12 and 13 values
    IPE_IPE_0_PPS_CLC_CHROMA_SUP_CS_CHROMA_TH_LUT_CFG_7 cfg7;  ///< Chroma threshold 14 and 15 values
} CAMX_PACKED;

/// @brief: This structure contains chroma enhancement slope related registers
struct IPEChromaSuppressionChromaSlope
{
    IPE_IPE_0_PPS_CLC_CHROMA_SUP_CS_CHROMA_SLP_LUT_CFG_0 cfg0;  ///< Chroma slope 0 and 1 values
    IPE_IPE_0_PPS_CLC_CHROMA_SUP_CS_CHROMA_SLP_LUT_CFG_1 cfg1;  ///< Chroma slope 2 and 3 values
    IPE_IPE_0_PPS_CLC_CHROMA_SUP_CS_CHROMA_SLP_LUT_CFG_2 cfg2;  ///< Chroma slope 4 and 5 values
    IPE_IPE_0_PPS_CLC_CHROMA_SUP_CS_CHROMA_SLP_LUT_CFG_3 cfg3;  ///< Chroma slope 6 and 7 values
    IPE_IPE_0_PPS_CLC_CHROMA_SUP_CS_CHROMA_SLP_LUT_CFG_4 cfg4;  ///< Chroma slope 8 and 9 values
    IPE_IPE_0_PPS_CLC_CHROMA_SUP_CS_CHROMA_SLP_LUT_CFG_5 cfg5;  ///< Chroma slope 10 and 11 values
    IPE_IPE_0_PPS_CLC_CHROMA_SUP_CS_CHROMA_SLP_LUT_CFG_6 cfg6;  ///< Chroma slope 12 and 13 values
    IPE_IPE_0_PPS_CLC_CHROMA_SUP_CS_CHROMA_SLP_LUT_CFG_7 cfg7;  ///< Chroma slope 14 and 15 values
} CAMX_PACKED;

/// @brief: This structure contains chroma enhancement knee inverse related registers
struct IPEChromaSuppressionKneeinverse
{
    IPE_IPE_0_PPS_CLC_CHROMA_SUP_CS_Y_KNEE_INV_LUT_CFG_0 cfg0;  ///< Inverse of luma knee points 0 and 1 values
    IPE_IPE_0_PPS_CLC_CHROMA_SUP_CS_Y_KNEE_INV_LUT_CFG_1 cfg1;  ///< Inverse of luma knee points 2 and 3 values
    IPE_IPE_0_PPS_CLC_CHROMA_SUP_CS_Y_KNEE_INV_LUT_CFG_2 cfg2;  ///< Inverse of luma knee points 4 and 5 values
    IPE_IPE_0_PPS_CLC_CHROMA_SUP_CS_Y_KNEE_INV_LUT_CFG_3 cfg3;  ///< Inverse of luma knee points 6 and 7 values
    IPE_IPE_0_PPS_CLC_CHROMA_SUP_CS_Y_KNEE_INV_LUT_CFG_4 cfg4;  ///< Inverse of luma knee points 8 and 9 values
    IPE_IPE_0_PPS_CLC_CHROMA_SUP_CS_Y_KNEE_INV_LUT_CFG_5 cfg5;  ///< Inverse of luma knee points 10 and 11 values
    IPE_IPE_0_PPS_CLC_CHROMA_SUP_CS_Y_KNEE_INV_LUT_CFG_6 cfg6;  ///< Inverse of luma knee points 12 and 13 values
    IPE_IPE_0_PPS_CLC_CHROMA_SUP_CS_Y_KNEE_INV_LUT_CFG_7 cfg7;  ///< Inverse of luma knee points 14 and 15 values
} CAMX_PACKED;

/// @brief: This structure contains chroma enhancement Q factor related registers
struct IPEChromaSuppressionQRes
{
    IPE_IPE_0_PPS_CLC_CHROMA_SUP_CS_Q_RES_LUMA_CFG      luma;   ///< Number of bits for Fractional part of Luma
    IPE_IPE_0_PPS_CLC_CHROMA_SUP_CS_Q_RES_CHROMA_CFG    chroma; ///< Number of bits for Fractional part of Chroma
} CAMX_PACKED;

/// @brief: This structure contains chroma enhancement registers programmed by software
struct IPEChromaSuppressionRegCmd
{
    IPEChromaSuppressionKneePoint    knee;       ///< Luma kneepoints
    IPEChromaSuppressionWeight       weight;     ///< Luma suppression ratio weights
    IPEChromaSuppressionThreshold    threshold;  ///< Chroma threshold
    IPEChromaSuppressionChromaSlope  slope;      ///< Chroma slope
    IPEChromaSuppressionKneeinverse  inverse;    ///< Inverse of luma knee points
    IPEChromaSuppressionQRes         qRes;       ///<  Number of bits for Fractional part
} CAMX_PACKED;

CAMX_END_PACKED

static const UINT32 IPEChromaSuppressionRegLength = sizeof(IPEChromaSuppressionRegCmd) / RegisterWidthInBytes;
CAMX_STATIC_ASSERT((42 * 4) == sizeof(IPEChromaSuppressionRegCmd));

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IPECS20Titan480::IPECS20Titan480
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
IPECS20Titan480::IPECS20Titan480()
{
    m_pRegCmd = CAMX_CALLOC(sizeof(IPEChromaSuppressionRegCmd));

    if (NULL != m_pRegCmd)
    {
        IPEChromaSuppressionRegCmd* pRegCmd = static_cast<IPEChromaSuppressionRegCmd*>(m_pRegCmd);

        /// @todo (CAMX-729) Consume 3A updates and call common IQ library
        pRegCmd->knee.cfg0.u32All      = 0x00100000;
        pRegCmd->knee.cfg1.u32All      = 0x00300020;
        pRegCmd->knee.cfg2.u32All      = 0x00500040;
        pRegCmd->knee.cfg3.u32All      = 0x01e000f0;
        pRegCmd->knee.cfg4.u32All      = 0x02e002b0;
        pRegCmd->knee.cfg5.u32All      = 0x03400310;
        pRegCmd->knee.cfg6.u32All      = 0x03a00370;
        pRegCmd->knee.cfg7.u32All      = 0x03fc03d0;

        pRegCmd->weight.cfg0.u32All    = 0x04000400;
        pRegCmd->weight.cfg1.u32All    = 0x04000400;
        pRegCmd->weight.cfg2.u32All    = 0x04000400;
        pRegCmd->weight.cfg3.u32All    = 0x04000400;
        pRegCmd->weight.cfg4.u32All    = 0x04000400;
        pRegCmd->weight.cfg5.u32All    = 0x04000400;
        pRegCmd->weight.cfg6.u32All    = 0x04000400;
        pRegCmd->weight.cfg7.u32All    = 0x04000400;

        pRegCmd->threshold.cfg0.u32All = 0x00000000;
        pRegCmd->threshold.cfg1.u32All = 0x00000000;
        pRegCmd->threshold.cfg2.u32All = 0x00000000;
        pRegCmd->threshold.cfg3.u32All = 0x00000000;
        pRegCmd->threshold.cfg4.u32All = 0x00000000;
        pRegCmd->threshold.cfg5.u32All = 0x00000000;
        pRegCmd->threshold.cfg6.u32All = 0x00000000;
        pRegCmd->threshold.cfg7.u32All = 0x00000000;

        pRegCmd->slope.cfg0.u32All     = 0x001d001d;
        pRegCmd->slope.cfg1.u32All     = 0x001d001d;
        pRegCmd->slope.cfg2.u32All     = 0x001d001d;
        pRegCmd->slope.cfg3.u32All     = 0x001d001d;
        pRegCmd->slope.cfg4.u32All     = 0x001d001d;
        pRegCmd->slope.cfg5.u32All     = 0x001d001d;
        pRegCmd->slope.cfg6.u32All     = 0x001d001d;
        pRegCmd->slope.cfg7.u32All     = 0x001d001d;

        pRegCmd->inverse.cfg0.u32All   = 0x02000200;
        pRegCmd->inverse.cfg1.u32All   = 0x02000200;
        pRegCmd->inverse.cfg2.u32All   = 0x00330200;
        pRegCmd->inverse.cfg3.u32All   = 0x00270022;
        pRegCmd->inverse.cfg4.u32All   = 0x00aa00aa;
        pRegCmd->inverse.cfg5.u32All   = 0x00aa00aa;
        pRegCmd->inverse.cfg6.u32All   = 0x00aa00aa;
        pRegCmd->inverse.cfg7.u32All   = 0x000100ba;

        pRegCmd->qRes.luma.u32All      = 0x00000000;
        pRegCmd->qRes.chroma.u32All    = 0x00000000;
    }
    else
    {
        CAMX_LOG_ERROR(CamxLogGroupPProc, "Failed to allocate memory for Cmd Buffer");
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IPECS20Titan480::CreateCmdList
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IPECS20Titan480::CreateCmdList(
    VOID*   pSettingData,
    UINT32* pCSTHWSettingParams)
{
    CamxResult    result     = CamxResultSuccess;
    ISPInputData* pInputData = static_cast<ISPInputData*>(pSettingData);

    CAMX_UNREFERENCED_PARAM(pCSTHWSettingParams);

    if ((NULL != pInputData)                                 &&
        (NULL != pInputData->pipelineIPEData.ppIPECmdBuffer) &&
        (NULL != m_pRegCmd))
    {
        CmdBuffer*  pCmdBuffer = pInputData->pipelineIPEData.ppIPECmdBuffer[CmdBufferPostLTM];

        if (NULL != pCmdBuffer)
        {
            result = PacketBuilder::WriteRegRange(pCmdBuffer,
                                                  regIPE_IPE_0_PPS_CLC_CHROMA_SUP_CS_Y_KNEE_PT_LUT_CFG_0,
                                                  IPEChromaSuppressionRegLength,
                                                  reinterpret_cast<UINT32*>(m_pRegCmd));
        }
        else
        {
            result = CamxResultEInvalidPointer;
            CAMX_LOG_ERROR(CamxLogGroupPProc, "NULL command buffer");
        }
        if (CamxResultSuccess != result)
        {
            CAMX_LOG_ERROR(CamxLogGroupPProc, "Failed to write reg range, result: %d", result);
        }
    }
    else
    {
        result = CamxResultEInvalidArg;
        CAMX_LOG_ERROR(CamxLogGroupPProc, "Invalid Input data or command buffer");
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IPECS20Titan480::PackIQRegisterSetting
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IPECS20Titan480::PackIQRegisterSetting(
    VOID*  pInput,
    VOID*  pOutputVal)
{
    CamxResult               result  = CamxResultSuccess;

    CAMX_UNREFERENCED_PARAM(pOutputVal);

    if ((NULL != pInput) && (NULL != m_pRegCmd))
    {
        IPEChromaSuppressionRegCmd* pRegCmd = static_cast<IPEChromaSuppressionRegCmd*>(m_pRegCmd);
        CS20UnpackedField*          pData   = static_cast<CS20UnpackedField*>(pInput);

        pRegCmd->inverse.cfg0.bitfields.Y_KNEE_INV_LUT_0   = pData->knee_point_inverse_lut[0];
        pRegCmd->inverse.cfg0.bitfields.Y_KNEE_INV_LUT_1   = pData->knee_point_inverse_lut[1];
        pRegCmd->inverse.cfg1.bitfields.Y_KNEE_INV_LUT_2   = pData->knee_point_inverse_lut[2];
        pRegCmd->inverse.cfg1.bitfields.Y_KNEE_INV_LUT_3   = pData->knee_point_inverse_lut[3];
        pRegCmd->inverse.cfg2.bitfields.Y_KNEE_INV_LUT_4   = pData->knee_point_inverse_lut[4];
        pRegCmd->inverse.cfg2.bitfields.Y_KNEE_INV_LUT_5   = pData->knee_point_inverse_lut[5];
        pRegCmd->inverse.cfg3.bitfields.Y_KNEE_INV_LUT_6   = pData->knee_point_inverse_lut[6];
        pRegCmd->inverse.cfg3.bitfields.Y_KNEE_INV_LUT_7   = pData->knee_point_inverse_lut[7];
        pRegCmd->inverse.cfg4.bitfields.Y_KNEE_INV_LUT_8   = pData->knee_point_inverse_lut[8];
        pRegCmd->inverse.cfg4.bitfields.Y_KNEE_INV_LUT_9   = pData->knee_point_inverse_lut[9];
        pRegCmd->inverse.cfg5.bitfields.Y_KNEE_INV_LUT_10  = pData->knee_point_inverse_lut[10];
        pRegCmd->inverse.cfg5.bitfields.Y_KNEE_INV_LUT_11  = pData->knee_point_inverse_lut[11];
        pRegCmd->inverse.cfg6.bitfields.Y_KNEE_INV_LUT_12  = pData->knee_point_inverse_lut[12];
        pRegCmd->inverse.cfg6.bitfields.Y_KNEE_INV_LUT_13  = pData->knee_point_inverse_lut[13];
        pRegCmd->inverse.cfg7.bitfields.Y_KNEE_INV_LUT_14  = pData->knee_point_inverse_lut[14];
        pRegCmd->inverse.cfg7.bitfields.Y_KNEE_INV_LUT_15  = pData->knee_point_inverse_lut[15];

        pRegCmd->knee.cfg0.bitfields.Y_KNEE_PT_LUT_0       = pData->knee_point_lut[0];
        pRegCmd->knee.cfg0.bitfields.Y_KNEE_PT_LUT_1       = pData->knee_point_lut[1];
        pRegCmd->knee.cfg1.bitfields.Y_KNEE_PT_LUT_2       = pData->knee_point_lut[2];
        pRegCmd->knee.cfg1.bitfields.Y_KNEE_PT_LUT_3       = pData->knee_point_lut[3];
        pRegCmd->knee.cfg2.bitfields.Y_KNEE_PT_LUT_4       = pData->knee_point_lut[4];
        pRegCmd->knee.cfg2.bitfields.Y_KNEE_PT_LUT_5       = pData->knee_point_lut[5];
        pRegCmd->knee.cfg3.bitfields.Y_KNEE_PT_LUT_6       = pData->knee_point_lut[6];
        pRegCmd->knee.cfg3.bitfields.Y_KNEE_PT_LUT_7       = pData->knee_point_lut[7];
        pRegCmd->knee.cfg4.bitfields.Y_KNEE_PT_LUT_8       = pData->knee_point_lut[8];
        pRegCmd->knee.cfg4.bitfields.Y_KNEE_PT_LUT_9       = pData->knee_point_lut[9];
        pRegCmd->knee.cfg5.bitfields.Y_KNEE_PT_LUT_10      = pData->knee_point_lut[10];
        pRegCmd->knee.cfg5.bitfields.Y_KNEE_PT_LUT_11      = pData->knee_point_lut[11];
        pRegCmd->knee.cfg6.bitfields.Y_KNEE_PT_LUT_12      = pData->knee_point_lut[12];
        pRegCmd->knee.cfg6.bitfields.Y_KNEE_PT_LUT_13      = pData->knee_point_lut[13];
        pRegCmd->knee.cfg7.bitfields.Y_KNEE_PT_LUT_14      = pData->knee_point_lut[14];
        pRegCmd->knee.cfg7.bitfields.Y_KNEE_PT_LUT_15      = pData->knee_point_lut[15];

        pRegCmd->qRes.chroma.bitfields.Q_RES_CHROMA        = pData->chroma_q;
        pRegCmd->qRes.luma.bitfields.Q_RES_LUMA            = pData->luma_q;

        pRegCmd->slope.cfg0.bitfields.CHROMA_SLP_LUT_0     = pData->c_slope_lut[0];
        pRegCmd->slope.cfg0.bitfields.CHROMA_SLP_LUT_1     = pData->c_slope_lut[1];
        pRegCmd->slope.cfg1.bitfields.CHROMA_SLP_LUT_2     = pData->c_slope_lut[2];
        pRegCmd->slope.cfg1.bitfields.CHROMA_SLP_LUT_3     = pData->c_slope_lut[3];
        pRegCmd->slope.cfg2.bitfields.CHROMA_SLP_LUT_4     = pData->c_slope_lut[4];
        pRegCmd->slope.cfg2.bitfields.CHROMA_SLP_LUT_5     = pData->c_slope_lut[5];
        pRegCmd->slope.cfg3.bitfields.CHROMA_SLP_LUT_6     = pData->c_slope_lut[6];
        pRegCmd->slope.cfg3.bitfields.CHROMA_SLP_LUT_7     = pData->c_slope_lut[7];
        pRegCmd->slope.cfg4.bitfields.CHROMA_SLP_LUT_8     = pData->c_slope_lut[8];
        pRegCmd->slope.cfg4.bitfields.CHROMA_SLP_LUT_9     = pData->c_slope_lut[9];
        pRegCmd->slope.cfg5.bitfields.CHROMA_SLP_LUT_10    = pData->c_slope_lut[10];
        pRegCmd->slope.cfg5.bitfields.CHROMA_SLP_LUT_11    = pData->c_slope_lut[11];
        pRegCmd->slope.cfg6.bitfields.CHROMA_SLP_LUT_12    = pData->c_slope_lut[12];
        pRegCmd->slope.cfg6.bitfields.CHROMA_SLP_LUT_13    = pData->c_slope_lut[13];
        pRegCmd->slope.cfg7.bitfields.CHROMA_SLP_LUT_14    = pData->c_slope_lut[14];
        pRegCmd->slope.cfg7.bitfields.CHROMA_SLP_LUT_15    = pData->c_slope_lut[15];

        pRegCmd->threshold.cfg0.bitfields.CHROMA_TH_LUT_0  = pData->c_thr_lut[0];
        pRegCmd->threshold.cfg0.bitfields.CHROMA_TH_LUT_1  = pData->c_thr_lut[1];
        pRegCmd->threshold.cfg1.bitfields.CHROMA_TH_LUT_2  = pData->c_thr_lut[2];
        pRegCmd->threshold.cfg1.bitfields.CHROMA_TH_LUT_3  = pData->c_thr_lut[3];
        pRegCmd->threshold.cfg2.bitfields.CHROMA_TH_LUT_4  = pData->c_thr_lut[4];
        pRegCmd->threshold.cfg2.bitfields.CHROMA_TH_LUT_5  = pData->c_thr_lut[5];
        pRegCmd->threshold.cfg3.bitfields.CHROMA_TH_LUT_6  = pData->c_thr_lut[6];
        pRegCmd->threshold.cfg3.bitfields.CHROMA_TH_LUT_7  = pData->c_thr_lut[7];
        pRegCmd->threshold.cfg4.bitfields.CHROMA_TH_LUT_8  = pData->c_thr_lut[8];
        pRegCmd->threshold.cfg4.bitfields.CHROMA_TH_LUT_9  = pData->c_thr_lut[9];
        pRegCmd->threshold.cfg5.bitfields.CHROMA_TH_LUT_10 = pData->c_thr_lut[10];
        pRegCmd->threshold.cfg5.bitfields.CHROMA_TH_LUT_11 = pData->c_thr_lut[11];
        pRegCmd->threshold.cfg6.bitfields.CHROMA_TH_LUT_12 = pData->c_thr_lut[12];
        pRegCmd->threshold.cfg6.bitfields.CHROMA_TH_LUT_13 = pData->c_thr_lut[13];
        pRegCmd->threshold.cfg7.bitfields.CHROMA_TH_LUT_14 = pData->c_thr_lut[14];
        pRegCmd->threshold.cfg7.bitfields.CHROMA_TH_LUT_15 = pData->c_thr_lut[15];
        pRegCmd->weight.cfg0.bitfields.Y_WEIGHT_LUT_0      = pData->y_weight_lut[0];
        pRegCmd->weight.cfg0.bitfields.Y_WEIGHT_LUT_1      = pData->y_weight_lut[1];
        pRegCmd->weight.cfg1.bitfields.Y_WEIGHT_LUT_2      = pData->y_weight_lut[2];
        pRegCmd->weight.cfg1.bitfields.Y_WEIGHT_LUT_3      = pData->y_weight_lut[3];
        pRegCmd->weight.cfg2.bitfields.Y_WEIGHT_LUT_4      = pData->y_weight_lut[4];
        pRegCmd->weight.cfg2.bitfields.Y_WEIGHT_LUT_5      = pData->y_weight_lut[5];
        pRegCmd->weight.cfg3.bitfields.Y_WEIGHT_LUT_6      = pData->y_weight_lut[6];
        pRegCmd->weight.cfg3.bitfields.Y_WEIGHT_LUT_7      = pData->y_weight_lut[7];
        pRegCmd->weight.cfg4.bitfields.Y_WEIGHT_LUT_8      = pData->y_weight_lut[8];
        pRegCmd->weight.cfg4.bitfields.Y_WEIGHT_LUT_9      = pData->y_weight_lut[9];
        pRegCmd->weight.cfg5.bitfields.Y_WEIGHT_LUT_10     = pData->y_weight_lut[10];
        pRegCmd->weight.cfg5.bitfields.Y_WEIGHT_LUT_11     = pData->y_weight_lut[11];
        pRegCmd->weight.cfg6.bitfields.Y_WEIGHT_LUT_12     = pData->y_weight_lut[12];
        pRegCmd->weight.cfg6.bitfields.Y_WEIGHT_LUT_13     = pData->y_weight_lut[13];
        pRegCmd->weight.cfg7.bitfields.Y_WEIGHT_LUT_14     = pData->y_weight_lut[14];
        pRegCmd->weight.cfg7.bitfields.Y_WEIGHT_LUT_15     = pData->y_weight_lut[15];
    }
    else
    {
        result = CamxResultEInvalidArg;
        CAMX_LOG_ERROR(CamxLogGroupPProc, "Input data is pInput %p m_pRegCmd %p", pInput, m_pRegCmd);
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IPECS20Titan480::SetupRegisterSetting
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IPECS20Titan480::SetupRegisterSetting(
    VOID*  pInput)
{
    CamxResult result = CamxResultSuccess;

    CAMX_UNREFERENCED_PARAM(pInput);

    CAMX_LOG_ERROR(CamxLogGroupPProc, "Function not implemented");

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// IPECS20Titan480::UpdateTuningMetadata
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IPECS20Titan480::UpdateTuningMetadata(
    VOID* pSettingData)
{
    CamxResult         result             = CamxResultSuccess;
    ISPInputData*      pInputData         = static_cast<ISPInputData*>(pSettingData);
    IPETuningMetadata* pIPETuningMetadata = static_cast<IPETuningMetadata*>(pInputData->pIPETuningMetadata);
    IpeIQSettings*     pIPEIQSettings     = static_cast<IpeIQSettings*>(pInputData->pipelineIPEData.pIPEIQSettings);

    IPEChromaSuppressionRegCmd* pRegCmd   = static_cast<IPEChromaSuppressionRegCmd*>(m_pRegCmd);

    // Post tuning metadata if setting is enabled
    if ((NULL != pInputData->pIPETuningMetadata) && (NULL != pRegCmd))
    {
        CAMX_STATIC_ASSERT(sizeof(IPEChromaSuppressionRegCmd) ==
            sizeof(pIPETuningMetadata->IPETuningMetadata480.IPEChromaSuppressionData.CSConfig));

        Utils::Memcpy(&pIPETuningMetadata->IPETuningMetadata480.IPEChromaSuppressionData.CSConfig,
                      pRegCmd,
                      sizeof(IPEChromaSuppressionRegCmd));

        if (TRUE == pIPEIQSettings->chromaSupressionParameters.moduleCfg.EN)
        {
            DebugDataTagID dataTagID;
            if (TRUE == pInputData->pipelineIPEData.realtimeFlag)
            {
                dataTagID = DebugDataTagID::TuningIPEChromasuppression20Register;
            }
            else
            {
                dataTagID = DebugDataTagID::TuningIPEChromasuppression20RegisterOffline;
            }

            result = pInputData->pipelineIPEData.pDebugDataWriter->AddDataTag(
                dataTagID,
                DebugDataTagType::UInt32,
                CAMX_ARRAY_SIZE(pIPETuningMetadata->IPETuningMetadata480.IPEChromaSuppressionData.CSConfig),
                &pIPETuningMetadata->IPETuningMetadata480.IPEChromaSuppressionData.CSConfig,
                sizeof(pIPETuningMetadata->IPETuningMetadata480.IPEChromaSuppressionData.CSConfig));

            if (CamxResultSuccess != result)
            {
                CAMX_LOG_WARN(CamxLogGroupPProc, "AddDataTag failed with error: %d", result);
            }
        }
    }
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// IPECS20Titan480::GetRegSize
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UINT IPECS20Titan480::GetRegSize()
{
    return sizeof(IPEChromaSuppressionRegCmd);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IPECS20Titan480::~IPECS20Titan480
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
IPECS20Titan480::~IPECS20Titan480()
{
    if (NULL != m_pRegCmd)
    {
        CAMX_FREE(m_pRegCmd);
        m_pRegCmd = NULL;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IPECS20Titan480::DumpRegConfig
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID IPECS20Titan480::DumpRegConfig()
{
    if (NULL != m_pRegCmd)
    {
        IPEChromaSuppressionRegCmd* pRegCmd = static_cast<IPEChromaSuppressionRegCmd*>(m_pRegCmd);

        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "pRegCmd->knee.cfg0      = %x\n", pRegCmd->knee.cfg0);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "pRegCmd->knee.cfg1      = %x\n", pRegCmd->knee.cfg1);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "pRegCmd->knee.cfg2      = %x\n", pRegCmd->knee.cfg2);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "pRegCmd->knee.cfg3      = %x\n", pRegCmd->knee.cfg3);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "pRegCmd->knee.cfg4      = %x\n", pRegCmd->knee.cfg4);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "pRegCmd->knee.cfg5      = %x\n", pRegCmd->knee.cfg5);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "pRegCmd->knee.cfg6      = %x\n", pRegCmd->knee.cfg6);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "pRegCmd->knee.cfg7      = %x\n", pRegCmd->knee.cfg7);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "pRegCmd->knee.cfg7      = %x\n", pRegCmd->knee.cfg7);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "pRegCmd->weight.cfg0    = %x\n", pRegCmd->weight.cfg0);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "pRegCmd->weight.cfg1    = %x\n", pRegCmd->weight.cfg1);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "pRegCmd->weight.cfg2    = %x\n", pRegCmd->weight.cfg2);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "pRegCmd->weight.cfg3    = %x\n", pRegCmd->weight.cfg3);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "pRegCmd->weight.cfg4    = %x\n", pRegCmd->weight.cfg4);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "pRegCmd->weight.cfg5    = %x\n", pRegCmd->weight.cfg5);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "pRegCmd->weight.cfg6    = %x\n", pRegCmd->weight.cfg6);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "pRegCmd->weight.cfg7    = %x\n", pRegCmd->weight.cfg7);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "pRegCmd->weight.cfg7    = %x\n", pRegCmd->weight.cfg7);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "pRegCmd->threshold.cfg0 = %x\n", pRegCmd->threshold.cfg0);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "pRegCmd->threshold.cfg1 = %x\n", pRegCmd->threshold.cfg1);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "pRegCmd->threshold.cfg2 = %x\n", pRegCmd->threshold.cfg2);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "pRegCmd->threshold.cfg3 = %x\n", pRegCmd->threshold.cfg3);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "pRegCmd->threshold.cfg4 = %x\n", pRegCmd->threshold.cfg4);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "pRegCmd->threshold.cfg5 = %x\n", pRegCmd->threshold.cfg5);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "pRegCmd->threshold.cfg6 = %x\n", pRegCmd->threshold.cfg6);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "pRegCmd->threshold.cfg7 = %x\n", pRegCmd->threshold.cfg7);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "pRegCmd->threshold.cfg7 = %x\n", pRegCmd->threshold.cfg7);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "pRegCmd->slope.cfg0     = %x\n", pRegCmd->slope.cfg0);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "pRegCmd->slope.cfg1     = %x\n", pRegCmd->slope.cfg1);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "pRegCmd->slope.cfg2     = %x\n", pRegCmd->slope.cfg2);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "pRegCmd->slope.cfg3     = %x\n", pRegCmd->slope.cfg3);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "pRegCmd->slope.cfg4     = %x\n", pRegCmd->slope.cfg4);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "pRegCmd->slope.cfg5     = %x\n", pRegCmd->slope.cfg5);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "pRegCmd->slope.cfg6     = %x\n", pRegCmd->slope.cfg6);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "pRegCmd->slope.cfg7     = %x\n", pRegCmd->slope.cfg7);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "pRegCmd->slope.cfg7     = %x\n", pRegCmd->slope.cfg7);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "pRegCmd->inverse.cfg0   = %x\n", pRegCmd->inverse.cfg0);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "pRegCmd->inverse.cfg1   = %x\n", pRegCmd->inverse.cfg1);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "pRegCmd->inverse.cfg2   = %x\n", pRegCmd->inverse.cfg2);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "pRegCmd->inverse.cfg3   = %x\n", pRegCmd->inverse.cfg3);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "pRegCmd->inverse.cfg4   = %x\n", pRegCmd->inverse.cfg4);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "pRegCmd->inverse.cfg5   = %x\n", pRegCmd->inverse.cfg5);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "pRegCmd->inverse.cfg6   = %x\n", pRegCmd->inverse.cfg6);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "pRegCmd->inverse.cfg7   = %x\n", pRegCmd->inverse.cfg7);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "pRegCmd->inverse.cfg7   = %x\n", pRegCmd->threshold.cfg7);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "pRegCmd->qRes.luma      = %x\n", pRegCmd->qRes.luma);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "pRegCmd->qRes.chroma    = %x\n", pRegCmd->qRes.chroma);
    }
    else
    {
        CAMX_LOG_ERROR(CamxLogGroupPProc, "Error m_pRegCmd is NULL");
    }
}

CAMX_NAMESPACE_END
