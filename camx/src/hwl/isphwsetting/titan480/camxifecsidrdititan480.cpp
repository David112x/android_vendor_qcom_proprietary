////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  camxifecsidrdititan480.cpp
/// @brief IFECSIDRDITitan480 class implementation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "camxutils.h"
#include "camxifecsidrdititan480.h"
#include "camxispiqmodule.h"
#include "camxiqinterface.h"

CAMX_NAMESPACE_BEGIN

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFECSIDRDITitan480::CreateCmdList
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFECSIDRDITitan480::CreateCmdList(
    VOID* pSettingData,
    UINT32* pDMIBufferOffset)
{
    CamxResult    result                 = CamxResultSuccess;
    ISPInputData* pInputData             = static_cast<ISPInputData*>(pSettingData);
    CmdBuffer*    pCmdBuffer             = NULL;
    UINT32        regOffset              = 0;
    IFECSIDExtractionInfo* pCSIDDropInfo = NULL;

    CAMX_UNREFERENCED_PARAM(pDMIBufferOffset);



    if ((NULL != pInputData)                &&
        (NULL != pInputData->pCmdBuffer)    &&
        (NULL != pInputData->pStripeConfig) &&
        (CamxResultSuccess == result))
    {
        switch (m_path)
        {
            case IFEPipelinePath::RDI0Path:
                regOffset     = IFECSIDRDI0RegCmdOffset;
                pCSIDDropInfo = &pInputData->pStripeConfig->pCSIDSubsampleInfo[IFECSIDRDI0];
                break;
            case IFEPipelinePath::RDI1Path:
                regOffset     = IFECSIDRDI1RegCmdOffset;
                pCSIDDropInfo = &pInputData->pStripeConfig->pCSIDSubsampleInfo[IFECSIDRDI1];
                break;
            case IFEPipelinePath::RDI2Path:
                regOffset     = IFECSIDRDI2RegCmdOffset;
                pCSIDDropInfo = &pInputData->pStripeConfig->pCSIDSubsampleInfo[IFECSIDRDI2];
                break;
            default:
                CAMX_LOG_ERROR(CamxLogGroupIQMod, "Invalid Path m_path %d", m_path);
                result = CamxResultEFailed;
        }

        pCmdBuffer = pInputData->pCmdBuffer;
        if ((NULL != pCSIDDropInfo) && (TRUE == pCSIDDropInfo->enableCSIDSubsample))
        {
            result = PacketBuilder::WriteRegRange(pCmdBuffer,
                                                  regOffset,
                                                  IFECSIDRDIRegLengthDWord,
                                                  reinterpret_cast<UINT32*>(&m_regCmd));
        }
        if (CamxResultSuccess != result)
        {
            CAMX_LOG_ERROR(CamxLogGroupISP, "Failed to write command buffer");
        }
    }
    else
    {
        result = CamxResultEInvalidArg;
        CAMX_LOG_ERROR(CamxLogGroupISP, "Invalid Input data or command buffer");
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFECSIDRDITitan480::PackIQRegisterSetting
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFECSIDRDITitan480::PackIQRegisterSetting(
    VOID*   pInput,
    VOID*   pOutput)
{
    CamxResult             result           = CamxResultSuccess;
    ISPInputData*          pInputData       = static_cast<ISPInputData*>(pInput);
    IFECSIDExtractionInfo* pCSIDDropInfo    = NULL;
    UINT32                 pixelSkipPattern = 0;
    UINT32                 lineSkipPattern  = 0;

    CAMX_UNREFERENCED_PARAM(pOutput);

    switch (m_path)
    {
        case IFEPipelinePath::RDI0Path:
            pCSIDDropInfo = &pInputData->pStripeConfig->pCSIDSubsampleInfo[IFECSIDRDI0];
            break;
        case IFEPipelinePath::RDI1Path:
            pCSIDDropInfo = &pInputData->pStripeConfig->pCSIDSubsampleInfo[IFECSIDRDI1];
            break;
        case IFEPipelinePath::RDI2Path:
            pCSIDDropInfo = &pInputData->pStripeConfig->pCSIDSubsampleInfo[IFECSIDRDI2];
            break;
        default:
            break;
    }
    if (NULL != pCSIDDropInfo)
    {
        pixelSkipPattern = pCSIDDropInfo->CSIDSubSamplePattern.pixelSkipPattern;
        lineSkipPattern  = pCSIDDropInfo->CSIDSubSamplePattern.lineSkipPattern;

        // Due to CSID HW Issue 16 bits pattern is not supported.. Only 32 bit Pattern is supported
        // So extend the 16 bit pixel drop pattern to 32 bit
        pixelSkipPattern = (pixelSkipPattern << 16) | pixelSkipPattern;
        m_regCmd.lineDropPattern.u32All  = lineSkipPattern;
        m_regCmd.pixelDropPattern.u32All = pixelSkipPattern;

    }
    m_regCmd.pixelDropPeriod.bitfields.PERIOD = 0x1F;
    m_regCmd.lineDropPeriod.bitfields.PERIOD  = 0xF;

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFECSIDRDITitan480::SetupRegisterSetting
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFECSIDRDITitan480::SetupRegisterSetting(
    VOID*  pInput)
{
    CamxResult result = CamxResultSuccess;

    CAMX_UNREFERENCED_PARAM(pInput);

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFECSIDRDITitan480::CopyRegCmd
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UINT32 IFECSIDRDITitan480::CopyRegCmd(
    VOID* pData)
{
    CAMX_UNREFERENCED_PARAM(pData);

    CAMX_LOG_VERBOSE(CamxLogGroupISP, "No need to implement");

    return 0;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFECSIDRDITitan480::~IFECSIDRDITitan480
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
IFECSIDRDITitan480::~IFECSIDRDITitan480()
{
    CAMX_LOG_VERBOSE(CamxLogGroupISP, "Destroyed");
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// IFECSIDRDITitan480::IFECSIDRDITitan480
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
IFECSIDRDITitan480::IFECSIDRDITitan480(
    IFEPipelinePath pipelinePath)
{
    SetCommandLength(PacketBuilder::RequiredWriteRegRangeSizeInDwords(IFECSIDRDIRegLengthDWord));
    Utils::Memset(&m_regCmd, 0, sizeof(m_regCmd));

    m_path = pipelinePath;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFECSIDRDITitan480::DumpRegConfig
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID IFECSIDRDITitan480::DumpRegConfig()
{
    CAMX_LOG_VERBOSE(CamxLogGroupISP, "CSID Pixel Skip Pattern: 0x%x", m_regCmd.pixelDropPattern.u32All);
    CAMX_LOG_VERBOSE(CamxLogGroupISP, "CSID Pixel Skip Period : 0x%x", m_regCmd.pixelDropPeriod.u32All);
    CAMX_LOG_VERBOSE(CamxLogGroupISP, "CSID Line  Skip Pattern: 0x%x", m_regCmd.lineDropPattern.u32All);
    CAMX_LOG_VERBOSE(CamxLogGroupISP, "CSID Line  Skip Period : 0x%x", m_regCmd.lineDropPeriod.u32All);
}

CAMX_NAMESPACE_END
