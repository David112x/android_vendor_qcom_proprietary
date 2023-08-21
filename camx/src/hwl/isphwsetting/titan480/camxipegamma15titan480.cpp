////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  camxipegamma15titan480.cpp
/// @brief IPEGamma15Titan480 class implementation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "titan480_ipe.h"
#include "gamma15setting.h"
#include "camxutils.h"
#include "camxipegamma15titan480.h"
#include "camxipegamma15.h"

CAMX_NAMESPACE_BEGIN

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IPEGamma15Titan480::IPEGamma15Titan480
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
IPEGamma15Titan480::IPEGamma15Titan480()
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IPEGamma15Titan480::CreateCmdList
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IPEGamma15Titan480::CreateCmdList(
    VOID*   pSettingData,
    UINT32* pGammaHWSettingParams)
{
    CamxResult result = CamxResultSuccess;

    ISPInputData*         pInputData             = static_cast<ISPInputData*>(pSettingData);
    GammaHWSettingParams* pModuleHWSettingParams = reinterpret_cast<GammaHWSettingParams*>(pGammaHWSettingParams);

    m_pLUTCmdBuffer       = pModuleHWSettingParams->pLUTCmdBuffer;
    m_pOffsetLUTCmdBuffer = pModuleHWSettingParams->pOffsetLUTCmdBuffer;


    result = WriteLUTtoDMI(pInputData);
    if (CamxResultSuccess != result)
    {
        CAMX_LOG_ERROR(CamxLogGroupPProc, "Fail to WriteLUTtoDMI, result: %d", result);
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IPEGamma15Titan480::WriteLUTtoDMI
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IPEGamma15Titan480::WriteLUTtoDMI(
    ISPInputData* pInputData)
{
    CamxResult result = CamxResultSuccess;

    if (NULL != pInputData)
    {
        CmdBuffer*  pDMICmdBuffer = pInputData->pipelineIPEData.ppIPECmdBuffer[CmdBufferDMIHeader];

        if ((NULL != pDMICmdBuffer) && (NULL != m_pLUTCmdBuffer))
        {
            result = PacketBuilder::WriteDMI(pDMICmdBuffer,
                                             regIPE_IPE_0_PPS_CLC_GLUT_DMI_CFG,
                                             (GammaLUTChannel0 + 1),
                                             m_pLUTCmdBuffer,
                                             m_pOffsetLUTCmdBuffer[GammaLUTChannel0],
                                             Gamma15LUTNumEntriesPerChannelSize);
            if (CamxResultSuccess != result)
            {
                CAMX_LOG_ERROR(CamxLogGroupPProc, "Failed to write DMI data for GammaLUTChannel0");
            }

            result = PacketBuilder::WriteDMI(pDMICmdBuffer,
                                             regIPE_IPE_0_PPS_CLC_GLUT_DMI_CFG,
                                             (GammaLUTChannel1 + 1),
                                             m_pLUTCmdBuffer,
                                             m_pOffsetLUTCmdBuffer[GammaLUTChannel1],
                                             Gamma15LUTNumEntriesPerChannelSize);
            if (CamxResultSuccess != result)
            {
                CAMX_LOG_ERROR(CamxLogGroupPProc, "Failed to write DMI data for GammaLUTChannel1");
            }

            result = PacketBuilder::WriteDMI(pDMICmdBuffer,
                                             regIPE_IPE_0_PPS_CLC_GLUT_DMI_CFG,
                                             (GammaLUTChannel2 + 1),
                                             m_pLUTCmdBuffer,
                                             m_pOffsetLUTCmdBuffer[GammaLUTChannel2],
                                             Gamma15LUTNumEntriesPerChannelSize);
            if (CamxResultSuccess != result)
            {
                CAMX_LOG_ERROR(CamxLogGroupPProc, "Failed to write DMI data for GammaLUTChannel2");
            }
        }
        else
        {
            result = CamxResultEFailed;
            CAMX_LOG_ERROR(CamxLogGroupPProc, "Invalid Input pointer");
        }
    }
    else
    {
        result = CamxResultEInvalidPointer;
        CAMX_LOG_ERROR(CamxLogGroupPProc, "Invalid Input pointer");
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IPEGamma15Titan480::PackIQRegisterSetting
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IPEGamma15Titan480::PackIQRegisterSetting(
    VOID*  pInput,
    VOID*  pOutputVal)
{
    CamxResult result = CamxResultSuccess;

    CAMX_UNREFERENCED_PARAM(pInput);
    CAMX_UNREFERENCED_PARAM(pOutputVal);

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IPEGamma15Titan480::SetupRegisterSetting
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IPEGamma15Titan480::SetupRegisterSetting(
    VOID*  pInput)
{
    CamxResult result = CamxResultSuccess;
    CAMX_UNREFERENCED_PARAM(pInput);

    CAMX_LOG_ERROR(CamxLogGroupPProc, "Function not implemented");

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IPEGamma15Titan480::~IPEGamma15Titan480
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
IPEGamma15Titan480::~IPEGamma15Titan480()
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IPEGamma15Titan480::DumpRegConfig
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID IPEGamma15Titan480::DumpRegConfig()
{
    CAMX_LOG_ERROR(CamxLogGroupPProc, "Function not implemented");
}

CAMX_NAMESPACE_END
