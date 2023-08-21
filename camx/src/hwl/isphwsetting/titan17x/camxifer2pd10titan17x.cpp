////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  camxifer2pd10titan17x.cpp
/// @brief CAMXIFER2PD10TITAN17X class implementation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "camxutils.h"
#include "camxifer2pd10titan17x.h"
#include "camxispiqmodule.h"
#include "camxiqinterface.h"

CAMX_NAMESPACE_BEGIN

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFER2PD10Titan17x::IFER2PD10Titan17x
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
IFER2PD10Titan17x::IFER2PD10Titan17x()
{
    SetCommandLength(
        PacketBuilder::RequiredWriteRegRangeSizeInDwords(sizeof(IFE_IFE_0_VFE_R2PD_1ST_CFG) / RegisterWidthInBytes) +
        PacketBuilder::RequiredWriteRegRangeSizeInDwords(sizeof(IFE_IFE_0_VFE_R2PD_2ND_CFG) / RegisterWidthInBytes));
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFER2PD10Titan17x::CreateCmdList
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFER2PD10Titan17x::CreateCmdList(
    VOID*   pSettingData,
    UINT32* pDMIBufferOffset)
{
    CamxResult    result            = CamxResultSuccess;
    ISPInputData* pInputData        = static_cast<ISPInputData*>(pSettingData);
    CmdBuffer*    pCmdBuffer        = NULL;
    UINT32        reg;
    UINT32        numberOfValues;
    UINT32*       pValues;

    CAMX_UNREFERENCED_PARAM(pDMIBufferOffset);

    if (NULL != pInputData && NULL != pInputData->pCmdBuffer)
    {
        pCmdBuffer = pInputData->pCmdBuffer;

        switch (m_modulePath)
        {
            case IFEPipelinePath::VideoDS4Path:
                reg            = regIFE_IFE_0_VFE_R2PD_1ST_CFG;
                numberOfValues = sizeof(IFE_IFE_0_VFE_R2PD_1ST_CFG) / RegisterWidthInBytes;
                pValues        = reinterpret_cast<UINT32*>(&m_regCmd.DS4Config);
                break;

            case IFEPipelinePath::VideoDS16Path:
                reg            = regIFE_IFE_0_VFE_R2PD_2ND_CFG;
                numberOfValues = sizeof(IFE_IFE_0_VFE_R2PD_2ND_CFG) / RegisterWidthInBytes;
                pValues        = reinterpret_cast<UINT32*>(&m_regCmd.DS16Config);
                break;

            case IFEPipelinePath::DisplayDS4Path:
                reg            = regIFE_IFE_0_VFE_DISP_R2PD_1ST_CFG;
                numberOfValues = sizeof(IFE_IFE_0_VFE_DISP_R2PD_1ST_CFG) / RegisterWidthInBytes;
                pValues        = reinterpret_cast<UINT32*>(&m_regCmd.displayDS4Config);
                break;

            case IFEPipelinePath::DisplayDS16Path:
                reg            = regIFE_IFE_0_VFE_DISP_R2PD_2ND_CFG;
                numberOfValues = sizeof(IFE_IFE_0_VFE_DISP_R2PD_2ND_CFG) / RegisterWidthInBytes;
                pValues        = reinterpret_cast<UINT32*>(&m_regCmd.displayDS16Config);
                break;

            default:
                CAMX_LOG_ERROR(CamxLogGroupIQMod, "We would never runinto this case");
                return CamxResultEInvalidState;
                break;
        }

        result = PacketBuilder::WriteRegRange(pCmdBuffer, reg, numberOfValues, pValues);

        if (CamxResultSuccess != result)
        {
            CAMX_LOG_ERROR(CamxLogGroupIQMod, "Failed to configure Crop FD path Register");
        }
    }
    else
    {
        result = CamxResultEInvalidArg;
        CAMX_LOG_ERROR(CamxLogGroupIQMod, "Invalid Input data or command buffer");
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFER2PD10Titan17x::PackIQRegisterSetting
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFER2PD10Titan17x::PackIQRegisterSetting(
    VOID*   pInput,
    VOID*   pOutput)
{
    CamxResult          result      = CamxResultSuccess;
    ISPInputData*       pInputData  = static_cast<ISPInputData*>(pInput);
    R2PD10OutputData*   pOutputData = static_cast<R2PD10OutputData*>(pOutput);
    m_modulePath                    = pOutputData->modulePath;

    if (IFEPipelinePath::VideoDS4Path == m_modulePath)
    {
        // Flush count is updated as per HLD guidelines
        m_regCmd.DS4Config.bitfields.FLUSH_PACE_CNT = 0x3;
        m_regCmd.DS4Config.bitfields.PACK_MODE      = pOutputData->packMode;
    }

    if (IFEPipelinePath::VideoDS16Path == m_modulePath)
    {
        // Flush count is updated as per HLD guidelines
        m_regCmd.DS16Config.bitfields.FLUSH_PACE_CNT = 0xf;
        m_regCmd.DS16Config.bitfields.PACK_MODE      = pOutputData->packMode;
    }

    if (IFEPipelinePath::DisplayDS4Path == m_modulePath)
    {
        // Flush count is updated as per HLD guidelines
        m_regCmd.displayDS4Config.bitfields.FLUSH_PACE_CNT = 0x3;
        m_regCmd.displayDS4Config.bitfields.PACK_MODE      = pOutputData->packMode;
    }

    if (IFEPipelinePath::DisplayDS16Path == m_modulePath)
    {
        // Flush count is updated as per HLD guidelines
        m_regCmd.displayDS16Config.bitfields.FLUSH_PACE_CNT = 0xf;
        m_regCmd.displayDS16Config.bitfields.PACK_MODE      = pOutputData->packMode;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFER2PD10Titan17x::SetupRegisterSetting
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFER2PD10Titan17x::SetupRegisterSetting(
    VOID*  pInput)
{
    CamxResult    result      = CamxResultSuccess;
    CAMX_UNREFERENCED_PARAM(pInput);

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFER2PD10Titan17x::CopyRegCmd
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UINT32 IFER2PD10Titan17x::CopyRegCmd(
    VOID* pData)
{
    UINT32 dataCopied = 0;

    if (NULL != pData)
    {
        Utils::Memcpy(pData, &m_regCmd, sizeof(m_regCmd));
        dataCopied = sizeof(m_regCmd);
    }

    return dataCopied;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFER2PD10Titan17x::~IFER2PD10Titan17x
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
IFER2PD10Titan17x::~IFER2PD10Titan17x()
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFER2PD10Titan17x::DumpRegConfig
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID IFER2PD10Titan17x::DumpRegConfig()
{
    if (IFEPipelinePath::VideoDS4Path == m_modulePath)
    {
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "DS4 Config [0x%x]", m_regCmd.DS4Config);
    }

    if (IFEPipelinePath::VideoDS16Path == m_modulePath)
    {
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "DS16 Config [0x%x]", m_regCmd.DS16Config);
    }

    if (IFEPipelinePath::DisplayDS4Path == m_modulePath)
    {
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "Display DS4 Config [0x%x]", m_regCmd.displayDS4Config);
    }

    if (IFEPipelinePath::DisplayDS16Path == m_modulePath)
    {
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "Display DS16 Config [0x%x]", m_regCmd.displayDS16Config);
    }
}

CAMX_NAMESPACE_END
