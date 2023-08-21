////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  camxipeupscaler12titan17x.cpp
/// @brief IPEUpscaler12Titan17x class implementation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "titan170_ipe.h"
#include "upscale12setting.h"
#include "camxutils.h"
#include "camxipeupscaler12titan17x.h"
#include "camxipeupscaler12.h"

CAMX_NAMESPACE_BEGIN

CAMX_BEGIN_PACKED

struct IPEUpscaleLiteRegCmd
{
    IPE_IPE_0_PPS_CLC_UPSCALE_LUMA_SCALE_CFG_0          lumaScaleCfg0;                     ///< luma config0
    IPE_IPE_0_PPS_CLC_UPSCALE_LUMA_SCALE_OUTPUT_CFG     lumaScaleOutputCfg;                ///< luma output Config0
    IPE_IPE_0_PPS_CLC_UPSCALE_LUMA_SCALE_PHASEH_INIT    lumaScaleHorizontalInitialPhase;   ///< luma hortizontal Initial Phase
    IPE_IPE_0_PPS_CLC_UPSCALE_LUMA_SCALE_PHASEH_STEP    lumaScaleHorizontalStepPhase;      ///< luma horizontal step phase
    IPE_IPE_0_PPS_CLC_UPSCALE_LUMA_SCALE_PHASEV_INIT    lumaScaleVerticalInitialPhase;     ///< luma verticial initial phase
    IPE_IPE_0_PPS_CLC_UPSCALE_LUMA_SCALE_PHASEV_STEP    lumaScaleVerticalStepPhase;        ///< luma verticial step phase
    IPE_IPE_0_PPS_CLC_UPSCALE_LUMA_SCALE_CFG_1          lumaScaleCfg1;                     ///< luma config1
    IPE_IPE_0_PPS_CLC_UPSCALE_CHROMA_SCALE_CFG_0        chromaScaleCfg0;                   ///< chroma config0
    IPE_IPE_0_PPS_CLC_UPSCALE_CHROMA_SCALE_OUTPUT_CFG   chromaScaleOutputCfg;              ///< chroma output Config0
    IPE_IPE_0_PPS_CLC_UPSCALE_CHROMA_SCALE_PHASEH_INIT  chromaScaleHorizontalInitialPhase; ///< chroma hortizontal Initial Phase
    IPE_IPE_0_PPS_CLC_UPSCALE_CHROMA_SCALE_PHASEH_STEP  chromaScaleHorizontalStepPhase;    ///< chroma horizontal step phase
    IPE_IPE_0_PPS_CLC_UPSCALE_CHROMA_SCALE_PHASEV_INIT  chromaScaleVerticalInitialPhase;   ///< chroma verticial initial phase
    IPE_IPE_0_PPS_CLC_UPSCALE_CHROMA_SCALE_PHASEV_STEP  chromaScaleVerticalStepPhase;      ///< chroma verticial step phase
    IPE_IPE_0_PPS_CLC_UPSCALE_CHROMA_SCALE_CFG_1        chromaScaleCfg1;                   ///< chroma config1
} CAMX_PACKED;

CAMX_END_PACKED

static const UINT32 IPEUpscaler12RegLength = sizeof(IPEUpscaleLiteRegCmd) / RegisterWidthInBytes;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IPEUpscaler12Titan17x::IPEUpscaler12Titan17x
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
IPEUpscaler12Titan17x::IPEUpscaler12Titan17x()
{
    m_pRegCmd = CAMX_CALLOC(sizeof(IPEUpscaleLiteRegCmd));

    if (NULL == m_pRegCmd)
    {
        CAMX_LOG_ERROR(CamxLogGroupISP, "Failed to allocate memory for Cmd Buffer");
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IPEUpscaler12Titan17x::CreateCmdList
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IPEUpscaler12Titan17x::CreateCmdList(
    VOID*   pSettingData,
    UINT32* pUpscalerHWSettingParams)
{
    CamxResult result = CamxResultSuccess;

    CAMX_UNREFERENCED_PARAM(pSettingData);
    CAMX_UNREFERENCED_PARAM(pUpscalerHWSettingParams);

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IPEUpscaler12Titan17x::PackIQRegisterSetting
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IPEUpscaler12Titan17x::PackIQRegisterSetting(
    VOID*  pInput,
    VOID*  pOutputVal)
{
    CamxResult result = CamxResultSuccess;

    CAMX_UNREFERENCED_PARAM(pOutputVal);

    if ((NULL != pInput) && (NULL != m_pRegCmd))
    {
        IPEUpscaleLiteRegCmd*   pRegCmdUpscale        = static_cast<IPEUpscaleLiteRegCmd*>(m_pRegCmd);
        Upscale12UnpackedField* pUnpackedRegisterData = static_cast<Upscale12UnpackedField*>(pInput);

        pRegCmdUpscale->lumaScaleCfg0.bitfields.H_SCALE_FIR_ALGORITHM     = pUnpackedRegisterData->lumaHScaleFirAlgorithm;
        pRegCmdUpscale->lumaScaleCfg0.bitfields.V_SCALE_FIR_ALGORITHM     = pUnpackedRegisterData->lumaVScaleFirAlgorithm;
        pRegCmdUpscale->lumaScaleCfg1.bitfields.INPUT_DITHERING_DISABLE   = pUnpackedRegisterData->lumaInputDitheringDisable;
        pRegCmdUpscale->lumaScaleCfg1.bitfields.INPUT_DITHERING_MODE      = pUnpackedRegisterData->lumaInputDitheringMode;
        pRegCmdUpscale->chromaScaleCfg0.bitfields.H_SCALE_FIR_ALGORITHM   = pUnpackedRegisterData->chromaHScaleFirAlgorithm;
        pRegCmdUpscale->chromaScaleCfg0.bitfields.V_SCALE_FIR_ALGORITHM   = pUnpackedRegisterData->chromaVScaleFirAlgorithm;
        pRegCmdUpscale->chromaScaleCfg1.bitfields.INPUT_DITHERING_DISABLE = pUnpackedRegisterData->chromaInputDitheringDisable;
        pRegCmdUpscale->chromaScaleCfg1.bitfields.INPUT_DITHERING_MODE    = pUnpackedRegisterData->chromaInputDitheringMode;
        pRegCmdUpscale->chromaScaleCfg1.bitfields.OUTPUT_ROUNDING_MODE_H  = pUnpackedRegisterData->chromaRoundingModeH;
        pRegCmdUpscale->chromaScaleCfg1.bitfields.OUTPUT_ROUNDING_MODE_V  = pUnpackedRegisterData->chromaRoundingModeV;
    }
    else
    {
        result = CamxResultEInvalidArg;
        CAMX_LOG_ERROR(CamxLogGroupIQMod, "Input data is NULL pInput = %p", pInput);
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IPEUpscaler12Titan17x::SetupRegisterSetting
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IPEUpscaler12Titan17x::SetupRegisterSetting(
    VOID*  pInput)
{
    CamxResult result = CamxResultSuccess;

    CAMX_UNREFERENCED_PARAM(pInput);

    CAMX_LOG_ERROR(CamxLogGroupISP, "Function not implemented");

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// IPEUpscaler12Titan17x::SetupInternalData
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IPEUpscaler12Titan17x::SetupInternalData(
    VOID* pData)
{
    CamxResult result = CamxResultSuccess;

    if ((NULL != pData) && (NULL != m_pRegCmd))
    {
        ISPInputData*           pInputData         = static_cast<ISPInputData*>(pData);
        IpeIQSettings*          pIPEIQSettings     =
            reinterpret_cast<IpeIQSettings*>(pInputData->pipelineIPEData.pIPEIQSettings);
        UpscalerLiteParameters* pUpscalerLiteParam = &pIPEIQSettings->upscalerLiteParameters;
        IPEUpscaleLiteRegCmd*   pRegCmdUpscaleLite = static_cast<IPEUpscaleLiteRegCmd*>(m_pRegCmd);

        pUpscalerLiteParam->lumaVScaleFirAlgorithm      = pRegCmdUpscaleLite->lumaScaleCfg0.bitfields.V_SCALE_FIR_ALGORITHM;
        pUpscalerLiteParam->lumaHScaleFirAlgorithm      = pRegCmdUpscaleLite->lumaScaleCfg0.bitfields.H_SCALE_FIR_ALGORITHM;
        pUpscalerLiteParam->lumaInputDitheringDisable   = pRegCmdUpscaleLite->lumaScaleCfg1.bitfields.INPUT_DITHERING_DISABLE;
        pUpscalerLiteParam->lumaInputDitheringMode      = pRegCmdUpscaleLite->lumaScaleCfg1.bitfields.INPUT_DITHERING_MODE;
        pUpscalerLiteParam->chromaVScaleFirAlgorithm    = pRegCmdUpscaleLite->chromaScaleCfg0.bitfields.V_SCALE_FIR_ALGORITHM;
        pUpscalerLiteParam->chromaHScaleFirAlgorithm    = pRegCmdUpscaleLite->chromaScaleCfg0.bitfields.H_SCALE_FIR_ALGORITHM;
        pUpscalerLiteParam->chromaInputDitheringDisable = pRegCmdUpscaleLite->chromaScaleCfg1.bitfields.INPUT_DITHERING_DISABLE;
        pUpscalerLiteParam->chromaInputDitheringMode    = pRegCmdUpscaleLite->chromaScaleCfg1.bitfields.INPUT_DITHERING_MODE;
        pUpscalerLiteParam->chromaRoundingModeV         = pRegCmdUpscaleLite->chromaScaleCfg1.bitfields.OUTPUT_ROUNDING_MODE_V;
        pUpscalerLiteParam->chromaRoundingModeH         = pRegCmdUpscaleLite->chromaScaleCfg1.bitfields.OUTPUT_ROUNDING_MODE_H;
    }
    else
    {
        result = CamxResultEInvalidArg;
        CAMX_LOG_ERROR(CamxLogGroupIQMod, "Input data is NULL pData = %p", pData);
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// IPEUpscaler12Titan17x::GetRegSize
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UINT IPEUpscaler12Titan17x::GetRegSize()
{
    return sizeof(IPEUpscaleLiteRegCmd);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IPEUpscaler12Titan17x::~IPEUpscaler12Titan17x
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
IPEUpscaler12Titan17x::~IPEUpscaler12Titan17x()
{
    if (NULL != m_pRegCmd)
    {
        CAMX_FREE(m_pRegCmd);
        m_pRegCmd = NULL;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IPEUpscaler12Titan17x::DumpRegConfig
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID IPEUpscaler12Titan17x::DumpRegConfig()
{
    if (NULL != m_pRegCmd)
    {
        IPEUpscaleLiteRegCmd* pRegCmdUpscaleLite = static_cast<IPEUpscaleLiteRegCmd*>(m_pRegCmd);

        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "chromaScaleCfg0.bitfields.HSCALE_ENABLE [0x%x] ",
            pRegCmdUpscaleLite->chromaScaleCfg0.bitfields.HSCALE_ENABLE);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "chromaScaleCfg0.bitfields.H_SCALE_FIR_ALGORITHM [0x%x] ",
            pRegCmdUpscaleLite->chromaScaleCfg0.bitfields.H_SCALE_FIR_ALGORITHM);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "chromaScaleCfg0.bitfields.VSCALE_ENABLE [0x%x] ",
            pRegCmdUpscaleLite->chromaScaleCfg0.bitfields.VSCALE_ENABLE);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "chromaScaleCfg0.bitfields.V_SCALE_FIR_ALGORITHM [0x%x] ",
            pRegCmdUpscaleLite->chromaScaleCfg0.bitfields.V_SCALE_FIR_ALGORITHM);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "chromaScaleCfg1.bitfields.INPUT_DITHERING_DISABLE [0x%x] ",
            pRegCmdUpscaleLite->chromaScaleCfg1.bitfields.INPUT_DITHERING_DISABLE);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "chromaScaleCfg1.bitfields.INPUT_DITHERING_MODE [0x%x] ",
            pRegCmdUpscaleLite->chromaScaleCfg1.bitfields.INPUT_DITHERING_MODE);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "chromaScaleCfg1.bitfields.OUTPUT_ROUNDING_MODE_H [0x%x] ",
            pRegCmdUpscaleLite->chromaScaleCfg1.bitfields.OUTPUT_ROUNDING_MODE_H);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "chromaScaleCfg1.bitfields.OUTPUT_ROUNDING_MODE_V [0x%x] ",
            pRegCmdUpscaleLite->chromaScaleCfg1.bitfields.OUTPUT_ROUNDING_MODE_V);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "chromaScaleCfg1.bitfields.H_PIXEL_OFFSET [0x%x] ",
            pRegCmdUpscaleLite->chromaScaleCfg1.bitfields.H_PIXEL_OFFSET);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "chromaScaleCfg1.bitfields.V_PIXEL_OFFSET [0x%x] ",
            pRegCmdUpscaleLite->chromaScaleCfg1.bitfields.V_PIXEL_OFFSET);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "chromaScaleHorizontalInitialPhase.bitfields.PHASE_INIT [0x%x] ",
            pRegCmdUpscaleLite->chromaScaleHorizontalInitialPhase.bitfields.PHASE_INIT);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "chromaScaleHorizontalStepPhase.bitfields.PHASE_STEP [0x%x] ",
            pRegCmdUpscaleLite->chromaScaleHorizontalStepPhase.bitfields.PHASE_STEP);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "chromaScaleOutputCfg.bitfields.BLOCK_HEIGHT [0x%x] ",
            pRegCmdUpscaleLite->chromaScaleOutputCfg.bitfields.BLOCK_HEIGHT);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "chromaScaleOutputCfg.bitfields.BLOCK_WIDTH [0x%x] ",
            pRegCmdUpscaleLite->chromaScaleOutputCfg.bitfields.BLOCK_WIDTH);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "chromaScaleVerticalInitialPhase.bitfields.PHASE_INIT [0x%x] ",
            pRegCmdUpscaleLite->chromaScaleVerticalInitialPhase.bitfields.PHASE_INIT);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "chromaScaleVerticalStepPhase.bitfields.PHASE_STEP [0x%x] ",
            pRegCmdUpscaleLite->chromaScaleVerticalStepPhase.bitfields.PHASE_STEP);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "lumaScaleCfg0.bitfields.HSCALE_ENABLE [0x%x] ",
            pRegCmdUpscaleLite->lumaScaleCfg0.bitfields.HSCALE_ENABLE);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "lumaScaleCfg0.bitfields.H_SCALE_FIR_ALGORITHM [0x%x] ",
            pRegCmdUpscaleLite->lumaScaleCfg0.bitfields.H_SCALE_FIR_ALGORITHM);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "lumaScaleCfg0.bitfields.VSCALE_ENABLE [0x%x] ",
            pRegCmdUpscaleLite->lumaScaleCfg0.bitfields.VSCALE_ENABLE);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "lumaScaleCfg0.bitfields.V_SCALE_FIR_ALGORITHM [0x%x] ",
            pRegCmdUpscaleLite->lumaScaleCfg0.bitfields.V_SCALE_FIR_ALGORITHM);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "lumaScaleCfg1.bitfields.INPUT_DITHERING_DISABLE [0x%x] ",
            pRegCmdUpscaleLite->lumaScaleCfg1.bitfields.INPUT_DITHERING_DISABLE);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "lumaScaleCfg1.bitfields.INPUT_DITHERING_MODE [0x%x] ",
            pRegCmdUpscaleLite->lumaScaleCfg1.bitfields.INPUT_DITHERING_MODE);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "lumaScaleCfg1.bitfields.H_PIXEL_OFFSET [0x%x] ",
            pRegCmdUpscaleLite->lumaScaleCfg1.bitfields.H_PIXEL_OFFSET);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "lumaScaleCfg1.bitfields.V_PIXEL_OFFSET [0x%x] ",
            pRegCmdUpscaleLite->lumaScaleCfg1.bitfields.V_PIXEL_OFFSET);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "lumaScaleHorizontalInitialPhase.bitfields.PHASE_INIT [0x%x] ",
            pRegCmdUpscaleLite->lumaScaleHorizontalInitialPhase.bitfields.PHASE_INIT);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "lumaScaleHorizontalStepPhase.bitfields.PHASE_STEP [0x%x] ",
            pRegCmdUpscaleLite->lumaScaleHorizontalStepPhase.bitfields.PHASE_STEP);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "lumaScaleOutputCfg.bitfields.BLOCK_HEIGHT [0x%x] ",
            pRegCmdUpscaleLite->lumaScaleOutputCfg.bitfields.BLOCK_HEIGHT);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "lumaScaleOutputCfg.bitfields.BLOCK_WIDTH [0x%x] ",
            pRegCmdUpscaleLite->lumaScaleOutputCfg.bitfields.BLOCK_WIDTH);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "lumaScaleVerticalInitialPhase.bitfields.PHASE_INIT [0x%x] ",
            pRegCmdUpscaleLite->lumaScaleVerticalInitialPhase.bitfields.PHASE_INIT);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "lumaScaleVerticalStepPhase.bitfields.PHASE_STEP [0x%x] ",
            pRegCmdUpscaleLite->lumaScaleVerticalStepPhase.bitfields.PHASE_STEP);
    }
}

CAMX_NAMESPACE_END
