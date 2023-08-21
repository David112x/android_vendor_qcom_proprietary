////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  camxifecamifpdaftitan480.cpp
/// @brief IFECAMIFPDAFTitan480 class implementation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "camxutils.h"
#include "camxifecamifpdaftitan480.h"
#include "camxispiqmodule.h"
#include "camxiqinterface.h"

CAMX_NAMESPACE_BEGIN

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFECAMIFPDAFTitan480::CreateCmdList
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFECAMIFPDAFTitan480::CreateCmdList(
    VOID* pSettingData,
    UINT32* pDMIBufferOffset)
{
    CamxResult    result        = CamxResultSuccess;
    ISPInputData* pInputData    = static_cast<ISPInputData*>(pSettingData);
    CmdBuffer*    pCmdBuffer    = NULL;

    CAMX_UNREFERENCED_PARAM(pDMIBufferOffset);

    if ((NULL != pInputData) && (NULL != pInputData->pCmdBuffer))
    {
        pCmdBuffer = pInputData->pCmdBuffer;
        result = PacketBuilder::WriteRegRange(pCmdBuffer,
            IFECamifPDAFRegCmd1Offset,
            IFECAMIFPDAFRegLengthDWord1,
            reinterpret_cast<UINT32*>(&m_regCmd1));
        if (CamxResultSuccess == result)
        {
            result = PacketBuilder::WriteRegRange(pCmdBuffer,
                IFECamifPDAFRegCmd2Offset,
                IFECAMIFPDAFRegLengthDWord2,
                reinterpret_cast<UINT32*>(&m_regCmd2));
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
// IFECAMIFPDAFTitan480::PackIQRegisterSetting
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFECAMIFPDAFTitan480::PackIQRegisterSetting(
    VOID*   pInput,
    VOID*   pBayerPattern)
{
    CamxResult result           = CamxResultSuccess;
    ISPInputData* pInputData    = static_cast<ISPInputData*>(pInput);
    UINT8*        pPixelPattern = static_cast<UINT8*>(pBayerPattern);

    // Setup camif configuration
    m_regCmd1.configRegister.bitfields.EN = 0x1; // enable camif module
    m_regCmd1.configRegister.bitfields.STRIPE_LOC    = 0x0;            // stripe location. For IFE, "0" is full frame
                                                                       // 0x0:RGRGRG, 0x1:GRGRGR, 0x2:BGBGBG, 0x3:GBGBGB,
                                                                       // 0x4:YCBYCR, 0x5:YCRYCB, 0x6:CBYCRY, 0x7:CRYCBY
    m_regCmd1.configRegister.bitfields.PIXEL_PATTERN = *pPixelPattern; // Bayer pattern

    m_regCmd2.periodConfigureRegister.u32All = 0x0; // period is 1

    if (1 < pInputData->pipelineIFEData.numBatchedFrames)
    {
        m_regCmd2.periodConfigureRegister.bitfields.IRQ_SUBSAMPLE_PERIOD =
            pInputData->pipelineIFEData.numBatchedFrames - 1;
        m_regCmd2.irqSubsamplePatternRegister.bitfields.PATTERN =
            1 << (pInputData->pipelineIFEData.numBatchedFrames - 1);
    }

    if (1 > m_regCmd2.irqSubsamplePatternRegister.bitfields.PATTERN)
    {
        m_regCmd2.irqSubsamplePatternRegister.bitfields.PATTERN          = 0x1;
        m_regCmd2.periodConfigureRegister.bitfields.IRQ_SUBSAMPLE_PERIOD = 0x0;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFECAMIFPDAFTitan480::SetupRegisterSetting
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFECAMIFPDAFTitan480::SetupRegisterSetting(
    VOID*  pInput)
{
    CamxResult result = CamxResultSuccess;

    CAMX_UNREFERENCED_PARAM(pInput);

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFECAMIFPDAFTitan480::CopyRegCmd
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UINT32 IFECAMIFPDAFTitan480::CopyRegCmd(
    VOID* pData)
{
    CAMX_UNREFERENCED_PARAM(pData);

    CAMX_LOG_VERBOSE(CamxLogGroupISP, "No need to implement");

    return 0;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFECAMIFPDAFTitan480::~IFECAMIFPDAFTitan480
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
IFECAMIFPDAFTitan480::~IFECAMIFPDAFTitan480()
{
    CAMX_LOG_VERBOSE(CamxLogGroupISP, "Destroyed");
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// IFECAMIFPDAFTitan480::IFECAMIFPDAFTitan480
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
IFECAMIFPDAFTitan480::IFECAMIFPDAFTitan480(
    UINT32 instance)
{

    SetCommandLength(PacketBuilder::RequiredWriteRegRangeSizeInDwords(IFECAMIFPDAFRegLengthDWord1) +
        PacketBuilder::RequiredWriteRegRangeSizeInDwords(IFECAMIFPDAFRegLengthDWord2));

    Utils::Memset(&m_regCmd1, 0, sizeof(m_regCmd1));
    Utils::Memset(&m_regCmd2, 0, sizeof(m_regCmd2));

    m_instance = instance;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFECAMIFPDAFTitan480::DumpRegConfig
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID IFECAMIFPDAFTitan480::DumpRegConfig()
{
    CAMX_LOG_VERBOSE(CamxLogGroupISP, "Camif Module config:         0x%x", m_regCmd1.configRegister.u32All);
    CAMX_LOG_VERBOSE(CamxLogGroupISP, "Camif irq subsample pattern: 0x%x", m_regCmd2.irqSubsamplePatternRegister.u32All);
    CAMX_LOG_VERBOSE(CamxLogGroupISP, "Camif subsample period:      0x%x", m_regCmd2.periodConfigureRegister.u32All);
}

CAMX_NAMESPACE_END
