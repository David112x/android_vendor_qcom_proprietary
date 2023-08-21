////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  camxipechromaenhancement12titan480.cpp
/// @brief CAMXIPECHROMAENHANCEMENT12TITAN480 class implementation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "camxutils.h"
#include "camxipechromaenhancement12titan480.h"
#include "camxispiqmodule.h"
#include "cv12setting.h"
#include "titan480_ipe.h"

CAMX_NAMESPACE_BEGIN

CAMX_BEGIN_PACKED

/// @brief: This structure contains chroma enhancement registers programmed by software
struct IPEChromaEnhancementRegCmd
{
    IPE_IPE_0_PPS_CLC_CHROMA_ENHAN_CHROMA_ENHAN_LUMA_CFG_0     lumaConfig0; ///< RGB2Y conversion multiplication coefficents
                                                                            ///< V0 and V1
    IPE_IPE_0_PPS_CLC_CHROMA_ENHAN_CHROMA_ENHAN_LUMA_CFG_1     lumaConfig1; ///< RGB2Y conversion multiplication coefficent
                                                                            ///< V2 and offset coefficent.
    IPE_IPE_0_PPS_CLC_CHROMA_ENHAN_CHROMA_ENHAN_COEFF_A_CFG    aConfig;     ///< RGB2Cb conversion matrix parameters (AM, AP)
    IPE_IPE_0_PPS_CLC_CHROMA_ENHAN_CHROMA_ENHAN_COEFF_B_CFG    bConfig;     ///< RGB2Cb conversion matrix parameters (BM, BP)
    IPE_IPE_0_PPS_CLC_CHROMA_ENHAN_CHROMA_ENHAN_COEFF_C_CFG    cConfig;     ///< RGB2Cb conversion matrix parameters (CM, CP)
    IPE_IPE_0_PPS_CLC_CHROMA_ENHAN_CHROMA_ENHAN_COEFF_D_CFG    dConfig;     ///< RGB2Cb conversion matrix parameters (DM, DP)
    IPE_IPE_0_PPS_CLC_CHROMA_ENHAN_CHROMA_ENHAN_OFFSET_KCB_CFG cbConfig;    ///< RGB2CbCr offset for matrix conversion(KCB)
    IPE_IPE_0_PPS_CLC_CHROMA_ENHAN_CHROMA_ENHAN_OFFSET_KCR_CFG crConfig;    ///< RGB2CbCr offset for matrix conversion(KCR)
} CAMX_PACKED;

CAMX_END_PACKED

static const UINT32 IPEChromaEnhancementRegLength = sizeof(IPEChromaEnhancementRegCmd) / RegisterWidthInBytes;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IPEChromaEnhancement12Titan480::IPEChromaEnhancement12Titan480
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
IPEChromaEnhancement12Titan480::IPEChromaEnhancement12Titan480()
{
    m_pRegCmd = CAMX_CALLOC(sizeof(IPEChromaEnhancementRegCmd));

    if (NULL != m_pRegCmd)
    {

        // Hardcode initial value for all the registers
    }
    else
    {
        CAMX_LOG_ERROR(CamxLogGroupPProc, "Failed to allocate memory for Cmd Buffer");
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IPEChromaEnhancement12Titan480::CreateCmdList
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IPEChromaEnhancement12Titan480::CreateCmdList(
    VOID*   pSettingData,
    UINT32* pDMIBufferOffset)
{
    CamxResult    result     = CamxResultSuccess;
    ISPInputData* pInputData = static_cast<ISPInputData*>(pSettingData);
    CmdBuffer*    pCmdBuffer = NULL;

    CAMX_UNREFERENCED_PARAM(pDMIBufferOffset);

    if ((NULL != pInputData)                                 &&
        (NULL != pInputData->pipelineIPEData.ppIPECmdBuffer) &&
        (NULL != m_pRegCmd))
    {
        pCmdBuffer = pInputData->pipelineIPEData.ppIPECmdBuffer[CmdBufferPostLTM];

        result = PacketBuilder::WriteRegRange(pCmdBuffer,
                                              regIPE_IPE_0_PPS_CLC_CHROMA_ENHAN_CHROMA_ENHAN_LUMA_CFG_0,
                                              IPEChromaEnhancementRegLength,
                                              reinterpret_cast<UINT32*>(m_pRegCmd));

        if (CamxResultSuccess != result)
        {
            CAMX_LOG_ERROR(CamxLogGroupPProc, "Failed to write Chroma Enhancement config. data");
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
// IPEChromaEnhancement12Titan480::PackIQRegisterSetting
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IPEChromaEnhancement12Titan480::PackIQRegisterSetting(
    VOID*  pInput,
    VOID*  pOutput)
{
    CamxResult                  result  = CamxResultSuccess;
    CV12UnpackedField*          pData   = static_cast<CV12UnpackedField*>(pInput);
    IPEChromaEnhancementRegCmd* pRegCmd = NULL;

    CAMX_UNREFERENCED_PARAM(pOutput);

    if ((NULL != pData) && (NULL != m_pRegCmd))
    {
        pRegCmd = static_cast<IPEChromaEnhancementRegCmd*>(m_pRegCmd);

        pRegCmd->lumaConfig0.bitfields.V0 = static_cast<INT16>(pData->rToY);
        pRegCmd->lumaConfig0.bitfields.V1 = static_cast<INT16>(pData->gToY);
        pRegCmd->lumaConfig1.bitfields.V2 = static_cast<INT16>(pData->bToY);
        pRegCmd->lumaConfig1.bitfields.K  = static_cast<INT16>(pData->yOffset);
        pRegCmd->aConfig.bitfields.AM     = static_cast<INT16>(pData->am);
        pRegCmd->aConfig.bitfields.AP     = static_cast<INT16>(pData->ap);
        pRegCmd->bConfig.bitfields.BM     = static_cast<INT16>(pData->bm);
        pRegCmd->bConfig.bitfields.BP     = static_cast<INT16>(pData->bp);
        pRegCmd->cConfig.bitfields.CM     = static_cast<INT16>(pData->cm);
        pRegCmd->cConfig.bitfields.CP     = static_cast<INT16>(pData->cp);
        pRegCmd->dConfig.bitfields.DM     = static_cast<INT16>(pData->dm);
        pRegCmd->dConfig.bitfields.DP     = static_cast<INT16>(pData->dp);
        pRegCmd->cbConfig.bitfields.KCB   = static_cast<INT16>(pData->kcb);
        pRegCmd->crConfig.bitfields.KCR   = static_cast<INT16>(pData->kcr);
    }
    else
    {
        CAMX_LOG_ERROR(CamxLogGroupPProc, "Input data is pData %p", pData);
        result = CamxResultEInvalidArg;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IPEChromaEnhancement12Titan480::SetupRegisterSetting
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IPEChromaEnhancement12Titan480::SetupRegisterSetting(
    VOID*  pInput)
{
    CamxResult result = CamxResultSuccess;
    CAMX_UNREFERENCED_PARAM(pInput);

    CAMX_LOG_ERROR(CamxLogGroupPProc, "Function not implemented");

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IPEChromaEnhancement12Titan480::~IPEChromaEnhancement12Titan480
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
IPEChromaEnhancement12Titan480::~IPEChromaEnhancement12Titan480()
{
    if (NULL != m_pRegCmd)
    {
        CAMX_FREE(m_pRegCmd);
        m_pRegCmd = NULL;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IPEChromaEnhancement12Titan480::DumpRegConfig
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID IPEChromaEnhancement12Titan480::DumpRegConfig()
{
    IPEChromaEnhancementRegCmd* pRegCmd = NULL;

    if (NULL != m_pRegCmd)
    {
        pRegCmd = static_cast<IPEChromaEnhancementRegCmd*>(m_pRegCmd);

        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "ChromaEnhancement.V0  = %x\n", pRegCmd->lumaConfig0.bitfields.V0);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "ChromaEnhancement.V1  = %x\n", pRegCmd->lumaConfig0.bitfields.V1);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "ChromaEnhancement.V2  = %x\n", pRegCmd->lumaConfig1.bitfields.V2);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "ChromaEnhancement.K   = %x\n", pRegCmd->lumaConfig1.bitfields.K);

        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "ChromaEnhancement.AM  = %x\n", pRegCmd->aConfig.bitfields.AM);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "ChromaEnhancement.AP  = %x\n", pRegCmd->aConfig.bitfields.AP);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "ChromaEnhancement.BM  = %x\n", pRegCmd->bConfig.bitfields.BM);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "ChromaEnhancement.BP  = %x\n", pRegCmd->bConfig.bitfields.BP);

        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "ChromaEnhancement.CM  = %x\n", pRegCmd->cConfig.bitfields.CM);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "ChromaEnhancement.CP  = %x\n", pRegCmd->cConfig.bitfields.CP);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "ChromaEnhancement.DM  = %x\n", pRegCmd->dConfig.bitfields.DM);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "ChromaEnhancement.DP  = %x\n", pRegCmd->dConfig.bitfields.DP);

        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "ChromaEnhancement.KCB = %x\n", pRegCmd->cbConfig.bitfields.KCB);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "ChromaEnhancement.KCR = %x\n", pRegCmd->crConfig.bitfields.KCR);
    }
    else
    {
        CAMX_LOG_ERROR(CamxLogGroupPProc, "Error m_pRegCmd is NULL");
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IPEChromaEnhancement12Titan480::GetRegSize
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UINT IPEChromaEnhancement12Titan480::GetRegSize()
{
    return sizeof(IPEChromaEnhancementRegCmd);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// IPEChromaEnhancement12Titan480::UpdateTuningMetadata
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IPEChromaEnhancement12Titan480::UpdateTuningMetadata(
    VOID* pSettingData)
{
    CamxResult         result             = CamxResultSuccess;
    ISPInputData*      pInputData         = static_cast<ISPInputData*>(pSettingData);
    IPETuningMetadata* pIPETuningMetadata = static_cast<IPETuningMetadata*>(pInputData->pIPETuningMetadata);
    IpeIQSettings*     pIPEIQSettings     = static_cast<IpeIQSettings*>(pInputData->pipelineIPEData.pIPEIQSettings);

    IPEChromaEnhancementRegCmd* pRegCmd   = static_cast<IPEChromaEnhancementRegCmd*>(m_pRegCmd);

    // Post tuning metadata if setting is enabled
    if ((NULL != pInputData->pIPETuningMetadata) && (NULL != pRegCmd))
    {
        CAMX_STATIC_ASSERT(sizeof(IPEChromaEnhancementRegCmd) ==
            sizeof(pIPETuningMetadata->IPETuningMetadata480.IPEChromaEnhancementData.CEConfig));

        Utils::Memcpy(&pIPETuningMetadata->IPETuningMetadata480.IPEChromaEnhancementData.CEConfig,
                      pRegCmd,
                      sizeof(IPEChromaEnhancementRegCmd));

        if (TRUE == pIPEIQSettings->chromaEnhancementParameters.moduleCfg.EN)
        {
            DebugDataTagID dataTagID;
            if (TRUE == pInputData->pipelineIPEData.realtimeFlag)
            {
                dataTagID = DebugDataTagID::TuningIPEChromaEnhancement12Register;
            }
            else
            {
                dataTagID = DebugDataTagID::TuningIPEChromaEnhancement12RegisterOffline;
            }

            result = pInputData->pipelineIPEData.pDebugDataWriter->AddDataTag(
                dataTagID,
                DebugDataTagType::UInt32,
                CAMX_ARRAY_SIZE(pIPETuningMetadata->IPETuningMetadata480.IPEChromaEnhancementData.CEConfig),
                &pIPETuningMetadata->IPETuningMetadata480.IPEChromaEnhancementData.CEConfig,
                sizeof(pIPETuningMetadata->IPETuningMetadata480.IPEChromaEnhancementData.CEConfig));

            if (CamxResultSuccess != result)
            {
                CAMX_LOG_WARN(CamxLogGroupPProc, "AddDataTag failed with error: %d", result);
            }
        }
    }
    return result;
}

CAMX_NAMESPACE_END
