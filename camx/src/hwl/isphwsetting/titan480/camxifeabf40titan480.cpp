////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  camxifeabf40titan480.cpp
/// @brief CAMXIFEABF40TITAN480 class implementation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "camxifeabf40titan480.h"
#include "camxiqinterface.h"

CAMX_NAMESPACE_BEGIN

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFEABF40Titan480::IFEABF40Titan480
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
IFEABF40Titan480::IFEABF40Titan480()
{
    SetCommandLength(PacketBuilder::RequiredWriteRegRangeSizeInDwords(IFEABF40RegLength1DWord) +
                     PacketBuilder::RequiredWriteRegRangeSizeInDwords(IFEABF40RegLength2DWord) +
                     PacketBuilder::RequiredWriteDMISizeInDwords() * 3);
    Set32bitDMILength(IFEABFTotalDMILengthDword);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFEABF40Titan480::CreateCmdList
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFEABF40Titan480::CreateCmdList(
    VOID* pSettingData,
    UINT32* pDMIBufferOffset)
{
    CamxResult      result          = CamxResultSuccess;
    ISPInputData*   pInputData      = static_cast<ISPInputData*>(pSettingData);
    CmdBuffer*      pCmdBuffer      = NULL;
    CmdBuffer*      pDMIBuffer      = NULL;
    UINT32          offset          = (*pDMIBufferOffset +
                                      (pInputData->pStripeConfig->stripeId * IFEABFTotalDMILengthDword)) *
                                      sizeof(UINT32);
    UINT32          lengthInByte    = IFEABFTotalDMILengthDword * sizeof(UINT32);

    if (NULL != pInputData                  &&
        NULL != pInputData->pCmdBuffer      &&
        NULL != pInputData->p32bitDMIBuffer)
    {
        pCmdBuffer = pInputData->pCmdBuffer;
        pDMIBuffer = pInputData->p32bitDMIBuffer;

        result = PacketBuilder::WriteRegRange(pCmdBuffer,
                                              regIFE_IFE_0_PP_CLC_ABF_DMI_LUT_BANK_CFG,
                                              IFEABF40RegLength1DWord,
                                              reinterpret_cast<UINT32*>(&m_regCmd1));

        if (CamxResultSuccess == result)
        {
            result = PacketBuilder::WriteRegRange(pCmdBuffer,
                                                  regIFE_IFE_0_PP_CLC_ABF_ABF_0_CFG,
                                                  IFEABF40RegLength2DWord,
                                                  reinterpret_cast<UINT32*>(&m_regCmd2));
        }

        if (CamxResultSuccess == result)
        {
            lengthInByte = IFEABF40NoiseLUTSize;
            result = PacketBuilder::WriteDMI(pCmdBuffer,
                                             regIFE_IFE_0_PP_CLC_ABF_DMI_CFG,
                                             IFENoiseLUT0,
                                             pDMIBuffer,
                                             offset,
                                             lengthInByte);
            offset += lengthInByte;
            lengthInByte = IFEABF40ActivityLUTSize;
            result = PacketBuilder::WriteDMI(pCmdBuffer,
                                             regIFE_IFE_0_PP_CLC_ABF_DMI_CFG,
                                             IFEActivityLUT,
                                             pDMIBuffer,
                                             offset,
                                             lengthInByte);
            offset += lengthInByte;
            lengthInByte = IFEABF40DarkLUTSize;
            result = PacketBuilder::WriteDMI(pCmdBuffer,
                                             regIFE_IFE_0_PP_CLC_ABF_DMI_CFG,
                                             IFEDarkLUT,
                                             pDMIBuffer,
                                             offset,
                                             lengthInByte);
        }
    }
    else
    {
        CAMX_LOG_ERROR(CamxLogGroupIQMod, "Null Input Data");
        result = CamxResultEFailed;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFEABF40Titan480::UpdateTuningMetadata
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFEABF40Titan480::UpdateTuningMetadata(
    VOID* pTuningMetaData)
{
    CamxResult         result             = CamxResultSuccess;
    IFETuningMetadata* pIFETuningMetadata = static_cast<IFETuningMetadata*>(pTuningMetaData);

    // Post tuning metadata if setting is enabled
    if (NULL != pIFETuningMetadata)
    {
        CAMX_STATIC_ASSERT(sizeof(IFEABF40RegCmd1) <= sizeof(pIFETuningMetadata->metadata480.IFEABFData.ABFConfig1));
        Utils::Memcpy(&pIFETuningMetadata->metadata480.IFEABFData.ABFConfig1, &m_regCmd1, sizeof(IFEABF40RegCmd1));

        CAMX_STATIC_ASSERT(sizeof(IFEABF40RegCmd2) <= sizeof(pIFETuningMetadata->metadata480.IFEABFData.ABFConfig2));
        Utils::Memcpy(&pIFETuningMetadata->metadata480.IFEABFData.ABFConfig2, &m_regCmd2, sizeof(IFEABF40RegCmd2));

        // Update DMI buffer to tuning meta data
        if (NULL != m_pNoiseLUT)
        {
            Utils::Memcpy(pIFETuningMetadata->metadata480.IFEDMIPacked.ABFLUT.noiseLUT,
                          m_pNoiseLUT,
                          IFEABF40NoiseLUTSize);
        }
        if (NULL != m_pActivityLUT)
        {
            Utils::Memcpy(pIFETuningMetadata->metadata480.IFEDMIPacked.ABFLUT.activityLUT,
                          m_pActivityLUT,
                          IFEABF40ActivityLUTSize);
        }
        if (NULL != m_pDarkLUT)
        {
            Utils::Memcpy(pIFETuningMetadata->metadata480.IFEDMIPacked.ABFLUT.darkLUT,
                          m_pDarkLUT,
                          IFEABF40DarkLUTSize);
        }
    }
    else
    {
        CAMX_LOG_WARN(CamxLogGroupIQMod, "IFE Tuning metadata is Null");
        result = CamxResultEFailed;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFEABF40Titan480::PackIQRegisterSetting
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFEABF40Titan480::PackIQRegisterSetting(
    VOID*   pInput,
    VOID*   pOutput)
{
    CamxResult                result      = CamxResultSuccess;
    ABF40BLS12UnpackedField*  pData       = static_cast<ABF40BLS12UnpackedField*>(pInput);
    ABF40UnpackedField*       pDataABF    = pData->pUnpackedRegisterDataABF;
    BLS12UnpackedField*       pDataBLS    = pData->pUnpackedRegisterDataBLS;
    ABF40OutputData*          pOutputData = static_cast<ABF40OutputData*>(pOutput);

    if ((NULL != pDataABF) && (NULL != pDataBLS) && (NULL != pOutputData))
    {
        m_regCmd2.config0.bitfields.EDGE_SOFTNESS_GR                = pDataABF->edgeSoftness[1];
        m_regCmd2.config0.bitfields.EDGE_SOFTNESS_GB                = pDataABF->edgeSoftness[2];
        m_regCmd2.config1.bitfields.EDGE_SOFTNESS_R                 = pDataABF->edgeSoftness[0];
        m_regCmd2.config1.bitfields.EDGE_SOFTNESS_B                 = pDataABF->edgeSoftness[3];
        m_regCmd2.config2.bitfields.DISTANCE_LEVEL0_GRGB_0          = pDataABF->distanceLevel[0][3];
        m_regCmd2.config2.bitfields.DISTANCE_LEVEL0_GRGB_1          = pDataABF->distanceLevel[0][4];
        m_regCmd2.config2.bitfields.DISTANCE_LEVEL0_GRGB_2          = pDataABF->distanceLevel[0][5];
        m_regCmd2.config2.bitfields.DISTANCE_LEVEL1_GRGB_0          = pDataABF->distanceLevel[1][3];
        m_regCmd2.config2.bitfields.DISTANCE_LEVEL1_GRGB_1          = pDataABF->distanceLevel[1][4];
        m_regCmd2.config2.bitfields.DISTANCE_LEVEL1_GRGB_2          = pDataABF->distanceLevel[1][5];
        m_regCmd2.config2.bitfields.DISTANCE_LEVEL2_GRGB_0          = pDataABF->distanceLevel[2][3];
        m_regCmd2.config2.bitfields.DISTANCE_LEVEL2_GRGB_1          = pDataABF->distanceLevel[2][4];
        m_regCmd2.config2.bitfields.DISTANCE_LEVEL2_GRGB_2          = pDataABF->distanceLevel[2][5];
        m_regCmd2.config3.bitfields.DISTANCE_LEVEL0_RB_0            = pDataABF->distanceLevel[0][0];
        m_regCmd2.config3.bitfields.DISTANCE_LEVEL0_RB_1            = pDataABF->distanceLevel[0][1];
        m_regCmd2.config3.bitfields.DISTANCE_LEVEL0_RB_2            = pDataABF->distanceLevel[0][2];
        m_regCmd2.config3.bitfields.DISTANCE_LEVEL1_RB_0            = pDataABF->distanceLevel[1][0];
        m_regCmd2.config3.bitfields.DISTANCE_LEVEL1_RB_1            = pDataABF->distanceLevel[1][1];
        m_regCmd2.config3.bitfields.DISTANCE_LEVEL1_RB_2            = pDataABF->distanceLevel[1][2];
        m_regCmd2.config3.bitfields.DISTANCE_LEVEL2_RB_0            = pDataABF->distanceLevel[2][0];
        m_regCmd2.config3.bitfields.DISTANCE_LEVEL2_RB_1            = pDataABF->distanceLevel[2][1];
        m_regCmd2.config3.bitfields.DISTANCE_LEVEL2_RB_2            = pDataABF->distanceLevel[2][2];
        m_regCmd2.config4.bitfields.CURVE_OFFSET_R                  = pDataABF->curveOffset[0];
        m_regCmd2.config4.bitfields.CURVE_OFFSET_GR                 = pDataABF->curveOffset[1];
        m_regCmd2.config4.bitfields.CURVE_OFFSET_GB                 = pDataABF->curveOffset[2];
        m_regCmd2.config4.bitfields.CURVE_OFFSET_B                  = pDataABF->curveOffset[3];
        m_regCmd2.config5.bitfields.FILTER_STRENGTH_R               = pDataABF->filterStrength[0];
        m_regCmd2.config5.bitfields.FILTER_STRENGTH_GR              = pDataABF->filterStrength[1];
        m_regCmd2.config6.bitfields.FILTER_STRENGTH_GB              = pDataABF->filterStrength[2];
        m_regCmd2.config6.bitfields.FILTER_STRENGTH_B               = pDataABF->filterStrength[3];
        m_regCmd2.config7.bitfields.MINMAX_BLS                      = pDataABF->minmaxBLS;
        m_regCmd2.config7.bitfields.MINMAX_MAX_SHIFT                = pDataABF->minmaxMaxShift;
        m_regCmd2.config7.bitfields.MINMAX_MIN_SHIFT                = pDataABF->minmaxMinShift;
        m_regCmd2.config7.bitfields.MINMAX_OFFSET                   = pDataABF->minmaxOffset;
        m_regCmd2.config8.bitfields.RNR_BX                          = pDataABF->bx;
        m_regCmd2.config8.bitfields.RNR_BY                          = pDataABF->by;
        m_regCmd2.config9.bitfields.RNR_INIT_RSQUARE                = pDataABF->rSquareInit;
        m_regCmd2.config10.bitfields.RNR_RSQUARE_SCALE              = pDataABF->rSquareScale;
        m_regCmd2.config10.bitfields.RNR_RSQUARE_SHIFT              = pDataABF->rSquareShift;
        m_regCmd2.config11.bitfields.RNR_ANCHOR_0                   = pDataABF->RNRAnchor[0];
        m_regCmd2.config11.bitfields.RNR_ANCHOR_1                   = pDataABF->RNRAnchor[1];
        m_regCmd2.config12.bitfields.RNR_ANCHOR_2                   = pDataABF->RNRAnchor[2];
        m_regCmd2.config12.bitfields.RNR_ANCHOR_3                   = pDataABF->RNRAnchor[3];
        m_regCmd2.config13.bitfields.RNR_NOISE_BASE_0               = pDataABF->RNRBase0[0];
        m_regCmd2.config13.bitfields.RNR_NOISE_BASE_1               = pDataABF->RNRBase0[1];
        m_regCmd2.config14.bitfields.RNR_NOISE_BASE_2               = pDataABF->RNRBase0[2];
        m_regCmd2.config14.bitfields.RNR_NOISE_BASE_3               = pDataABF->RNRBase0[3];
        m_regCmd2.config15.bitfields.RNR_NOISE_SLOPE_0              = pDataABF->RNRSlope0[0];
        m_regCmd2.config15.bitfields.RNR_NOISE_SLOPE_1              = pDataABF->RNRSlope0[1];
        m_regCmd2.config16.bitfields.RNR_NOISE_SLOPE_2              = pDataABF->RNRSlope0[2];
        m_regCmd2.config16.bitfields.RNR_NOISE_SLOPE_3              = pDataABF->RNRSlope0[3];
        m_regCmd2.config17.bitfields.RNR_NOISE_SHIFT_0              = pDataABF->RNRShift0[0];
        m_regCmd2.config17.bitfields.RNR_NOISE_SHIFT_1              = pDataABF->RNRShift0[1];
        m_regCmd2.config17.bitfields.RNR_NOISE_SHIFT_2              = pDataABF->RNRShift0[2];
        m_regCmd2.config17.bitfields.RNR_NOISE_SHIFT_3              = pDataABF->RNRShift0[3];
        m_regCmd2.config17.bitfields.RNR_THRESH_SHIFT_0             = pDataABF->RNRShift1[0];
        m_regCmd2.config17.bitfields.RNR_THRESH_SHIFT_1             = pDataABF->RNRShift1[1];
        m_regCmd2.config17.bitfields.RNR_THRESH_SHIFT_2             = pDataABF->RNRShift1[2];
        m_regCmd2.config17.bitfields.RNR_THRESH_SHIFT_3             = pDataABF->RNRShift1[3];
        m_regCmd2.config18.bitfields.RNR_THRESH_BASE_0              = pDataABF->RNRBase1[0];
        m_regCmd2.config18.bitfields.RNR_THRESH_BASE_1              = pDataABF->RNRBase1[1];
        m_regCmd2.config19.bitfields.RNR_THRESH_BASE_2              = pDataABF->RNRBase1[2];
        m_regCmd2.config19.bitfields.RNR_THRESH_BASE_3              = pDataABF->RNRBase1[3];
        m_regCmd2.config20.bitfields.RNR_THRESH_SLOPE_0             = pDataABF->RNRSlope1[0];
        m_regCmd2.config20.bitfields.RNR_THRESH_SLOPE_1             = pDataABF->RNRSlope1[1];
        m_regCmd2.config21.bitfields.RNR_THRESH_SLOPE_2             = pDataABF->RNRSlope1[2];
        m_regCmd2.config21.bitfields.RNR_THRESH_SLOPE_3             = pDataABF->RNRSlope1[3];
        m_regCmd2.config22.bitfields.NP_ANCHOR_0                    = pDataABF->nprsvAnchor[0];
        m_regCmd2.config22.bitfields.NP_ANCHOR_1                    = pDataABF->nprsvAnchor[1];
        m_regCmd2.config23.bitfields.NP_ANCHOR_2                    = pDataABF->nprsvAnchor[2];
        m_regCmd2.config23.bitfields.NP_ANCHOR_3                    = pDataABF->nprsvAnchor[3];
        m_regCmd2.config24.bitfields.NP_BASE_RB_0                   = pDataABF->nprsvBase[0][0];
        m_regCmd2.config24.bitfields.NP_BASE_RB_1                   = pDataABF->nprsvBase[0][1];
        m_regCmd2.config25.bitfields.NP_BASE_RB_2                   = pDataABF->nprsvBase[0][2];
        m_regCmd2.config25.bitfields.NP_BASE_RB_3                   = pDataABF->nprsvBase[0][3];
        m_regCmd2.config26.bitfields.NP_SLOPE_RB_0                  = pDataABF->nprsvSlope[0][0];
        m_regCmd2.config26.bitfields.NP_SLOPE_RB_1                  = pDataABF->nprsvSlope[0][1];
        m_regCmd2.config27.bitfields.NP_SLOPE_RB_2                  = pDataABF->nprsvSlope[0][2];
        m_regCmd2.config27.bitfields.NP_SLOPE_RB_3                  = pDataABF->nprsvSlope[0][3];
        m_regCmd2.config28.bitfields.NP_SHIFT_GRGB_0                = pDataABF->nprsvShift[1][0];
        m_regCmd2.config28.bitfields.NP_SHIFT_GRGB_1                = pDataABF->nprsvShift[1][1];
        m_regCmd2.config28.bitfields.NP_SHIFT_GRGB_2                = pDataABF->nprsvShift[1][2];
        m_regCmd2.config28.bitfields.NP_SHIFT_GRGB_3                = pDataABF->nprsvShift[1][3];
        m_regCmd2.config28.bitfields.NP_SHIFT_RB_0                  = pDataABF->nprsvShift[0][0];
        m_regCmd2.config28.bitfields.NP_SHIFT_RB_1                  = pDataABF->nprsvShift[0][1];
        m_regCmd2.config28.bitfields.NP_SHIFT_RB_2                  = pDataABF->nprsvShift[0][2];
        m_regCmd2.config28.bitfields.NP_SHIFT_RB_3                  = pDataABF->nprsvShift[0][3];
        m_regCmd2.config29.bitfields.NP_BASE_GRGB_0                 = pDataABF->nprsvBase[1][0];
        m_regCmd2.config29.bitfields.NP_BASE_GRGB_1                 = pDataABF->nprsvBase[1][1];
        m_regCmd2.config30.bitfields.NP_BASE_GRGB_2                 = pDataABF->nprsvBase[1][2];
        m_regCmd2.config30.bitfields.NP_BASE_GRGB_3                 = pDataABF->nprsvBase[1][3];
        m_regCmd2.config31.bitfields.NP_SLOPE_GRGB_0                = pDataABF->nprsvSlope[1][0];
        m_regCmd2.config31.bitfields.NP_SLOPE_GRGB_1                = pDataABF->nprsvSlope[1][1];
        m_regCmd2.config32.bitfields.NP_SLOPE_GRGB_2                = pDataABF->nprsvSlope[1][2];
        m_regCmd2.config32.bitfields.NP_SLOPE_GRGB_3                = pDataABF->nprsvSlope[1][3];
        m_regCmd2.config33.bitfields.ACT_FAC0                       = pDataABF->activityFactor0;
        m_regCmd2.config33.bitfields.ACT_FAC1                       = pDataABF->activityFactor1;
        m_regCmd2.config34.bitfields.ACT_THD0                       = pDataABF->activityThreshold0;
        m_regCmd2.config34.bitfields.ACT_THD1                       = pDataABF->activityThreshold1;
        m_regCmd2.config35.bitfields.ACT_SMOOTH_THD0                = pDataABF->activitySmoothThreshold0;
        m_regCmd2.config35.bitfields.ACT_SMOOTH_THD1                = pDataABF->activitySmoothThreshold1;
        m_regCmd2.config35.bitfields.DARK_THD                       = pDataABF->darkThreshold;
        m_regCmd2.config36.bitfields.GR_RATIO                       = pDataABF->grRatio;
        m_regCmd2.config36.bitfields.RG_RATIO                       = pDataABF->rgRatio;
        m_regCmd2.config37.bitfields.BG_RATIO                       = pDataABF->bgRatio;
        m_regCmd2.config37.bitfields.GB_RATIO                       = pDataABF->gbRatio;
        m_regCmd2.config38.bitfields.BR_RATIO                       = pDataABF->brRatio;
        m_regCmd2.config38.bitfields.RB_RATIO                       = pDataABF->rbRatio;
        m_regCmd2.config39.bitfields.EDGE_COUNT_THD                 = pDataABF->edgeCountLow;
        m_regCmd2.config39.bitfields.EDGE_DETECT_THD                = pDataABF->edgeDetectThreshold;
        m_regCmd2.config39.bitfields.EDGE_DETECT_NOISE_SCALAR       = pDataABF->edgeDetectNoiseScalar;
        m_regCmd2.config39.bitfields.EDGE_SMOOTH_STRENGTH           = pDataABF->edgeSmoothStrength;
        m_regCmd2.config40.bitfields.EDGE_SMOOTH_NOISE_SCALAR_R     = pDataABF->edgeSmoothNoiseScalar[0];
        m_regCmd2.config40.bitfields.EDGE_SMOOTH_NOISE_SCALAR_GR    = pDataABF->edgeSmoothNoiseScalar[1];
        m_regCmd2.config41.bitfields.EDGE_SMOOTH_NOISE_SCALAR_GB    = pDataABF->edgeSmoothNoiseScalar[2];
        m_regCmd2.config41.bitfields.EDGE_SMOOTH_NOISE_SCALAR_B     = pDataABF->edgeSmoothNoiseScalar[3];


        m_regCmd2.config42.bitfields.BLS_THRESH_GR              = pDataBLS->thresholdGR;
        m_regCmd2.config42.bitfields.BLS_THRESH_R               = pDataBLS->thresholdR;
        m_regCmd2.config43.bitfields.BLS_THRESH_B               = pDataBLS->thresholdB;
        m_regCmd2.config43.bitfields.BLS_THRESH_GB              = pDataBLS->thresholdGB;
        m_regCmd2.config44.bitfields.BLS_OFFSET                 = pDataBLS->offset;
        m_regCmd2.config45.bitfields.BLS_SCALE                  = pDataBLS->scale;
        m_regCmd1.configReg.bitfields.ACT_ADJ_EN                = pDataABF->actAdjEnable;
        m_regCmd1.configReg.bitfields.BLOCK_MATCH_PATTERN_RB    = pDataABF->blockOpt;
        m_regCmd1.configReg.bitfields.CROSS_PLANE_EN            = pDataABF->crossProcessEnable;
        m_regCmd1.configReg.bitfields.DARK_DESAT_EN             = pDataABF->darkDesatEnable;
        m_regCmd1.configReg.bitfields.DARK_SMOOTH_EN            = pDataABF->darkSmoothEnable;
        m_regCmd1.configReg.bitfields.DIR_SMOOTH_EN             = pDataABF->directSmoothEnable;
        m_regCmd1.configReg.bitfields.EN                        = pDataABF->enable;
        m_regCmd1.configReg.bitfields.FILTER_EN                 = pDataABF->bilateralEnable;
        m_regCmd1.configReg.bitfields.MINMAX_EN                 = pDataABF->minmaxEnable;
        m_regCmd1.configReg.bitfields.PIX_MATCH_LEVEL_RB        = pDataABF->blockPixLevel[0];
        m_regCmd1.configReg.bitfields.PIX_MATCH_LEVEL_G         = pDataABF->blockPixLevel[1];
        m_regCmd1.configReg.bitfields.BLS_EN                    = pDataBLS->enable;
        m_regCmd1.DMILUTBankConfig.bitfields.BANK_SEL           = pDataABF->LUTBankSel;
        m_regCmd1.moduleLUTBankConfig.bitfields.BANK_SEL        = pDataABF->LUTBankSel;

        pOutputData->actEnable                                  = pDataABF->actAdjEnable;
        pOutputData->blockMatchPatternRB                        = pDataABF->blockOpt;
        pOutputData->crossPlaneEnable                           = pDataABF->crossProcessEnable;
        pOutputData->darkDesatEnable                            = pDataABF->darkDesatEnable;
        pOutputData->darkSmoothEnable                           = pDataABF->darkSmoothEnable;
        pOutputData->dirSmoothEnable                            = pDataABF->directSmoothEnable;
        pOutputData->enable                                     = pDataABF->enable;
        pOutputData->filterEnable                               = pDataABF->bilateralEnable;
        pOutputData->minmaxEnable                               = pDataABF->minmaxEnable;
        pOutputData->pixMatchLevelG                             = pDataABF->blockPixLevel[1];
        pOutputData->pixMatchLevelRB                            = pDataABF->blockPixLevel[0];
        pOutputData->blackLevelOffset                           = pDataBLS->offset;

        /// Updating Data to fill in DMI Buffers
        UINT8 bankSelect = m_regCmd1.DMILUTBankConfig.bitfields.BANK_SEL;
        for (UINT32 index = 0; index < DMIRAM_ABF40_NOISESTD_LENGTH; index++)
        {
            pOutputData->pNoiseLUT[index]  = pDataABF->noiseStdLUT[bankSelect][index];
        }

        for (UINT32 index = 0; index < DMIRAM_ABF40_ACTIVITY_LENGTH; index++)
        {
            pOutputData->pActivityLUT[index] = pDataABF->activityFactorLUT[bankSelect][index];
        }

        for (UINT32 index = 0; index < DMIRAM_ABF40_DARK_LENGTH; index++)
        {
            pOutputData->pDarkLUT[index] = pDataABF->darkFactorLUT[bankSelect][index];
        }

        // Store DMI address to use meta data dump
        m_pNoiseLUT     = pOutputData->pNoiseLUT;
        m_pActivityLUT  = pOutputData->pActivityLUT;
        m_pDarkLUT      = pOutputData->pDarkLUT;
    }
    else
    {
        result = CamxResultEInvalidArg;
        CAMX_LOG_ERROR(CamxLogGroupIQMod, "Input params are NULL pData=0x%p pOutputData=0x%p", pData, pOutputData);
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFEABF40Titan480::CreateSubCmdList
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFEABF40Titan480::CreateSubCmdList(
    VOID*   pSettingData,
    UINT32* pDMIBufferOffset)
{
    CamxResult    result = CamxResultSuccess;
    ISPInputData* pInputData = static_cast<ISPInputData*>(pSettingData);

    CAMX_UNREFERENCED_PARAM(pDMIBufferOffset);

    if (NULL != pInputData &&
        NULL != pInputData->pCmdBuffer)
    {
        CmdBuffer* pCmdBuffer = pInputData->pCmdBuffer;

        result = PacketBuilder::WriteRegRange(pCmdBuffer,
            regIFE_IFE_0_PP_CLC_ABF_MODULE_CFG,
            sizeof(m_regCmd1.configReg) / RegisterWidthInBytes,
            reinterpret_cast<UINT32*>(&m_regCmd1.configReg));
    }
    else
    {
        CAMX_LOG_ERROR(CamxLogGroupIQMod, "Null Input Data");
        result = CamxResultEFailed;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFEABF40Titan480::SetupRegisterSetting
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFEABF40Titan480::SetupRegisterSetting(
    VOID*  pInput)
{
    CamxResult result = CamxResultSuccess;

    BOOL* pModuleEnable = static_cast<BOOL*>(pInput);

    if (NULL != pInput)
    {
        m_regCmd1.configReg.bitfields.EN = *pModuleEnable;
    }
    else
    {
        result = CamxResultEInvalidArg;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFEABF40Titan480::~IFEABF40Titan480
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
IFEABF40Titan480::~IFEABF40Titan480()
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFEABF40Titan480::DumpRegConfig
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID IFEABF40Titan480::DumpRegConfig()
{
    /// @brief Local debug dump register structure
    struct DumpInfo
    {
        UINT32  startRegAddr;    ///< Start address of the register of range
        UINT32  numRegs;         ///< The number of registers to be programmed.
        UINT32* pRegRangeAddr;   ///< The pointer to the structure in memory or a single varaible.
    };

    DumpInfo dumpRegInfoArray[] =
    {
        {
            regIFE_IFE_0_PP_CLC_ABF_DMI_LUT_BANK_CFG,
            IFEABF40RegLength1DWord,
            reinterpret_cast<UINT32*>(&m_regCmd1)
        },
        {
            regIFE_IFE_0_PP_CLC_ABF_ABF_0_CFG,
            IFEABF40RegLength2DWord,
            reinterpret_cast<UINT32*>(&m_regCmd2)
        }
    };

    for (UINT i = 0; i < CAMX_ARRAY_SIZE(dumpRegInfoArray); i++)
    {
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "SECTION[%d]: %08x: %08x",
            i, dumpRegInfoArray[i].startRegAddr,
            dumpRegInfoArray[i].startRegAddr + (dumpRegInfoArray[i].numRegs - 1) * RegisterWidthInBytes);

        for (UINT j = 0; j < dumpRegInfoArray[i].numRegs; j++)
        {
            CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "%08x: %08x",
                dumpRegInfoArray[i].startRegAddr + j * 4, *(dumpRegInfoArray[i].pRegRangeAddr + j));
        }
    }
}

CAMX_NAMESPACE_END
