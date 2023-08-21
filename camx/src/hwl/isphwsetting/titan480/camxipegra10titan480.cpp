////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  camxipegra10titan480.cpp
/// @brief IPEGRA10Titan480 class implementation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "titan480_ipe.h"
#include "gra10setting.h"
#include "camxutils.h"
#include "camxipegra10titan480.h"
#include "camxipegrainadder10.h"

CAMX_NAMESPACE_BEGIN

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IPEGRA10Titan480::IPEGRA10Titan480
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
IPEGRA10Titan480::IPEGRA10Titan480()
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IPEGRA10Titan480::CreateCmdList
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IPEGRA10Titan480::CreateCmdList(
    VOID*   pSettingData,
    UINT32* pGRAHWSettingParams)
{
    CamxResult result = CamxResultSuccess;

    ISPInputData*       pInputData             = static_cast<ISPInputData*>(pSettingData);
    GRAHWSettingParams* pModuleHWSettingParams = reinterpret_cast<GRAHWSettingParams*>(pGRAHWSettingParams);

    m_pLUTCmdBuffer       = pModuleHWSettingParams->pLUTCmdBuffer;
    m_pOffsetLUTCmdBuffer = pModuleHWSettingParams->pOffsetLUTCmdBuffer;

    result = WriteLUTtoDMI(pInputData);
    if (CamxResultSuccess != result)
    {
        result = CamxResultEFailed;
        CAMX_LOG_ERROR(CamxLogGroupPProc, "GrainAdder configuration failed");
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IPEGRA10Titan480::WriteLUTtoDMI
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IPEGRA10Titan480::WriteLUTtoDMI(
    ISPInputData* pInputData)
{
    CamxResult result = CamxResultSuccess;
    CmdBuffer* pDMICmdBuffer = pInputData->pipelineIPEData.ppIPECmdBuffer[CmdBufferDMIHeader];

    if ((NULL != pDMICmdBuffer) && (NULL != m_pLUTCmdBuffer))
    {
        result = PacketBuilder::WriteDMI(pDMICmdBuffer,
                                         regIPE_IPE_0_PPS_CLC_GRA_DMI_CFG,
                                         0x1,
                                         m_pLUTCmdBuffer,
                                         m_pOffsetLUTCmdBuffer[GRALUTChannel0],
                                         GRA10LUTNumEntriesPerChannelSize);
        if (CamxResultSuccess != result)
        {
            CAMX_LOG_ERROR(CamxLogGroupPProc, "Failed to write on DMIBank 0x1 result: %d", result);
        }

        result = PacketBuilder::WriteDMI(pDMICmdBuffer,
                                         regIPE_IPE_0_PPS_CLC_GRA_DMI_CFG,
                                         0x2,
                                         m_pLUTCmdBuffer,
                                         m_pOffsetLUTCmdBuffer[GRALUTChannel1],
                                         GRA10LUTNumEntriesPerChannelSize);
        if (CamxResultSuccess != result)
        {
            CAMX_LOG_ERROR(CamxLogGroupPProc, "Failed to write on DMIBank 0x2 result: %d", result);
        }

        result = PacketBuilder::WriteDMI(pDMICmdBuffer,
                                         regIPE_IPE_0_PPS_CLC_GRA_DMI_CFG,
                                         0x3,
                                         m_pLUTCmdBuffer,
                                         m_pOffsetLUTCmdBuffer[GRALUTChannel2],
                                         GRA10LUTNumEntriesPerChannelSize);
        if (CamxResultSuccess != result)
        {
            CAMX_LOG_ERROR(CamxLogGroupPProc, "Failed to write on DMIBank 0x3 result: %d", result);
        }
    }
    else
    {
        result = CamxResultEInvalidPointer;
        CAMX_LOG_ERROR(CamxLogGroupPProc, "NULL pointer m_pLUTCmdBuffer = %p, pDMICmdBuffer = %p",
            m_pLUTCmdBuffer, pDMICmdBuffer);
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IPEGRA10Titan480::PackIQRegisterSetting
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IPEGRA10Titan480::PackIQRegisterSetting(
    VOID*  pInput,
    VOID*  pOutputVal)
{
    CamxResult result = CamxResultSuccess;

    CAMX_UNREFERENCED_PARAM(pInput);
    CAMX_UNREFERENCED_PARAM(pOutputVal);

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IPEGRA10Titan480::SetupRegisterSetting
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IPEGRA10Titan480::SetupRegisterSetting(
    VOID*  pInput)
{
    CamxResult result = CamxResultSuccess;
    CAMX_UNREFERENCED_PARAM(pInput);

    CAMX_LOG_ERROR(CamxLogGroupPProc, "Function not implemented");

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IPEGRA10Titan480::~IPEGRA10Titan480
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
IPEGRA10Titan480::~IPEGRA10Titan480()
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IPEGRA10Titan480::DumpRegConfig
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID IPEGRA10Titan480::DumpRegConfig()
{
    CAMX_LOG_ERROR(CamxLogGroupPProc, "Function not implemented");
}

CAMX_NAMESPACE_END
