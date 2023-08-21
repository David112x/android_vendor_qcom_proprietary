////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  camxipecac23titan480.cpp
/// @brief CAMXIPECAC23TITAN480 class implementation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "cac23setting.h"
#include "camxutils.h"
#include "camxipecac23titan480.h"
#include "camxispiqmodule.h"
#include "titan480_ipe.h"

CAMX_NAMESPACE_BEGIN

CAMX_BEGIN_PACKED

/// @brief: This structure contains CAC registers programmed by software
struct IPECACRegCmd
{
    IPE_IPE_0_PPS_CLC_CAC_CFG_0 lumaThreshold;   ///< Contains CAC luma thresholds
    IPE_IPE_0_PPS_CLC_CAC_CFG_1 chromaThreshold; ///< Contains CAC chroma thresholds
} CAMX_PACKED;

CAMX_END_PACKED

static const UINT32 IPECACRegLength = sizeof(IPECACRegCmd) / RegisterWidthInBytes;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IPECAC23Titan480::IPECAC23Titan480
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
IPECAC23Titan480::IPECAC23Titan480()
{
    m_pRegCmd = CAMX_CALLOC(sizeof(IPECACRegCmd));

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
// IPECAC23Titan480::CreateCmdList
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IPECAC23Titan480::CreateCmdList(
    VOID*   pSettingData,
    UINT32* pDMIBufferOffset)
{
    CamxResult    result     = CamxResultSuccess;
    ISPInputData* pInputData = static_cast<ISPInputData*>(pSettingData);
    CmdBuffer*    pCmdBuffer = NULL;

    CAMX_UNREFERENCED_PARAM(pDMIBufferOffset);

    if ((NULL != pInputData)                  &&
        (NULL != pInputData->pipelineIPEData.ppIPECmdBuffer) &&
        (NULL != m_pRegCmd))
    {
        pCmdBuffer = pInputData->pipelineIPEData.ppIPECmdBuffer[CmdBufferPreLTM];

        result = PacketBuilder::WriteRegRange(pCmdBuffer,
                                              regIPE_IPE_0_PPS_CLC_CAC_CFG_0,
                                              IPECACRegLength,
                                              reinterpret_cast<UINT32*>(m_pRegCmd));

        if (CamxResultSuccess != result)
        {
            CAMX_LOG_ERROR(CamxLogGroupPProc, "Failed to write command buffer");
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
// IPECAC23Titan480::PackIQRegisterSetting
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IPECAC23Titan480::PackIQRegisterSetting(
    VOID*  pInput,
    VOID*  pOutput)
{
    CamxResult           result  = CamxResultSuccess;
    CAC23UnpackedField*  pData   = static_cast<CAC23UnpackedField*>(pInput);
    IPECACRegCmd*        pRegCmd = NULL;

    CAMX_UNREFERENCED_PARAM(pOutput);

    if ((NULL != m_pRegCmd) && (NULL != pData))
    {

        pRegCmd = static_cast<IPECACRegCmd*>(m_pRegCmd);

        pRegCmd->lumaThreshold.bitfields.Y_SPOT_THR         = pData->ySpotThreshold;
        pRegCmd->lumaThreshold.bitfields.Y_SATURATION_THR   = pData->ySaturationThreshold;
        pRegCmd->chromaThreshold.bitfields.C_SPOT_THR       = pData->cSpotThreshold;
        pRegCmd->chromaThreshold.bitfields.C_SATURATION_THR = pData->cSaturationThreshold;
    }
    else
    {
        CAMX_LOG_ERROR(CamxLogGroupPProc, "Abort! NULL data pointer  pData %p m_pRegCmd ", pData, m_pRegCmd);
        result = CamxResultEInvalidArg;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IPECAC23Titan480::SetupRegisterSetting
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IPECAC23Titan480::SetupRegisterSetting(
    VOID*  pInput)
{
    CamxResult result = CamxResultSuccess;
    CAMX_UNREFERENCED_PARAM(pInput);

    CAMX_LOG_ERROR(CamxLogGroupPProc, "Function not implemented");

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IPECAC23Titan480::~IPECAC23Titan480
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
IPECAC23Titan480::~IPECAC23Titan480()
{
    if (NULL != m_pRegCmd)
    {
        CAMX_FREE(m_pRegCmd);
        m_pRegCmd = NULL;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IPECAC23Titan480::DumpRegConfig
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID IPECAC23Titan480::DumpRegConfig()
{
    IPECACRegCmd*        pRegCmd = NULL;

    if (NULL != m_pRegCmd)
    {
        pRegCmd = static_cast<IPECACRegCmd*>(m_pRegCmd);

        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "CAC23.lumaThreshold.bitfields.Y_SATURATION_THR     = %x\n",
            pRegCmd->lumaThreshold.bitfields.Y_SATURATION_THR);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "CAC23.lumaThreshold.bitfields.Y_SPOT_THR           = %x\n",
            pRegCmd->lumaThreshold.bitfields.Y_SPOT_THR);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "CAC23.chromaThreshold.bitfields.C_SATURATION_THR   = %x\n",
            pRegCmd->chromaThreshold.bitfields.C_SATURATION_THR);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "CAC23.chromaThreshold.bitfields.C_SPOT_THR         = %x\n",
            pRegCmd->chromaThreshold.bitfields.C_SPOT_THR);
    }
    else
    {
        CAMX_LOG_ERROR(CamxLogGroupPProc, "Error m_pRegCmd is NULL");
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// IPECAC23Titan480::UpdateTuningMetadata
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IPECAC23Titan480::UpdateTuningMetadata(
    VOID* pSettingData)
{
    CamxResult         result             = CamxResultSuccess;
    ISPInputData*      pInputData         = static_cast<ISPInputData*>(pSettingData);
    IPETuningMetadata* pIPETuningMetadata = static_cast<IPETuningMetadata*>(pInputData->pIPETuningMetadata);
    IpeIQSettings*     pIPEIQSettings     = static_cast<IpeIQSettings*>(pInputData->pipelineIPEData.pIPEIQSettings);
    IPECACRegCmd*      pRegCmd            = static_cast<IPECACRegCmd*>(m_pRegCmd);

    // Post tuning metadata if setting is enabled
    if ((NULL != pInputData->pIPETuningMetadata) && (NULL != pRegCmd))
    {
        CAMX_STATIC_ASSERT(sizeof(IPECACRegCmd) <= sizeof(pIPETuningMetadata->IPETuningMetadata480.IPECACData.CACConfig));

        Utils::Memcpy(&pIPETuningMetadata->IPETuningMetadata480.IPECACData.CACConfig,
                      pRegCmd,
                      sizeof(IPECACRegCmd));

        if (TRUE == pIPEIQSettings->cacParameters.moduleCfg.EN)
        {
            DebugDataTagID dataTagID;
            if (TRUE == pInputData->pipelineIPEData.realtimeFlag)
            {
                dataTagID = DebugDataTagID::TuningIPECAC2xRegister;
            }
            else
            {
                dataTagID = DebugDataTagID::TuningIPECAC2xRegisterOffline;
            }

            result = pInputData->pipelineIPEData.pDebugDataWriter->AddDataTag(
                dataTagID,
                DebugDataTagType::UInt32,
                CAMX_ARRAY_SIZE(pInputData->pIPETuningMetadata->IPETuningMetadata480.IPECACData.CACConfig),
                &pInputData->pIPETuningMetadata->IPETuningMetadata480.IPECACData.CACConfig,
                sizeof(pInputData->pIPETuningMetadata->IPETuningMetadata480.IPECACData.CACConfig));

            if (CamxResultSuccess != result)
            {
                CAMX_LOG_WARN(CamxLogGroupPProc, "AddDataTag failed with error: %d", result);
            }
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IPECAC23Titan480::GetRegSize
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UINT IPECAC23Titan480::GetRegSize()
{
    return sizeof(IPECACRegCmd);
}

CAMX_NAMESPACE_END
