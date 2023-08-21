////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  camxipecac22titan17x.cpp
/// @brief CAMXIPECAC22TITAN17x class implementation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "cac22setting.h"
#include "camxutils.h"
#include "camxipecac22titan17x.h"
#include "camxispiqmodule.h"
#include "titan170_ipe.h"

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
CAMX_STATIC_ASSERT((2 * 4) == sizeof(IPECACRegCmd));

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IPECAC22Titan17x::IPECAC22Titan17x
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
IPECAC22Titan17x::IPECAC22Titan17x()
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
// IPECAC22Titan17x::CreateCmdList
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IPECAC22Titan17x::CreateCmdList(
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
// IPECAC22Titan17x::PackIQRegisterSetting
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IPECAC22Titan17x::PackIQRegisterSetting(
    VOID*  pInput,
    VOID*  pOutput)
{
    CamxResult           result  = CamxResultSuccess;
    CAC22UnpackedField*  pData   = static_cast<CAC22UnpackedField*>(pInput);
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
// IPECAC22Titan17x::SetupRegisterSetting
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IPECAC22Titan17x::SetupRegisterSetting(
    VOID*  pInput)
{
    CamxResult result = CamxResultSuccess;
    CAMX_UNREFERENCED_PARAM(pInput);

    CAMX_LOG_ERROR(CamxLogGroupPProc, "Function not implemented");

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IPECAC22Titan17x::GetRegSize
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UINT IPECAC22Titan17x::GetRegSize()
{
    return sizeof(IPECACRegCmd);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IPECAC22Titan17x::~IPECAC22Titan17x
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
IPECAC22Titan17x::~IPECAC22Titan17x()
{
    if (NULL != m_pRegCmd)
    {
        CAMX_FREE(m_pRegCmd);
        m_pRegCmd = NULL;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IPECAC22Titan17x::DumpRegConfig
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID IPECAC22Titan17x::DumpRegConfig()
{
    IPECACRegCmd*        pRegCmd = NULL;

    if (NULL != m_pRegCmd)
    {
        pRegCmd = static_cast<IPECACRegCmd*>(m_pRegCmd);

        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "CAC22.lumaThreshold.bitfields.Y_SATURATION_THR     = %x\n",
            pRegCmd->lumaThreshold.bitfields.Y_SATURATION_THR);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "CAC22.lumaThreshold.bitfields.Y_SPOT_THR           = %x\n",
            pRegCmd->lumaThreshold.bitfields.Y_SPOT_THR);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "CAC22.chromaThreshold.bitfields.C_SATURATION_THR   = %x\n",
            pRegCmd->chromaThreshold.bitfields.C_SATURATION_THR);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "CAC22.chromaThreshold.bitfields.C_SPOT_THR         = %x\n",
            pRegCmd->chromaThreshold.bitfields.C_SPOT_THR);
    }
    else
    {
        CAMX_LOG_ERROR(CamxLogGroupPProc, "Error m_pRegCmd is NULL");
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// IPECAC22Titan17x::UpdateTuningMetadata
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IPECAC22Titan17x::UpdateTuningMetadata(
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
        CAMX_STATIC_ASSERT(sizeof(IPECACRegCmd) <= sizeof(pIPETuningMetadata->IPETuningMetadata17x.IPECACData.CACConfig));

        Utils::Memcpy(&pIPETuningMetadata->IPETuningMetadata17x.IPECACData.CACConfig,
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
                CAMX_ARRAY_SIZE(pIPETuningMetadata->IPETuningMetadata17x.IPECACData.CACConfig),
                &pIPETuningMetadata->IPETuningMetadata17x.IPECACData.CACConfig,
                sizeof(pIPETuningMetadata->IPETuningMetadata17x.IPECACData.CACConfig));

            if (CamxResultSuccess != result)
            {
                CAMX_LOG_WARN(CamxLogGroupPProc, "AddDataTag failed with error: %d", result);
            }
        }
    }

    return result;
}

CAMX_NAMESPACE_END
