////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  camxipeupscaler20titan480.cpp
/// @brief IPEUpscaler20Titan480 class implementation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "titan480_ipe.h"
#include "upscale20setting.h"
#include "camxutils.h"
#include "camxipeupscaler20titan480.h"
#include "camxipeupscaler20.h"

CAMX_NAMESPACE_BEGIN

CAMX_BEGIN_PACKED

/// @brief Upscaler20 register Configuration
struct IPEUpscaleRegCmd
{
    IPE_IPE_0_PPS_CLC_UPSCALE_OP_MODE           opMode;                       ///< op mode register
    IPE_IPE_0_PPS_CLC_UPSCALE_PRELOAD           preload;                      ///< preload register
    IPE_IPE_0_PPS_CLC_UPSCALE_PHASE_STEP_Y_H    horizontalPhaseStepY;         ///< Y channel horizontal phase step
    IPE_IPE_0_PPS_CLC_UPSCALE_PHASE_STEP_Y_V    verticalPhaseStepY;           ///< Y channel vertical phase step
    IPE_IPE_0_PPS_CLC_UPSCALE_PHASE_STEP_UV_H   horizontalPhaseStepUV;        ///< UV channel horizontal phase step
    IPE_IPE_0_PPS_CLC_UPSCALE_PHASE_STEP_UV_V   verticalPhaseStepUV;          ///< UV channel vertical phase step
    IPE_IPE_0_PPS_CLC_UPSCALE_DE_SHARPEN        detailEnhancerSharpen;        ///< detail enhancer sharpen register
    IPE_IPE_0_PPS_CLC_UPSCALE_DE_SHARPEN_CTL    detailEnhancerSharpenControl; ///< detail enhancer sharpen control
    IPE_IPE_0_PPS_CLC_UPSCALE_DE_SHAPE_CTL      detailEnhancerShapeControl;   ///< detail enhancer shape control
    IPE_IPE_0_PPS_CLC_UPSCALE_DE_THRESHOLD      detailEnhancerThreshold;      ///< detail enhancer threshold
    IPE_IPE_0_PPS_CLC_UPSCALE_DE_ADJUST_DATA_0  detailEnhancerAdjustData0;    ///< detail enhancer adjust data 0
    IPE_IPE_0_PPS_CLC_UPSCALE_DE_ADJUST_DATA_1  detailEnhancerAdjustData1;    ///< detail enhancer adjust data 1
    IPE_IPE_0_PPS_CLC_UPSCALE_DE_ADJUST_DATA_2  detailEnhancerAdjustData2;    ///< detail enhancer adjust data 2
    IPE_IPE_0_PPS_CLC_UPSCALE_SRC_SIZE          sourceSize;                   ///< input before upscaler size
    IPE_IPE_0_PPS_CLC_UPSCALE_DST_SIZE          destinationSize;              ///< output after upscaler size
    IPE_IPE_0_PPS_CLC_UPSCALE_Y_PHASE_INIT_H    horizontalInitialPhaseY;      ///< Y channel horizontal initial phase
    IPE_IPE_0_PPS_CLC_UPSCALE_Y_PHASE_INIT_V    verticalInitialPhaseY;        ///< Y channel vertical initial phase
    IPE_IPE_0_PPS_CLC_UPSCALE_UV_PHASE_INIT_H   horizontalInitialPhaseUV;     ///< UV channel horizontal initial phase
    IPE_IPE_0_PPS_CLC_UPSCALE_UV_PHASE_INIT_V   verticalInitialPhaseUV;       ///< UV channel vertical initial phase
} CAMX_PACKED;

CAMX_END_PACKED

static const UINT32 IPEUpscaler20RegLength = sizeof(IPEUpscaleRegCmd) / RegisterWidthInBytes;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IPEUpscaler20Titan480::IPEUpscaler20Titan480
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
IPEUpscaler20Titan480::IPEUpscaler20Titan480()
{
    m_pRegCmd = CAMX_CALLOC(sizeof(IPEUpscaleRegCmd));

    if (NULL == m_pRegCmd)
    {
        CAMX_LOG_ERROR(CamxLogGroupISP, "Failed to allocate memory for Cmd Buffer");
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IPEUpscaler20Titan480::CreateCmdList
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IPEUpscaler20Titan480::CreateCmdList(
    VOID*   pSettingData,
    UINT32* pUpscalerHWSettingParams)
{
    CamxResult result = CamxResultSuccess;

    ISPInputData*            pInputData             = static_cast<ISPInputData*>(pSettingData);
    UpscalerHWSettingParams* pModuleHWSettingParams = reinterpret_cast<UpscalerHWSettingParams*>(pUpscalerHWSettingParams);

    m_pLUTCmdBuffer = pModuleHWSettingParams->pLUTCmdBuffer;

    // result = WriteLUTtoDMI(pInputData);

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IPEUpscaler20Titan480::WriteLUTtoDMI
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IPEUpscaler20Titan480::WriteLUTtoDMI(
    ISPInputData* pInputData)
{
    CamxResult  result        = CamxResultSuccess;
    CmdBuffer*  pDMICmdBuffer = pInputData->pipelineIPEData.ppIPECmdBuffer[CmdBufferDMIHeader];
    UINT32      offset        = 0;

    if ((NULL != m_pLUTCmdBuffer) && (NULL != pDMICmdBuffer))
    {
        result = PacketBuilder::WriteDMI(pDMICmdBuffer,
                                         regIPE_IPE_0_PPS_CLC_UPSCALE_DMI_CFG,
                                         IPE_IPE_0_PPS_CLC_UPSCALE_DMI_LUT_CFG_LUT_SEL_A_LUT,
                                         m_pLUTCmdBuffer,
                                         offset,
                                         IPEUpscaleLUTSizes[DMI_LUT_A]);
        if (CamxResultSuccess != result)
        {
            CAMX_LOG_ERROR(CamxLogGroupISP, "Fail to IPE_IPE_0_PPS_CLC_UPSCALE_DMI_LUT_CFG_LUT_SEL_A_LUT");
        }

        offset += IPEUpscaleLUTSizes[DMI_LUT_A];
        result = PacketBuilder::WriteDMI(pDMICmdBuffer,
                                         regIPE_IPE_0_PPS_CLC_UPSCALE_DMI_CFG,
                                         IPE_IPE_0_PPS_CLC_UPSCALE_DMI_LUT_CFG_LUT_SEL_B_LUT,
                                         m_pLUTCmdBuffer,
                                         offset,
                                         IPEUpscaleLUTSizes[DMI_LUT_B]);
        if (CamxResultSuccess != result)
        {
            CAMX_LOG_ERROR(CamxLogGroupISP, "Fail to IPE_IPE_0_PPS_CLC_UPSCALE_DMI_LUT_CFG_LUT_SEL_B_LUT");
        }

        offset += IPEUpscaleLUTSizes[DMI_LUT_B];
        result = PacketBuilder::WriteDMI(pDMICmdBuffer,
                                         regIPE_IPE_0_PPS_CLC_UPSCALE_DMI_CFG,
                                         IPE_IPE_0_PPS_CLC_UPSCALE_DMI_LUT_CFG_LUT_SEL_C_LUT,
                                         m_pLUTCmdBuffer,
                                         offset,
                                         IPEUpscaleLUTSizes[DMI_LUT_C]);
        if (CamxResultSuccess != result)
        {
            CAMX_LOG_ERROR(CamxLogGroupISP, "Fail to IPE_IPE_0_PPS_CLC_UPSCALE_DMI_LUT_CFG_LUT_SEL_C_LUT");
        }

        offset += IPEUpscaleLUTSizes[DMI_LUT_C];
        result = PacketBuilder::WriteDMI(pDMICmdBuffer,
                                         regIPE_IPE_0_PPS_CLC_UPSCALE_DMI_CFG,
                                         IPE_IPE_0_PPS_CLC_UPSCALE_DMI_LUT_CFG_LUT_SEL_D_LUT,
                                         m_pLUTCmdBuffer,
                                         offset,
                                         IPEUpscaleLUTSizes[DMI_LUT_D]);
        if (CamxResultSuccess != result)
        {
            CAMX_LOG_ERROR(CamxLogGroupISP, "Fail to IPE_IPE_0_PPS_CLC_UPSCALE_DMI_LUT_CFG_LUT_SEL_D_LUT");
        }
    }
    else
    {
        result = CamxResultEInvalidPointer;
        CAMX_LOG_ERROR(CamxLogGroupISP, "NULL m_pLUTCmdBuffer = %p, pDMICmdBuffer = %p", m_pLUTCmdBuffer, pDMICmdBuffer);
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IPEUpscaler20Titan480::PackIQRegisterSetting
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IPEUpscaler20Titan480::PackIQRegisterSetting(
    VOID*  pInput,
    VOID*  pOutputVal)
{
    CamxResult result = CamxResultSuccess;

    CAMX_UNREFERENCED_PARAM(pOutputVal);

    if ((NULL != pInput) && (NULL != m_pRegCmd))
    {
        Upscale20UnpackedField* pUnpackedRegisterData = static_cast<Upscale20UnpackedField*>(pInput);
        UpScaleHwConfig*        pHwConfig = &(pUnpackedRegisterData->upscaleHwConfig);
        IPEUpscaleRegCmd*       pRegCmdUpscale = static_cast<IPEUpscaleRegCmd*>(m_pRegCmd);

        // op mode register
        pRegCmdUpscale->opMode.bitfields.RGN_SEL                           = pHwConfig->opMode.alphaEnable;
        pRegCmdUpscale->opMode.bitfields.DE_EN                             = pHwConfig->opMode.detailEnhanceEnable;
        pRegCmdUpscale->opMode.bitfields.BIT_WIDTH                         = pHwConfig->opMode.bitWidth;
        pRegCmdUpscale->opMode.bitfields.DIR_EN                            = pHwConfig->opMode.directionEnable;
        pRegCmdUpscale->opMode.bitfields.Y_CFG                             = pHwConfig->opMode.yConfig;
        pRegCmdUpscale->opMode.bitfields.UV_CFG                            = pHwConfig->opMode.uvConfig;
        pRegCmdUpscale->opMode.bitfields.BLEND_CFG                         = pHwConfig->opMode.blendConfig;
        pRegCmdUpscale->opMode.bitfields.EN                                = pHwConfig->opMode.enable;
        pRegCmdUpscale->opMode.bitfields.WEIGHT_GAIN                       = pHwConfig->opMode.weightGain;

        // preload register
        pRegCmdUpscale->preload.bitfields.UV_PRELOAD_H                     = pHwConfig->preload.horizontalPreloadUV;
        pRegCmdUpscale->preload.bitfields.UV_PRELOAD_V                     = pHwConfig->preload.verticalPreloadUV;
        pRegCmdUpscale->preload.bitfields.Y_PRELOAD_H                      = pHwConfig->preload.horizontalPreloadY;
        pRegCmdUpscale->preload.bitfields.Y_PRELOAD_V                      = pHwConfig->preload.verticalPreloadY;

        // Y channel horizontal / vertical phase step register
        pRegCmdUpscale->horizontalPhaseStepY.bitfields.Y_STEP_H            = pHwConfig->horizontalPhaseStepY;
        pRegCmdUpscale->verticalPhaseStepY.bitfields.Y_STEP_V              = pHwConfig->verticalPhaseStepY;

        // UV channel horizontal / vertical phase step register
        pRegCmdUpscale->horizontalPhaseStepUV.bitfields.UV_STEP_H          = pHwConfig->horizontalPhaseStepUV;
        pRegCmdUpscale->verticalPhaseStepUV.bitfields.UV_STEP_V            = pHwConfig->verticalPhaseStepUV;

        // detail enhancer sharpen level register
        pRegCmdUpscale->detailEnhancerSharpen.bitfields.SHARPEN_LEVEL1     = pHwConfig->sharpenStrength.sharpenLevel1;
        pRegCmdUpscale->detailEnhancerSharpen.bitfields.SHARPEN_LEVEL2     = pHwConfig->sharpenStrength.sharpenLevel2;

        // detail enhancer control register
        pRegCmdUpscale->detailEnhancerSharpenControl.bitfields.DE_CLIP     = pHwConfig->detailEnhanceControl.detailEnhanceClip;
        pRegCmdUpscale->detailEnhancerSharpenControl.bitfields.DE_PREC     =
            pHwConfig->detailEnhanceControl.detailEnhancePrecision;

        // detail enhancer shape control register
        pRegCmdUpscale->detailEnhancerShapeControl.bitfields.THR_DIEOUT    = pHwConfig->curveShape.thresholdDieout;
        pRegCmdUpscale->detailEnhancerShapeControl.bitfields.THR_QUIET     = pHwConfig->curveShape.thresholdQuiet;

        // detail enhancer threshold register
        pRegCmdUpscale->detailEnhancerThreshold.bitfields.THR_HIGH         = pHwConfig->detailEnhanceThreshold.thresholdHigh;
        pRegCmdUpscale->detailEnhancerThreshold.bitfields.THR_LOW          = pHwConfig->detailEnhanceThreshold.thresholdLow;

        // detail enhancer adjust data registers
        pRegCmdUpscale->detailEnhancerAdjustData0.bitfields.ADJUST_A0      = pHwConfig->curveAs.adjustA0;
        pRegCmdUpscale->detailEnhancerAdjustData0.bitfields.ADJUST_A1      = pHwConfig->curveAs.adjustA1;
        pRegCmdUpscale->detailEnhancerAdjustData0.bitfields.ADJUST_A2      = pHwConfig->curveAs.adjustA2;
        pRegCmdUpscale->detailEnhancerAdjustData1.bitfields.ADJUST_B0      = pHwConfig->curveBs.adjustB0;
        pRegCmdUpscale->detailEnhancerAdjustData1.bitfields.ADJUST_B1      = pHwConfig->curveBs.adjustB1;
        pRegCmdUpscale->detailEnhancerAdjustData1.bitfields.ADJUST_B2      = pHwConfig->curveBs.adjustB2;
        pRegCmdUpscale->detailEnhancerAdjustData2.bitfields.ADJUST_C0      = pHwConfig->curveCs.adjustC0;
        pRegCmdUpscale->detailEnhancerAdjustData2.bitfields.ADJUST_C1      = pHwConfig->curveCs.adjustC1;
        pRegCmdUpscale->detailEnhancerAdjustData2.bitfields.ADJUST_C2      = pHwConfig->curveCs.adjustC2;

        // source / destination size registers
        pRegCmdUpscale->sourceSize.bitfields.SRC_HEIGHT                    = pUnpackedRegisterData->dataPath.inputHeight[2];
        pRegCmdUpscale->sourceSize.bitfields.SRC_WIDTH                     = pUnpackedRegisterData->dataPath.inputWidth[2];
        pRegCmdUpscale->destinationSize.bitfields.DST_HEIGHT               = pUnpackedRegisterData->dataPath.outputHeight[2];
        pRegCmdUpscale->destinationSize.bitfields.DST_WIDTH                = pUnpackedRegisterData->dataPath.outputWidth[2];

        // Y channel horizontal / vertical initial phase registers
        pRegCmdUpscale->horizontalInitialPhaseY.bitfields.Y_PHASE_INIT_H   = pHwConfig->phaseInit.horizontalPhaseInitY;
        pRegCmdUpscale->verticalInitialPhaseY.bitfields.Y_PHASE_INIT_V     = pHwConfig->phaseInit.verticalPhaseInitY;

        // UV channel horizontal / vertical initial phase registers
        pRegCmdUpscale->horizontalInitialPhaseUV.bitfields.UV_PHASE_INIT_H = pHwConfig->phaseInit.horizontalPhaseInitUV;
        pRegCmdUpscale->verticalInitialPhaseUV.bitfields.UV_PHASE_INIT_V   = pHwConfig->phaseInit.verticalPhaseInitUV;
    }
    else
    {
        result = CamxResultEInvalidArg;
        CAMX_LOG_ERROR(CamxLogGroupISP, "Input data is NULL pInput %p m_pRegCmd %p", pInput, m_pRegCmd);
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IPEUpscaler20Titan480::SetupRegisterSetting
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IPEUpscaler20Titan480::SetupRegisterSetting(
    VOID*  pInput)
{
    CamxResult result = CamxResultSuccess;

    CAMX_UNREFERENCED_PARAM(pInput);

    CAMX_LOG_ERROR(CamxLogGroupISP, "Function not implemented");

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// IPEUpscaler20Titan480::SetupInternalData
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IPEUpscaler20Titan480::SetupInternalData(
    VOID* pData)
{
    CamxResult result = CamxResultSuccess;

    if ((NULL != pData) && (NULL != m_pRegCmd))
    {
        IPEUpscaleRegCmd*   pRegCmdUpscale = static_cast<IPEUpscaleRegCmd*>(m_pRegCmd);
        UpScalerParameters* pUpscalerParam = static_cast<UpScalerParameters*>(pData);

        pUpscalerParam->directionalUpscalingEnabled = pRegCmdUpscale->opMode.bitfields.DIR_EN;
        pUpscalerParam->sharpeningEnabled           = pRegCmdUpscale->opMode.bitfields.DE_EN;
        pUpscalerParam->lumaCfg                     = pRegCmdUpscale->opMode.bitfields.Y_CFG;
        pUpscalerParam->chromaCfg                   = pRegCmdUpscale->opMode.bitfields.UV_CFG;
        pUpscalerParam->bitwidth                    = pRegCmdUpscale->opMode.bitfields.BIT_WIDTH;
        pUpscalerParam->blendCfg                    = pRegCmdUpscale->opMode.bitfields.BLEND_CFG;
        pUpscalerParam->weightGain                  = pRegCmdUpscale->opMode.bitfields.WEIGHT_GAIN;
        pUpscalerParam->sharpenLevel1               = pRegCmdUpscale->detailEnhancerSharpen.bitfields.SHARPEN_LEVEL1;
        pUpscalerParam->sharpenLevel2               = pRegCmdUpscale->detailEnhancerSharpen.bitfields.SHARPEN_LEVEL2;
        pUpscalerParam->sharpenPrecision            = pRegCmdUpscale->detailEnhancerSharpenControl.bitfields.DE_PREC;
        pUpscalerParam->sharpenClip                 = pRegCmdUpscale->detailEnhancerSharpenControl.bitfields.DE_CLIP;
        pUpscalerParam->shapeThreshQuiet            = pRegCmdUpscale->detailEnhancerShapeControl.bitfields.THR_QUIET;
        pUpscalerParam->shapeThreshDieout           = pRegCmdUpscale->detailEnhancerShapeControl.bitfields.THR_DIEOUT;
        pUpscalerParam->threshLow                   = pRegCmdUpscale->detailEnhancerThreshold.bitfields.THR_LOW;
        pUpscalerParam->threshHigh                  = pRegCmdUpscale->detailEnhancerThreshold.bitfields.THR_HIGH;
        pUpscalerParam->adjustDataA0                = pRegCmdUpscale->detailEnhancerAdjustData0.bitfields.ADJUST_A0;
        pUpscalerParam->adjustDataA1                = pRegCmdUpscale->detailEnhancerAdjustData0.bitfields.ADJUST_A1;
        pUpscalerParam->adjustDataA2                = pRegCmdUpscale->detailEnhancerAdjustData0.bitfields.ADJUST_A2;
        pUpscalerParam->adjustDataB0                = pRegCmdUpscale->detailEnhancerAdjustData1.bitfields.ADJUST_B0;
        pUpscalerParam->adjustDataB1                = pRegCmdUpscale->detailEnhancerAdjustData1.bitfields.ADJUST_B1;
        pUpscalerParam->adjustDataB2                = pRegCmdUpscale->detailEnhancerAdjustData1.bitfields.ADJUST_B2;
        pUpscalerParam->adjustDataC0                = pRegCmdUpscale->detailEnhancerAdjustData2.bitfields.ADJUST_C0;
        pUpscalerParam->adjustDataC1                = pRegCmdUpscale->detailEnhancerAdjustData2.bitfields.ADJUST_C1;
        pUpscalerParam->adjustDataC2                = pRegCmdUpscale->detailEnhancerAdjustData2.bitfields.ADJUST_C2;
    }
    else
    {
        CAMX_LOG_ERROR(CamxLogGroupISP, "Input data is pData %p m_pRegCmd %p", pData, m_pRegCmd);
        result = CamxResultEInvalidArg;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// IPEUpscaler20Titan480::GetRegSize
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UINT IPEUpscaler20Titan480::GetRegSize()
{
    return sizeof(IPEUpscaleRegCmd);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IPEUpscaler20Titan480::~IPEUpscaler20Titan480
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
IPEUpscaler20Titan480::~IPEUpscaler20Titan480()
{
    if (NULL != m_pRegCmd)
    {
        CAMX_FREE(m_pRegCmd);
        m_pRegCmd = NULL;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IPEUpscaler20Titan480::DumpRegConfig
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID IPEUpscaler20Titan480::DumpRegConfig()
{
    if (NULL != m_pRegCmd)
    {
        IPEUpscaleRegCmd* pRegCmd = static_cast<IPEUpscaleRegCmd*>(m_pRegCmd);

        CAMX_LOG_VERBOSE(CamxLogGroupISP, "Upscaler20.directionalUpscalingEnabled = %x\n",
            pRegCmd->opMode.bitfields.DIR_EN);
        CAMX_LOG_VERBOSE(CamxLogGroupISP, "Upscaler20.sharpeningEnabled           = %x\n",
            pRegCmd->opMode.bitfields.DE_EN);
        CAMX_LOG_VERBOSE(CamxLogGroupISP, "Upscaler20.lumaConfig                  = %x\n",
            pRegCmd->opMode.bitfields.Y_CFG);
        CAMX_LOG_VERBOSE(CamxLogGroupISP, "Upscaler20.chromaConfig                = %x\n",
            pRegCmd->opMode.bitfields.UV_CFG);
        CAMX_LOG_VERBOSE(CamxLogGroupISP, "Upscaler20.bitWidth                    = %x\n",
            pRegCmd->opMode.bitfields.BIT_WIDTH);
        CAMX_LOG_VERBOSE(CamxLogGroupISP, "Upscaler20.blendConfig                 = %x\n",
            pRegCmd->opMode.bitfields.BLEND_CFG);
        CAMX_LOG_VERBOSE(CamxLogGroupISP, "Upscaler20.weightGain                  = %x\n",
            pRegCmd->opMode.bitfields.WEIGHT_GAIN);
        CAMX_LOG_VERBOSE(CamxLogGroupISP, "Upscaler20.sharpenLevel1               = %x\n",
            pRegCmd->detailEnhancerSharpen.bitfields.SHARPEN_LEVEL1);
        CAMX_LOG_VERBOSE(CamxLogGroupISP, "Upscaler20.sharpenLevel2               = %x\n",
            pRegCmd->detailEnhancerSharpen.bitfields.SHARPEN_LEVEL2);
        CAMX_LOG_VERBOSE(CamxLogGroupISP, "Upscaler20.sharpenPrecision            = %x\n",
            pRegCmd->detailEnhancerSharpenControl.bitfields.DE_PREC);
        CAMX_LOG_VERBOSE(CamxLogGroupISP, "Upscaler20.sharpenClip                 = %x\n",
            pRegCmd->detailEnhancerSharpenControl.bitfields.DE_CLIP);
        CAMX_LOG_VERBOSE(CamxLogGroupISP, "Upscaler20.shapeThreshQuiet            = %x\n",
            pRegCmd->detailEnhancerShapeControl.bitfields.THR_QUIET);
        CAMX_LOG_VERBOSE(CamxLogGroupISP, "Upscaler20.shapeThreshDieout           = %x\n",
            pRegCmd->detailEnhancerShapeControl.bitfields.THR_DIEOUT);
        CAMX_LOG_VERBOSE(CamxLogGroupISP, "Upscaler20.threshLow                   = %x\n",
            pRegCmd->detailEnhancerThreshold.bitfields.THR_LOW);
        CAMX_LOG_VERBOSE(CamxLogGroupISP, "Upscaler20.threshHigh                  = %x\n",
            pRegCmd->detailEnhancerThreshold.bitfields.THR_HIGH);
        CAMX_LOG_VERBOSE(CamxLogGroupISP, "Upscaler20.adjustDataA0                = %x\n",
            pRegCmd->detailEnhancerAdjustData0.bitfields.ADJUST_A0);
        CAMX_LOG_VERBOSE(CamxLogGroupISP, "Upscaler20.adjustDataA1                = %x\n",
            pRegCmd->detailEnhancerAdjustData0.bitfields.ADJUST_A1);
        CAMX_LOG_VERBOSE(CamxLogGroupISP, "Upscaler20.adjustDataA2                = %x\n",
            pRegCmd->detailEnhancerAdjustData0.bitfields.ADJUST_A2);
        CAMX_LOG_VERBOSE(CamxLogGroupISP, "Upscaler20.adjustDataB0                = %x\n",
            pRegCmd->detailEnhancerAdjustData1.bitfields.ADJUST_B0);
        CAMX_LOG_VERBOSE(CamxLogGroupISP, "Upscaler20.adjustDataB1                = %x\n",
            pRegCmd->detailEnhancerAdjustData1.bitfields.ADJUST_B1);
        CAMX_LOG_VERBOSE(CamxLogGroupISP, "Upscaler20.adjustDataB2                = %x\n",
            pRegCmd->detailEnhancerAdjustData1.bitfields.ADJUST_B2);
        CAMX_LOG_VERBOSE(CamxLogGroupISP, "Upscaler20.adjustDataC0                = %x\n",
            pRegCmd->detailEnhancerAdjustData2.bitfields.ADJUST_C0);
        CAMX_LOG_VERBOSE(CamxLogGroupISP, "Upscaler20.adjustDataC1                = %x\n",
            pRegCmd->detailEnhancerAdjustData2.bitfields.ADJUST_C1);
        CAMX_LOG_VERBOSE(CamxLogGroupISP, "Upscaler20.adjustDataC2                = %x\n",
            pRegCmd->detailEnhancerAdjustData2.bitfields.ADJUST_C2);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IPEUpscaler20Titan480::UpdateTuningMetadata
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IPEUpscaler20Titan480::UpdateTuningMetadata(
    VOID* pSettingData)
{
    CamxResult         result             = CamxResultSuccess;
    ISPInputData*      pInputData         = static_cast<ISPInputData*>(pSettingData);
    IPETuningMetadata* pIPETuningMetadata = static_cast<IPETuningMetadata*>(pInputData->pIPETuningMetadata);
    IPEUpscaleRegCmd*  pRegCmd            = static_cast<IPEUpscaleRegCmd*>(m_pRegCmd);

    // Post tuning metadata if setting is enabled
    if ((NULL != pInputData->pIPETuningMetadata) && (NULL != pRegCmd))
    {
        CAMX_STATIC_ASSERT(sizeof(IPEUpscaleRegCmd) ==
                sizeof(pIPETuningMetadata->IPETuningMetadata480.IPEUpscalerData.UpscalerConfig));

        Utils::Memcpy(&pIPETuningMetadata->IPETuningMetadata480.IPEUpscalerData.UpscalerConfig,
                pRegCmd,
                sizeof(IPEUpscaleRegCmd));

    }

    return result;
}

CAMX_NAMESPACE_END
